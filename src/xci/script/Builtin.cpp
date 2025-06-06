// Builtin.cpp created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Builtin.h"
#include "dump.h"
#include <xci/core/string.h>
#include <xci/compat/macros.h>
#include <xci/script/typing/type_index.h>
#include <xci/script/Error.h>

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
        case Op::Call:          return nullptr;
    }
    XCI_UNREACHABLE;
}


BuiltinModule::BuiltinModule(ModuleManager& module_manager) : Module(module_manager, intern("builtin"))
{
    get_main_function().signature().set_parameter(ti_void());
    get_main_function().signature().set_return_type(ti_void());
    get_main_function().set_bytecode();
    get_main_function().bytecode().add_opcode(Opcode::Ret);
    add_symbol("void", Symbol::Value, add_value(TypedValue{ti_void()}));
    add_symbol("false", Symbol::Value, add_value(TypedValue{value::Bool(false)}));
    add_symbol("true", Symbol::Value, add_value(TypedValue{value::Bool(true)}));
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
    add_symbol("__noop", Symbol::Instruction, Index(Opcode::Noop));
    add_symbol("__logical_not", Symbol::Instruction, Index(Opcode::LogicalNot));
    add_symbol("__logical_or", Symbol::Instruction, Index(Opcode::LogicalOr));
    add_symbol("__logical_and", Symbol::Instruction, Index(Opcode::LogicalAnd));
    add_symbol("__bitwise_not_8", Symbol::Instruction, Index(Opcode::BitwiseNot_8));
    add_symbol("__bitwise_not_16", Symbol::Instruction, Index(Opcode::BitwiseNot_16));
    add_symbol("__bitwise_not_32", Symbol::Instruction, Index(Opcode::BitwiseNot_32));
    add_symbol("__bitwise_not_64", Symbol::Instruction, Index(Opcode::BitwiseNot_64));
    add_symbol("__bitwise_not_128", Symbol::Instruction, Index(Opcode::BitwiseNot_128));
    add_symbol("__bitwise_or_8", Symbol::Instruction, Index(Opcode::BitwiseOr_8));
    add_symbol("__bitwise_or_16", Symbol::Instruction, Index(Opcode::BitwiseOr_16));
    add_symbol("__bitwise_or_32", Symbol::Instruction, Index(Opcode::BitwiseOr_32));
    add_symbol("__bitwise_or_64", Symbol::Instruction, Index(Opcode::BitwiseOr_64));
    add_symbol("__bitwise_or_128", Symbol::Instruction, Index(Opcode::BitwiseOr_128));
    add_symbol("__bitwise_and_8", Symbol::Instruction, Index(Opcode::BitwiseAnd_8));
    add_symbol("__bitwise_and_16", Symbol::Instruction, Index(Opcode::BitwiseAnd_16));
    add_symbol("__bitwise_and_32", Symbol::Instruction, Index(Opcode::BitwiseAnd_32));
    add_symbol("__bitwise_and_64", Symbol::Instruction, Index(Opcode::BitwiseAnd_64));
    add_symbol("__bitwise_and_128", Symbol::Instruction, Index(Opcode::BitwiseAnd_128));
    add_symbol("__bitwise_xor_8", Symbol::Instruction, Index(Opcode::BitwiseXor_8));
    add_symbol("__bitwise_xor_16", Symbol::Instruction, Index(Opcode::BitwiseXor_16));
    add_symbol("__bitwise_xor_32", Symbol::Instruction, Index(Opcode::BitwiseXor_32));
    add_symbol("__bitwise_xor_64", Symbol::Instruction, Index(Opcode::BitwiseXor_64));
    add_symbol("__bitwise_xor_128", Symbol::Instruction, Index(Opcode::BitwiseXor_128));
    add_symbol("__shift_left_8", Symbol::Instruction, Index(Opcode::ShiftLeft_8));
    add_symbol("__shift_left_16", Symbol::Instruction, Index(Opcode::ShiftLeft_16));
    add_symbol("__shift_left_32", Symbol::Instruction, Index(Opcode::ShiftLeft_32));
    add_symbol("__shift_left_64", Symbol::Instruction, Index(Opcode::ShiftLeft_64));
    add_symbol("__shift_left_128", Symbol::Instruction, Index(Opcode::ShiftLeft_128));
    add_symbol("__shift_right_8", Symbol::Instruction, Index(Opcode::ShiftRight_8));
    add_symbol("__shift_right_16", Symbol::Instruction, Index(Opcode::ShiftRight_16));
    add_symbol("__shift_right_32", Symbol::Instruction, Index(Opcode::ShiftRight_32));
    add_symbol("__shift_right_64", Symbol::Instruction, Index(Opcode::ShiftRight_64));
    add_symbol("__shift_right_128", Symbol::Instruction, Index(Opcode::ShiftRight_128));
    add_symbol("__shift_right_se_8", Symbol::Instruction, Index(Opcode::ShiftRightSE_8));
    add_symbol("__shift_right_se_16", Symbol::Instruction, Index(Opcode::ShiftRightSE_16));
    add_symbol("__shift_right_se_32", Symbol::Instruction, Index(Opcode::ShiftRightSE_32));
    add_symbol("__shift_right_se_64", Symbol::Instruction, Index(Opcode::ShiftRightSE_64));
    add_symbol("__shift_right_se_128", Symbol::Instruction, Index(Opcode::ShiftRightSE_128));

    // one arg
    add_symbol("__equal", Symbol::Instruction, Index(Opcode::Equal));
    add_symbol("__not_equal", Symbol::Instruction, Index(Opcode::NotEqual));
    add_symbol("__less_equal", Symbol::Instruction, Index(Opcode::LessEqual));
    add_symbol("__greater_equal", Symbol::Instruction, Index(Opcode::GreaterEqual));
    add_symbol("__less_than", Symbol::Instruction, Index(Opcode::LessThan));
    add_symbol("__greater_than", Symbol::Instruction, Index(Opcode::GreaterThan));
    add_symbol("__neg", Symbol::Instruction, Index(Opcode::Neg));
    add_symbol("__add", Symbol::Instruction, Index(Opcode::Add));
    add_symbol("__sub", Symbol::Instruction, Index(Opcode::Sub));
    add_symbol("__mul", Symbol::Instruction, Index(Opcode::Mul));
    add_symbol("__div", Symbol::Instruction, Index(Opcode::Div));
    add_symbol("__mod", Symbol::Instruction, Index(Opcode::Mod));
    add_symbol("__exp", Symbol::Instruction, Index(Opcode::Exp));
    add_symbol("__unsafe_add", Symbol::Instruction, Index(Opcode::UnsafeAdd));
    add_symbol("__unsafe_sub", Symbol::Instruction, Index(Opcode::UnsafeSub));
    add_symbol("__unsafe_mul", Symbol::Instruction, Index(Opcode::UnsafeMul));
    add_symbol("__unsafe_div", Symbol::Instruction, Index(Opcode::UnsafeDiv));
    add_symbol("__unsafe_mod", Symbol::Instruction, Index(Opcode::UnsafeMod));
    add_symbol("__load_static", Symbol::Instruction, Index(Opcode::LoadStatic));
    add_symbol("__list_subscript", Symbol::Instruction, Index(Opcode::ListSubscript));
    add_symbol("__list_length", Symbol::Instruction, Index(Opcode::ListLength));
    add_symbol("__list_slice", Symbol::Instruction, Index(Opcode::ListSlice));
    add_symbol("__list_concat", Symbol::Instruction, Index(Opcode::ListConcat));
    add_symbol("__cast", Symbol::Instruction, Index(Opcode::Cast));

    // two args
    add_symbol("__copy", Symbol::Instruction, Index(Opcode::Copy));
    add_symbol("__drop", Symbol::Instruction, Index(Opcode::Drop));
    /*
    // not yet found any use for these, uncomment when needed
    add_symbol("__execute", Symbol::Instruction, Index(Opcode::Execute));
    add_symbol("__load_module", Symbol::Instruction, Index(Opcode::LoadModule));
    add_symbol("__load_function", Symbol::Instruction, Index(Opcode::LoadFunction));
    add_symbol("__call0", Symbol::Instruction, Index(Opcode::Call0));
    add_symbol("__call1", Symbol::Instruction, Index(Opcode::Call1));
    add_symbol("__make_closure", Symbol::Instruction, Index(Opcode::MakeClosure));
    add_symbol("__inc_ref", Symbol::Instruction, Index(Opcode::IncRef));
    add_symbol("__dec_ref", Symbol::Instruction, Index(Opcode::DecRef));
    add_symbol("__jump", Symbol::Instruction, Index(Opcode::Jump));
    add_symbol("__jump_if_not", Symbol::Instruction, Index(Opcode::JumpIfNot));
    add_symbol("__invoke", Symbol::Instruction, Index(Opcode::Invoke));
    add_symbol("__call", Symbol::Instruction, Index(Opcode::Call));
    add_symbol("__make_list", Symbol::Instruction, Index(Opcode::MakeList));
    */

    // `__module` is current Module, `__module 1` is imported module by index 1
    add_symbol("__module", Symbol::Module);
    // `__type_index<Int>` is index of Int type
    add_symbol("__type_index", Symbol::TypeIndex);
    // `__value 42` is index of static value 42 (e.g. `__load_static (__value 42)`)
    add_symbol("__value", Symbol::Value);
}


void BuiltinModule::add_types()
{
    add_symbol("Void", Symbol::TypeName, add_type(ti_void()));
    add_symbol("Bool", Symbol::TypeName, add_type(ti_bool()));
    add_symbol("Byte", Symbol::TypeName, add_type(ti_byte()));
    add_symbol("Char", Symbol::TypeName, add_type(ti_char()));

    add_symbol("UInt8", Symbol::TypeName, add_type(ti_uint8()));
    add_symbol("UInt16", Symbol::TypeName, add_type(ti_uint16()));
    add_symbol("UInt32", Symbol::TypeName, add_type(ti_uint32()));
    add_symbol("UInt64", Symbol::TypeName, add_type(ti_uint64()));
    add_symbol("UInt128", Symbol::TypeName, add_type(ti_uint128()));
    add_symbol("UInt", Symbol::TypeName, add_type(ti_uint()));

    add_symbol("Int8", Symbol::TypeName, add_type(ti_int8()));
    add_symbol("Int16", Symbol::TypeName, add_type(ti_int16()));
    add_symbol("Int32", Symbol::TypeName, add_type(ti_int32()));
    add_symbol("Int64", Symbol::TypeName, add_type(ti_int64()));
    add_symbol("Int128", Symbol::TypeName, add_type(ti_int128()));
    add_symbol("Int", Symbol::TypeName, add_type(ti_int()));

    add_symbol("Float32", Symbol::TypeName, add_type(ti_float32()));
    add_symbol("Float64", Symbol::TypeName, add_type(ti_float64()));
    add_symbol("Float128", Symbol::TypeName, add_type(ti_float128()));
    add_symbol("Float", Symbol::TypeName, add_type(ti_float()));

    add_symbol("String", Symbol::TypeName, add_type(ti_string()));
    add_symbol("Bytes", Symbol::TypeName, add_type(ti_bytes()));
    add_symbol("Module", Symbol::TypeName, add_type(ti_module()));
    add_symbol("TypeIndex", Symbol::TypeName, add_type(ti_type_index()));

    // -------------------------------------------------------------------------
    // Types for interfacing with C

    // These types also carry the C-like unsafe behavior, e.g. unchecked integer overflow.
    [[maybe_unused]] Index cuint8 = add_named_type("CUInt8", ti_uint8());
    [[maybe_unused]] Index cuint16 = add_named_type("CUInt16", ti_uint16());
    [[maybe_unused]] Index cuint32 = add_named_type("CUInt32", ti_uint32());
    [[maybe_unused]] Index cuint64 = add_named_type("CUInt64", ti_uint64());
    [[maybe_unused]] Index cint8 = add_named_type("CInt8", ti_int8());
    [[maybe_unused]] Index cint16 = add_named_type("CInt16", ti_int16());
    [[maybe_unused]] Index cint32 = add_named_type("CInt32", ti_int32());
    [[maybe_unused]] Index cint64 = add_named_type("CInt64", ti_int64());

    // Variably-sized type aliases
    static_assert(sizeof(int) == 4);
    add_symbol("CInt", Symbol::TypeName, cint32);
    add_symbol("CUInt", Symbol::TypeName, cuint32);
#if UINTPTR_MAX == UINT64_MAX
    static_assert(sizeof(size_t) == 8);
    add_symbol("COffset", Symbol::TypeName, cint64);
    add_symbol("CSize", Symbol::TypeName, cuint64);
#else
    static_assert(sizeof(size_t) == 4);
    add_symbol("COffset", Symbol::TypeName, cint32);
    add_symbol("CSize", Symbol::TypeName, cuint32);
#endif
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
    const auto* data = in.get<ListV>().raw_data();
    const auto size = in.get<ListV>().length();
    const std::u32string u32s(reinterpret_cast<const char32_t*>(data), size);
    const auto utf8 = core::to_utf8(u32s);
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
    int res;
    if (s1.get<StringV>().slot == s2.get<StringV>().slot)
        res = 0;  // same instance on heap
    else
        res = s1.value().compare(s2.value());
    s1.decref();
    s2.decref();
    stack.push(value::Int(res));
}


void BuiltinModule::add_string_functions()
{
    add_native_function("cast_to_chars", ti_string(), ti_chars(), cast_string_to_chars);
    add_native_function("cast_to_bytes", ti_string(), ti_bytes(), cast_string_to_bytes);
    add_native_function("cast_to_string", ti_chars(), ti_string(), cast_chars_to_string);
    add_native_function("cast_to_string", ti_bytes(), ti_string(), cast_bytes_to_string);

    add_native_function("string_equal", ti_tuple(ti_string(), ti_string()), ti_bool(), string_equal);
    add_native_function("string_compare", ti_tuple(ti_string(), ti_string()), ti_int(), string_compare);
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
    auto arg = stack.pull<value::UInt>();
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
            ti_key("in", ti_stream()),
            ti_key("out", ti_stream()),
            ti_key("err", ti_stream())
    });
    add_symbol("Streams", Symbol::TypeName, add_type(streams));

    // values
    add_symbol("stdin", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdin())}));
    add_symbol("stdout", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdout())}));
    add_symbol("stderr", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stderr())}));
    add_symbol("null", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::null())}));

    // functions
    add_native_function("write", ti_string(), ti_void(), write_string);
    add_native_function("write", ti_bytes(), ti_void(), write_bytes);
    add_native_function("flush", ti_void(), ti_void(), flush_out);
    add_native_function("error", ti_string(), ti_void(), write_error);
    add_native_function("read", ti_uint(), ti_string(), read_string);
    add_native_function("open", ti_tuple(ti_string(), ti_string()), ti_stream(), open_file);
    add_native_function("__streams", ti_void(), TypeInfo(streams), internal_streams);

    add_native_function("enter", ti_stream(), ti_stream(), output_stream_enter1);
    add_native_function("leave", ti_stream(), ti_void(), output_stream_leave1);
    add_native_function("enter", ti_tuple(ti_stream(), ti_stream()), ti_tuple(ti_stream(), ti_stream()), output_stream_enter2);
    add_native_function("leave", ti_tuple(ti_stream(), ti_stream()), ti_void(), output_stream_leave2);
    add_native_function("enter", ti_tuple(ti_stream(), ti_stream(), ti_stream()), ti_tuple(ti_stream(), ti_stream(), ti_stream()), output_stream_enter3);
    add_native_function("leave", ti_tuple(ti_stream(), ti_stream(), ti_stream()), ti_void(), output_stream_leave3);
    add_native_function("enter", TypeInfo(streams), TypeInfo(streams), output_stream_enter3);
    add_native_function("leave", TypeInfo(streams), ti_void(), output_stream_leave3);
}


static const TypeInfo& read_type_index(Stack& stack)
{
    const auto arg = stack.pull<value::Int32>().value();
    return get_type_info(stack.module_manager(),
                         arg < 0 ? no_index : Index(arg));
}


static void introspect_type_size(Stack& stack, void*, void*)
{
    const TypeInfo& ti = read_type_index(stack);
    stack.push(value::UInt{ti.size()});
}


static void introspect_type_name(Stack& stack, void*, void*)
{
    const TypeInfo& ti = read_type_index(stack);
    std::ostringstream os;
    os << ti;
    stack.push(value::String{os.str()});
}


static void introspect_underlying_type(Stack& stack, void*, void*)
{
    const TypeInfo& ti = read_type_index(stack);
    const ModuleManager& mm = stack.module_manager();
    stack.push(value::TypeIndex{get_type_index(mm, ti.underlying())});
}


static void introspect_subtypes(Stack& stack, void*, void*)
{
    const TypeInfo& ti = read_type_index(stack);
    if (ti.is_tuple() || ti.is_struct()) {
        const auto& subtypes = ti.subtypes();
        value::List res(subtypes.size(), ti_string());
        for (const auto& [i, sub] : subtypes | enumerate) {
            std::ostringstream os;
            os << sub;
            res.set_value(i, value::String(os.str()));
        }
        stack.push(res);
        return;
    }
    const ModuleManager& mm = stack.module_manager();
    value::List res(1, ti_type_index());
    if (ti.is_list()) {
        res.set_value(0, value::TypeIndex(int32_t(get_type_index(mm, ti.elem_type()))));
    } else {
        res.set_value(0, value::TypeIndex(int32_t(get_type_index(mm, ti))));
    }
    stack.push(res);
}


static void introspect_module_by_name(Stack& stack, void*, void*)
{
    auto name = stack.pull<value::String>();
    name.decref();
    const auto idx = stack.module().get_imported_module_index(intern(name.value()));
    if (idx == no_index)
        throw module_not_found(name.value());
    stack.push(value::Module{stack.module().get_imported_module(idx)});
}


static void introspect_stack_n_frames(Stack& stack, void*, void*)
{
    stack.push(value::UInt{stack.n_frames()});
}


void BuiltinModule::add_introspections()
{
    add_native_function("__type_size", ti_type_index(), ti_uint(), introspect_type_size);
    add_native_function("__type_name", ti_type_index(), ti_string(), introspect_type_name);
    add_native_function("__underlying_type", ti_type_index(), ti_type_index(), introspect_underlying_type);
    add_native_function("__subtypes", ti_type_index(), ti_list(ti_type_index()), introspect_subtypes);
    // return the builtin module
    add_native_function("__builtin",
            [](void* m) -> Module& { return *static_cast<Module*>(m); },
            this);
    add_native_function("__module_name", [](Module& m) -> std::string { return m.name().str(); });
    add_native_function("__module_by_name", ti_string(), ti_module(), introspect_module_by_name);
    add_native_function("__n_fn", [](Module& m) { return (uint64_t) m.num_functions(); });
    add_native_function("__n_types", [](Module& m) { return (uint64_t) m.num_types(); });
    add_native_function("__n_frames", ti_void(), ti_uint(), introspect_stack_n_frames);
}


} // namespace xci::script
