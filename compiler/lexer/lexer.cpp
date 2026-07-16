#include "lexer.hpp"
#include "compiler/diagnostics.hpp"
#include <cctype>
#include <map>

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_INT: return "KEYWORD_INT";
        case TokenType::KEYWORD_FLOAT: return "KEYWORD_FLOAT";
        case TokenType::KEYWORD_VOID: return "KEYWORD_VOID";
        case TokenType::KEYWORD_RETURN: return "KEYWORD_RETURN";
        case TokenType::KEYWORD_IF: return "KEYWORD_IF";
        case TokenType::KEYWORD_ELSE: return "KEYWORD_ELSE";
        case TokenType::KEYWORD_WHILE: return "KEYWORD_WHILE";
        case TokenType::KEYWORD_CLASS: return "KEYWORD_CLASS";
        case TokenType::KEYWORD_STRUCT: return "KEYWORD_STRUCT";
        case TokenType::KEYWORD_TEMPLATE: return "KEYWORD_TEMPLATE";
        case TokenType::KEYWORD_NAMESPACE: return "KEYWORD_NAMESPACE";
        case TokenType::KEYWORD_TRY: return "KEYWORD_TRY";
        case TokenType::KEYWORD_CATCH: return "KEYWORD_CATCH";
        case TokenType::KEYWORD_THROW: return "KEYWORD_THROW";
        case TokenType::KEYWORD_NEW: return "KEYWORD_NEW";
        case TokenType::KEYWORD_DELETE: return "KEYWORD_DELETE";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::LITERAL_INT: return "LITERAL_INT";
        case TokenType::LITERAL_FLOAT: return "LITERAL_FLOAT";
        case TokenType::LITERAL_STRING: return "LITERAL_STRING";
        case TokenType::OP_ASSIGN: return "OP_ASSIGN";
        case TokenType::OP_ADD: return "OP_ADD";
        case TokenType::OP_SUB: return "OP_SUB";
        case TokenType::OP_MUL: return "OP_MUL";
        case TokenType::OP_DIV: return "OP_DIV";
        case TokenType::OP_MOD: return "OP_MOD";
        case TokenType::OP_EQ: return "OP_EQ";
        case TokenType::OP_NE: return "OP_NE";
        case TokenType::OP_LT: return "OP_LT";
        case TokenType::OP_LE: return "OP_LE";
        case TokenType::OP_GT: return "OP_GT";
        case TokenType::OP_GE: return "OP_GE";
        case TokenType::OP_LSHIFT: return "OP_LSHIFT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::DOUBLE_COLON: return "DOUBLE_COLON";
        case TokenType::TOK_EOF: return "TOK_EOF";
        case TokenType::TOK_ERROR: return "TOK_ERROR";
        default: return "UNKNOWN";
    }
}

static const std::map<std::string, TokenType> keywords = {
    {"int", TokenType::KEYWORD_INT},
    {"float", TokenType::KEYWORD_FLOAT},
    {"void", TokenType::KEYWORD_VOID},
    {"return", TokenType::KEYWORD_RETURN},
    {"if", TokenType::KEYWORD_IF},
    {"else", TokenType::KEYWORD_ELSE},
    {"while", TokenType::KEYWORD_WHILE},
    {"class", TokenType::KEYWORD_CLASS},
    {"struct", TokenType::KEYWORD_STRUCT},
    {"template", TokenType::KEYWORD_TEMPLATE},
    {"namespace", TokenType::KEYWORD_NAMESPACE},
    {"try", TokenType::KEYWORD_TRY},
    {"catch", TokenType::KEYWORD_CATCH},
    {"throw", TokenType::KEYWORD_THROW},
    {"new", TokenType::KEYWORD_NEW},
    {"delete", TokenType::KEYWORD_DELETE}
};

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source(source), filename(filename), position(0), line(1), column(1) {}

char Lexer::peek() const {
    if (position >= source.length()) return '\0';
    return source[position];
}

char Lexer::next() {
    if (position >= source.length()) return '\0';
    char c = source[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (position < source.length()) {
        char c = peek();
        if (isspace(c)) {
            next();
        } else if (c == '/' && position + 1 < source.length() && source[position + 1] == '/') {
            // Single-line comment
            next(); next(); // consume '//'
            while (position < source.length() && peek() != '\n') {
                next();
            }
        } else if (c == '/' && position + 1 < source.length() && source[position + 1] == '*') {
            // Multi-line comment
            next(); next(); // consume '/*'
            while (position < source.length()) {
                if (peek() == '*' && position + 1 < source.length() && source[position + 1] == '/') {
                    next(); next(); // consume '*/'
                    break;
                }
                next();
            }
        } else if (c == '#') {
            // Handle preprocessor or macro directive gracefully by skipping the line
            while (position < source.length() && peek() != '\n') {
                next();
            }
        } else {
            break;
        }
    }
}

Token Lexer::readWord() {
    int startCol = column;
    std::string value;
    while (isalnum(peek()) || peek() == '_') {
        value += next();
    }

    TokenType type = TokenType::IDENTIFIER;
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        type = it->second;
    }

    return Token{type, value, line, startCol};
}

Token Lexer::readNumber() {
    int startCol = column;
    std::string value;
    bool isFloat = false;

    while (isdigit(peek())) {
        value += next();
    }

    if (peek() == '.' && position + 1 < source.length() && isdigit(source[position + 1])) {
        isFloat = true;
        value += next(); // consume '.'
        while (isdigit(peek())) {
            value += next();
        }
    }

    TokenType type = isFloat ? TokenType::LITERAL_FLOAT : TokenType::LITERAL_INT;
    return Token{type, value, line, startCol};
}

Token Lexer::readString() {
    int startCol = column;
    next(); // consume opening quote '"'
    std::string value;
    
    while (position < source.length() && peek() != '"') {
        char c = peek();
        if (c == '\\' && position + 1 < source.length()) {
            next(); // consume '\\'
            char escaped = next();
            if (escaped == 'n') value += '\n';
            else if (escaped == 't') value += '\t';
            else value += escaped;
        } else {
            value += next();
        }
    }

    if (peek() == '"') {
        next(); // consume closing quote '"'
        return Token{TokenType::LITERAL_STRING, value, line, startCol};
    }

    return Token{TokenType::TOK_ERROR, formatDiagnostic("NX-102", "Lexical Error", "Unterminated string literal", filename, line, startCol), line, startCol};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();
        if (position >= source.length()) {
            tokens.push_back(Token{TokenType::TOK_EOF, "", line, column});
            break;
        }

        char c = peek();
        int startCol = column;

        if (isalpha(c) || c == '_') {
            tokens.push_back(readWord());
        } else if (isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (c == '"') {
            tokens.push_back(readString());
        } else {
            // Symbols & Operators
            next();
            switch (c) {
                case '(': tokens.push_back(Token{TokenType::LPAREN, "(", line, startCol}); break;
                case ')': tokens.push_back(Token{TokenType::RPAREN, ")", line, startCol}); break;
                case '{': tokens.push_back(Token{TokenType::LBRACE, "{", line, startCol}); break;
                case '}': tokens.push_back(Token{TokenType::RBRACE, "}", line, startCol}); break;
                case ';': tokens.push_back(Token{TokenType::SEMICOLON, ";", line, startCol}); break;
                case ',': tokens.push_back(Token{TokenType::COMMA, ",", line, startCol}); break;
                case '%': tokens.push_back(Token{TokenType::OP_MOD, "%", line, startCol}); break;
                case '+': tokens.push_back(Token{TokenType::OP_ADD, "+", line, startCol}); break;
                case '-': tokens.push_back(Token{TokenType::OP_SUB, "-", line, startCol}); break;
                case '*': tokens.push_back(Token{TokenType::OP_MUL, "*", line, startCol}); break;
                case '/': tokens.push_back(Token{TokenType::OP_DIV, "/", line, startCol}); break;
                
                case ':':
                    if (peek() == ':') {
                        next();
                        tokens.push_back(Token{TokenType::DOUBLE_COLON, "::", line, startCol});
                    } else {
                        tokens.push_back(Token{TokenType::COLON, ":", line, startCol});
                    }
                    break;
                
                case '=':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(Token{TokenType::OP_EQ, "==", line, startCol});
                    } else {
                        tokens.push_back(Token{TokenType::OP_ASSIGN, "=", line, startCol});
                    }
                    break;

                case '!':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(Token{TokenType::OP_NE, "!=", line, startCol});
                    } else {
                        tokens.push_back(Token{TokenType::TOK_ERROR, formatDiagnostic("NX-101", "Lexical Error", "Unexpected character '!'", filename, line, startCol), line, startCol});
                    }
                    break;

                case '<':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(Token{TokenType::OP_LE, "<=", line, startCol});
                    } else if (peek() == '<') {
                        next();
                        tokens.push_back(Token{TokenType::OP_LSHIFT, "<<", line, startCol});
                    } else {
                        tokens.push_back(Token{TokenType::OP_LT, "<", line, startCol});
                    }
                    break;

                case '>':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(Token{TokenType::OP_GE, ">=", line, startCol});
                    } else {
                        tokens.push_back(Token{TokenType::OP_GT, ">", line, startCol});
                    }
                    break;

                default:
                    std::string errStr(1, c);
                    tokens.push_back(Token{TokenType::TOK_ERROR, formatDiagnostic("NX-101", "Lexical Error", "Unexpected character '" + errStr + "'", filename, line, startCol), line, startCol});
                    break;
            }
        }
    }

    return tokens;
}
