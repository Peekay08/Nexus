#ifndef IR_HPP
#define IR_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>

enum class IROp {
    ALLOCA,      // ALLOCA <var>, <type>
    STORE,       // STORE <val>, <var>
    LOAD,        // LOAD <reg>, <var>
    CONST,       // CONST <reg>, <val>
    ADD,         // ADD <dest>, <lhs>, <rhs>
    SUB,         // SUB <dest>, <lhs>, <rhs>
    MUL,         // MUL <dest>, <lhs>, <rhs>
    DIV,         // DIV <dest>, <lhs>, <rhs>
    MOD,         // MOD <dest>, <lhs>, <rhs>
    EQ,          // EQ <dest>, <lhs>, <rhs>
    NE,          // NE <dest>, <lhs>, <rhs>
    LT,          // LT <dest>, <lhs>, <rhs>
    LE,          // LE <dest>, <lhs>, <rhs>
    GT,          // GT <dest>, <lhs>, <rhs>
    GE,          // GE <dest>, <lhs>, <rhs>
    LABEL,       // LABEL <name>
    BR,          // BR <label>
    CBR,         // CBR <cond>, <then_label>, <else_label>
    CALL,        // CALL <dest>, <func>, <args...>
    RET,         // RET <val>
    PRINT        // PRINT <val> (special print instruction to support stdout)
};

std::string irOpToString(IROp op);

struct IRInstruction {
    IROp op;
    std::string dest;
    std::string arg1;
    std::string arg2;
    std::vector<std::string> callArgs;

    std::string toString() const;
};

struct IRFunction {
    std::string name;
    std::string returnType;
    std::vector<std::string> paramNames;
    std::vector<IRInstruction> instructions;

    std::string toString() const;
};

struct IRProgram {
    std::vector<IRFunction> functions;

    std::string toString() const;
};

class IRGenerator {
    friend class FunctionDeclNode;
    friend class ProgramNode;
public:
    IRGenerator();
    IRProgram generate(const class ASTNode& root);

    // Dynamic builders
    std::string nextRegister();
    std::string nextLabel(const std::string& prefix = "label");
    
    void emit(IROp op, const std::string& dest = "", const std::string& arg1 = "", const std::string& arg2 = "", const std::vector<std::string>& callArgs = {});

    // Scopes and symbol management
    void enterScope();
    void exitScope();
    void declareVariable(const std::string& name, const std::string& type);
    std::string getVariableType(const std::string& name);

    // Helpers to visit nodes
    std::string codegenLiteralInt(int val);
    std::string codegenLiteralFloat(float val);
    std::string codegenLiteralString(const std::string& val);
    std::string codegenIdentifier(const std::string& name);
    std::string codegenBinaryOp(const std::string& op, const class ExpressionNode& left, const class ExpressionNode& right);
    std::string codegenFuncCall(const std::string& name, const std::vector<std::unique_ptr<class ExpressionNode>>& args);
    std::string codegenVarDecl(const std::string& type, const std::string& name, const class ExpressionNode* init);
    std::string codegenAssign(const std::string& name, const class ExpressionNode& val);
    std::string codegenBlock(const std::vector<std::unique_ptr<class StatementNode>>& statements);
    std::string codegenIf(const class ExpressionNode& cond, const class StatementNode& thenBr, const class StatementNode* elseBr);
    std::string codegenWhile(const class ExpressionNode& cond, const class StatementNode& body);
    std::string codegenReturn(const class ExpressionNode* expr);
    std::string codegenExprStmt(const class ExpressionNode& expr);

private:
    IRProgram program;
    IRFunction currentFunc;
    int regCounter;
    int labelCounter;

    // Standard lexical scoping
    std::vector<std::map<std::string, std::string>> symbolTables;
};

#endif // IR_HPP
