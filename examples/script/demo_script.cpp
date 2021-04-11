// demo_script.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/script/Interpreter.h>
#include <xci/script/NativeDelegate.h>
#include <fmt/core.h>
#include <string_view>
#include <cassert>

using namespace xci::script;


void hello_fun(Stack& stack, void*, void*)
{
    // pull arguments according to function signature
    auto arg = stack.pull<value::String>();

    // push return value (this can be done as early as we know the value)
    // (failing to pull/push expected values will cause hard-to-track errors
    // e.g. StackUnderflow at some later point in another function)
    stack.push(value::Int32{42});

    // here comes our native code
    fmt::print("Hello, {}!\n", arg.value());

    // some values live on heap - they need to be explicitly released
    // - normally, only the instances on the stack are counted
    // - by pulling the value from the stack, we removed one instance
    // - unless we're going to push it back as a result,
    //   its refcount needs to be decreased
    arg.decref();
}


static std::string toupper_at(std::string_view word, int32_t index)
{
    std::string res(word);
    auto i = unsigned(index);
    if (i < word.size())
        res[i] = toupper(res[i]);
    return res;
}


void toupper_at_wrapped(Stack& stack, void*, void*)
{
    auto arg1 = stack.pull<value::String>();
    auto arg2 = stack.pull<value::Int32>();
    arg1.decref();
    arg2.decref();
    auto result = toupper_at(arg1.value(), arg2.value());
    stack.push(value::String{result});
}


int main()
{
    // this is a convenient class which manages everything needed to interpret a script
    Interpreter interpreter;

    // create module with our native function
    Module module;

    // low level interface - the native function has to operate directly on Stack
    // and its signature is specified explicitly
    module.add_native_function(
            // symbolic name
            "hello",
            // signature: String -> Int32
            {TypeInfo{Type::String}}, TypeInfo{Type::Int32},
            // native function to be called
            hello_fun);

    // still low level interface
    module.add_native_function(
            "toupper_at_wrapped",
            {TypeInfo{Type::String}, TypeInfo{Type::Int32}},
            TypeInfo{Type::String},
            toupper_at_wrapped);
    // the same function with auto-generated wrapper function
    // (which is essentially the same as our manually written `toupper_at_wrapped`)
    module.add_native_function("toupper_at", toupper_at);

    // capture-less lambda works, too
    module.add_native_function("add2", [](int a, int b) { return a + b; });

    // lambda with capture and other function objects can't be passed directly,
    // but they can be wrapped in captureless lambda
    auto lambda_with_capture = [v=1](int a, int b) { return a + b + v; };
    module.add_native_function("add_v", [](void* l, int a, int b)
        { return (*static_cast<decltype(lambda_with_capture)*>(l)) (a, b); },
        &lambda_with_capture);

    // add our module as if it was imported by the script
    // (another possibility is to inject the function directly into `main_module`)
    interpreter.add_imported_module(module);

    // evaluate a script
    // (this one would give the same result: `add2 39 3`)
    auto result = interpreter.eval(R"(hello (toupper_at "world" 0))");

    // result contains value of the last expression in the script
    assert(result.type() == Type::Int32);
    assert(result.get<int32_t>() == 42);

    return 0;
}
