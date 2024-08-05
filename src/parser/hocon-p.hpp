#pragma once
#include <lexer.hpp>
#include <token.hpp>
#include <unordered_map>
#include <memory>

struct HArray;
struct HSimpleValue;
//struct HKey;

struct HTree {
    std::unordered_map<std::string, std::variant<HTree*, HArray*, HSimpleValue*>> members;
    std::vector<std::string> memberOrder; 
    HTree();
    ~HTree();
    void addMember(std::string key, std::variant<HTree*, HArray*, HSimpleValue*> value);
    std::string str();
};

struct HArray {
    std::vector<std::variant<HTree*, HArray*, HSimpleValue*>> elements;
    HArray();
    ~HArray();
    void addElement(std::variant<HTree*, HArray*, HSimpleValue*> val);
    std::string str();
};

struct HSimpleValue {
    std::variant<int, double, bool, std::string> svalue; 
    std::vector<Token> tokenParts;
    HSimpleValue(std::variant<int, double, bool, std::string> s, std::vector<Token> tokenParts);
    std::string str();
};

//struct HKey {
//    std::string key;
//    std::vector<Token> tokens;
//    HKey(std::string k, std::vector<Token> t);
//};

struct HSubstitution {
    std::string path;
    HSubstitution(std::string s);
    ~HSubstitution();
};

class HParser {
    public: // change to private later
        //file properties
        bool rootBrace = true; // rootBrace must be true if the root object is HArray.
        bool validConf = true;
        std::variant<HTree *, HArray *> rootObject;
        std::vector<Token> tokenList;
        int start = 0;      // refactor to size_t later.
        int current = 0;
        int length;

        //look ahead/back
        Token peek();
        Token peekNext();
        Token previous();
        bool check(TokenType type);
        bool check(std::vector<TokenType> types);

        //state checking
        bool atEnd();
        
        //consume
        Token advance();
        bool match(TokenType type);
        bool match(std::vector<TokenType> types);
        void ignoreAllWhitespace();
        void ignoreInlineWhitespace();
        void consumeMember();
        void consumeElement();
        void consumeToNextMember();
        void consumeToNextRootMember();
        void consumeToNextElement();

        //create parsed objects :: Assignment
        HTree * rootTree();
        HTree * hoconTree();
        HArray * hoconArray();
        HSimpleValue * hoconSimpleValue();
        std::string hoconKey();

        //object merge/concatenation
        HTree * mergeTrees(HTree * first, HTree * second);

        //concatenation
        HArray * concatArrays(HArray * first, HArray * second);
        //HSimpleValue * concatSimpleValues(HSimpleValue * first, HSimpleValue * second);
        // ^ is automatically performed in hoconSimpleValue();

        //parsing steps:
        void parseTokens(); // first pass, creating AST and merging whenever possible.
        void resolveSubstitutions(); // second pass, resolving substitutions and resolving the remaining merges dependent on substitutions.
        /*
         * Note: to do substitutions, we need to keep an auxillary file keeping track of all object member additions and modifications
         * also, we need to give the substitution a handle on where to enter the file, if it is a self referential substitution.
         */
        //access methods:
        std::variant<HTree*, HArray*> getRoot();
        std::variant<HTree*, HArray*, HSimpleValue*> getByPath(std::string path);


        //error reporting
        void error(int line, std::string message);
        void report(int line, std::string where, std::string message);
    public:
        bool run(); 
        HParser(std::vector<Token> tokens): tokenList(tokens), length(tokens.size()) {};
        ~HParser();
};


