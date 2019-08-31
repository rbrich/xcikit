// TypeResolver.cpp created on 2019-06-13, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "TypeResolver.h"
#include "Value.h"
#include "Builtin.h"
#include "Function.h"
#include "Error.h"
#include <xci/compat/macros.h>

using namespace std;

namespace xci::script {


class TypeCheckerVisitor: public ast::Visitor {
public:
    explicit TypeCheckerVisitor(TypeResolver& processor, Function& func)
        : m_processor(processor), m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // in case this is function definition, we might need partial type
        // for recursive calls - pass the definition index to Function visitor
        Index idx = m_function.add_value({});
        dfn.identifier.symbol->set_index(idx);
        m_definition = &dfn;
        dfn.expression->apply(*this);
        m_definition = nullptr;
        // type might be partially filled (see above) - replace it with final type
        m_function.set_value(idx, move(m_value_type));
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Integer& v) override { m_value_type = TypeInfo{Type::Int32}; }
    void visit(ast::Float& v) override { m_value_type = TypeInfo{Type::Float32}; }
    void visit(ast::String& v) override { m_value_type = TypeInfo{Type::String}; }

    void visit(ast::Tuple& v) override {
        // build TypeInfo from subtypes
        std::vector<TypeInfo> subtypes;
        subtypes.reserve(v.items.size());
        for (auto& item : v.items) {
            item->apply(*this);
            subtypes.push_back(move(m_value_type));
        }
        m_value_type = TypeInfo(move(subtypes));
    }

    struct CallArg {
        TypeInfo type_info;
        SourceInfo source_info;
    };
    using CallArgs = std::vector<CallArg>;

    void visit(ast::Call& v) override {
        CallArgs args;
        for (auto& arg : v.args) {
            arg->apply(*this);
            assert(arg->source_info.source != nullptr);
            args.push_back({move(m_value_type), arg->source_info});
        }
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Function: {
                // find matching overload
                auto symptr = v.identifier.symbol;
                bool found = false;
                while (symptr) {
                    auto* symmod = symptr.symtab()->module();
                    if (symmod == nullptr)
                        symmod = &module();
                    auto& fn = symmod->get_function(symptr->index());
                    auto sig_ptr = fn.signature_ptr();
                    if (fn.is_generic()) {
                        // instantiate the specialization
                        auto fspec = resolve_specialization(fn, args);
                        sig_ptr = fspec->signature_ptr();
                        Symbol sym_copy {*symptr};
                        sym_copy.set_index(module().add_function(move(fspec)));
                        v.identifier.symbol = module().symtab().add(move(sym_copy));
                        m_value_type = TypeInfo{sig_ptr};
                        found = true;
                        break;
                    } else if (match_params(*sig_ptr, args)) {
                        v.identifier.symbol = symptr;
                        m_value_type = TypeInfo{sig_ptr};
                        found = true;
                        break;
                    }
                    symptr = symptr->next();
                }
                if (found)
                    break;
                // ERROR couldn't find matching function for `args`
                stringstream o_candidates;
                symptr = v.identifier.symbol;
                while (symptr) {
                    auto* symmod = symptr.symtab()->module();
                    if (symmod == nullptr)
                        symmod = &module();
                    auto& fn = symmod->get_function(symptr->index());
                    o_candidates << "   " << fn.signature() << endl;
                    symptr = symptr->next();
                }
                stringstream o_args;
                o_args << "| ";
                for (const auto& arg : args) {
                    o_args << arg.type_info << ' ';
                }
                o_args << '|';
                throw FunctionNotFound(v.identifier.name, o_args.str(), o_candidates.str());
            }
            case Symbol::Module:
                m_value_type = TypeInfo{Type::Module};
                break;
            case Symbol::Nonlocal: {
                assert(sym.ref());
                auto& nl_sym = *sym.ref();
                auto* nl_func = sym.ref().symtab()->function();
                assert(nl_func != nullptr);
                if (nl_sym.type() == Symbol::Value)
                    m_value_type = nl_func->get_value(nl_sym.index());
                else if (nl_sym.type() == Symbol::Parameter)
                    m_value_type = nl_func->get_parameter(nl_sym.index());
                else
                    assert(!"Bad nonlocal reference.");
                break;
            }
            case Symbol::Parameter:
                m_value_type = m_function.get_parameter(sym.index());
                break;
            case Symbol::Value:
                if (symtab.module() != nullptr) {
                    // static value
                    m_value_type = symtab.module()->get_value(sym.index()).type_info();
                } else {
                    m_value_type = m_function.get_value(sym.index());
                }
                break;
            case Symbol::Unresolved:
                UNREACHABLE;
        }

        if (!m_value_type.is_callable() && !args.empty()) {
            throw UnexpectedArgument(1, args[0].source_info);
        }
        v.identifier.symbol->set_callable(m_value_type.is_callable());
        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            auto new_signature = resolve_params(m_value_type.signature(), args);
            if (new_signature->params.empty())
                // effective type of zero-arg function is its return type
                m_value_type = new_signature->return_type;
            else
                m_value_type = TypeInfo{new_signature};
        }
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        v.identifier.name = builtin::op_to_function_name(v.op.op);
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        if (m_value_type != TypeInfo{Type::Bool})
            throw ConditionNotBool();
        v.then_expr->apply(*this);
        auto then_type = m_value_type;
        v.else_expr->apply(*this);
        auto else_type = m_value_type;
        if (then_type != else_type) {
            throw BranchTypeMismatch(then_type, else_type);
        }
    }

    void visit(ast::Function& v) override {
        Function& fn = module().get_function(v.index);
        for (auto& p : v.type.params) {
            if (p.type)
                p.type->apply(*this);
            else
                m_type_info = TypeInfo{Type::Auto};
            fn.signature().add_parameter(move(m_type_info));
        }
        if (v.type.result_type)
            v.type.result_type->apply(*this);
        else
            m_type_info = TypeInfo{Type::Auto};
        fn.signature().set_return_type(move(m_type_info));
        m_value_type = TypeInfo{fn.signature_ptr()};

        if (m_definition != nullptr) {
            // partial type for recursive functions - no auto resolution
            m_function.set_value(m_definition->identifier.symbol.index(), move(m_value_type));
        }

        // compile body and resolve return type
        m_processor.process_block(fn, v.body);
        m_value_type = TypeInfo{fn.signature_ptr()};
    }

    void visit(ast::TypeName& t) final {
        m_type_info = builtin::type_by_name(t.name);
    }

    void visit(ast::FunctionType& t) final {
        auto signature = std::make_shared<Signature>();
        for (const auto& p : t.params) {
            p.type->apply(*this);
            signature->add_parameter(move(m_type_info));
        }
        t.result_type->apply(*this);
        signature->set_return_type(m_type_info);
        m_type_info = TypeInfo{signature};
    }

    TypeInfo value_type() const { return m_value_type; }

private:
    Module& module() { return m_function.module(); }

    // return new function according to requested signature
    // throw when the signature doesn't match
    std::unique_ptr<Function> resolve_specialization(const Function& orig_fn, const CallArgs& args) const
    {
        auto fn = make_unique<Function>(orig_fn.module(), orig_fn.symtab());
        fn->signature() = orig_fn.signature();
        assert(args.size() == fn->signature().params.size());
        for (size_t i = 0; i < fn->signature().params.size(); i++) {
            const auto& arg = args[i];
            auto& out_type = fn->signature().params[i];
            if (arg.type_info.is_generic())
                continue;
            if (out_type.is_generic()) {
                auto var = out_type.generic_var();
                // resolve this generic var to received type
                for (size_t j = i; j < fn->signature().params.size(); j++) {
                    auto& outj_type = fn->signature().params[j];
                    if (outj_type.is_generic() && outj_type.generic_var() == var)
                        outj_type = arg.type_info;
                }
                auto& ret_type = fn->signature().return_type;
                if (ret_type.is_generic() && ret_type.generic_var() == var)
                    ret_type = arg.type_info;
            }
        }
        fn->values() = orig_fn.values();
        fn->code() = orig_fn.code();
        return fn;
    }

    // Check and consume params from `called`, creating new signature
    std::shared_ptr<Signature> resolve_params(const Signature& orig_signature, const CallArgs& args) const
    {
        auto res = std::make_shared<Signature>(orig_signature);
        int i = 0;
        for (const auto& arg : args) {
            i += 1;
            // check there are more params to consume
            while (res->params.empty()) {
                if (res->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    res = std::make_unique<Signature>(res->return_type.signature());
                } else {
                    throw UnexpectedArgument(i, arg.source_info);
                }
            }
            // check type of next param
            if (res->params[0] != arg.type_info) {
                throw UnexpectedArgumentType(i, res->params[0], arg.type_info, arg.source_info);
            }
            // consume next param
            res->params.erase(res->params.begin());
        }
        return res;
    }

    // Check params from `args` against `signature`
    bool match_params(const Signature& signature, const CallArgs& args) const
    {
        auto sig = std::make_unique<Signature>(signature);
        int i = 0;
        for (const auto& arg : args) {
            i += 1;
            // check there are more params to consume
            while (sig->params.empty()) {
                if (sig->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    sig = std::make_unique<Signature>(sig->return_type.signature());
                } else {
                    // unexpected argument
                    return false;
                }
            }
            // check type of next param
            if (sig->params[0] != arg.type_info) {
                return false;
            }
            // consume next param
            sig->params.erase(sig->params.begin());
        }
        return true;
    }

private:
    TypeResolver& m_processor;
    Function& m_function;
    TypeInfo m_type_info;
    TypeInfo m_value_type;
    ast::Definition* m_definition = nullptr;
};


void TypeResolver::process_block(Function& func, const ast::Block& block)
{
    TypeCheckerVisitor visitor {*this, func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    func.signature().resolve_return_type(visitor.value_type());
}


} // namespace xci::script
