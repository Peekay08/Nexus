#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <ostream>

enum class TokenType {
    // Keywords
    KEYWORD_INT,
    KEYWORD_FLOAT,
    KEYWORD_VOID,
    KEYWORD_RETURN,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    
    // Unsupported keywords in NEXUS 1.0 (to reject cleanly)
    KEYWORD_CLASS,
    KEYWORD_STRUCT,
    KEYWORD_TEMPLATE,
    KEYWORD_NAMESPACE,
    KEYWORD_TRY,
    KEYWORD_CATCH,
    KEYWORD_THROW,
    KEYWORD_NEW,
    KEYWORD_DELETE,

    // Identifiers and Literals
    IDENTIFIER,
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_STRING,

    // Operators
    OP_ASSIGN,       // =
    OP_ADD,          // +
    OP_SUB,          // -
    OP_MUL,          // *
    OP_DIV,          // /
    OP_MOD,          // %
    OP_EQ,           // ==
    OP_NE,           // !=
    OP_LT,           // <
    OP_LE,           // <=
    OP_GT,           // >
    OP_GE,           // >=
    OP_LSHIFT,       // << (for std::cout)

    // Delimiters
    LPAREN,          // (
    RPAREN,          // )
    LBRACE,          // {
    RBRACE,          // }
    SEMICOLON,       // ;
    COMMA,           // ,
    COLON,           // :
    DOUBLE_COLON,    // ::

    // Special
    TOK_EOF,
    TOK_ERROR
};

std::string tokenTypeToString(TokenType type);

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    friend std::ostream& operator<<(std::ostream& os, const Token& token) {
        os << "Token(" << tokenTypeToString(token.type) << ", \"" << token.value 
           << "\", L" << token.line << ":C" << token.column << ")";
        return os;
    }
};

class Lexer {
public:
    explicit Lexer(const std::string& source, const std::string& filename = "unknown.cpp");
    std::vector<Token> tokenize();

private:
    std::string source;
    std::string filename;
    size_t position;
    int line;
    int column;

    char peek() const;
    char next();
    void skipWhitespaceAndComments();
    Token readWord();
    Token readNumber();
    Token readString();
};

#endif // LEXER_HPP
