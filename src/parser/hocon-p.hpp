#pragma once
#include <lexer.hpp>
#include <token.hpp>
#include <unordered_map>
#include <memory>

class HParser {
    private:
        //file properties
        bool rootBrace = true; // rootBrace must be true if the root object is HArray.
        bool validConf = true;
        std::variant<HTree, HArray> rootObject;

        //look ahead/back
        Token peek();
        Token peekNext();
        Token previous();
        bool check(TokenType type);
        
        //consume
        Token advance();
        bool match(TokenType type);
        bool match(std::vector<TokenType> type);

        //create parsed objects :: Assignment
        HTree * hoconTree();
        HArray * hoconArray();
        HSimpleValue * hoconSimpleValue();

        //object merge/concatenation
        HTree * mergeTrees(HTree * first, HTree * second);

        //concatenation
        HArray * concatArrays(HArray * first, HArray * second);
        HSimpleValue * concatSimpleValues(HSimpleValue * first, HSimpleValue * second);

        //parsing steps:
        void parseTokens(); // first pass, creating AST and merging whenever possible.
        void resolveSubstitutions(); // second pass, resolving substitutions and resolving the remaining merges dependent on substitutions.

        //error reporting
        void error(int line, std::string message);
        void report(int line, std::string where, std::string message);
    public:
        void run(); 
        HParser();
        ~HParser();
};


class HTree {
    std::unordered_map<std::string, std::variant<HTree, HArray, HSimpleValue>> members;
    HTree() : members(std::unordered_map<std::string, std::variant<HTree, HArray, HSimpleValue>>()) {}
    ~HTree();
    void addMember(std::variant<HTree, HArray, HSimpleValue> val);
};

class HArray {
    std::vector<std::variant<HTree, HArray, HSimpleValue>> elements;
    HArray() : elements(std::vector<std::variant<HTree, HArray, HSimpleValue>>()) {}
    ~HArray();
    void addMember(std::variant<HTree, HArray, HSimpleValue> val);
};

class HSimpleValue {
    std::variant<int, double, bool, std::string> svalue;
    HSimpleValue (std::variant<int, double, bool, std::string> s): svalue(s) {}
    ~HSimpleValue();
};

class HSubstitution {

};