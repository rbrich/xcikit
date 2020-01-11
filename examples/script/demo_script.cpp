// demo_script.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/script/Interpreter.h>
#include <iostream>
#include <cassert>

using namespace xci::script;


void hello_fun(Stack& stack)
{
    // pull arguments according to function signature
    auto arg = stack.pull<value::String>();

    // push return value (this can be done as early as we know the value)
    // (failing to pull/push expected values will cause hard-to-track errors
    // e.g. StackUnderflow at some later point in another function)
    stack.push(value::Int32{42});

    // here comes our native code
    std::cout << "Hello, " << arg.value() << "!\n";

    // some values live on heap - they need to be explicitly released
    // - normally, only the instances on the stack are counted
    // - by pulling the value from the stack, we removed one instance
    // - unless we're going to push it back as a result,
    //   its refcount needs to be decreased
    arg.decref();
}


int main()
{
    // this is a convenient class which manages everything needed to interpret a script
    Interpreter interpreter;

    // create module with our native function
    Module module;
    module.add_native_function(
            // symbolic name
            "hello",
            // signature: String -> Int32
            {TypeInfo{Type::String}}, TypeInfo{Type::Int32},
            // native wrapper to be called
            hello_fun);

    // add our module as if it was imported by the script
    // (another possibility is to inject the function directly into `main_module`)
    interpreter.add_imported_module(module);

    // evaluate a simple script
    auto result = interpreter.eval(R"(hello "World")");

    // result contains value of the last expression in the script
    assert(result->type() == Type::Int32);
    assert(result->as<value::Int32>().value() == 42);

    return 0;
}
