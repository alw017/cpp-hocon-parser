#include "hocon-p.hpp"

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

auto deleteHObj = Overload {                                     
    [](HTree * obj) { delete obj; },
    [](HArray * arr) { delete arr; },
    [](HSimpleValue * val) { delete val; },
};

auto stringify = Overload {                                     
    [](HTree * obj) { return obj->str(); },
    [](HArray * arr) { return arr->str(); },
    [](HSimpleValue * val) { return val->str(); },
};


HTree::HTree() : members(std::unordered_map<HKey *, std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
        delete pair.first;
    }
}

void HTree::addMember(HKey * key, std::variant<HTree*, HArray*, HSimpleValue*> value) {
    members.insert(std::make_pair(key, value));
    memberOrder.push_back(key);
}

std::string HTree::str() {
    const std::string INDENT = "    ";
    std::string out = "{\n";
    for(HKey* keyPointer : memberOrder) {
        auto value = members[keyPointer];
        out += INDENT + keyPointer->key + " : ";
        if (std::holds_alternative<HTree*>(value)) {
            std::string string = std::get<HTree*>(value)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += INDENT + word + "\n"; 
            }
        } else {
            out += std::visit(stringify, value) + "\n";
        }
    }
    out += "}";
    return out;
}

HArray::HArray() : elements(std::vector<std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HArray::~HArray() {
    for (auto e : elements) {
        std::visit(deleteHObj, e);
    }
}

std::string HArray::str() {
    return "[]";
}

void HArray::addElement(std::variant<HTree*, HArray*, HSimpleValue*> val) {
    elements.push_back(val);
}

HSimpleValue::HSimpleValue(std::variant<int, double, bool, std::string> s, std::vector<Token> tokenParts): svalue(s), tokenParts(tokenParts) {}

std::string HSimpleValue::str() {
    std::string output;
    for (auto t : tokenParts) {
        output += t.lexeme;
    }
    return output;
}

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
/*
    panic mode method to consume until the next member to parse.
*/
void HParser::consumeMember() {
    while(!check(std::vector<TokenType>{NEWLINE, COMMA, RIGHT_BRACE}) && !atEnd()) {
        advance();
    }
}

/* 
    Helper method to consume all tokens until the next member. 
*/
void HParser::consumeToNextMember() {
    ignoreAllWhitespace();
    if(match(COMMA)) {
        ignoreAllWhitespace();
    } 
    if (!check(SIMPLE_VALUES) && !check(RIGHT_BRACE)) {
        error(peek().line, "Unexpected symbol " + peek().lexeme + " after member");
        consumeMember();
    }
}

void HParser::consumeToNextRootMember() {
    ignoreAllWhitespace();
    if(match(COMMA)) {
        ignoreAllWhitespace();
    } 
    if (!check(SIMPLE_VALUES) && !atEnd()) {
        error(peek().line, "Unexpected symbol " + peek().lexeme + " after member");
        consumeMember();
    }
}


// create parsed objects :: assignment

/*
    Attempts to create a hocon object with the following tokens, consuming all tokens including the ending '}'.
    Assumes you are within the object, after the first {
*/
HTree * HParser::rootTree() { 
    HTree * output = new HTree();
    while(!atEnd()) { // loop through members
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
            } else if (match(LEFT_BRACKET)) {
                //output->addMember(key, hoconArray());
            } else {   
                output->addMember(key, hoconSimpleValue());
            }
            consumeToNextRootMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key->key + "'");
            consumeToNextRootMember();
        }
    }
    return output;
}

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
            } else if (match(LEFT_BRACKET)) {
                //output->addMember(key, hoconArray());
            } else {   
                output->addMember(key, hoconSimpleValue());
            }
            consumeToNextMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key->key + "'");
            consumeMember();
        }
    }
    return output;
}

/* 
    Assume you have just consumed the k-v separator. Consume until newline or comma.
*/
HSimpleValue * HParser::hoconSimpleValue() {
    std::vector<Token> valTokens = std::vector<Token>();
    while(check(SIMPLE_VALUES) || check(WHITESPACE)) {
        valTokens.push_back(advance());
    }
    if (valTokens.size() > 1) { // value concat, parse as string
        std::stringstream ss {""};
        while ((valTokens.end() - 1)->type == WHITESPACE) {
            valTokens.pop_back();
        }
        for (auto t : valTokens) {
            ss << t.lexeme;
        }
        return new HSimpleValue(ss.str(), valTokens);
    } else if (valTokens.size() == 1) { // parse normally
        return new HSimpleValue(valTokens.begin()->literal, valTokens);
    } else {
        error(peek().line, "Expected a value, got nothing");
        return new HSimpleValue(0, valTokens);
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
    while(check(SIMPLE_VALUES) || check(WHITESPACE)) {
        keyTokens.push_back(advance());
    } // newline implies one of { : =, otherwise it's an error. left brace is implicit separator.
    ignoreAllWhitespace(); // for newline case only.
    if (keyTokens.size() > 0) { // value concat, parse as string
        while ((keyTokens.end() - 1)->type == WHITESPACE) {
            keyTokens.pop_back();
        }
        for (auto t : keyTokens) {
            ss << t.lexeme;
        }
    } else {
        error(peek().line, "Expected a value, got nothing");
        return new HKey("", keyTokens);
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
        ignoreAllWhitespace();
        if (rootBrace) {
            rootObject = hoconTree();
        } else {
            rootObject = rootTree();
        }
        ignoreAllWhitespace();
        if (!atEnd()){
            error(peek().line, "Expected EOF, got " + peek().lexeme);
        }
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
