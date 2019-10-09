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
    struct CallArg {
        TypeInfo type_info;
        SourceInfo source_info;
    };
    using CallArgs = std::vector<CallArg>;

public:
    explicit TypeCheckerVisitor(TypeResolver& processor, Function& func)
        : m_processor(processor), m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // Evaluate specified type
        if (dfn.variable.type) {
            dfn.variable.type->apply(*this);
        } else {
            m_type_info = {};
        }

        if (m_class != nullptr) {
            auto idx = m_class->add_function_type(move(m_type_info));
            dfn.variable.identifier.symbol->set_index(idx);
            return;
        }

        if (m_instance != nullptr) {
            // evaluate type according to class and type var
            auto& symptr = dfn.variable.identifier.symbol;
            TypeInfo eval_type = m_instance->class_().get_function_type(symptr->ref()->index());
            eval_type.replace_var(1, m_instance->type_inst());

            // specified type is basically useless here, let's just check
            // it matches evaluated type from class instance
            if (m_type_info && m_type_info != eval_type)
                throw DefinitionTypeMismatch(m_type_info, eval_type);

            m_type_info = move(eval_type);
        }

        // Expression might use the specified type from `m_type_info`
        if (dfn.expression)
            dfn.expression->apply(*this);

        Index idx = m_function.add_value(move(m_value_type));
        dfn.variable.identifier.symbol->set_index(idx);
        // if the function was just a parameterless block, change symbol type to a value
        if (dfn.variable.identifier.symbol->type() == Symbol::Type::Function
            && !m_value_type.is_callable()) {
            dfn.variable.identifier.symbol->set_type(Symbol::Type::Value);
            dfn.variable.identifier.symbol->set_callable(false);
        }
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        inv.type_index = module().add_type(move(m_value_type));
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
        m_function.signature().resolve_return_type(m_value_type);
    }

    void visit(ast::Class& v) override {
        m_class = &module().get_class(v.index);
        for (auto& dfn : v.defs)
            dfn.apply(*this);
        m_class = nullptr;
    }

    void visit(ast::Instance& v) override {
        m_instance = &module().get_instance(v.index);
        // resolve instance type
        v.type_inst->apply(*this);
        m_instance->set_type_inst(move(m_type_info));
        // resolve each Definition from the class,
        // fill-in FunctionType, match with possible named arguments and body
        for (auto& dfn : v.defs)
            dfn.apply(*this);
        m_instance = nullptr;
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

    void visit(ast::List& v) override {
        // check all items have same type
        TypeInfo elem_type;
        for (auto& item : v.items) {
            item->apply(*this);
            if (item.get() == v.items.front().get()) {
                // first item
                elem_type = move(m_value_type);
            } else {
                // other items
                if (elem_type != m_value_type)
                    throw ListElemTypeMismatch(elem_type, m_value_type);
            }
        }
        v.item_size = elem_type.size();
        m_value_type = TypeInfo(Type::List, move(elem_type));
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Instruction:
                m_value_type = {};
                return;
            case Symbol::Class:
            case Symbol::Instance:
                // TODO
                return;
            case Symbol::ClassFunction: {
                // TODO
                break;
            }
            case Symbol::Function: {
                // find matching instance
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
                        auto fspec = resolve_specialization(fn);
                        sig_ptr = fspec->signature_ptr();
                        Symbol sym_copy {*symptr};
                        sym_copy.set_index(module().add_function(move(fspec)));
                        v.identifier.symbol = module().symtab().add(move(sym_copy));
                        m_value_type = TypeInfo{sig_ptr};
                        found = true;
                        break;
                    } else if (match_params(*sig_ptr)) {
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
                for (const auto& arg : m_call_args) {
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
        v.identifier.symbol->set_callable(m_value_type.is_callable());
    }

    void visit(ast::Call& v) override {
        // resolve each argument
        CallArgs args;
        for (auto& arg : v.args) {
            arg->apply(*this);
            assert(arg->source_info.source != nullptr);
            args.push_back({move(m_value_type), arg->source_info});
        }

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        assert(m_call_args.empty());
        m_call_args = move(args);
        v.callable->apply(*this);

        if (!m_value_type.is_callable() && !m_call_args.empty()) {
            throw UnexpectedArgument(1, m_call_args[0].source_info);
        }

        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            auto new_signature = resolve_params(m_value_type.signature(), v.wrapped_execs);
            if (new_signature->params.empty())
                // effective type of zero-arg function is its return type
                m_value_type = new_signature->return_type;
            else
                m_value_type = TypeInfo{new_signature};
        }
        m_call_args.clear();
    }

    void visit(ast::OpCall& v) override {
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
        // specified type (left hand side of '=')
        TypeInfo specified_type = move(m_type_info);
        // lambda type (right hand side of '=')
        v.type.apply(*this);
        // fill in / check type from specified type
        if (specified_type.is_callable()) {
            if (!m_type_info.signature().return_type && specified_type.signature().return_type)
                m_type_info.signature().set_return_type(specified_type.signature().return_type);
            size_t idx = 0;
            auto& params = m_type_info.signature().params;
            for (const auto& sp : specified_type.signature().params) {
                if (idx >= params.size())
                    params.emplace_back(sp);
                else if (!params[idx])
                    params[idx] = sp;
                // specified param must match now
                if (params[idx] != sp)
                    throw DefinitionParamTypeMismatch(idx, sp, params[idx]);
                ++idx;
            }
        }
        m_value_type = move(m_type_info);

        Function& fn = m_instance ? m_instance->get_function(v.index)
                                  : module().get_function(v.index);
        fn.set_signature(m_value_type.signature_ptr());

        // compile body and resolve return type
        m_processor.process_block(fn, v.body);
        m_value_type = TypeInfo{fn.signature_ptr()};

        // parameterless function is equivalent to its return type (eager evaluation)
        while (m_value_type.is_callable() && m_value_type.signature().params.empty()) {
            m_value_type = m_value_type.signature().return_type;
        }
        // check specified type again - in case it wasn't Function
        if (!m_value_type.is_callable() && specified_type) {
            if (m_value_type != specified_type)
                throw DefinitionTypeMismatch(specified_type, m_value_type);
        }
    }

    void visit(ast::TypeName& t) final {
        switch (t.symbol->type()) {
            case Symbol::TypeName:
                m_type_info = TypeInfo{ Type(t.symbol->index()) };
                break;
            case Symbol::TypeVar:
                m_type_info = TypeInfo{ Type::Unknown, uint8_t(t.symbol->index()) };
                break;
        }
    }

    void visit(ast::FunctionType& t) final {
        auto signature = std::make_shared<Signature>();
        for (const auto& p : t.params) {
            if (p.type)
                p.type->apply(*this);
            else
                m_type_info = TypeInfo{Type::Unknown};
            signature->add_parameter(move(m_type_info));
        }
        if (t.result_type)
            t.result_type->apply(*this);
        else
            m_type_info = TypeInfo{Type::Unknown};
        signature->set_return_type(m_type_info);
        m_type_info = TypeInfo{signature};
    }

    void visit(ast::ListType& t) final {
        t.elem_type->apply(*this);
        m_type_info = TypeInfo{Type::List, move(m_type_info)};
    }

    TypeInfo value_type() const { return m_value_type; }

private:
    Module& module() { return m_function.module(); }

    // return new function according to requested signature
    // throw when the signature doesn't match
    std::unique_ptr<Function> resolve_specialization(const Function& orig_fn) const
    {
        auto fn = make_unique<Function>(orig_fn.module(), orig_fn.symtab());
        fn->signature() = orig_fn.signature();
        assert(m_call_args.size() == fn->signature().params.size());
        for (size_t i = 0; i < fn->signature().params.size(); i++) {
            const auto& arg = m_call_args[i];
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

    // Consume params from `orig_signature` according to `m_call_args`, creating new signature
    std::shared_ptr<Signature> resolve_params(const Signature& orig_signature, size_t& wrapped_execs) const
    {
        auto res = std::make_shared<Signature>(orig_signature);
        int i = 0;
        for (const auto& arg : m_call_args) {
            i += 1;
            // check there are more params to consume
            while (res->params.empty()) {
                if (res->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    res = std::make_unique<Signature>(res->return_type.signature());
                    ++wrapped_execs;
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

    // Check params from `signature` against `m_call_args`
    bool match_params(const Signature& signature) const
    {
        auto sig = std::make_unique<Signature>(signature);
        int i = 0;
        for (const auto& arg : m_call_args) {
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
    CallArgs m_call_args;
    Class* m_class = nullptr;
    Instance* m_instance = nullptr;
};


void TypeResolver::process_block(Function& func, const ast::Block& block)
{
    TypeCheckerVisitor visitor {*this, func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
