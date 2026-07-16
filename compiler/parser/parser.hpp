#ifndef PARSER_HPP
#define PARSER_HPP

#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

class ParserException : public std::runtime_error {
public:
    explicit ParserException(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens, const std::string& filename = "unknown.cpp");
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens;
    std::string filename;
    size_t current;

    // Helper functions
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& errorMessage);
    void error(const Token& token, const std::string& message);

    // Parsing grammar rules
    std::unique_ptr<FunctionDeclNode> parseFunctionDecl();
    std::unique_ptr<StatementNode> parseStatement();
    std::unique_ptr<VarDeclNode> parseVarDecl();
    std::unique_ptr<StatementNode> parseAssignOrExprStmt();
    std::unique_ptr<BlockNode> parseBlock();
    std::unique_ptr<IfStmtNode> parseIf();
    std::unique_ptr<WhileStmtNode> parseWhile();
    std::unique_ptr<ReturnStmtNode> parseReturn();
    
    // Expressions (Precedence climbing / Recursive descent)
    std::unique_ptr<ExpressionNode> parseExpression();
    std::unique_ptr<ExpressionNode> parseEquality();
    std::unique_ptr<ExpressionNode> parseComparison();
    std::unique_ptr<ExpressionNode> parseTerm();
    std::unique_ptr<ExpressionNode> parseFactor();
    std::unique_ptr<ExpressionNode> parseLShift();
    std::unique_ptr<ExpressionNode> parsePrimary();
};

#endif // PARSER_HPP
