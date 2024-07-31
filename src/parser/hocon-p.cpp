#include "hocon-p.hpp"

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

auto deleteHObj = Overload {                                     
    [](HTree * obj) { delete obj; },
    [](HArray * arr) { delete arr; },
    [](HSimpleValue * val) {},
};

HTree::HTree() : members(std::unordered_map<HKey *, std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
    }
}

HArray::HArray() : elements(std::vector<std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HArray::~HArray() {
    for (auto e : elements) {
        std::visit(deleteHObj, e);
    }
}

HSimpleValue::HSimpleValue(std::variant<int, double, bool, std::string> s): svalue(s) {}

HKey::HKey(std::string k, std::vector<Token> t) : key(k), tokens(t) {}

HSubstitution::HSubstitution(std::string s) : path(s) {}

HParser::~HParser() {
    std::visit(deleteHObj, rootObject);
}

// look ahead/back

Token HParser::peek() {
    return tokenList[current];
}

Token HParser::peekNext() {
    return atEnd() ? peek() : tokenList[current + 1];
}

Token HParser::previous() {
    return current > 0 ? tokenList[current - 1] : tokenList[current];
}

bool HParser::check(TokenType type) {
    return !atEnd() && peek().type == type;
}

bool HParser::check(std::vector<TokenType> types) {
    if(atEnd()) return false;
    for (auto type : types) {
        if(peek().type == type) {
            return true;
        }
    }
    return false;
}

// state checking

bool HParser::atEnd() {
    return peek().type == ENDFILE;
}

// consume
Token HParser::advance() {
    return atEnd()? tokenList[current] : tokenList[current++];
}

bool HParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    } else {
        return false;
    }
}

void HParser::ignoreAllWhitespace() {
    while(check(WHITESPACE) || check(NEWLINE)) {
        advance();
    }
}

bool HParser::match(std::vector<TokenType> types) {
    if(check(types)) {
        advance();
        return true;
    } else return false;
}

void HParser::advanceToNextLine() {
    while(!check(NEWLINE) && !atEnd()) {
        advance();
    }
}

// create parsed objects :: assignment


/*
    Attempts to create a hocon object with the following tokens, consuming all tokens including the ending '}'.
    Assumes you are within the object, after the first {
*/
HTree * HParser::hoconTree() { 
    HTree * output = new HTree();
    ignoreAllWhitespace();
    while(!match(RIGHT_BRACE)) { // empty object case
        HKey * key = hoconKey(); 
        if (match(LEFT_BRACE)) {
            output->addMember(key, hoconTree()); // implied separator case. ex: foo {}
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                output->addMember(key, hoconTree());
            } else if (match(LEFT_BRACKET)) {
                output->addMember(key, hoconArray());
            } else {   
                output->addMember(key, hoconSimpleValue());
            }
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key->key + "'");
        }

    }
    return output;
}

/* 
    Checks if the tokens up to a key-value separator make a valid key. Consumes up to but not including the separator.
    Substitutions are not supported in keys. Any Simple value is allowed in a key. The whitespace between simple values is preserved.
    Assumes you have consumed up to a whitespace before the first token of the key.
*/
HKey * HParser::hoconKey() {
    ignoreAllWhitespace();
    std::vector<Token> keyTokens = std::vector<Token>();
    std::stringstream ss {""};
    do {
        if (check(SIMPLE_VALUES) || check(WHITESPACE)) {
            keyTokens.push_back(advance());
        } else {
            error(peek().line, "Expected a simple value, got " + peek().lexeme);
            // put error recovery later. maybe in HParser::error() and not here specifically.
        }   
    } while(!(check(KEY_VALUE_SEP) || check(std::vector<TokenType> {LEFT_BRACE, NEWLINE}))); // newline implies one of { : =, otherwise it's an error. left brace is implicit separator.
    ignoreAllWhitespace();
    while ((keyTokens.end() - 1)->type == WHITESPACE) {
        keyTokens.pop_back();
    }
    for (auto t : keyTokens) {
        ss << t.lexeme;
    }
    return new HKey(ss.str(), keyTokens);
}

// object merge

// concatenation

// parsing steps:

void HParser::parseTokens() {
    if (match(LEFT_BRACKET)) { // root array
        rootObject = hoconArray();
    } else { // if not array, implicitly assumed to be object.
        rootBrace = match(LEFT_BRACE);
        rootObject = hoconTree();
    }
}

// void resolveSubstitutions() {}

// access methods:

// error reporting:

void HParser::error(int line, std::string message) {
    report(line, "", message);
    validConf = false;
}

void HParser::report(int line, std::string where, std::string message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
}