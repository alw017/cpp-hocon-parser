#pragma once
#include <lexer.hpp>
#include <token.hpp>
#include <unordered_map>
#include <memory>

enum ASTNodeType {
    AST_STRING, AST_NULL, AST_FALSE, AST_TRUE, AST_NUMBER, AST_OBJECT, AST_ARRAY, AST_MEMBER, AST_VALUE
};

struct ASTValue;

struct ASTArray {
    std::vector<ASTValue*> arr;
    ASTArray() : arr(std::vector<ASTValue*>()) {}
    ASTArray(std::vector<ASTValue*> arr) : arr(arr) {}
    ~ASTArray();
};

struct ASTMember {
    std::string key;
    ASTValue* value;
    ASTMember(std::string key, ASTValue* value) : key(key), value(value) {} 
    ASTMember(): key(""), value(nullptr){}
    ~ASTMember();
};

struct ASTObject {
    std::vector<ASTMember*> members;
    ASTObject() : members(std::vector<ASTMember*>()) {}
    ASTObject(std::vector<ASTMember*> members) : members(members) {}
    ~ASTObject();
};

struct ASTMapObject {
    std::unordered_map<std::string, ASTValue *> members;
    ASTMapObject() : members(std::unordered_map<std::string, ASTValue *>()) {}
    ASTMapObject(std::unordered_map<std::string, ASTValue *> members) : members(members) {}
    ~ASTMapObject();
};

struct ASTValue {
    std::variant<int, double, bool, ASTObject *, ASTArray *, std::string, ASTMapObject *> value;
    ASTValue(int val) : value(val) {}
    ASTValue(double val) : value(val) {}
    ASTValue(bool val) : value(val) {}
    ASTValue(ASTObject * val) : value(val) {}
    ASTValue(ASTMapObject * val) : value(val) {}
    ASTValue(ASTArray * val) : value(val) {}
    ASTValue(std::string val) : value(val) {}
    ~ASTValue();
};

namespace parse_util {
    std::string parseValues(std::vector<ASTValue> values); // remove later. debug function
    std::string string(ASTMapObject * obj);
    std::string array(ASTArray * arr);
    std::string to_string(ASTValue * val);
}

class Parser {
    private:
        int start = 0;
        int current = 0;
        int line;
        int length;
        const std::vector<Token> tokens;
        ASTMapObject * mapObject();
        ASTObject * object();
        ASTMember * member();
        std::pair<std::string, ASTValue *> mapMember();
        ASTArray * array();
        double number();
        bool boolean();
        int nullvalue();
        std::string string();
        ASTValue * value();
    public: 
        ASTMapObject * root;
        bool valid = true;
        bool parseTokens();
        bool atEnd();
        bool scanExpr();
        void error(int line, std::string message);
        void report(int line, std::string where, std::string message);
        bool match(std::vector<TokenType> types);
        bool match(TokenType type);
        bool check(TokenType type);
        Token advance();
        Token peek();
        Token previous();
        Parser(std::vector<Token> tokens) : tokens(tokens), length(tokens.size()) {}
        ~Parser();
};

