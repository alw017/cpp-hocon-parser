#include "hocon-p.hpp"

const std::string INDENT = "    "; 

bool debug = false;

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

auto getDeepCopy = Overload {
    [](HTree * obj) { std::variant<HTree*, HArray *, HSimpleValue *> out = obj->deepCopy(); return out; },
    [](HArray * arr) { std::variant<HTree*, HArray *, HSimpleValue *> out = arr->deepCopy(); return out; },
    [](HSimpleValue * val) { std::variant<HTree*, HArray *, HSimpleValue *> out = val->deepCopy(); return out; },
};

auto getPathStr = Overload {
    [](HTree * obj) { return obj->getPath(); },
    [](HArray * arr) { return arr->getPath(); },
    [](HSimpleValue * val) { return val->getPath(); },
};

auto getKey = Overload {
    [](HTree * obj) { return obj->key==""?"root":obj->key; },
    [](HArray * arr) { return arr->key==""?"root":arr->key; },
    [](HSimpleValue * val) { return val->key; }
};

HTree::HTree() : members(std::unordered_map<std::string, std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
    }
}

void HTree::addMember(std::string key, std::variant<HTree*, HArray*, HSimpleValue*> value) {
    if(std::holds_alternative<HTree*>(value)) {
        HTree * obj = std::get<HTree*>(value);
        obj->parent = this;
        obj->key = key;
        obj->root = false;
    } else if (std::holds_alternative<HArray*>(value)) {
        HArray * arr = std::get<HArray*>(value);
        arr->parent = this;
        arr->key = key;
        arr->root = false;
    } else {
        HSimpleValue * val = std::get<HSimpleValue*>(value);
        val->parent = this;
        val->key = key;
    }
    if(members.count(key) == 0) { // new key case
        //std::cout << "added key " << key << " with value " << std::visit(stringify, value) << std::endl;
        memberOrder.push_back(key);
        members.insert(std::make_pair(key, value));
    } else if (std::holds_alternative<HTree*>(members[key]) && std::holds_alternative<HTree*>(value)) { // object merge case
        std::get<HTree*>(members[key])->mergeTrees(std::get<HTree*>(value));
        //std::cout << "merged object key " << key << " with object " << std::visit(stringify, value) << std::endl;
    } else { // duplicate key, override case
        std::visit(deleteHObj, members[key]);
        //std::cout << "overrided key " << key << " with value " << std::visit(stringify, value) << std::endl;
        members[key] = value;
    }
}

bool HTree::memberExists(std::string key) {
    return members.count(key) != 0;
}

HTree * HTree::deepCopy() {
    HTree * copy = new HTree();
    for (auto pair : members) {
        copy->addMember(pair.first, std::visit(getDeepCopy, pair.second));
    }
    return copy;
} 

/* 
    Merge method for trees. second parameter members take precedence (overwrite) the first.
    deletes the pointer after being done. this may need to be changed later. 
*/
void HTree::mergeTrees(HTree * second) {
    for(auto pair : second->members) {
        if (members.count(pair.first) == 0) { // not exist case;
            addMember(pair.first, std::visit(getDeepCopy, pair.second));
        } else if (std::holds_alternative<HTree*>(members[pair.first]) && std::holds_alternative<HTree*>(pair.second)) { // exists, both objects
            std::get<HTree*>(members[pair.first])->mergeTrees(std::get<HTree*>(pair.second));
        } else { // exists but not both objects
            //std::visit(deleteHObj, members[pair.first]);
            addMember(pair.first, std::visit(getDeepCopy, pair.second));
        }
    }
    delete second;
}

std::string HTree::str() {
    if(members.empty()) {
        return "{}";
    }
    std::string out = "{\n";
    if (debug) {
        if(!root) {
            out = "{ ... parent key = " + std::visit(getKey, parent) + "\n";
        } else {
            out = "{ ... root obj \n";
        }
    }
    
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

// TODO
std::vector<std::string> HTree::getPath() {
    if (root == true) {
        return std::vector<std::string>();
    } else {
        std::vector<std::string> parentPath = std::visit(getPathStr, parent);
        parentPath.push_back(key);
        return parentPath;
    }
}

HArray::HArray() : elements(std::vector<std::variant<HTree*, HArray*, HSimpleValue*>>()) {}

HArray::~HArray() {
    for (auto e : elements) {
        std::visit(deleteHObj, e);
    }
}

HArray * HArray::deepCopy() {
    HArray * copy = new HArray();
    for(auto e : elements) {
        copy->addElement(std::visit(getDeepCopy, e));
    }
    return copy;
}

std::string HArray::str() { // tested,
    if(elements.empty()) {
        return "[]";
    }
    std::string out = "[ \n";
    if (debug) {
        if (!root) {
            out = "[ ... parent key = " + std::visit(getKey, parent) + "\n";
        } else {
            out = "[ ... root array\n";
        }
    }

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

std::vector<std::string> HArray::getPath() {
    if (root == true) {
        return std::vector<std::string>();
    } else {
        std::vector<std::string> parentPath = std::visit(getPathStr, parent);
        parentPath.push_back(key);
        return parentPath;
    }
}

void HArray::addElement(std::variant<HTree*, HArray*, HSimpleValue*> val) {
    if(std::holds_alternative<HTree*>(val)) {
        HTree * obj = std::get<HTree*>(val);
        obj->parent = this;
        obj->key = "array element";
        obj->root = false;
    } else if (std::holds_alternative<HArray*>(val)) {
        HArray * arr = std::get<HArray*>(val);
        arr->parent = this;
        arr->key = "array element";
        arr->root = false;
    } else {
        HSimpleValue * value = std::get<HSimpleValue*>(val);
        value->parent = this;
        value->key = "array element";
    }
    elements.push_back(val);
}

/*
    concatenate two arrays, appending elements of second onto the first, preserving original order.
    deletes second after finishing.
*/
void HArray::concatArrays(HArray * second) {
    for(auto e : second->elements) {
        addElement(std::visit(getDeepCopy, e));
    }
    delete second;
}

HSimpleValue::HSimpleValue(std::variant<int, double, bool, std::string> s, std::vector<Token> tokenParts): svalue(s), tokenParts(tokenParts) {}

std::string HSimpleValue::str() {
    std::string output;
    for (auto t : tokenParts) {
        output += t.lexeme;
    }
    return output;
}

std::vector<std::string> HSimpleValue::getPath() {
    std::vector<std::string> parentPath = std::visit(getPathStr, parent);
    parentPath.push_back(key);
    return parentPath;
}

HSimpleValue* HSimpleValue::deepCopy() {
    return new HSimpleValue(svalue, tokenParts);
}

HSubstitution::HSubstitution(std::vector<std::string> s) : path(s) {}

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
    HTree * target = output;
    while(!atEnd()) { // loop through members
        std::vector<std::string> path = hoconKey(); 
        std::string keyValue = path[path.size()-1];
        if (path.size() > 1) {
            target = findOrCreatePath(path, output);
        } else {
            target = output;
        }
        if (match(LEFT_BRACE)) { // implied separator case. ex: foo {}
            target->addMember(keyValue, mergeAdjacentTrees()); 
            consumeToNextRootMember();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                target->addMember(keyValue, mergeAdjacentTrees());
            } else if (match(LEFT_BRACKET)) {
                target->addMember(keyValue, concatAdjacentArrays());
            } else {   
                target->addMember(keyValue, hoconSimpleValue());
            }
            consumeToNextRootMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + keyValue + "'");
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
    HTree * target = output;
    while(!match(RIGHT_BRACE)) { // loop through members
        if(atEnd()) {
            error(peek().line, "Imbalanced {}");
            break;
        }
        std::vector<std::string> path = hoconKey(); 
        std::string keyValue = path[path.size()-1];
        if (path.size() > 1) {
            target = findOrCreatePath(path, output);
        } else {
            target = output;
        }
        if (match(LEFT_BRACE)) {            // implied separator case. ex: foo {}
            target->addMember(keyValue, mergeAdjacentTrees());   // object concatentation here.
            consumeToNextMember();
        } else if(match(KEY_VALUE_SEP)) {   // explicit separator ex: foo = {}
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                target->addMember(keyValue, mergeAdjacentTrees());
            } else if (match(LEFT_BRACKET)) {
                target->addMember(keyValue, concatAdjacentArrays());
            } else {   
                target->addMember(keyValue, hoconSimpleValue());
            }
            consumeToNextMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + keyValue + "'");
            consumeMember();
        }
    }
    return output;
}

/* 
    assume you have just consumed the left bracket. consumes the right bracket token.
*/
HArray * HParser::hoconArray() {
    HArray * output = new HArray();
    while(!match(RIGHT_BRACKET)) { // loop through members, assumption is that the current token is the first token for a given value.
        if(atEnd()) {
            error(peek().line, "Imbalanced []");
            break;
        }
        if (match(LEFT_BRACE)) { // object case
            output->addElement(mergeAdjacentTrees()); // implied separator case. ex: foo {}
            consumeToNextElement(); 
        } else if (match(LEFT_BRACKET)) { // array case
            output->addElement(concatAdjacentArrays());
            consumeToNextElement();
        } else if (check(SIMPLE_VALUES)) { // simple value case
            output->addElement(hoconSimpleValue());
            consumeToNextElement();
        } else {
            error(peek().line, "Expected a {, [ or a simple value, got " + peek().lexeme + "', instead");
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
std::vector<std::string> HParser::hoconKey() {
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
        return std::vector<std::string>();
    }
    //return ss.str();
    return splitPath(keyTokens);
}

// helper methods for creating parsed objects

/*
    given a path via a vector of string keys, create a series of nested objects to which maps to the path, if it doesn't exist.
    @return HTree * 
    @returns a pointer to the inmost object
*/
HTree * HParser::findOrCreatePath(std::vector<std::string> path, HTree * parent) {
    bool pathExists = true;
    HTree * current = parent;
    for(auto iter = path.begin(); iter != path.end()-1; iter++) {
        if (pathExists) {
            if(current->memberExists(*iter) && std::holds_alternative<HTree*>(current->members[*iter])) { // obj corresponds to key. traverse to next obj.
                current = std::get<HTree*>(current->members[*iter]);
            } else { // non obj corresponds to key, or key doesn't exist. create/override key to be a new blank obj.
                pathExists = false;
                HTree * obj = new HTree();
                current->addMember(*iter, obj);
                current = obj;
            }
        } else { // loop found a non-existent key in the past, no need to double check when there will never be a key.
            HTree * obj = new HTree();
            current->addMember(*iter, obj);
            current = obj;
        }
    }
    return current;
}

/*
    Takes a vector of tokens containing a key, and splits it into strings for each path section.
*/
std::vector<std::string> HParser::splitPath(std::vector<Token> keyTokens) { 
    std::vector<std::string> path;
    std::string part = "";
    for (auto token : keyTokens) {
        size_t start = 0;
        size_t curr = 0;    
        switch (token.type) {
            case NUMBER:
            case TRUE:
            case FALSE:
            case NULLVALUE:
            case WHITESPACE:
            case UNQUOTED_STRING:
                start = 0; 
                curr = 0; 
                while ((curr = token.lexeme.find(".", start)) != std::string::npos) {   
                    part += token.lexeme.substr(start, curr-start);  
                    path.push_back(part);
                    part = ""; 
                    start = curr + 1; 
                } 
                part += token.lexeme.substr(start);
                break;
            case QUOTED_STRING:
                part += std::get<std::string>(token.literal);
                break;
            default:
                break;
        }
    }
    path.push_back(part);
    return path;
}


HArray * HParser::concatAdjacentArrays() {
    HArray * curr = hoconArray();
    while (match(LEFT_BRACKET)) { // array concatenation here;
        curr->concatArrays(hoconArray());
    }
    return curr;
}

HTree * HParser::mergeAdjacentTrees() {
    HTree * curr = hoconTree(); // ends after consuming right brace.
    while (match(LEFT_BRACE)) { // obj concatenation here
        curr->mergeTrees(hoconTree());
    }
    return curr;
}


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

std::variant<HTree*, HArray*, HSimpleValue*> getByPath(std::string path);

// split string by delimiter helper. 


// access methods:

// error reporting:

void HParser::error(int line, std::string message) {
    report(line, "", message);
    validConf = false;
}

void HParser::report(int line, std::string where, std::string message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
}
