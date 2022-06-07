// test_script.cpp created on 2019-05-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/script/Parser.h>
#include <xci/script/Interpreter.h>
#include <xci/script/Error.h>
#include <xci/script/Stack.h>
#include <xci/script/SymbolTable.h>
#include <xci/script/NativeDelegate.h>
#include <xci/script/ast/fold_tuple.h>
#include <xci/script/dump.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/config.h>

#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace xci::script;
using namespace xci::core;
using namespace std::literals::string_literals;


struct Context {
    Vfs vfs;
    Interpreter interpreter {vfs};

    Context() {
        Logger::init(Logger::Level::Warning);
        vfs.mount(XCI_SHARE);
    }
};


static Context& context()
{
    static Context context;
    return context;
}


// Check parsing into AST and dumping back to code
std::string parse(const std::string& input)
{
    SourceManager src_man;
    auto src_id = src_man.add_source("<input>", input);
    Parser parser {src_man};
    ast::Module ast;
    parser.parse(src_id, ast);
    fold_tuple(ast.body);
    std::ostringstream os;
    os << ast;
    return os.str();
}


// Format parser input/output by hand, don't use Catch2's automatic expansion.
// This is easier to read in the log and it also workarounds a bug in MSVC:
//    CHECK(parse(R"("escape sequences: \"\n\0\x12 ")", R"("escape sequences: \"\n\0\x12 ")"));
//    > error C2017: illegal escape sequence
//    > error C2146: syntax error: missing ')' before identifier 'n'
#define PARSE(input, expected)                          \
    do {                                                \
        auto output = parse(input);                     \
        INFO("Input:    " << indent((input), 12));      \
        INFO("Output:   " << indent((output), 12));     \
        INFO("Expected: " << indent((expected), 12));   \
        if ((output) == (expected))                     \
            SUCCEED();                                  \
        else                                            \
            FAIL();                                     \
    } while(false)


// Check parsing into AST, optimizing the AST, and dumping back to code
std::string optimize(const std::string& input)
{
    SourceManager src_man;
    auto src_id = src_man.add_source("<input>", input);

    ast::Module ast;
    Parser parser {src_man};
    parser.parse(src_id, ast);

    Context& ctx = context();
    Module module {ctx.interpreter.module_manager()};
    module.import_module("builtin");
    module.import_module("std");

    auto& symtab = module.symtab().add_child("main");
    Function fn {module, symtab};
    Compiler compiler(Compiler::Flags::O1);
    compiler.compile(fn, ast);

    std::ostringstream os;
    os << ast;
    return os.str();
}


std::string interpret(const std::string& input, bool import_std=false)
{
    Context& ctx = context();
    Module module {ctx.interpreter.module_manager()};
    module.import_module("builtin");
    if (import_std) {
        module.import_module("std");
    }

    UNSCOPED_INFO(input);
    std::ostringstream os;
    try {
        auto result = ctx.interpreter.eval(module, input, [&os](TypedValue&& invoked) {
            os << invoked << ';';
            invoked.decref();
        });
        os << result;
        result.decref();
        REQUIRE(ctx.interpreter.machine().stack().empty());
        REQUIRE(ctx.interpreter.machine().stack().n_frames() == 0);
    } catch (const ScriptError& e) {
        UNSCOPED_INFO("Exception: " << e.what() << "\n" << e.detail());
        REQUIRE(ctx.interpreter.machine().stack().empty());
        REQUIRE(ctx.interpreter.machine().stack().n_frames() == 0);
        throw;
    }
    return os.str();
}


std::string interpret_std(const std::string& input)
{
    return interpret(input, true);
}


#ifndef NDEBUG
TEST_CASE( "Analyze grammar", "[script][parser]" )
{
   REQUIRE(Parser::analyze_grammar() == 0);
}
#endif


TEST_CASE( "Comments", "[script][parser]" )
{
    CHECK(parse("a  // C-style comment") == "a");
    CHECK(parse("/**/a/*C++-style\n \ncomment*/+b/*\n*/") == "(a + b)");
}


TEST_CASE( "Optional semicolon", "[script][parser]" )
{
    CHECK(parse("a = 1") == parse("a = 1;"));
    CHECK(parse("a = 1\nb = 2\n") == parse("a = 1; b = 2;"));
    CHECK(parse("(\n 1\n  + \n2\n)\n\na = 1  // nl still counted\nb=2\nc=3") ==
          parse("(1+2); a=1; b=2; c=3"));  // newlines are allowed inside brackets
    CHECK(parse("40\n.add 2\n50\n.sub 8") == parse("40 .add 2; 50 .sub 8;"));  // dotcall can continue after linebreak
    CHECK(parse("a =\n1") == parse("a=1"));  // linebreak is allowed after '=' in definition
    CHECK(parse("1 + \n 2") == parse("1+2"));  // linebreak is allowed after infix operator
    CHECK(parse("add 1 \\\n 2") == parse("add 1 2"));  // newline can be escaped
    CHECK(parse("(add 1 \\\n 2)") == parse("(add 1 2)"));
    REQUIRE_THROWS_WITH(parse("a=1;;"),  // empty statement is not allowed, semicolon is only used as a separator
            Catch::Matchers::StartsWith("parse error: invalid syntax"));
}


TEST_CASE( "Values", "[script][parser]" )
{
    CHECK(parse("identifier") == "identifier");
    CHECK(parse("123") == "123");
    CHECK(parse("1.") == "1.0");
    CHECK(parse("1.23") == "1.23");
    CHECK(parse("42b") == "b'*'");  // byte (8-bit integer)
    CHECK(parse("b'B'") == "b'B'");
    PARSE(R"(b"bytes literal")", R"(b"bytes literal")");
    CHECK(parse("'c'") == "'c'");
    PARSE(R"("string literal")", R"("string literal")");
    PARSE(R"("escape sequences: \"\n\0\x12 ")", R"("escape sequences: \"\n\0\x12 ")");
    CHECK(parse("1,2,3") == "1, 2, 3");  // naked tuple
    PARSE(R"((1,2,"str"))", R"((1, 2, "str"))");  // bracketed tuple
    CHECK(parse("[1,2,3]") == "[1, 2, 3]");  // list
    CHECK(parse("[(1,2,3,4)]") == "[(1, 2, 3, 4)]");  // list with a tuple item
    CHECK(parse("[(1,2,3,4), 5]") == "[(1, 2, 3, 4), 5]");
}


TEST_CASE( "Raw strings", "[script][parser]" )
{
    PARSE(R"("""Hello""")", R"("Hello")");
    PARSE("\"\"\"\nHello\n\"\"\"", R"("Hello")");
    PARSE("\"\"\"\n  Hello\n  \"\"\"", R"("Hello")");
    PARSE("\"\"\"\n  Hello\n\"\"\"", R"("  Hello")");
    PARSE("\"\"\"\n    Hello\n  \"\"\"", R"("  Hello")");
    PARSE("\"\"\"\n  \\nHello\\0\n  \"\"\"", R"("\\nHello\\0")");
    PARSE("\"\"\"Hello\n\"\"\"", R"("Hello\n")");
    PARSE("\"\"\"\n  Hello\"\"\"", R"("\n  Hello")");
    PARSE("\"\"\"\n\nHello\n\n\"\"\"", R"("\nHello\n")");
    PARSE("\"\"\"\n  \\\"\"\"Hello\\\"\"\"\n  \"\"\"", R"("\"\"\"Hello\"\"\"")");
    PARSE("\"\"\"\n  \\\"\"\"\"Hello\\\"\"\"\"\n  \"\"\"", R"("\"\"\"\"Hello\"\"\"\"")");
    PARSE("\"\"\"\n  \\\"\"\"\n  \\\\\"\"\"\n  \\\\\\\"\"\"\n  \"\"\"",
                R"("\"\"\"\n\\\"\"\"\n\\\\\"\"\"")");
    CHECK_THROWS_AS(parse("\"\"\"\\\"\"\""), ParseError);
    PARSE("\"\"\"\n\\\n\"\"\"", R"("\\")");
}


TEST_CASE( "Parsing types", "[script][parser]")
{
    CHECK(parse("type MyInt = Int") == "type MyInt = Int");
    CHECK(parse("type MyList = [Int]") == "type MyList = [Int]");
    CHECK(parse("type MyTuple = (String, Int)") == "type MyTuple = (String, Int)");
    CHECK(parse("type MyTuple = String, Int") == "type MyTuple = (String, Int)");  // The round brackets are optional, added in AST dump for clarity
    CHECK(parse("type MyListOfTuples = [String, Int]") == "type MyListOfTuples = [(String, Int)]");
    CHECK(parse("type MyListOfTuples2 = [(String, Int), Int]") == "type MyListOfTuples2 = [((String, Int), Int)]");
    CHECK(parse("MyAlias = Int") == "MyAlias = Int");
    CHECK(parse("MyAlias2 = [Int]") == "MyAlias2 = [Int]");
}


TEST_CASE( "Trailing comma", "[script][parser]" )
{
    CHECK(parse("1,2,3,") == "1, 2, 3");
    CHECK(parse("[1,2,3,]") == "[1, 2, 3]");
    CHECK(parse("(1,2,3,)") == "(1, 2, 3)");
    CHECK_THROWS_AS(parse("1,2,3,,"), ParseError);  // two commas not allowed
    CHECK_THROWS_AS(parse("1,2,,3"), ParseError);
    CHECK_THROWS_AS(parse("(1,2,3,,)"), ParseError);
    CHECK_THROWS_AS(parse("[1,2,3,,]"), ParseError);
    CHECK_THROWS_AS(parse("[,]"), ParseError);
    CHECK(parse("([1,],[2,],[1,2,],)") == "([1], [2], [1, 2])");
    // multiline
    CHECK(parse("1,\n2,\n3,\n") == "1, 2, 3");  // expression continues on next line after operator
    CHECK(parse("1,;\n2,\n3,\n") == "1\n2, 3");  // semicolon splits the multiline expression
    CHECK(parse("(\n1,\n2,\n3,\n)") == "(1, 2, 3)");
    CHECK(parse("[\n1,\n2,\n3,\n]") == "[1, 2, 3]");
    CHECK(parse("[\n1\n,\n2\n,\n3\n,\n]") == "[1, 2, 3]");
}


TEST_CASE( "Operator precedence", "[script][parser]" )
{
    CHECK(parse("a+b") == "(a + b)");
    CHECK(parse("a + b*c") == "(a + (b * c))");
    CHECK(parse("a*b + c") == "((a * b) + c)");
    CHECK(parse("a + b*c + d") == "((a + (b * c)) + d)");
    CHECK(parse("a || b && c | d << e + f * g ** h") == "(a || (b && (c | (d << (e + (f * (g ** h)))))))");
    CHECK(parse("a ** b * c + d << e | f && g || h") == "(((((((a ** b) * c) + d) << e) | f) && g) || h)");
    CHECK(parse("a * b + c | d || e & f - g * h / i") == "((((a * b) + c) | d) || (e & (f - ((g * h) / i))))");
    CHECK(parse("a || b & c + d * e + f & g && h / i") == "(a || (((b & ((c + (d * e)) + f)) & g) && (h / i)))");
    // left associative:
    CHECK(parse("a + b + c + d") == "(((a + b) + c) + d)");
    // right associative:
    CHECK(parse("a ** b ** c ** d") == "(a ** (b ** (c ** d)))");
    // functions
    CHECK(parse("a fun b {} c") == "a fun b {} c");
    CHECK(parse("a (fun b {}) c") == "a (fun b {}) c");
    // function calls
    CHECK(interpret_std("succ 9 + larger 5 4 + 1") == "16");
    CHECK(interpret_std("(succ 9) + (larger 5 4) + 1") == "16");
    CHECK(interpret_std("succ 9 + 5 .larger 4 + 1") == "16");
    CHECK(interpret_std("1 .add 2 .mul 3") == "9");
    CHECK(interpret_std("(1 .add 2).mul 3") == "9");
    CHECK(interpret_std("1 .add (2 .mul 3)") == "7");
    CHECK(interpret_std("pred (neg (succ (14)))") == "-16");
    CHECK(interpret_std("14 .succ .neg .pred") == "-16");
    CHECK(interpret_std("(((14) .succ) .neg) .pred") == "-16");
}


TEST_CASE( "Dot call", "[script][parser]")
{
    CHECK(parse("12 . sign") == "(12 . sign)");
    CHECK(parse("12 .sign") == "(12 . sign)");
    CHECK_THROWS_AS(parse("12. sign"), ParseError);  // parses as 12. + ref name sign, which is not allowed
    CHECK_THROWS_AS(parse("12.sign"), ParseError);  // parses as 12. + type specifier "sign", which is not known
    CHECK(parse("(12).sign") == "((12) . sign)");  // use parentheses to disambiguate
}


TEST_CASE( "Type argument or comparison?", "[script][parser]" )
{
    PARSE("1 > 2", "(1 > 2)");
    PARSE("1 < 2", "(1 < 2)");
    PARSE("a<b> 3", "((a < b) > 3)");
    PARSE("a<T> 3", "a<T> 3");
}


TEST_CASE( "Value size on stack", "[script][machine]" )
{
    CHECK(Value().size_on_stack() == type_size_on_stack(Type::Void));
    CHECK(Value(false).size_on_stack() == type_size_on_stack(Type::Bool));
    CHECK(Value(0).size_on_stack() == type_size_on_stack(Type::Int32));
    CHECK(Value(int64_t{0}).size_on_stack() == type_size_on_stack(Type::Int64));
    CHECK(Value(0.0f).size_on_stack() == type_size_on_stack(Type::Float32));
    CHECK(Value(0.0).size_on_stack() == type_size_on_stack(Type::Float64));
    CHECK(Value(Value::StringTag{}).size_on_stack() == type_size_on_stack(Type::String));
    CHECK(Value(Value::ListTag{}).size_on_stack() == type_size_on_stack(Type::List));
    CHECK(Value(TypeInfo::Subtypes{}).size_on_stack() == type_size_on_stack(Type::Tuple));
    CHECK(Value(Value::ClosureTag{}).size_on_stack() == type_size_on_stack(Type::Function));
    CHECK(Value(Value::ModuleTag{}).size_on_stack() == type_size_on_stack(Type::Module));
}


TEST_CASE( "Stack grow", "[script][machine]" )
{
    xci::script::Stack stack(4);
    CHECK(stack.capacity() == 4);
    CHECK(stack.size() == 0);

    stack.push(value::Int32{73});
    CHECK(stack.size() == 4);
    CHECK(stack.capacity() == 4);

    stack.push(value::Int32{42});
    CHECK(stack.size() == 8);
    CHECK(stack.capacity() == 8);

    stack.push(value::Int32{333});
    CHECK(stack.size() == 12);
    CHECK(stack.capacity() == 16);

    CHECK(stack.pull<value::Int32>().value() == 333);
    CHECK(stack.pull<value::Int32>().value() == 42);
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.empty());
    CHECK(stack.capacity() == 16);
}


TEST_CASE( "Stack push/pull", "[script][machine]" )
{
    xci::script::Stack stack;

    CHECK(stack.empty());
    stack.push(value::Bool{true});
    CHECK(stack.size() == 1);
    stack.push(value::Int32{73});
    CHECK(stack.size() == 1+4);
    value::String str{"hello"};
    stack.push(str);
    CHECK(stack.size() == 1+4 + sizeof(void*));
    CHECK(stack.n_values() == 3);

    CHECK(stack.pull<value::String>().value() == "hello");
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.pull<value::Bool>().value() == true);  // NOLINT

    CHECK(str.heapslot()->refcount() == 1);
    str.decref();
}


TEST_CASE( "SymbolTable", "[script][compiler]" )
{
    SymbolTable symtab;

    auto alpha = symtab.add({"alpha", Symbol::Value});
    auto beta = symtab.add({"beta", Symbol::Value});
    auto gamma = symtab.add({"Gamma", Symbol::Instance});
    auto delta = symtab.add({"delta", Symbol::Value});

    CHECK(alpha == symtab.find_last_of("alpha", Symbol::Value));
    CHECK(beta == symtab.find_last_of("beta", Symbol::Value));
    CHECK(gamma == symtab.find_last_of("Gamma", Symbol::Instance));
    CHECK(delta == symtab.find_last_of("delta", Symbol::Value));
    CHECK(! symtab.find_last_of("zeta", Symbol::Value));
}


TEST_CASE( "Literals", "[script][interpreter]" )
{
    // Integer literal out of 32bit range is promoted to Int64
    CHECK(interpret("2147483647") == "2147483647");
    CHECK(interpret("2147483648") == "2147483648L");  // promoted
    CHECK(interpret("-2147483648") == "-2147483648");
    CHECK(interpret("-2147483649") == "-2147483649L");  // promoted
    CHECK(interpret("4294967295u") == "4294967295U");
    CHECK(interpret("4294967296u") == "4294967296UL");  // promoted
    CHECK(interpret("-1u") == "4294967295U");
    CHECK(interpret("-1ul") == "18446744073709551615UL");
    // Integer literal out of 64bit range doesn't compile
    CHECK(interpret("9223372036854775807L") == "9223372036854775807L");
    CHECK_THROWS_AS(interpret("9223372036854775808L"), ParseError);
    CHECK(interpret("-9223372036854775808L") == "-9223372036854775808L");
    CHECK_THROWS_AS(interpret("-9223372036854775809L"), ParseError);
    CHECK(interpret("18446744073709551615ul") == "18446744073709551615UL");
    CHECK_THROWS_AS(interpret("18446744073709551616UL"), ParseError);
    // Chars and strings (UTF-8)
    CHECK(interpret("'@'") == "'@'");
    CHECK(interpret("'웃'") == "'웃'"); // multi-byte UTF-8
    CHECK(interpret("\"hello\"") == "\"hello\"");
    CHECK(interpret("\"řečiště\"") == "\"řečiště\"");
    // Lists
    CHECK(interpret("[]") == "[]");  // no type -> [Void]
    CHECK(interpret_std("[]:[Void]") == "[]");  // same
    CHECK(interpret_std("[]:[Int]") == "[]");
    CHECK(interpret_std("[].len") == "0U");
    CHECK(interpret("[1,2,3]") == "[1, 2, 3]");
}


TEST_CASE( "Variables", "[script][interpreter]" )
{
    CHECK(interpret_std("a = 42; b = a; b") == "42");
    CHECK_THROWS_AS(interpret_std("a=1; a=\"asb\""), RedefinedName);
    CHECK_THROWS_AS(interpret_std("k = 1; k = k + 1"), RedefinedName);
    CHECK(interpret_std("m = 1; { m = 2; m }") == "2");
    CHECK_THROWS_AS(interpret("m = m"), MissingExplicitType);
    CHECK_THROWS_AS(interpret("m = {m}"), MissingExplicitType);
    CHECK_THROWS_AS(interpret("m = { m = m }"), MissingExplicitType);
    CHECK_THROWS_AS(interpret("m = { m = m }"), MissingExplicitType);
    CHECK_THROWS_AS(interpret_std("m = { m = m + 1 }"), MissingExplicitType);
    CHECK(interpret_std("m = { m = 1; m }; m") == "1");
    // "m = { m + 1 }" compiles fine, but infinitely recurses
    CHECK_THROWS_AS(interpret_std("a:[Char] = [1,2,3]"), DefinitionTypeMismatch);
}


TEST_CASE( "Expressions", "[script][interpreter]" )
{
    CHECK(interpret_std("add 1 2") == "3");
    CHECK(interpret_std("sub (add 1 2) 3") == "0");
    CHECK(interpret_std("sub (1 + 2) 3") == "0");
    CHECK(interpret_std("(1 + 2) - 3") == "0");
    CHECK(interpret_std("1 + 2 - 3") == "0");

    CHECK(interpret_std("1 + 6/5") == "2");
    CHECK(interpret_std("1 + 2 / 3 == 1 + (2 / 3)") == "true");
    CHECK(interpret_std("-(1 + 2)") == "-3");
    CHECK(interpret_std("1+1, {2+2}") == "(2, 4)");
    CHECK(interpret_std("f=fun a:Int {a+1}; [1, f 2]") == "[1, 3]");

    CHECK(interpret_std("16 >> 2") == "4");
    CHECK(interpret_std("-16 >> 2") == "-4");
    CHECK(interpret_std("16 << 3") == "128");
    CHECK(interpret_std("-16 << 3") == "-128");
    CHECK(interpret_std("32u >> 3u") == "4U");
    CHECK(interpret_std("4u << 3u") == "32U");
    CHECK(interpret_std("-1u >> 20u") == "4095U");  // -1u = 2**32-1
    CHECK(interpret_std("-1u << 31u") == "2147483648U");

    CHECK(interpret_std("sign -32") == "-1");
    CHECK(interpret_std("32 .sign") == "1");
}


TEST_CASE( "Types", "[script][interpreter]" )
{
    // each definition can have explicit type
    CHECK(interpret("a:Int = 1 ; a") == "1");
    CHECK_THROWS_AS(interpret("a:Int = 1.0 ; a"), DefinitionTypeMismatch);
    CHECK(interpret_std("a:Int = 42; b:Int = a; b") == "42");
    CHECK_THROWS_AS(interpret("a = 42; b:String = a"), FunctionNotFound);

    // function type can be specified in lambda or specified explicitly
    CHECK(interpret_std("f = fun a:Int b:Int -> Int {a+b}; f 1 2") == "3");
    CHECK(interpret_std("f : Int Int -> Int = fun a b {a+b}; f 1 2") == "3");

    // TODO: narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // CHECK(interpret("f : Int Int -> Int = add ; f 1 2") == "3");
}


TEST_CASE( "User-defined types", "[script][interpreter]" )
{
    // 'type' keyword makes strong types
    CHECK(interpret_std("type X=Int; x:X = 42; x:Int") == "42");
    CHECK_THROWS_AS(interpret_std("type X=Int; x:X = 42; x:Int64"), FunctionNotFound);  // cast X -> Int64
    CHECK(interpret_std("type X=Int; x:X = 42; (x:Int):Int64") == "42L");  // OK with intermediate cast to Int
    // alias (not a strong type)
    CHECK(interpret_std("X=Int; x:X = 42; x:Int64") == "42L");
    // tuple
    std::string my_tuple = "type MyTuple = (String, Int); ";
    CHECK(interpret(R"(TupleAlias = (String, Int); a:TupleAlias = "hello", 42; a)") == R"(("hello", 42))");
    CHECK(interpret(my_tuple + R"(a:MyTuple = "hello", 42; a)") == R"(("hello", 42))");
    CHECK(interpret(my_tuple + R"(type Tuple2 = (String, MyTuple); a:Tuple2 = ("hello", ("a", 1)); a)") == R"(("hello", ("a", 1)))");
    // struct
    CHECK(interpret(R"(a:(name: String, age: Int) = ("hello", 42); a)") == R"((name="hello", age=42))");  // anonymous
    CHECK(interpret(R"(Rec = (name: String, age: Int); a:Rec = (name="hello", age=42); a)") == R"((name="hello", age=42))");  // alias
    std::string my_struct = "type MyStruct = (name:String, age:Int); ";  // named struct
    CHECK(interpret(my_struct + R"( a:MyStruct = (name="hello", age=42); a)") == R"((name="hello", age=42))");
    CHECK(interpret(my_struct + R"( a:MyStruct = "hello", 42; a)") == R"((name="hello", age=42))");
    // cast from underlying type
    CHECK(interpret_std(my_struct + R"( a = ("hello", 42):MyStruct; a)") == R"((name="hello", age=42))");
    CHECK(interpret_std(my_struct + R"( a = ("hello", 42):MyStruct; a:(String, Int))") == R"(("hello", 42))");
    CHECK(interpret_std(my_struct + R"( a = (name="hello", age=42):MyStruct; a)") == R"((name="hello", age=42))");
    CHECK_THROWS_AS(interpret(my_struct + R"(a = ("Luke", 10); b: MyStruct = a)"), FunctionNotFound);
    CHECK_THROWS_AS(interpret(my_struct + R"(type OtherStruct = (name:String, age:Int); a:MyStruct = ("Luke", 10); b: OtherStruct = a)"), FunctionNotFound);
    CHECK(interpret_std(my_struct + R"(a = ("Luke", 10); b: MyStruct = a: MyStruct; b)") == R"((name="Luke", age=10))");
    CHECK(interpret_std(my_struct + R"(a = ("Luke", 10); b = a: MyStruct; b)") == R"((name="Luke", age=10))");
    CHECK(interpret_std(my_tuple + R"(a = ("hello", 42):MyTuple; a)") == R"(("hello", 42))");
    CHECK_THROWS_AS(interpret_std(my_tuple + "(1, 2):MyTuple"), FunctionNotFound);  // bad cast
}


TEST_CASE( "Blocks", "[script][interpreter]" )
{
    CHECK(parse("{}") == "{}");
    CHECK(parse("{{}}") == "{{}}");

    // blocks are evaluated and return a value
    CHECK(interpret("{}") == "");  // empty function (has Void type)
    CHECK(interpret("{{}}") == "");  // empty function in empty function
    CHECK(interpret("{};{};{}") == "");  // three empty functions
    CHECK(interpret_std("{1+2}") == "3"); // non-empty
    CHECK(interpret_std("{{{1+2}}}") == "3"); // three wrapped functions, each returns the result of inner one
    CHECK(interpret_std("{1+2;4;{}}") == "3;4;");  // {} as the last statement changes function result to Void, intermediate results are "invoked"
    CHECK(interpret_std("x=4; b = 3 + {x+1}; b") == "8");

    // blocks can be assigned to a name
    CHECK(interpret("a = {}; a") == "");  // empty block can be named too, `a` is a named function of Void type
    CHECK(interpret_std("b = {1+2}; b") == "3");
    CHECK(interpret("b = { a = 1; a }; b") == "1");
    CHECK(interpret_std("b:Int = {1+2}; b") == "3");
}


TEST_CASE( "Functions and lambdas", "[script][interpreter]" )
{
    CHECK(parse("fun Int -> Int {}") == "fun Int -> Int {}");

    // returned lambda
    CHECK(interpret_std("fun x:Int->Int { x + 1 }") == "<lambda> Int32 -> Int32");  // non-generic is fine
    CHECK(interpret_std("fun x { x + 1 }") == "<lambda> Int32 -> Int32");   // also fine, function type deduced from `1` (Int) and `add: Int Int -> Int`
    CHECK_THROWS_AS(interpret_std("fun x { x }"), UnexpectedGenericFunction);  // generic lambda must be either assigned or resolved by calling

    // immediately called lambda
    CHECK(interpret_std("fun x:Int {x+1} 2") == "3");
    CHECK(interpret_std("fun x {x+1} 2") == "3");  // generic lambda
    CHECK(interpret_std("b = 3 + fun x {2*x} 2; b") == "7");

    // generic function in local scope
    CHECK(interpret_std("outer = fun y { inner = fun x { x+1 }; inner y }; outer 2") == "3");

    // argument propagation:
    CHECK(interpret_std("f = fun a:Int { fun b:Int { a+b } }; f 1 2") == "3");  //  `f` returns a function which consumes the second arg
    CHECK(interpret_std("f = fun a:Int { fun b:Int { fun c:Int { a+b+c } } }; f 1 2 3") == "6");
    CHECK(interpret_std("{ fun x:Int {x*2} } 3") == "6");  // lambda propagates through wrapped blocks and is then called
    CHECK(interpret_std("{{{ fun x:Int {x*2} }}} 3") == "6");  // lambda propagates through wrapped blocks and is then called

    // closure: inner function uses outer function's parameter
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "w=fun c1:Int {a / b - c1}; w c }; f 10 2 3") == "2");
    // closure: outer closure used by inner function
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "g=fun c1:Int {a * b - c1}; "
                        "h=fun c1:Int {g c1}; "
                        "h c }; f 4 2 3") == "5");
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 + b}; "
                        "w=fun b1:Int c1:Int {a + u b1 + v c1}; "
                        "w b c }; f 1 2 3") == "9");

    CHECK(interpret_std("outer = fun y:Int {"
                        "inner = fun x:Int { x + y }; inner y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y:Int {"
                        "  inner = fun x:Int { x + y };"
                        "  alias = inner; alias y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x:Int { x + y };"
                        "  wrapped = fun x:Int { inner x };"
                        "  wrapped y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x:Int { in2 = fun z:Int{ x + z }; in2 y };"
                        "  wrapped = fun x:Int { inner x }; wrapped y"
                        "}; outer 2") == "4");

    // closure: fully generic
    CHECK(interpret_std("outer = fun y { inner = fun x { x + y }; inner 3 * inner y }; outer 2") == "20");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x { x + y };"
                        "  wrapped = fun x { inner x };"
                        "  wrapped y "
                        "}; outer 2") == "4");

    // multiple specializations
    // * each specialization is generated only once
    CHECK(interpret_std("outer = fun<T> y:T { inner = fun<U> x:U { x + y:U }; inner 3 + inner 4 }; "
                        "outer 1; outer 2; __module.__n_fn") == "9;11;5");
    // * specializations with different types from the same template
    CHECK(interpret_std("outer = fun<T> y:T { inner = fun<U> x:U { x + y:U }; inner 3 + (inner 4l):T }; outer 2") == "11");

    // "Funarg problem" (upwards)
    auto def_succ = "succ = fun Int->Int { __value 1 .__load_static; __add 0x88 }; "s;
    auto def_compose = " compose = fun f g { fun x { f (g x) } }; "s;
    auto def_succ_compose = def_succ + def_compose;
    CHECK(interpret(def_succ_compose + "plustwo = compose succ succ; plustwo 42") == "44");
    CHECK(interpret(def_succ_compose + "plustwo = {compose succ succ}; plustwo 42") == "44");
    //CHECK(interpret_std(def_compose + "same = compose pred succ; same 42") == "42");
}


TEST_CASE( "Partial function call", "[script][interpreter]" )
{
    // partial call: `add 1` returns a lambda which takes single argument
    CHECK(interpret_std("(add 1) 2") == "3");
    CHECK(interpret_std("{add 1} 2") == "3");
    CHECK(interpret_std("f=add 1; f 2") == "3");
    CHECK(interpret_std("f={add 1}; f 2") == "3");
    CHECK(interpret_std("f=fun x:Int {add x}; f 2 1") == "3");
    CHECK(interpret_std("f=fun x:Int {add 3}; f 2 1") == "4");
    CHECK(interpret_std("f=fun x:Int y:Int z:Int { (x - y) * z}; g=fun x1:Int { f 3 x1 }; g 4 5") == "-5");
    CHECK(interpret_std("f=fun x:Int y:Int { g=fun x1:Int z1:Int { (y - x1) / z1 }; g x }; f 1 10 3") == "3");
    CHECK(interpret_std("f = fun a:Int b:Int { "
                        "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 - b}; "
                        "w=fun b1:Int c1:Int {a * u b1 / v c1}; "
                        "w b }; f 1 2 3") == "3");
    // [closure.fire] return closure with captured closures, propagate arguments into the closure
    CHECK(interpret_std("f = fun a:Int { "
                        "u=fun b2:Int {a / b2}; v=fun c2:Int {c2 - a}; "
                        "fun b1:Int c1:Int {a + u b1 + v c1} }; f 4 2 3") == "5");
}


TEST_CASE( "Forward declarations", "[script][interpreter]")
{
    // `x` name not yet seen (the symbol resolution is strictly single-pass)
    CHECK_THROWS_AS(interpret("y=x; x=7; y"), UndefinedName);
    CHECK_THROWS_AS(interpret("y={x}; x=7; y"), UndefinedName);  // block doesn't change anything
    CHECK_THROWS_AS(interpret_std("y=fun a {a+x}; x=7; y 2"), UndefinedName);  // neither does a function
    // Inside a block or function, a forward-declared value or function can be used.
    // A similar principle is used in recursion, where the function itself is considered
    // declared while processing its own body.
    CHECK(interpret("decl x:Int; y={x}; x=7; y") == "7");
    CHECK(interpret("decl f:Int->Int; w=fun x {f x}; f=fun x {x}; w 7") == "7");
    // Forward-declared template function
    // TODO: implement (parses fine, but the specialization is done too early, it needs to be postponed)
    //CHECK(interpret("decl f:<T> T->T; y={f 7}; f=fun x {x}; y") == "7");

    // Types must match
    CHECK_THROWS_AS(interpret("x: Int->Int = fun<T> a:T->Int64 {a}"), DeclarationTypeMismatch);
    CHECK_THROWS_AS(interpret("decl x: Int->Int; x = fun<T> a:T->Int64 {a}"), DeclarationTypeMismatch);
    CHECK_THROWS_AS(interpret("decl x: Int->Int; x: Float->Float = fun a {a}"), DeclarationTypeMismatch);
    CHECK(interpret("decl x: Int->Int; x = fun a {a}; x 7") == "7");
    CHECK(interpret("decl x: <T> T->T; x = fun a {a}; x 7") == "7");

    // Multiple forward declarations for same name
    CHECK_THROWS_AS(interpret("decl over:Int->Int; decl over:String->String"), RedefinedName);
}


TEST_CASE( "Generic functions", "[script][interpreter]" )
{
    // `f` is a generic function, instantiated to Int->Int by the call
    CHECK(interpret_std("f=fun x {x + 1}; f (f (f 2))") == "5");
    // generic functions can capture from outer scope
    CHECK(interpret_std("a=3; f=fun x {a + x}; f 4") == "7");
    // generic type declaration, type constraint
    CHECK(interpret_std("f = fun<T> x:T y:T -> Bool with (Eq T) { x == y }; f 1 2") == "false");

    // === Propagating and deducing function types ===
    // arg to ret via type parameter
    CHECK(interpret("fun<T> T->T { __noop } 1") == "1");
    CHECK(interpret("f = fun<T> T->T { __noop }; f 2") == "2");
    CHECK(interpret("f = fun<T> T->T { __noop }; f (f 3)") == "3");
    CHECK(interpret("f = fun<T> T->T { __noop }; 4 .f .f .f") == "4");
    CHECK(interpret("f = fun<T> T->T { __noop }; same = fun<T> x:T -> T { f (f x) }; same 5") == "5");
}


TEST_CASE( "Overloaded functions", "[script][interpreter]" )
{
    CHECK(interpret("f: <T> T -> T = fun a { a }\n"
                    "f: String -> String = fun a { a }\n"
                    "f: Int -> Int = fun a { a }\n"
                    "f 3.f; f 1; f \"abc\";") == "3.0f;1;\"abc\"");
    CHECK_THROWS_AS(interpret("f = fun a:Int -> Int { a }\n"
                              "f = fun a:Float -> Float { a }"), RedefinedName);

    // Overloaded function with forward declaration - only the first overload may be declared
    CHECK_THROWS_AS(interpret("decl f:Int->Int; f:String->String = fun a {a}; f:Int->Int = fun a {a}"), DeclarationTypeMismatch);
    CHECK(interpret("decl f:Int->Int; f:Int->Int = fun a {a}; f:String->String = fun a {a}; f 1; f \"abc\"") == "1;\"abc\"");

    // Variables are also functions and so can be overloaded
    CHECK(interpret_std("a:Int=2; a:String=\"two\";a:Int;a:String") == "2;\"two\"");
    CHECK_THROWS_AS(interpret_std("a:Int=2; a:String=\"two\";a:Int64"), FunctionConflict);
}


TEST_CASE( "Lexical scope", "[script][interpreter]" )
{
    CHECK(interpret("{a=1; b=2}") == "");
    CHECK_THROWS_AS(interpret("{a=1; b=2} a"), UndefinedName);

    CHECK(interpret_std("x=1; y = { x + 2 }; y") == "3");
    CHECK(interpret_std("a=1; {b=2; {a + b}}") == "3");
    CHECK(interpret_std("a=1; f=fun b:Int {a + b}; f 2") == "3");

    // recursion
    CHECK(interpret_std("f=fun x:Int->Int { x; if x <= 1 then 0 else f (x-1) }; f 5") == "5;4;3;2;1;0");      // yield intermediate steps
    CHECK(interpret_std("f=fun n:Int->Int { if n == 1 then 1 else n * f (n-1) }; f 7") == "5040");      // factorial
    CHECK(interpret_std("f=fun x:Int->Int { if x < 2 then x else f (x-1) + f (x-2) }; f 7") == "13");   // Fibonacci number

    // iteration (with tail-recursive functions)
    CHECK(interpret_std("fi=fun prod:Int cnt:Int max:Int -> Int { if cnt > max then prod else fi (cnt*prod) (cnt+1) max };\n"
                        "f=fun n:Int->Int { fi 1 1 n }; f 7") == "5040");  // factorial
    CHECK(interpret_std("fi=fun a:Int b:Int n:Int -> Int { if n==0 then b else fi (a+b) a (n-1) };\n"
                        "f=fun n:Int->Int { fi 1 0 n }; f 7") == "13");    // Fibonacci number
}


TEST_CASE( "If-expression", "[script][interpreter]" )
{
    CHECK(parse("if 1>2 then 1 else 2") == "if (1 > 2) then 1\nelse 2;");
    CHECK(parse("if 1>2 then 1 "
                "if 2>2 then 2 "
                "else 3") == "if (1 > 2) then 1\n"
                             "if (2 > 2) then 2\n"
                             "else 3;");

    CHECK(interpret_std("x = 3; if x < 0 then -1 else 1") == "1");
    CHECK(interpret_std("x = 3; if x < 0 then -1 if x > 1 then 1 else 0") == "1");

    // The result can be assigned
    CHECK(interpret_std("x = 3; y = if x < 0 then -1 else 1; x + y") == "4");
}


TEST_CASE( "Casting", "[script][interpreter]" )
{
    CHECK(interpret_std("\"drop this\":Void") == "");
    CHECK(interpret_std("\"noop\":String") == "\"noop\"");
    CHECK(interpret_std("42:Int64") == "42L");
    CHECK(interpret_std("42L:Int32") == "42");
    CHECK(interpret_std("42:Float32") == "42.0f");
    CHECK(interpret_std("42:Float64") == "42.0");
    CHECK(interpret_std("12.9:Int") == "12");
    CHECK(interpret_std("-12.9:Int") == "-12");
    CHECK(interpret_std("a = 42; a:Byte") == "b'*'");
    CHECK(interpret_std("(1 + 2):Int64") == "3L");
    CHECK(interpret_std("0:Bool") == "false");
    CHECK(interpret_std("42:Bool") == "true");
    CHECK(interpret_std("(42:Bool):Int") == "1");
    CHECK(interpret_std("!42:Bool") == "false");  // '!' is prefix operator, cast has higher precedence
    CHECK(interpret_std("-42:Bool") == "true");  // '-' is part of Int literal, not an operator
    CHECK_THROWS_AS(interpret_std("- 42:Bool"), FunctionNotFound);  // now it's operator and that's an error: "neg Bool" not defined
    CHECK(interpret_std("(- 42):Bool") == "true");
    CHECK(interpret_std("(cast 42):Int64") == "42L");
    CHECK(interpret_std("a:Int64 = cast 42; a") == "42L");
    CHECK_THROWS_AS(interpret_std("cast 42"), FunctionConflict);  // must specify the result type
    CHECK(interpret_std("{23L}:Int") == "23");
    CHECK(interpret_std("min:Int") == "-2147483648");
    CHECK(interpret_std("max:UInt") == "4294967295U");
    CHECK(interpret_std("a:Int = min; a") == "-2147483648");
    // [Char] <-> String
    CHECK(interpret_std("cast_to_string ['a','b','č']") == "\"abč\"");
    CHECK(interpret_std("['a','b','č']:String") == "\"abč\"");
    CHECK(interpret_std("cast_to_chars \"abč\"") == "['a', 'b', 'č']");
    CHECK(interpret_std("\"abč\":[Char]") == "['a', 'b', 'č']");
    // [Byte] <-> String
    CHECK(interpret_std("cast_to_string b\"fire\"") == "\"fire\"");
    CHECK(interpret_std("b\"fire\":String") == "\"fire\"");
    CHECK(interpret_std("cast_to_bytes \"fire\"") == "b\"fire\"");
    CHECK(interpret_std("\"fire\":[Byte]") == "b\"fire\"");
    // [Byte] <-> String (UTF-8)
    CHECK(interpret_std("cast_to_string b\"\\xc5\\x99e\\xc5\\xbe\"") == "\"řež\"");
    CHECK(interpret_std("b\"\\xc5\\x99e\\xc5\\xbe\":String") == "\"řež\"");
    CHECK(interpret_std("cast_to_bytes \"řež\"") == "b\"\\xc5\\x99e\\xc5\\xbe\"");
    CHECK(interpret_std("\"řež\":[Byte]") == "b\"\\xc5\\x99e\\xc5\\xbe\"");
    // "multi-cast" (parentheses are required)
    CHECK(interpret_std("((\"fire\":[Byte]):String):[Char]") == "['f', 'i', 'r', 'e']");
}


TEST_CASE( "String operations", "[script][interpreter]" )
{
    CHECK(interpret_std("string_equal \"fire\" \"fire\"") == "true");
    CHECK(interpret_std("string_equal \"fire\" \"water\"") == "false");
    CHECK(interpret_std("\"fire\" == \"fire\"") == "true");
    CHECK(interpret_std("\"fire\" != \"water\"") == "true");
    CHECK(interpret_std("string_compare \"fire\" \"fire\"") == "0");
    CHECK(interpret_std("string_compare \"fire\" \"earth\" .sign") == "1");
    CHECK(interpret_std("string_compare \"fire\" \"air\" .sign") == "1");
    CHECK(interpret_std("string_compare \"fire\" \"water\" .sign") == "-1");
    CHECK(interpret_std("\"fire\" > \"fire\"") == "false");
    CHECK(interpret_std("\"fire\" < \"fire\"") == "false");
    CHECK(interpret_std("\"fire\" >= \"fire\"") == "true");
    CHECK(interpret_std("\"fire\" <= \"fire\"") == "true");
    CHECK(interpret_std("\"fire\" > \"earth\"") == "true");
    CHECK(interpret_std("\"fire\" < \"air\"") == "false");
    CHECK(interpret_std("\"fire\" < \"water\"") == "true");
}


TEST_CASE( "Subscript", "[script][interpreter]" )
{
    // custom implementation (same as in std.fire)
    CHECK(interpret("__type_id<Void>") == "0");
    CHECK_THROWS_AS(interpret("__type_id<X>"), UndefinedTypeName);
    CHECK(interpret("subscript = fun<T> [T] Int -> T { __subscript __type_id<T> }; subscript [1,2,3] 1") == "2");
    // std implementation
    CHECK(interpret_std("subscript [1,2,3] 1") == "2");
    CHECK(interpret_std("[1,2,3] .subscript 0") == "1");
    CHECK(interpret_std("[1,2,3] ! 2") == "3");
    CHECK_THROWS_AS(interpret_std("[1,2,3]!3"), IndexOutOfBounds);
    CHECK(interpret_std("['a','b','c'] ! 1") == "'b'");
    CHECK(interpret_std("l=[1,2,3,4,5]; l!0 + l!1 + l!2 + l!3 + l!4") == "15");  // inefficient sum
    CHECK(interpret_std("[[1,2],[3,4],[5,6]] ! 1 ! 0") == "3");
    CHECK(interpret_std("head = fun l:[Int] -> Int { l!0 }; head [1,2,3]") == "1");
    CHECK(interpret_std("head = fun<T> l:[T] -> T { l!0 }; head ['a','b','c']") == "'a'");
}


TEST_CASE( "Slice", "[script][interpreter]" )
{
    CHECK(interpret("slice = fun<T> [T] start:Int stop:Int step:Int -> [T] { __slice __type_id<T> }; [1,2,3,4,5] .slice 1 4 1") == "[2, 3, 4]");
    // step=0 -- pick one element for a new list
    CHECK(interpret_std("[1,2,3,4,5] .slice 3 max:Int 0") == "[4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 3 max:Int max:Int") == "[4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 3 min:Int min:Int") == "[4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 0 max:Int 1") == "[1, 2, 3, 4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 1 max:Int 1") == "[2, 3, 4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 3 max:Int 1") == "[4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 5 max:Int 1") == "[]");
    CHECK(interpret_std("[1,2,3,4,5] .slice -1 max:Int 1") == "[5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice -2 max:Int 1") == "[4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice -5 max:Int 1") == "[1, 2, 3, 4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice min:Int max:Int 1") == "[1, 2, 3, 4, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice min:Int max:Int 2") == "[1, 3, 5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice min:Int max:Int 3") == "[1, 4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 2 4 1") == "[3, 4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 1 4 2") == "[2, 4]");
    CHECK(interpret_std("[1,2,3,4,5] .slice max:Int min:Int -1") == "[5, 4, 3, 2, 1]");
    CHECK(interpret_std("[1,2,3,4,5] .slice max:Int min:Int -2") == "[5, 3, 1]");
    CHECK(interpret_std("[1,2,3,4,5] .slice max:Int min:Int -3") == "[5, 2]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 4 3 -1") == "[5]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 4 1 -1") == "[5, 4, 3]");
    CHECK(interpret_std("[1,2,3,4,5] .slice -1 -4 -1") == "[5, 4, 3]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 5 1 1") == "[]");
    CHECK(interpret_std("[1,2,3,4,5] .slice 1 5 -1") == "[]");
    CHECK(interpret_std("[] .slice 0 5 1") == "[]");
    CHECK(interpret_std("[]:[Int] .slice 0 5 1") == "[]");
    CHECK(interpret_std("tail [1,2,3]") == "[2, 3]");
}


TEST_CASE( "Type classes", "[script][interpreter]" )
{
    CHECK(interpret("class XEq T { xeq : T T -> Bool }; "
                    "instance XEq Int32 { xeq = { __equal 0x88 } }; "
                    "xeq 1 2") == "false");
    // Instance function may reference the method that is being instantiated
    CHECK(interpret("class Ord T (Eq T) { lt : T T -> Bool }; "
                    "instance Ord Int32 { lt = { __less_than 0x88 } }; "
                    "instance Ord String { lt = fun a b { string_compare a b < 0 } }; "
                    "\"a\" < \"b\"") == "true");
    // Instantiate type class from another module
    CHECK(interpret_std("instance Ord Bool { lt = { __less_than 0x11 }; gt = false; le = false; ge = false }; "
                        "false < true; 2 < 1") == "true;false");
}


TEST_CASE( "With expression, I/O streams", "[script][interpreter]" )
{
    CHECK(parse("with stdout { 42 }") == "with stdout {42};");
    // __streams is a builtin function that returns a tuple of current streams: (in, out, err)
    CHECK(interpret("__streams") == "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with null { __streams }") == "(in=<stream:stdin>, out=<stream:null>, err=<stream:stderr>)");
    CHECK_THROWS_AS(interpret("with 42 {}"), FunctionNotFound);  // function not found: enter Int32
    CHECK(interpret("with (null, null) { __streams }") == "(in=<stream:null>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (null, null, null) { __streams }; __streams")  // `with` scope
          == "(in=<stream:null>, out=<stream:null>, err=<stream:null>);"
             "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with (in=null) { __streams }") == "(in=<stream:null>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with (out=null) { __streams }") == "(in=<stream:stdin>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (in=null, out=null) { __streams }") == "(in=<stream:null>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (in=stdin, out=stdout, err=stderr) { __streams }") == "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with\n(in=stdin, out=stdout, err=stderr)\n{ 42 }") == "42");
    CHECK(interpret_std("with (in=null):Streams { __streams }") == "(in=<stream:null>, out=<stream:stdout>, err=<stream:stderr>)");  // the missing fields are inherited in `enter` function
    CHECK(interpret_std("(in=null):Streams") == "(in=<stream:null>, out=<stream:undef>, err=<stream:undef>)");  // the missing fields are default initialized => undef stream
    CHECK(interpret_std("(in=null)") == "(in=<stream:null>)");  // anonymous struct

    // write an actual file
    auto filename = fs::temp_directory_path() / "xci_test_script.txt";
    CHECK(interpret("with (open \""s + escape(filename.string()) + "\" \"w\")\n"
                    "    write \"this goes to the file\"") == "");
    CHECK(interpret("with (in=(open \""s + escape(filename.string()) + "\" \"r\"))\n"
                    "    read 9") == "\"this goes\"");
    CHECK(fs::remove(filename));
}


TEST_CASE( "Compiler intrinsics", "[script][interpreter]" )
{
    // Function signature must be explicitly declared, it's never inferred from intrinsics.
    // Parameter names are not needed (and not used), intrinsics work directly with stack.
    // E.g. `__equal 0x88` pulls two Int32 values and pushes 8bit Bool value back.
    CHECK(interpret_std("my_eq = fun Int32 Int32 -> Bool { __equal 0x88 }; my_eq 42 (2*21)") == "true");
    // alternative style - essentially the same
    CHECK(interpret("my_eq : Int32 Int32 -> Bool = { __equal 0x88 }; my_eq 42 43") == "false");
    // intrinsic with arguments
    CHECK(interpret("my_cast : Int32 -> Int64 = { __cast 0x89 }; my_cast 42") == "42L");
    // Static value
    CHECK(interpret("add42 = fun Int->Int { __load_static (__value 42); __add 0x88 }; add42 8") == "50");
    CHECK(interpret("add42 = fun Int->Int { __value 42 . __load_static; __add 0x88 }; add42 8") == "50");
}


TEST_CASE( "Native to TypeInfo mapping", "[script][native]" )
{
    CHECK(native::make_type_info<void>().type() == Type::Void);
    CHECK(native::make_type_info<bool>().type() == Type::Bool);
    CHECK(native::make_type_info<uint8_t>().type() == Type::Byte);
    CHECK(native::make_type_info<char>().type() == Type::Char);
    CHECK(native::make_type_info<char16_t>().type() == Type::Char);
    CHECK(native::make_type_info<char32_t>().type() == Type::Char);
    CHECK(native::make_type_info<int>().type() == Type::Int32);  // depends on sizeof(int)
    CHECK(native::make_type_info<long long>().type() == Type::Int64);  // depends on sizeof(long long)
    CHECK(native::make_type_info<int32_t>().type() == Type::Int32);
    CHECK(native::make_type_info<int64_t>().type() == Type::Int64);
    CHECK(native::make_type_info<float>().type() == Type::Float32);
    CHECK(native::make_type_info<double>().type() == Type::Float64);
    CHECK(native::make_type_info<std::string>().type() == Type::String);
    CHECK(native::make_type_info<char*>().type() == Type::String);
    CHECK(native::make_type_info<const char*>().type() == Type::String);
}


TEST_CASE( "Native to Value mapping", "[script][native]" )
{
    CHECK(native::ValueType<void>().type() == Type::Void);
    CHECK(native::ValueType<bool>{true}.value() == true);
    CHECK(native::ValueType<byte>(255).value() == 255);
    CHECK(native::ValueType<char>('y').value() == 'y');
    CHECK(native::ValueType<int32_t>(-1).value() == -1);
    CHECK(native::ValueType<int64_t>(1ll << 60).value() == 1ll << 60);
    CHECK(native::ValueType<float>(3.14f).value() == 3.14f);
    CHECK(native::ValueType<double>(2./3).value() == 2./3);
    native::ValueType<std::string>str ("test"s);
    CHECK(str.value() == "test"s);
    str.decref();
}


int test_fun1(int a, int b, int c) { return (a - b) / c; }

TEST_CASE( "Native functions: free function", "[script][native]" )
{
    Context& ctx = context();
    Module main_module {ctx.interpreter.module_manager()};
    auto native_module = std::make_shared<Module>(ctx.interpreter.module_manager());
    main_module.import_module("builtin");
    main_module.import_module("std");
    main_module.add_imported_module(native_module);

    // free function
    native_module->add_native_function("test_fun1a", &test_fun1);
    native_module->add_native_function("test_fun1b", test_fun1);  // function pointer is deduced

    auto result = ctx.interpreter.eval(main_module, R"(
        ((test_fun1a 10 4 2)     //  3
        + (test_fun1b 0 6 3))    // -2
    )");
    CHECK(result.type() == Type::Int32);
    CHECK(result.get<int32_t>() == 1);
}


TEST_CASE( "Native functions: lambda", "[script][native]" )
{
    Context& ctx = context();
    Module module {ctx.interpreter.module_manager()};

    // lambdas
    module.add_native_function("add1", [](int a, int b) { return a + b; });

    // lambda with state (can't use capture)
    int state = 10;
    module.add_native_function("add2",
            [](void* s, int a, int b) { return a + b + *(int*)(s); },
            &state);

    auto result = ctx.interpreter.eval(module, R"(
        (add1 (add1 1 6)       //  7
              (add2 3 4))      //  8  (+10 from state)
    )");
    CHECK(result.type() == Type::Int32);
    CHECK(result.get<int32_t>() == 24);
}


TEST_CASE( "Fold const expressions", "[script][optimizer]" )
{
    // fold constant if-expression
    CHECK(optimize("if false then 10 else 0;") == "0");
    CHECK(optimize("if true then 10 else 0;") == "10");
    CHECK(optimize("if false then 10 if true then 42 else 0;") == "42");
    CHECK(optimize("if false then 10 if false then 42 else 0;") == "0");

    // partially fold if-expression (only some branches)
    CHECK(optimize("num = 16; if true then 10 if num>5 then 5 else 0;") == "/*def*/ num = (16);\n10");
    CHECK(optimize("num = 16; if false then 10 if num>5 then 5 else 0;") == "/*def*/ num = (16);\nif (num > 5) then 5\nelse 0;");
    CHECK(optimize("num = 16; if num>10 then 10 if true then 5 else 0;") == "/*def*/ num = (16);\nif (num > 10) then 10\nelse 5;");
    CHECK(optimize("num = 16; if num>10 then 10 if false then 5 else 0;") == "/*def*/ num = (16);\nif (num > 10) then 10\nelse 0;");
    CHECK(optimize("num = 16; if num>10 then 10 if true then 5 if num==3 then 3 else 0;") == "/*def*/ num = (16);\nif (num > 10) then 10\nelse 5;");
    CHECK(optimize("num = 16; if num>10 then 10 if false then 5 if num==3 then 3 else 0;") == "/*def*/ num = (16);\nif (num > 10) then 10\nif (num == 3) then 3\nelse 0;");

    // collapse block with single statement
    CHECK(optimize("{ 1 }") == "1");
    CHECK(optimize("{{{ 1 }}}") == "1");
    CHECK(optimize("a = {{1}}") == "/*def*/ a = (1);");

    // cast to Void eliminates the expression
    CHECK(optimize("42:Void") == "");
    CHECK(optimize("{42}:Void") == "");
    CHECK(optimize("(fun x { x + 1 }):Void") == "");

    // cast to the same type is eliminated
    CHECK(optimize("42:Int") == "42");

    // cast of constant value is collapsed to the value
    CHECK(optimize("42l:Int") == "42");
}
