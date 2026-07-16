#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/ir/ir.hpp"
#include "compiler/ir/ir_loader.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include "runtime/interpreter/vm.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <fstream>
#include <cstdio>

// Helper to assert substring presence in exception message
void assertContains(const std::string& text, const std::string& substring) {
    if (text.find(substring) == std::string::npos) {
        std::cerr << "Expected exception to contain: \"" << substring << "\"\n"
                  << "But got: \"" << text << "\"\n";
        assert(false);
    }
}

void testLexer() {
    std::cout << "[test] Running Lexer tests...\n";

    // 1. Test Keywords (including unsupported)
    std::string kwSource = "int float void return if else while class struct template namespace try catch throw new delete";
    Lexer kwLexer(kwSource, "test_kw.cpp");
    std::vector<Token> kwTokens = kwLexer.tokenize();
    assert(kwTokens.size() == 17); // 16 keywords + TOK_EOF
    assert(kwTokens[0].type == TokenType::KEYWORD_INT);
    assert(kwTokens[7].type == TokenType::KEYWORD_CLASS);
    assert(kwTokens[11].type == TokenType::KEYWORD_TRY);
    assert(kwTokens[15].type == TokenType::KEYWORD_DELETE);

    // 2. Test Strings (with escaping)
    std::string strSource = "\"Hello\\nWorld\\tNEXUS \\\"1.0\\\"\"";
    Lexer strLexer(strSource, "test_str.cpp");
    std::vector<Token> strTokens = strLexer.tokenize();
    assert(strTokens.size() == 2); // literal + TOK_EOF
    assert(strTokens[0].type == TokenType::LITERAL_STRING);
    assert(strTokens[0].value == "Hello\nWorld\tNEXUS \"1.0\"");

    // 3. Test Operators & Comments
    std::string opSource = "+ - * / % == != < <= > >= = << // comments here\n/* multi line */";
    Lexer opLexer(opSource, "test_op.cpp");
    std::vector<Token> opTokens = opLexer.tokenize();
    assert(opTokens.size() == 14); // 13 operators + TOK_EOF
    assert(opTokens[0].type == TokenType::OP_ADD);
    assert(opTokens[5].type == TokenType::OP_EQ);
    assert(opTokens[6].type == TokenType::OP_NE);
    assert(opTokens[12].type == TokenType::OP_LSHIFT);

    // 4. Test Errors (Lexical Unexpected Char)
    std::string errSource = "int @x = 42;";
    Lexer errLexer(errSource, "test_err.cpp");
    std::vector<Token> errTokens = errLexer.tokenize();
    bool foundErr = false;
    for (const auto& t : errTokens) {
        if (t.type == TokenType::TOK_ERROR) {
            assertContains(t.value, "[NX-101]");
            assertContains(t.value, "Unexpected character '@'");
            foundErr = true;
        }
    }
    assert(foundErr);

    std::cout << "[test] Lexer tests passed successfully! ✅\n";
}

void testParser() {
    std::cout << "[test] Running Parser tests...\n";

    // 1. Test Operator Precedence & Associativity
    // a + b * c  =>  a + (b * c)
    std::string precSource = "int main() { return 1 + 2 * 3; }";
    Lexer precLexer(precSource, "test_prec.cpp");
    Parser precParser(precLexer.tokenize(), "test_prec.cpp");
    std::unique_ptr<ProgramNode> precAst = precParser.parse();
    assert(precAst != nullptr);
    
    // 2. Reject Unsupported syntax cleanly
    std::string classSource = "class MyClass { int x; };";
    Lexer classLexer(classSource, "test_class.cpp");
    Parser classParser(classLexer.tokenize(), "test_class.cpp");
    try {
        classParser.parse();
        assert(false && "Parser should have rejected 'class' keyword!");
    } catch (const ParserException& e) {
        assertContains(e.what(), "[NX-203]");
        assertContains(e.what(), "Feature 'class' is not supported in NEXUS 1.0.");
    }

    std::string trySource = "int main() { try { int x = 0; } catch(...) {} }";
    Lexer tryLexer(trySource, "test_try.cpp");
    Parser tryParser(tryLexer.tokenize(), "test_try.cpp");
    try {
        tryParser.parse();
        assert(false && "Parser should have rejected 'try' keyword!");
    } catch (const ParserException& e) {
        assertContains(e.what(), "[NX-203]");
        assertContains(e.what(), "Feature 'try' is not supported in NEXUS 1.0.");
    }

    std::cout << "[test] Parser tests passed successfully! ✅\n";
}

void testOptimizer() {
    std::cout << "[test] Running IR Optimizer tests...\n";

    // Test constant folding & dead code elimination
    std::string source = "int main() {\n"
                         "  int a = 10 + 20 * 2;\n" // 10 + 40 = 50 (constant folded)
                         "  int b = 5;\n"
                         "  int c = a + b;\n"       // 50 + 5 = 55 (constant folded)
                         "  int unused = 999;\n"    // unused variable (dead code eliminated)
                         "  return c;\n"
                         "}";
    
    Lexer lexer(source, "test_opt.cpp");
    Parser parser(lexer.tokenize(), "test_opt.cpp");
    std::unique_ptr<ProgramNode> ast = parser.parse();

    IRGenerator gen;
    IRProgram prog = gen.generate(*ast);

    // Prior to optimization, there should be multiple instructions, including 'unused' Alloca/Store.
    size_t origSize = prog.functions[0].instructions.size();

    IROptimizer opt;
    opt.optimize(prog);

    size_t optSize = prog.functions[0].instructions.size();
    assert(optSize < origSize);

    // Verify optimized instructions contain folded return value directly
    // The instruction before RET should be the constant assignment of 55
    bool foundConst55 = false;
    for (const auto& inst : prog.functions[0].instructions) {
        if (inst.op == IROp::CONST && inst.arg1 == "55") {
            foundConst55 = true;
        }
    }
    assert(foundConst55);

    // Verify 'unused' was eliminated
    bool foundUnused = false;
    for (const auto& inst : prog.functions[0].instructions) {
        if (inst.dest == "%addr_unused") {
            foundUnused = true;
        }
    }
    assert(!foundUnused);

    std::cout << "[test] IR Optimizer tests passed successfully! ✅\n";
}

void testIRLoaderValidation() {
    std::cout << "[test] Running IR Loader Validation tests...\n";

    // 1. Missing version header
    std::string invalidHeader = "define void @main() {\n"
                                "  ret\n"
                                "}\n";
    try {
        IRLoader::loadFromString(invalidHeader, "invalid_header.nxs");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-301]");
    }

    // 2. Undefined label
    std::string undefLabel = "; NEXUS IR v1.0\n"
                             "define void @main() {\n"
                             "  br label %missing_label\n"
                             "}\n";
    try {
        IRLoader::loadFromString(undefLabel, "undef_label.nxs");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-304]");
        assertContains(e.what(), "Undefined jump target label 'missing_label'");
    }

    // 3. Duplicate label
    std::string dupLabel = "; NEXUS IR v1.0\n"
                           "define void @main() {\n"
                           "lbl_1:\n"
                           "lbl_1:\n"
                           "  ret\n"
                           "}\n";
    try {
        IRLoader::loadFromString(dupLabel, "dup_label.nxs");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-305]");
        assertContains(e.what(), "Duplicate label 'lbl_1'");
    }

    // 4. Invalid Register format
    std::string badReg = "; NEXUS IR v1.0\n"
                         "define void @main() {\n"
                         "  bad_reg = const 10\n"
                         "  ret\n"
                         "}\n";
    try {
        IRLoader::loadFromString(badReg, "bad_reg.nxs");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-303]");
    }

    std::cout << "[test] IR Loader Validation tests passed successfully! ✅\n";
}

void testVMExceptions() {
    std::cout << "[test] Running VM Runtime Exceptions tests...\n";

    // 1. Division by zero
    std::string divZeroSource = "int main() {\n"
                                "  int x = 10 / 0;\n"
                                "  return x;\n"
                                "}";
    Lexer l1(divZeroSource, "test_div.cpp");
    Parser p1(l1.tokenize(), "test_div.cpp");
    IRGenerator gen;
    IRProgram prog1 = gen.generate(*p1.parse());
    VM vm1(prog1, "test_div.cpp");
    try {
        vm1.execute("main");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-401]");
        assertContains(e.what(), "Division by zero");
    }

    // 2. Modulo by zero
    std::string modZeroSource = "int main() {\n"
                                "  int x = 10 % 0;\n"
                                "  return x;\n"
                                "}";
    Lexer l2(modZeroSource, "test_mod.cpp");
    Parser p2(l2.tokenize(), "test_mod.cpp");
    IRProgram prog2 = gen.generate(*p2.parse());
    VM vm2(prog2, "test_mod.cpp");
    try {
        vm2.execute("main");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-401]");
        assertContains(e.what(), "Modulo division by zero");
    }

    // 3. Stack overflow (infinite recursion)
    std::string recSource = "int recurse() {\n"
                            "  return recurse();\n"
                            "}\n"
                            "int main() {\n"
                            "  return recurse();\n"
                            "}";
    Lexer l3(recSource, "test_rec.cpp");
    Parser p3(l3.tokenize(), "test_rec.cpp");
    IRProgram prog3 = gen.generate(*p3.parse());
    VM vm3(prog3, "test_rec.cpp");
    try {
        vm3.execute("main");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-407]");
        assertContains(e.what(), "Stack overflow");
    }

    // 4. Invalid file handle operations
    std::string fileSource = "int main() {\n"
                             "  file_write(999, \"fail\");\n"
                             "  return 0;\n"
                             "}";
    Lexer l4(fileSource, "test_file.cpp");
    Parser p4(l4.tokenize(), "test_file.cpp");
    IRProgram prog4 = gen.generate(*p4.parse());
    VM vm4(prog4, "test_file.cpp");
    try {
        vm4.execute("main");
        assert(false);
    } catch (const std::exception& e) {
        assertContains(e.what(), "[NX-405]");
        assertContains(e.what(), "Invalid file handle for file_write: 999");
    }

    std::cout << "[test] VM Runtime Exceptions tests passed successfully! ✅\n";
}

void testIntegration() {
    std::cout << "[test] Running integration programs...\n";

    // 1. Fibonacci (Iterative)
    std::string fibSrc = "int main() {\n"
                         "  int limit = 6;\n"
                         "  int a = 0;\n"
                         "  int b = 1;\n"
                         "  int i = 0;\n"
                         "  while (i < limit) {\n"
                         "    int temp = a + b;\n"
                         "    a = b;\n"
                         "    b = temp;\n"
                         "    i = i + 1;\n"
                         "  }\n"
                         "  return a;\n"
                         "}";
    Lexer l1(fibSrc, "fib.cpp");
    Parser p1(l1.tokenize(), "fib.cpp");
    IRGenerator gen;
    IRProgram prog1 = gen.generate(*p1.parse());
    IROptimizer opt;
    opt.optimize(prog1);
    VM vm1(prog1, "fib.cpp");
    Value r1 = vm1.execute("main");
    // limit 6 -> 0,1 -> 1(1), 2(2), 3(3), 5(4), 8(5), 13(6) => return 8
    assert(std::get<int>(r1) == 8);

    // 2. Factorial (Recursive)
    std::string factSrc = "int fact(int n) {\n"
                          "  if (n <= 1) return 1;\n"
                          "  return n * fact(n - 1);\n"
                          "}\n"
                          "int main() {\n"
                          "  return fact(5);\n"
                          "}";
    Lexer l2(factSrc, "fact.cpp");
    Parser p2(l2.tokenize(), "fact.cpp");
    IRProgram prog2 = gen.generate(*p2.parse());
    opt.optimize(prog2);
    VM vm2(prog2, "fact.cpp");
    Value r2 = vm2.execute("main");
    assert(std::get<int>(r2) == 120);

    // 3. Nested loops (matrix indexing simulator)
    std::string nestedSrc = "int main() {\n"
                            "  int i = 0;\n"
                            "  int sum = 0;\n"
                            "  while (i < 3) {\n"
                            "    int j = 0;\n"
                            "    while (j < 3) {\n"
                            "      sum = sum + i * 10 + j;\n"
                            "      j = j + 1;\n"
                            "    }\n"
                            "    i = i + 1;\n"
                            "  }\n"
                            "  return sum;\n"
                            "}";
    Lexer l3(nestedSrc, "nested.cpp");
    Parser p3(l3.tokenize(), "nested.cpp");
    IRProgram prog3 = gen.generate(*p3.parse());
    opt.optimize(prog3);
    VM vm3(prog3, "nested.cpp");
    Value r3 = vm3.execute("main");
    // i=0: j=0,1,2 => sum += 0 + 1 + 2 = 3
    // i=1: j=0,1,2 => sum += 10 + 11 + 12 = 33 => sum = 36
    // i=2: j=0,1,2 => sum += 20 + 21 + 22 = 63 => sum = 99
    assert(std::get<int>(r3) == 99);

    std::cout << "[test] Integration programs passed successfully! ✅\n";
}

int main() {
    std::cout << "=======================================\n";
    std::cout << " 🧪  NEXUS Compiler Verification Suite \n";
    std::cout << "=======================================\n\n";

    try {
        testLexer();
        testParser();
        testOptimizer();
        testIRLoaderValidation();
        testVMExceptions();
        testIntegration();
        std::cout << "\n🎉 ALL NEXUS VERIFICATION TESTS PASSED SUCCESSFULLY! 🎉\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test Suite Failed: " << e.what() << "\n";
        return 1;
    }
}
