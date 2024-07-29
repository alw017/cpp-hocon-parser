#pragma once

enum TokenType {
    LEFT_BRACKET, RIGHT_BRACKET, LEFT_BRACE, RIGHT_BRACE, // bracket = [] brace = {}
    COMMA, COLON, WHITESPACE, DOLLAR, EQUAL, QUESTION, NEWLINE, 
    LEFT_PAREN, RIGHT_PAREN,

    PLUS_EQUAL,

    QUOTED_STRING, UNQUOTED_STRING, NUMBER,

    TRUE, FALSE, NULLVALUE,

    ENDFILE
};

// note this may be better defined as a struct.
class Token {
    public:
        const TokenType type;
        const std::string lexeme;
        const int line;
        const std::variant<double, int, std::string> literal;

        Token(TokenType type, std::string lexeme, std::variant<double, int, std::string> literal, int line) : type(type), lexeme(lexeme), literal(literal), line(line) {}
        std::string str();
};
