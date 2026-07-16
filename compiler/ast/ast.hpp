#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward declarations
class IRGenerator;

enum class ASTNodeType {
    PROGRAM,
    FUNCTION_DECL,
    VAR_DECL,
    ASSIGN,
    BLOCK,
    IF_STMT,
    WHILE_STMT,
    RETURN_STMT,
    EXPR_STMT,
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_STRING,
    IDENTIFIER,
    BINARY_OP,
    FUNC_CALL
};

/**
 * @brief Base class for all Abstract Syntax Tree (AST) nodes.
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual ASTNodeType getType() const = 0;
    virtual void print(int indent = 0) const = 0;
    virtual std::string codegen(IRGenerator& gen) const = 0;

protected:
    void printIndent(int indent) const {
        for (int i = 0; i < indent; ++i) {
            std::cout << "  ";
        }
    }
};

/**
 * @brief Base class for all expression AST nodes.
 */
class ExpressionNode : public ASTNode {};

/**
 * @brief Base class for all statement AST nodes.
 */
class StatementNode : public ASTNode {};

// --- Expression Nodes ---

/**
 * @brief Represents an integer literal expression.
 * 
 * Example: 42
 */
class LiteralIntNode : public ExpressionNode {
public:
    int value;
    explicit LiteralIntNode(int val) : value(val) {}
    
    ASTNodeType getType() const override { return ASTNodeType::LITERAL_INT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "LiteralInt: " << value << "\n";
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a float literal expression.
 * 
 * Example: 3.14
 */
class LiteralFloatNode : public ExpressionNode {
public:
    float value;
    explicit LiteralFloatNode(float val) : value(val) {}
    
    ASTNodeType getType() const override { return ASTNodeType::LITERAL_FLOAT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "LiteralFloat: " << value << "\n";
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a string literal expression.
 * 
 * Example: "Hello World"
 */
class LiteralStringNode : public ExpressionNode {
public:
    std::string value;
    explicit LiteralStringNode(const std::string& val) : value(val) {}
    
    ASTNodeType getType() const override { return ASTNodeType::LITERAL_STRING; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "LiteralString: \"" << value << "\"\n";
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents an identifier (variable name).
 * 
 * Example: x
 */
class IdentifierNode : public ExpressionNode {
public:
    std::string name;
    explicit IdentifierNode(const std::string& name) : name(name) {}
    
    ASTNodeType getType() const override { return ASTNodeType::IDENTIFIER; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Identifier: " << name << "\n";
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a binary operator expression.
 * 
 * Example: a + b, x <= y
 */
class BinaryOpNode : public ExpressionNode {
public:
    std::string op;
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    
    BinaryOpNode(const std::string& op, std::unique_ptr<ExpressionNode> lhs, std::unique_ptr<ExpressionNode> rhs)
        : op(op), left(std::move(lhs)), right(std::move(rhs)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::BINARY_OP; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "BinaryOp (" << op << "):\n";
        left->print(indent + 1);
        right->print(indent + 1);
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a function call expression.
 * 
 * Example: add(1, 2)
 */
class FuncCallNode : public ExpressionNode {
public:
    std::string funcName;
    std::vector<std::unique_ptr<ExpressionNode>> args;
    
    FuncCallNode(const std::string& name, std::vector<std::unique_ptr<ExpressionNode>> arguments)
        : funcName(name), args(std::move(arguments)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::FUNC_CALL; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "FuncCall (" << funcName << "):\n";
        for (const auto& arg : args) {
            arg->print(indent + 1);
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

// --- Statement Nodes ---

/**
 * @brief Represents a variable declaration statement.
 * 
 * Example: int x = 10;
 */
class VarDeclNode : public StatementNode {
public:
    std::string typeName;
    std::string name;
    std::unique_ptr<ExpressionNode> initVal; // can be nullptr
    
    VarDeclNode(const std::string& type, const std::string& name, std::unique_ptr<ExpressionNode> init = nullptr)
        : typeName(type), name(name), initVal(std::move(init)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::VAR_DECL; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "VarDecl (" << typeName << " " << name << ")";
        if (initVal) {
            std::cout << " =\n";
            initVal->print(indent + 1);
        } else {
            std::cout << "\n";
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a variable assignment statement.
 * 
 * Example: x = 42;
 */
class AssignNode : public StatementNode {
public:
    std::string name;
    std::unique_ptr<ExpressionNode> val;
    
    AssignNode(const std::string& name, std::unique_ptr<ExpressionNode> value)
        : name(name), val(std::move(value)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::ASSIGN; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Assign (" << name << ") =\n";
        val->print(indent + 1);
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a block statement containing a list of statements.
 * 
 * Example: { x = 1; y = 2; }
 */
class BlockNode : public StatementNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    explicit BlockNode(std::vector<std::unique_ptr<StatementNode>> stmts)
        : statements(std::move(stmts)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::BLOCK; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Block:\n";
        for (const auto& stmt : statements) {
            stmt->print(indent + 1);
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents an if/else conditional statement.
 * 
 * Example: if (x > 0) { return 1; } else { return 0; }
 */
class IfStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> cond;
    std::unique_ptr<StatementNode> thenBranch;
    std::unique_ptr<StatementNode> elseBranch; // can be nullptr
    
    IfStmtNode(std::unique_ptr<ExpressionNode> condition, std::unique_ptr<StatementNode> thenBr, std::unique_ptr<StatementNode> elseBr = nullptr)
        : cond(std::move(condition)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::IF_STMT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "If:\n";
        cond->print(indent + 1);
        printIndent(indent);
        std::cout << "Then:\n";
        thenBranch->print(indent + 1);
        if (elseBranch) {
            printIndent(indent);
            std::cout << "Else:\n";
            elseBranch->print(indent + 1);
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a while loop statement.
 * 
 * Example: while (i < 10) { i = i + 1; }
 */
class WhileStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> cond;
    std::unique_ptr<StatementNode> body;
    
    WhileStmtNode(std::unique_ptr<ExpressionNode> condition, std::unique_ptr<StatementNode> bodyNode)
        : cond(std::move(condition)), body(std::move(bodyNode)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::WHILE_STMT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "While:\n";
        cond->print(indent + 1);
        printIndent(indent);
        std::cout << "Body:\n";
        body->print(indent + 1);
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents a return statement.
 * 
 * Example: return x;
 */
class ReturnStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expr; // can be nullptr
    
    explicit ReturnStmtNode(std::unique_ptr<ExpressionNode> expression = nullptr)
        : expr(std::move(expression)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::RETURN_STMT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Return:\n";
        if (expr) {
            expr->print(indent + 1);
        } else {
            printIndent(indent + 1);
            std::cout << "void\n";
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents an expression statement.
 * 
 * Example: add(1, 2);
 */
class ExprStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expr;
    
    explicit ExprStmtNode(std::unique_ptr<ExpressionNode> expression)
        : expr(std::move(expression)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::EXPR_STMT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ExprStmt:\n";
        expr->print(indent + 1);
    }
    std::string codegen(IRGenerator& gen) const override;
};

// --- Top Level Declarations ---

/**
 * @brief Represents a function argument.
 */
struct FunctionArg {
    std::string type;
    std::string name;
};

/**
 * @brief Represents a function declaration.
 * 
 * Example: int main() { return 0; }
 */
class FunctionDeclNode : public ASTNode {
public:
    std::string returnType;
    std::string name;
    std::vector<FunctionArg> args;
    std::unique_ptr<BlockNode> body;
    
    FunctionDeclNode(const std::string& ret, const std::string& name, std::vector<FunctionArg> args, std::unique_ptr<BlockNode> body)
        : returnType(ret), name(name), args(std::move(args)), body(std::move(body)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::FUNCTION_DECL; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "FunctionDecl (" << returnType << " " << name << ") args:[";
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << args[i].type << " " << args[i].name;
            if (i + 1 < args.size()) std::cout << ", ";
        }
        std::cout << "]:\n";
        body->print(indent + 1);
    }
    std::string codegen(IRGenerator& gen) const override;
};

/**
 * @brief Represents the entire program containing global function declarations.
 */
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDeclNode>> functions;
    
    explicit ProgramNode(std::vector<std::unique_ptr<FunctionDeclNode>> funcs)
        : functions(std::move(funcs)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::PROGRAM; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Program:\n";
        for (const auto& func : functions) {
            func->print(indent + 1);
        }
    }
    std::string codegen(IRGenerator& gen) const override;
};

#endif // AST_HPP
