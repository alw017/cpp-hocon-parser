#pragma once
#include <lexer.hpp>
#include <token.hpp>
#include <unordered_map>
#include <memory>

class HParser {
    private:
        //look ahead/back
        Token peek();
        Token peekNext();
        Token previous();
        bool check(TokenType type);
        
        //consume
        Token advance();
        bool match(TokenType type);
        bool match(std::vector<TokenType> type);

        //create parsed objects
        HTree hoconTree();
        HArray hoconArray();
        HSimpleValue hoconSimpleValue();

        //merging or concatenation
        HTree mergeTrees(HTree first, HTree second);
        HArray mergeArrays(HArray first, HArray second);
        HSimpleValue mergeSimpleValues(HSimpleValue first, HSimpleValue second);

        //parsing steps:
        void parseTokens(); // first pass to construct AST
        void resolveSubstitutions(); //

        //error reporting
        void error(int line, std::string message);
        void report(int line, std::string where, std::string message);
    public:
        void run(); 
};


class HTree {

};

class HArray {

};

class HSimpleValue {

};