#include "hocon-p.hpp"

const std::string INDENT = "    "; 

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


HTree::HTree() : members(std::unordered_map<std::string, std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
    }
}

void HTree::addMember(std::string key, std::variant<HTree*, HArray*, HSimpleValue*> value) {
    if(members.count(key) == 0) {
        memberOrder.emplace_back(key);
        members.insert(std::make_pair(key, value));
    } else {
        members[key] = value;
    }
}

std::string HTree::str() {
    if(members.empty()) {
        return "{}";
    }
    std::string out = "{\n";
    for(std::string keyval : memberOrder) {
        auto value = members[keyval];
        out += INDENT + keyval + " : ";
        if (std::holds_alternative<HTree*>(value)) {
            std::string string = std::get<HTree*>(value)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += word == "}" ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else if (std::holds_alternative<HArray*>(value)) {
            std::string string = std::get<HArray*>(value)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += word == "]" ? INDENT + word + ",\n" : INDENT + word + "\n";
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

std::string HArray::str() { // tested,
    if(elements.empty()) {
        return "[]";
    }
    std::string out = "[\n";
    for(auto e : elements) {
        out += INDENT;
        if (std::holds_alternative<HTree*>(e)) {
            std::string string = std::get<HTree*>(e)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += word == "}" ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else if (std::holds_alternative<HArray*>(e)) {
            std::string string = std::get<HArray*>(e)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += word == "]" ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else {
            out += std::visit(stringify, e) + ",\n";
        }
    }
    out += "]";
    return out;
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

HSubstitution::HSubstitution(std::string s) : path(s) {}

HParser::~HParser() {
    std::visit(deleteHObj, rootObject);
}

// look ahead/back helpers

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

// consume helper methods
Token HParser::advance() {
    return atEnd() ? tokenList[current] : tokenList[current++];
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
        if(match(LEFT_BRACE)) {
            hoconTree();
        } else if (match(LEFT_BRACKET)) {
            hoconArray();
        }
        else advance();
    }
    match(std::vector<TokenType>{NEWLINE, COMMA});
    ignoreAllWhitespace();
}

/*
    panic mode method to consume until the next element to parse.
*/
void HParser::consumeElement() {
    while(!check(std::vector<TokenType>{NEWLINE, COMMA, RIGHT_BRACKET}) && !atEnd()) {
        if(match(LEFT_BRACE)) {
            hoconTree();
        } else if (match(LEFT_BRACKET)) {
            hoconArray();
        }
        else advance();
    }
    match(std::vector<TokenType>{NEWLINE, COMMA});
    ignoreAllWhitespace();
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
        error(peek().line, "Unexpected symbol " + peek().lexeme + " after object member");
        consumeMember();
    }
}

/* 
    // note: consumeToNextElement requires a different logic:
    if there are two elements written in a HOCON array, 
    [
        obj {}
    ]
    The above is invalid. This requires a specific check for a separator that consumetoNextMember can avoid because 
    the parser only expects a simple value after the member, which must be separated by a newline/comma or it will be concatenated 
    into the stored value.
*/
void HParser::consumeToNextElement() { 
    ignoreInlineWhitespace();
    bool sepExists = match(std::vector<TokenType>{COMMA, NEWLINE});
    if (check(RIGHT_BRACKET)) { 
        return;
    } else if(sepExists) {
        ignoreAllWhitespace();
        match(COMMA);
        ignoreAllWhitespace();
    } else {
        error(peek().line, "Expected comma or newline, got " + peek().lexeme + " after array element");
        consumeElement();
    }
}

// create parsed objects :: assignment

/*
    Attempts to create a hocon object with the following tokens, consuming all tokens including the ending '}'.
    Assumes you are within the object, after the first {
    This is only used if a root hocon object is not wrapped in braces.
*/
HTree * HParser::rootTree() { 
    HTree * output = new HTree();
    while(!atEnd()) { // loop through members
        std::string key = hoconKey(); 
        if (match(LEFT_BRACE)) {
            HTree * curr = hoconTree(); // ends after consuming right brace.
            while (match(LEFT_BRACE)) {
                //curr = mergeTrees(curr, hoconTree());
                hoconTree();
            }
            output->addMember(key, curr); // implied separator case. ex: foo {}
            consumeToNextRootMember();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                output->addMember(key, hoconTree());
            } else if (match(LEFT_BRACKET)) {
                output->addMember(key, hoconArray());
            } else {   
                output->addMember(key, hoconSimpleValue());
            }
            consumeToNextRootMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key + "'");
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
        std::string key = hoconKey(); 
        if (match(LEFT_BRACE)) {
            HTree * curr = hoconTree(); // ends after consuming right brace.
            while (match(LEFT_BRACE)) {
                //curr = mergeTrees(curr, hoconTree());
                hoconTree();
            }
            output->addMember(key, curr); // implied separator case. ex: foo {}
            consumeToNextMember();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                output->addMember(key, hoconTree());
            } else if (match(LEFT_BRACKET)) {
                output->addMember(key, hoconArray());
            } else {   
                output->addMember(key, hoconSimpleValue());
            }
            consumeToNextMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + key + "'");
            consumeMember();
        }
    }
    return output;
}

/* 
    assume you have just consumed the left bracket.
*/
HArray * HParser::hoconArray() {
    HArray * output = new HArray();
    while(!match(RIGHT_BRACKET)) { // loop through members, assumption is that the current token is the first token for a given value.
        if(atEnd()) {
            error(peek().line, "Imbalanced []");
            break;
        }
        if (match(LEFT_BRACE)) { // object case
            HTree * curr = hoconTree(); // ends after consuming right brace.
            while (match(LEFT_BRACE)) {
                //curr = mergeTrees(curr, hoconTree());
                hoconTree();
            }
            output->addElement(curr); // implied separator case. ex: foo {}
            consumeToNextElement(); 
            // need to add object concatentation here.
        } else if (match(LEFT_BRACKET)) { // array case
            HArray * curr = hoconArray();
            output->addElement(curr);
            consumeToNextElement();
            // add array concatenation here;
        } else if (check(SIMPLE_VALUES)) { // simple value case
            output->addElement(hoconSimpleValue());
            consumeToNextElement();
        } else {
            error(peek().line, "Expected '=' or ':', got '" + peek().lexeme + "', instead");
            consumeMember();
        }
    }
    return output;
}

/* 
    Assume you have just consumed the k-v separator (obj context), or peeked the first simple value token (array context). 
    Consume until non-simple value
*/
HSimpleValue * HParser::hoconSimpleValue() {
    std::vector<Token> valTokens = std::vector<Token>();
    while(check(SIMPLE_VALUES) || check(WHITESPACE)) { // note, the WHITESPACE TokenType differentiates between newlines and traditional whitespace.
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
std::string HParser::hoconKey() {
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
        return "";
    }

    return ss.str();
}

// object merge

// array concatenation



// parsing steps:

void HParser::parseTokens() {
    if (match(LEFT_BRACKET)) { // root array
        ignoreAllWhitespace();
        rootObject = hoconArray();
        ignoreAllWhitespace();
    } else { // if not array, implicitly assumed to be object.
        rootBrace = match(LEFT_BRACE);
        ignoreAllWhitespace();
        if (rootBrace) {
            rootObject = hoconTree();
        } else {
            rootObject = rootTree();
        }
        ignoreAllWhitespace();

    }
    if (!atEnd()){
        error(peek().line, "Expected EOF, got " + peek().lexeme);
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
