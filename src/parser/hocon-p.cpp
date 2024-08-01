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

void HTree::addMember(HKey * key, std::variant<HTree*, HArray*, HSimpleValue*> value) {
    members.insert(std::make_pair(key, value));
}

HArray::HArray() : elements(std::vector<std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HArray::~HArray() {
    for (auto e : elements) {
        std::visit(deleteHObj, e);
    }
}

void HArray::addElement(std::variant<HTree*, HArray*, HSimpleValue*> val) {
    elements.push_back(val);
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

bool HParser::match(std::vector<TokenType> types) {
    if(check(types)) {
        advance();
        return true;
    } else return false;
}


void HParser::ignoreAllWhitespace() {
    while(check(WHITESPACE) || check(NEWLINE)) {
        advance();
    }
}

void HParser::ignoreInlineWhitespace() {
    while(check(WHITESPACE)) {
        advance();
    }
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
    while(!match(RIGHT_BRACE)) { // loop through members
        if(atEnd()) {
            error(peek().line, "Imbalanced {}");
            break;
        }
        HKey * key = hoconKey(); 
        if (match(LEFT_BRACE)) {
            HTree * curr = hoconTree(); // ends after consuming right brace.
            while (match(LEFT_BRACE)) {
                //curr = mergeTrees(curr, hoconTree());
                hoconTree();
            }
            output->addMember(key, curr); // implied separator case. ex: foo {}
            ignoreAllWhitespace();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                output->addMember(key, hoconTree());
                ignoreAllWhitespace();
            } else if (match(LEFT_BRACKET)) {
                //output->addMember(key, hoconArray());
                ignoreAllWhitespace();
            } else {   
                //output->addMember(key, hoconSimpleValue());
                ignoreAllWhitespace();
            }
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key->key + "'");
        }
    }
    return output;
}

/* 
    Assume you have just consumed the k-v separator. Consume until newline or comma.
*/
HSimpleValue * HParser::hoconSimpleValue() {
    std::vector<Token> valTokens = std::vector<Token>();
    do {
        if (check(SIMPLE_VALUES) || check(WHITESPACE)) {
            valTokens.push_back(advance());
        } else {
            error(peek().line, "Expected a simple value, got " + peek().lexeme);
            return new HSimpleValue(0);
            // put error recovery later. maybe in HParser::error() and not here specifically.
        }   
    } while(!check(std::vector<TokenType> {COMMA, NEWLINE}));
    if (valTokens.size() > 1) { // value concat
        
    } else if (valTokens.size() == 1) { // parse normally

    } else {
        error(peek().line, "Expected a value, got nothing");
    }
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
            return new HKey(ss.str(), keyTokens);
            // put error recovery later. maybe in HParser::error() and not here specifically.
        }   
    } while(!(check(KEY_VALUE_SEP) || check(std::vector<TokenType> {LEFT_BRACE, NEWLINE}))); // newline implies one of { : =, otherwise it's an error. left brace is implicit separator.
    ignoreAllWhitespace(); // for newline case only.
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
        //rootObject = hoconArray();
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