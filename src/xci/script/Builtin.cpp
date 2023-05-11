// Builtin.cpp created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Builtin.h"
#include "dump.h"
#include <xci/core/string.h>
#include <xci/compat/macros.h>
#include <xci/script/typing/type_index.h>

#include <range/v3/view/enumerate.hpp>

#include <sstream>

namespace xci::script {

using ranges::views::enumerate;


const char* builtin::op_to_function_name(ast::Operator::Op op)
{
    using Op = ast::Operator;
    switch (op) {
        case Op::Undefined:     return nullptr;
        case Op::Comma:         return nullptr;
        case Op::LogicalOr:     return "or";
        case Op::LogicalAnd:    return "and";
        case Op::Equal:         return "eq";
        case Op::NotEqual:      return "ne";
        case Op::LessEqual:     return "le";
        case Op::GreaterEqual:  return "ge";
        case Op::LessThan:      return "lt";
        case Op::GreaterThan:   return "gt";
        case Op::BitwiseOr:     return "bit_or";
        case Op::BitwiseAnd:    return "bit_and";
        case Op::BitwiseXor:    return "bit_xor";
        case Op::ShiftLeft:     return "shift_left";
        case Op::ShiftRight:    return "shift_right";
        case Op::Add:           return "add";
        case Op::Sub:           return "sub";
        case Op::Mul:           return "mul";
        case Op::Div:           return "div";
        case Op::Mod:           return "mod";
        case Op::Exp:           return "exp";
        case Op::Subscript:     return "subscript";
        case Op::LogicalNot:    return "not";
        case Op::BitwiseNot:    return "bit_not";
        case Op::UnaryMinus:    return "neg";
        case Op::UnaryPlus:     return nullptr;
        case Op::DotCall:       return nullptr;
    }
    XCI_UNREACHABLE;
}


BuiltinModule::BuiltinModule(ModuleManager& module_manager) : Module(module_manager, "builtin")
{
    get_main_function().signature().set_return_type(ti_void());
    get_main_function().set_code();
    symtab().add({"void", Symbol::Value, add_value(TypedValue{ti_void()})});
    symtab().add({"false", Symbol::Value, add_value(TypedValue{value::Bool(false)})});
    symtab().add({"true", Symbol::Value, add_value(TypedValue{value::Bool(true)})});
    add_intrinsics();
    add_types();
    add_string_functions();
    add_io_functions();
    add_introspections();
}


void BuiltinModule::add_intrinsics()
{
    // directly write instructions to function code

    // no args
    symtab().add({"__noop", Symbol::Instruction, Index(Opcode::Noop)});
    symtab().add({"__logical_not", Symbol::Instruction, Index(Opcode::LogicalNot)});
    symtab().add({"__logical_or", Symbol::Instruction, Index(Opcode::LogicalOr)});
    symtab().add({"__logical_and", Symbol::Instruction, Index(Opcode::LogicalAnd)});
    symtab().add({"__bitwise_not_8", Symbol::Instruction, Index(Opcode::BitwiseNot_8)});
    symtab().add({"__bitwise_not_32", Symbol::Instruction, Index(Opcode::BitwiseNot_32)});
    symtab().add({"__bitwise_not_64", Symbol::Instruction, Index(Opcode::BitwiseNot_64)});
    symtab().add({"__bitwise_or_8", Symbol::Instruction, Index(Opcode::BitwiseOr_8)});
    symtab().add({"__bitwise_or_32", Symbol::Instruction, Index(Opcode::BitwiseOr_32)});
    symtab().add({"__bitwise_or_64", Symbol::Instruction, Index(Opcode::BitwiseOr_64)});
    symtab().add({"__bitwise_and_8", Symbol::Instruction, Index(Opcode::BitwiseAnd_8)});
    symtab().add({"__bitwise_and_32", Symbol::Instruction, Index(Opcode::BitwiseAnd_32)});
    symtab().add({"__bitwise_and_64", Symbol::Instruction, Index(Opcode::BitwiseAnd_64)});
    symtab().add({"__bitwise_xor_8", Symbol::Instruction, Index(Opcode::BitwiseXor_8)});
    symtab().add({"__bitwise_xor_32", Symbol::Instruction, Index(Opcode::BitwiseXor_32)});
    symtab().add({"__bitwise_xor_64", Symbol::Instruction, Index(Opcode::BitwiseXor_64)});

    // one arg
    symtab().add({"__equal", Symbol::Instruction, Index(Opcode::Equal)});
    symtab().add({"__not_equal", Symbol::Instruction, Index(Opcode::NotEqual)});
    symtab().add({"__less_equal", Symbol::Instruction, Index(Opcode::LessEqual)});
    symtab().add({"__greater_equal", Symbol::Instruction, Index(Opcode::GreaterEqual)});
    symtab().add({"__less_than", Symbol::Instruction, Index(Opcode::LessThan)});
    symtab().add({"__greater_than", Symbol::Instruction, Index(Opcode::GreaterThan)});
    symtab().add({"__shift_left", Symbol::Instruction, Index(Opcode::ShiftLeft)});
    symtab().add({"__shift_right", Symbol::Instruction, Index(Opcode::ShiftRight)});
    symtab().add({"__neg", Symbol::Instruction, Index(Opcode::Neg)});
    symtab().add({"__add", Symbol::Instruction, Index(Opcode::Add)});
    symtab().add({"__sub", Symbol::Instruction, Index(Opcode::Sub)});
    symtab().add({"__mul", Symbol::Instruction, Index(Opcode::Mul)});
    symtab().add({"__div", Symbol::Instruction, Index(Opcode::Div)});
    symtab().add({"__mod", Symbol::Instruction, Index(Opcode::Mod)});
    symtab().add({"__exp", Symbol::Instruction, Index(Opcode::Exp)});
    symtab().add({"__load_static", Symbol::Instruction, Index(Opcode::LoadStatic)});
    symtab().add({"__subscript", Symbol::Instruction, Index(Opcode::Subscript)});
    symtab().add({"__length", Symbol::Instruction, Index(Opcode::Length)});
    symtab().add({"__slice", Symbol::Instruction, Index(Opcode::Slice)});
    symtab().add({"__cast", Symbol::Instruction, Index(Opcode::Cast)});

    // two args
    symtab().add({"__copy", Symbol::Instruction, Index(Opcode::Copy)});
    symtab().add({"__drop", Symbol::Instruction, Index(Opcode::Drop)});
    /*
    // not yet found any use for these, uncomment when needed
    symtab().add({"__execute", Symbol::Instruction, Index(Opcode::Execute)});
    symtab().add({"__load_module", Symbol::Instruction, Index(Opcode::LoadModule)});
    symtab().add({"__load_function", Symbol::Instruction, Index(Opcode::LoadFunction)});
    symtab().add({"__call0", Symbol::Instruction, Index(Opcode::Call0)});
    symtab().add({"__call1", Symbol::Instruction, Index(Opcode::Call1)});
    symtab().add({"__partial_execute", Symbol::Instruction, Index(Opcode::PartialExecute)});
    symtab().add({"__make_closure", Symbol::Instruction, Index(Opcode::MakeClosure)});
    symtab().add({"__inc_ref", Symbol::Instruction, Index(Opcode::IncRef)});
    symtab().add({"__dec_ref", Symbol::Instruction, Index(Opcode::DecRef)});
    symtab().add({"__jump", Symbol::Instruction, Index(Opcode::Jump)});
    symtab().add({"__jump_if_not", Symbol::Instruction, Index(Opcode::JumpIfNot)});
    symtab().add({"__invoke", Symbol::Instruction, Index(Opcode::Invoke)});
    symtab().add({"__call", Symbol::Instruction, Index(Opcode::Call)});
    symtab().add({"__partial0", Symbol::Instruction, Index(Opcode::Partial0)});
    symtab().add({"__partial1", Symbol::Instruction, Index(Opcode::Partial1)});
    symtab().add({"__make_list", Symbol::Instruction, Index(Opcode::MakeList)});
    symtab().add({"__partial", Symbol::Instruction, Index(Opcode::Partial)});
    */

    // `__type_index<Int>` is index of Int type
    symtab().add({"__type_index", Symbol::TypeIndex});
    // `__value 42` is index of static value 42 (e.g. `__load_static (__value 42)`)
    symtab().add({"__value", Symbol::Value});
}


void BuiltinModule::add_types()
{
    symtab().add({"Void", Symbol::TypeName, add_type(ti_void())});
    symtab().add({"Bool", Symbol::TypeName, add_type(ti_bool())});
    symtab().add({"Byte", Symbol::TypeName, add_type(ti_byte())});
    symtab().add({"Char", Symbol::TypeName, add_type(ti_char())});
    symtab().add({"UInt", Symbol::TypeName, add_type(ti_uint32())});
    symtab().add({"UInt32", Symbol::TypeName, add_type(ti_uint32())});
    symtab().add({"UInt64", Symbol::TypeName, add_type(ti_uint64())});
    symtab().add({"Int", Symbol::TypeName, add_type(ti_int32())});
    symtab().add({"Int32", Symbol::TypeName, add_type(ti_int32())});
    symtab().add({"Int64", Symbol::TypeName, add_type(ti_int64())});
    symtab().add({"Float", Symbol::TypeName, add_type(ti_float32())});
    symtab().add({"Float32", Symbol::TypeName, add_type(ti_float32())});
    symtab().add({"Float64", Symbol::TypeName, add_type(ti_float64())});
    symtab().add({"String", Symbol::TypeName, add_type(ti_string())});
    symtab().add({"TypeIndex", Symbol::TypeName, add_type(ti_type_index())});
}


static void cast_string_to_chars(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::String>();
    auto utf32 = core::to_utf32(in.value());
    in.decref();
    Value out(ListV(utf32.length(), ti_char(), reinterpret_cast<const std::byte*>(utf32.data())));
    stack.push(out);
}


static void cast_string_to_bytes(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::String>();
    auto utf8 = in.value();
    in.decref();
    Value out(ListV(utf8.length(), ti_byte(), reinterpret_cast<const std::byte*>(utf8.data())));
    stack.push(out);
}


static void cast_chars_to_string(Stack& stack, void*, void*)
{
    auto in = stack.pull(ti_chars());
    const auto * data = in.get<ListV>().raw_data();
    const auto size = in.get<ListV>().length();
    auto utf8 = core::to_utf8(std::u32string_view(reinterpret_cast<const char32_t*>(data), size));
    in.decref();
    stack.push(value::String{utf8});
}


static void cast_bytes_to_string(Stack& stack, void*, void*)
{
    auto in = stack.pull(ti_bytes());
    const auto * data = in.get<ListV>().raw_data();
    const auto size = in.get<ListV>().length();
    value::String out{std::string_view(reinterpret_cast<const char*>(data), size)};
    in.decref();
    stack.push(out);
}


static void string_equal(Stack& stack, void*, void*)
{
    auto s1 = stack.pull<value::String>();
    auto s2 = stack.pull<value::String>();
    bool res;
    if (s1.get<StringV>().slot == s2.get<StringV>().slot)
        res = true;  // same instance on heap
    else
        res = s1.value() == s2.value();
    s1.decref();
    s2.decref();
    stack.push(value::Bool(res));
}


static void string_compare(Stack& stack, void*, void*)
{
    auto s1 = stack.pull<value::String>();
    auto s2 = stack.pull<value::String>();
    int32_t res;
    if (s1.get<StringV>().slot == s2.get<StringV>().slot)
        res = 0;  // same instance on heap
    else
        res = s1.value().compare(s2.value());
    s1.decref();
    s2.decref();
    stack.push(value::Int32(res));
}


void BuiltinModule::add_string_functions()
{
    add_native_function("cast_to_chars", {ti_string()}, ti_chars(), cast_string_to_chars);
    add_native_function("cast_to_bytes", {ti_string()}, ti_bytes(), cast_string_to_bytes);
    add_native_function("cast_to_string", {ti_chars()}, ti_string(), cast_chars_to_string);
    add_native_function("cast_to_string", {ti_bytes()}, ti_string(), cast_bytes_to_string);

    add_native_function("string_equal", {ti_string(), ti_string()}, ti_bool(), string_equal);
    add_native_function("string_compare", {ti_string(), ti_string()}, ti_int32(), string_compare);
    add_native_function("string_concat", [](std::string_view a, std::string_view b) -> std::string { return std::string(a) + std::string(b); });
}


static void write_bytes(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::Bytes>();
    stack.stream_out().write(arg.value());
    arg.decref();
}


static void write_string(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::String>();
    stack.stream_out().write(arg.value());
    arg.decref();
}


static void flush_out(Stack& stack, void*, void*)
{
    stack.stream_out().flush();
}


static void write_error(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::String>();
    stack.stream_err().write(arg.value());
    arg.decref();
}


static void read_string(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::Int32>();
    auto s = stack.stream_in().read(arg.value());
    stack.push(value::String(s));
}


static void open_file(Stack& stack, void*, void*)
{
    auto path = stack.pull<value::String>();
    std::string path_s {path.value()};
    path.decref();

    auto flags = stack.pull<value::String>();
    std::string flags_s {flags.value()};
    flags.decref();

    FILE* f = fopen(path_s.c_str(), flags_s.c_str());

    stack.push(value::Stream(script::Stream{script::Stream::CFile{f}}));
}


static void internal_streams(Stack& stack, void*, void*)
{
    stack.push(value::Tuple{stack.get_stream_in(), stack.get_stream_out(), stack.get_stream_err()});
}


static void output_stream_enter1(Stack& stack, void*, void*)
{
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_out(out);
    stack.push(out);  // return the original stream -> pass it to leave function
}


static void output_stream_leave1(Stack& stack, void*, void*)
{
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_out(out);
    out.decref();  // dispose the stream which was used inside the context
}


static void output_stream_enter2(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_in(in);
    stack.swap_stream_out(out);
    stack.push(value::Tuple{in, out});
}


static void output_stream_leave2(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_in(in);
    stack.swap_stream_out(out);
    in.decref();
    out.decref();
}


static void output_stream_enter3(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    auto err = stack.pull<value::Stream>();
    // undef value can be passed via incomplete Streams struct - keep original stream
    if (in.value())
        stack.swap_stream_in(in);
    if (out.value())
        stack.swap_stream_out(out);
    if (err.value())
        stack.swap_stream_err(err);
    stack.push(value::Tuple{in, out, err});
}


static void output_stream_leave3(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    auto err = stack.pull<value::Stream>();
    // undef on stack means the stream wasn't swapped - see above (enter3)
    if (in.value())
        stack.swap_stream_in(in);
    if (out.value())
        stack.swap_stream_out(out);
    if (err.value())
        stack.swap_stream_err(err);
    in.decref();
    out.decref();
    err.decref();
}


void BuiltinModule::add_io_functions()
{
    // types
    auto streams = ti_struct({
            {"in", ti_stream()},
            {"out", ti_stream()},
            {"err", ti_stream()}
    });
    symtab().add({"Streams", Symbol::TypeName, add_type(streams)});

    // values
    symtab().add({"stdin", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdin())})});
    symtab().add({"stdout", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdout())})});
    symtab().add({"stderr", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stderr())})});
    symtab().add({"null", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::null())})});

    // functions
    add_native_function("write", {ti_string()}, ti_void(), write_string);
    add_native_function("write", {ti_bytes()}, ti_void(), write_bytes);
    add_native_function("flush", {}, ti_void(), flush_out);
    add_native_function("error", {ti_string()}, ti_void(), write_error);
    add_native_function("read", {ti_int32()}, ti_string(), read_string);
    add_native_function("open", {ti_string(), ti_string()}, ti_stream(), open_file);
    add_native_function("__streams", {}, TypeInfo(streams), internal_streams);

    add_native_function("enter", {ti_stream()}, ti_stream(), output_stream_enter1);
    add_native_function("leave", {ti_stream()}, ti_void(), output_stream_leave1);
    add_native_function("enter", {ti_tuple(ti_stream(), ti_stream())}, ti_tuple(ti_stream(), ti_stream()), output_stream_enter2);
    add_native_function("leave", {ti_tuple(ti_stream(), ti_stream())}, ti_void(), output_stream_leave2);
    add_native_function("enter", {ti_tuple(ti_stream(), ti_stream(), ti_stream())}, ti_tuple(ti_stream(), ti_stream(), ti_stream()), output_stream_enter3);
    add_native_function("leave", {ti_tuple(ti_stream(), ti_stream(), ti_stream())}, ti_void(), output_stream_leave3);
    add_native_function("enter", {streams}, TypeInfo(streams), output_stream_enter3);
    add_native_function("leave", {streams}, ti_void(), output_stream_leave3);
}


static void introspect_module(Stack& stack, void*, void*)
{
    stack.push(value::Module{stack.frame().function.module()});
}


static void introspect_type_size(Stack& stack, void*, void*)
{
    auto type_idx = stack.pull<value::Int32>().value();
    const TypeInfo& ti = get_type_info(stack.module_manager(), type_idx);
    stack.push(value::Int32{(int32_t)ti.size()});
}


static void introspect_type_name(Stack& stack, void*, void*)
{
    auto type_idx = stack.pull<value::Int32>().value();
    const TypeInfo& ti = get_type_info(stack.module_manager(), type_idx);
    std::ostringstream os;
    os << ti;
    stack.push(value::String{os.str()});
}


static void introspect_underlying_type(Stack& stack, void*, void*)
{
    auto type_idx = stack.pull<value::Int32>().value();
    const ModuleManager& mm = stack.module_manager();
    const TypeInfo& ti = get_type_info(mm, type_idx);
    stack.push(value::TypeIndex{int32_t(get_type_index(mm, ti.underlying()))});
}


static void introspect_subtypes(Stack& stack, void*, void*)
{
    auto type_idx = stack.pull<value::Int32>().value();
    const ModuleManager& mm = stack.module_manager();
    const TypeInfo& ti = get_type_info(mm, type_idx);
    if (ti.is_tuple() || ti.is_struct()) {
        const auto& subtypes = ti.struct_or_tuple_subtypes();
        value::List res(subtypes.size(), ti_string());
        for (const auto& [i, sub] : subtypes | enumerate) {
            std::ostringstream os;
            os << sub;
            res.set_value(i, value::String(os.str()));
        }
        stack.push(res);
        return;
    }
    value::List res(1, ti_type_index());
    if (ti.is_list()) {
        res.set_value(0, value::TypeIndex(int32_t(get_type_index(mm, ti.elem_type()))));
    } else {
        res.set_value(0, value::TypeIndex(int32_t(get_type_index(mm, ti))));
    }
    stack.push(res);
}


void BuiltinModule::add_introspections()
{
    add_native_function("__type_size", {ti_type_index()}, ti_int32(), introspect_type_size);
    add_native_function("__type_name", {ti_type_index()}, ti_string(), introspect_type_name);
    add_native_function("__underlying_type", {ti_type_index()}, ti_type_index(), introspect_underlying_type);
    add_native_function("__subtypes", {ti_type_index()}, ti_list(ti_type_index()), introspect_subtypes);
    // return the builtin module
    add_native_function("__builtin",
            [](void* m) -> Module& { return *static_cast<Module*>(m); },
            this);
    // return current module
    add_native_function("__module", {}, ti_module(), introspect_module);
    // get number of functions in a module
    add_native_function("__n_fn", [](Module& m) { return (int) m.num_functions(); });
    add_native_function("__n_types", [](Module& m) { return (int) m.num_types(); });
}


} // namespace xci::script
