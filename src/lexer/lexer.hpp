#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include "token.hpp"

class Lexer {
    public:
        Lexer(std::string text);
        std::vector<Token> run();
    //private: temporary to dbug.
        int start = 0; //TODO Refactor to size_t later.
        int current = 0;
        int line = 1;
        int length;
        bool hasError = false;
        std::string source;
        std::vector<Token> tokens;

        void setSource(std::string newSource);
        bool atEnd();
        void scanToken();
        void error(int line, std::string message);
        void report(int line, std::string where, std::string message);
        char advance();
        void addToken(TokenType type);
        void addToken(TokenType type, std::string str);
        void addToken(TokenType type, int num);
        void addToken(TokenType type, double num);
        void addToken(TokenType type, bool b);
        char peek();
        char peekNext();
        char peekNextNext();
        void multiLineString();
        void quotedString();
        void unquotedString(char c);
        void number();
        void comment();
        void whitespace();
        void newline();
        void substitution(bool optional);
        void pruneInlineWhitespace();
        void pruneAllWhitespace();
        void pruneWsAndComments();
        bool isDigit(char c);
        bool isAlpha(char c);
        bool isHex(char c);
        bool isWhitespace(char c);
        bool isForbiddenChar(char c);
        bool isNextSequenceComment();
        void keyword();
};
