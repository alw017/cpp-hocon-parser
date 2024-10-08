#pragma once

enum TokenType {
    LEFT_BRACKET, RIGHT_BRACKET, LEFT_BRACE, RIGHT_BRACE, // bracket = [] brace = {}
    COMMA, COLON, WHITESPACE, SUB, SUB_OPTIONAL, EQUAL, QUESTION, NEWLINE, 
    LEFT_PAREN, RIGHT_PAREN,

    PLUS_EQUAL,

    QUOTED_STRING, UNQUOTED_STRING, NUMBER,

    TRUE, FALSE, NULLVALUE,

    ENDFILE
};

const std::vector<TokenType> SIMPLE_VALUES {QUOTED_STRING, UNQUOTED_STRING, NUMBER, TRUE, FALSE, NULLVALUE}; 
const std::vector<TokenType> KEY_VALUE_SEP {EQUAL, COLON};

// note this may be better defined as a struct.
class Token {
    public:
        const TokenType type;
        const std::string lexeme;
        const int line;
        const std::variant<int, double, bool, std::string> literal;

        Token(TokenType type, std::string lexeme, std::variant<int, double, bool, std::string> literal, int line) : type(type), lexeme(lexeme), literal(literal), line(line) {}
        std::string str();
};
