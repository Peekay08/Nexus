#include "parser.hpp"
#include "compiler/diagnostics.hpp"
#include <iostream>
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens, const std::string& filename) : tokens(tokens), filename(filename), current(0) {}

std::unique_ptr<ProgramNode> Parser::parse() {
    std::vector<std::unique_ptr<FunctionDeclNode>> functions;
    
    try {
        while (!isAtEnd()) {
            if (check(TokenType::KEYWORD_CLASS) || check(TokenType::KEYWORD_STRUCT) ||
                check(TokenType::KEYWORD_TEMPLATE) || check(TokenType::KEYWORD_NAMESPACE) ||
                check(TokenType::KEYWORD_TRY) || check(TokenType::KEYWORD_CATCH) ||
                check(TokenType::KEYWORD_THROW) || check(TokenType::KEYWORD_NEW) ||
                check(TokenType::KEYWORD_DELETE)) {
                Token tok = advance();
                error(tok, "Feature '" + tok.value + "' is not supported in NEXUS 1.0.");
            }
            functions.push_back(parseFunctionDecl());
        }
    } catch (const ParserException& e) {
        throw;
    }

    return std::make_unique<ProgramNode>(std::move(functions));
}

// --- Helpers ---

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOK_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) return advance();
    error(peek(), errorMessage);
    return Token{}; // unreachable
}

void Parser::error(const Token& token, const std::string& message) {
    if (token.type == TokenType::TOK_ERROR) {
        throw ParserException(token.value);
    }
    
    std::string code = "NX-201";
    std::string errType = "Parser Error";
    
    if (token.type == TokenType::KEYWORD_CLASS || token.type == TokenType::KEYWORD_STRUCT ||
        token.type == TokenType::KEYWORD_TEMPLATE || token.type == TokenType::KEYWORD_NAMESPACE ||
        token.type == TokenType::KEYWORD_TRY || token.type == TokenType::KEYWORD_CATCH ||
        token.type == TokenType::KEYWORD_THROW || token.type == TokenType::KEYWORD_NEW ||
        token.type == TokenType::KEYWORD_DELETE) {
        code = "NX-203";
        errType = "Parser Error (Unsupported Feature)";
    } else if (message.find("Expected expression") != std::string::npos ||
               message.find("Expected parameter") != std::string::npos) {
        code = "NX-202";
    }
    
    std::string formatted = formatDiagnostic(code, errType, message, filename, token.line, token.column);
    throw ParserException(formatted);
}

// --- Grammar Rules ---

std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDecl() {
    std::string returnType = "";
    if (match({TokenType::KEYWORD_INT, TokenType::KEYWORD_FLOAT, TokenType::KEYWORD_VOID})) {
        returnType = previous().value;
    } else {
        error(peek(), "Expected return type ('int', 'float', 'void') for function declaration.");
    }

    Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name.");
    std::string name = nameToken.value;

    consume(TokenType::LPAREN, "Expected '(' after function name.");
    std::vector<FunctionArg> args;
    
    if (!check(TokenType::RPAREN)) {
        do {
            std::string argType = "";
            if (match({TokenType::KEYWORD_INT, TokenType::KEYWORD_FLOAT})) {
                argType = previous().value;
            } else {
                error(peek(), "Expected parameter type ('int', 'float').");
            }
            
            Token argNameToken = consume(TokenType::IDENTIFIER, "Expected parameter name.");
            args.push_back(FunctionArg{argType, argNameToken.value});
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameter list.");

    consume(TokenType::LBRACE, "Expected '{' before function body.");
    std::unique_ptr<BlockNode> body = parseBlock();

    return std::make_unique<FunctionDeclNode>(returnType, name, args, std::move(body));
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    std::vector<std::unique_ptr<StatementNode>> statements;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }

    consume(TokenType::RBRACE, "Expected '}' after block.");
    return std::make_unique<BlockNode>(std::move(statements));
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    if (check(TokenType::KEYWORD_CLASS) || check(TokenType::KEYWORD_STRUCT) ||
        check(TokenType::KEYWORD_TEMPLATE) || check(TokenType::KEYWORD_NAMESPACE) ||
        check(TokenType::KEYWORD_TRY) || check(TokenType::KEYWORD_CATCH) ||
        check(TokenType::KEYWORD_THROW) || check(TokenType::KEYWORD_NEW) ||
        check(TokenType::KEYWORD_DELETE)) {
        Token tok = advance();
        error(tok, "Feature '" + tok.value + "' is not supported in NEXUS 1.0.");
    }

    if (match({TokenType::KEYWORD_IF})) return parseIf();
    if (match({TokenType::KEYWORD_WHILE})) return parseWhile();
    if (match({TokenType::KEYWORD_RETURN})) return parseReturn();
    if (check(TokenType::LBRACE)) {
        advance();
        return parseBlock();
    }
    
    if (check(TokenType::KEYWORD_INT) || check(TokenType::KEYWORD_FLOAT)) {
        return parseVarDecl();
    }

    return parseAssignOrExprStmt();
}

std::unique_ptr<VarDeclNode> Parser::parseVarDecl() {
    advance();
    std::string typeName = previous().value;

    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name.");
    std::string name = nameToken.value;

    std::unique_ptr<ExpressionNode> initVal = nullptr;
    if (match({TokenType::OP_ASSIGN})) {
        initVal = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return std::make_unique<VarDeclNode>(typeName, name, std::move(initVal));
}

std::unique_ptr<StatementNode> Parser::parseAssignOrExprStmt() {
    if (check(TokenType::IDENTIFIER)) {
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::OP_ASSIGN) {
            Token nameToken = advance();
            advance(); // consume '='
            std::unique_ptr<ExpressionNode> val = parseExpression();
            consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
            return std::make_unique<AssignNode>(nameToken.value, std::move(val));
        }
    }

    std::unique_ptr<ExpressionNode> expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement.");
    return std::make_unique<ExprStmtNode>(std::move(expr));
}

std::unique_ptr<IfStmtNode> Parser::parseIf() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    std::unique_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after 'if' condition.");

    std::unique_ptr<StatementNode> thenBr = parseStatement();
    std::unique_ptr<StatementNode> elseBr = nullptr;

    if (match({TokenType::KEYWORD_ELSE})) {
        elseBr = parseStatement();
    }

    return std::make_unique<IfStmtNode>(std::move(condition), std::move(thenBr), std::move(elseBr));
}

std::unique_ptr<WhileStmtNode> Parser::parseWhile() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    std::unique_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after 'while' condition.");

    std::unique_ptr<StatementNode> bodyNode = parseStatement();
    return std::make_unique<WhileStmtNode>(std::move(condition), std::move(bodyNode));
}

std::unique_ptr<ReturnStmtNode> Parser::parseReturn() {
    std::unique_ptr<ExpressionNode> expr = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        expr = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return statement.");
    return std::make_unique<ReturnStmtNode>(std::move(expr));
}

// --- Expressions ---

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
    return parseEquality();
}

std::unique_ptr<ExpressionNode> Parser::parseEquality() {
    std::unique_ptr<ExpressionNode> expr = parseComparison();

    while (match({TokenType::OP_EQ, TokenType::OP_NE})) {
        std::string op = previous().value;
        std::unique_ptr<ExpressionNode> right = parseComparison();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseComparison() {
    std::unique_ptr<ExpressionNode> expr = parseLShift();

    while (match({TokenType::OP_LT, TokenType::OP_LE, TokenType::OP_GT, TokenType::OP_GE})) {
        std::string op = previous().value;
        std::unique_ptr<ExpressionNode> right = parseLShift();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseLShift() {
    std::unique_ptr<ExpressionNode> expr = parseTerm();

    while (match({TokenType::OP_LSHIFT})) {
        std::string op = previous().value;
        std::unique_ptr<ExpressionNode> right = parseTerm();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseTerm() {
    std::unique_ptr<ExpressionNode> expr = parseFactor();

    while (match({TokenType::OP_ADD, TokenType::OP_SUB})) {
        std::string op = previous().value;
        std::unique_ptr<ExpressionNode> right = parseFactor();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseFactor() {
    std::unique_ptr<ExpressionNode> expr = parsePrimary();

    while (match({TokenType::OP_MUL, TokenType::OP_DIV, TokenType::OP_MOD})) {
        std::string op = previous().value;
        std::unique_ptr<ExpressionNode> right = parsePrimary();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    if (match({TokenType::LITERAL_INT})) {
        return std::make_unique<LiteralIntNode>(std::stoi(previous().value));
    }

    if (match({TokenType::LITERAL_FLOAT})) {
        return std::make_unique<LiteralFloatNode>(std::stof(previous().value));
    }

    if (match({TokenType::LITERAL_STRING})) {
        return std::make_unique<LiteralStringNode>(previous().value);
    }

    if (check(TokenType::IDENTIFIER)) {
        Token firstIdent = advance();
        std::string name = firstIdent.value;
        
        if (check(TokenType::DOUBLE_COLON)) {
            advance(); // consume '::'
            Token secondIdent = consume(TokenType::IDENTIFIER, "Expected identifier after '::'.");
            name = name + "::" + secondIdent.value;
        }

        if (match({TokenType::LPAREN})) {
            std::vector<std::unique_ptr<ExpressionNode>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expected ')' after argument list.");
            return std::make_unique<FuncCallNode>(name, std::move(args));
        }

        return std::make_unique<IdentifierNode>(name);
    }

    if (match({TokenType::LPAREN})) {
        std::unique_ptr<ExpressionNode> expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }

    error(peek(), "Expected expression.");
    return nullptr; // unreachable
}
