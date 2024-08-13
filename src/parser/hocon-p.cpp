#include "hocon-p.hpp"

const std::string INDENT = "    "; 

bool debug = true;

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

auto deleteHObj = Overload {                                     
    [](HTree * obj) { delete obj; },
    [](HArray * arr) { delete arr; },
    [](HSimpleValue * val) { delete val; },
    [](HSubstitution * sub) { delete sub; },
    [](HPath * path) { delete path; },
};

auto stringify = Overload {                                     
    [](HTree * obj) { return obj->str(); },
    [](HArray * arr) { return arr->str(); },
    [](HSimpleValue * val) { return val->str(); },
    [](HSubstitution * sub) { return sub->str(); },
    [](HPath * path) { return path->str(); },
};

auto getDeepCopy = Overload {
    [](HTree * obj) { std::variant<HTree*, HArray *, HSimpleValue *, HSubstitution*> out = obj->deepCopy(); return out; },
    [](HArray * arr) { std::variant<HTree*, HArray *, HSimpleValue *, HSubstitution*> out = arr->deepCopy(); return out; },
    [](HSimpleValue * val) { std::variant<HTree*, HArray *, HSimpleValue *, HSubstitution*> out = val->deepCopy(); return out; },
    [](HSubstitution * sub) { std::variant<HTree*, HArray *, HSimpleValue *, HSubstitution*> out = sub->deepCopy(); return out; },
};

auto subDeepCopy = Overload {
    [](HTree * obj) { std::variant<HTree*, HArray *, HSimpleValue *, HPath*> out = obj->deepCopy(); return out; },
    [](HArray * arr) { std::variant<HTree*, HArray *, HSimpleValue *, HPath*> out = arr->deepCopy(); return out; },
    [](HSimpleValue * val) { std::variant<HTree*, HArray *, HSimpleValue *, HPath*> out = val->deepCopy(); return out; },
    [](HPath * path) { std::variant<HTree*, HArray *, HSimpleValue *, HPath*> out = path->deepCopy(); return out; },
};

auto getPathStr = Overload {
    [](HTree * obj) { return obj->getPath(); },
    [](HArray * arr) { return arr->getPath(); },
    [](HSimpleValue * val) { return val->getPath(); },
    [](HSubstitution * sub) { return sub->getPath(); },
};

auto getKey = Overload {
    [](HTree * obj) { return obj->key==""?"root":obj->key; },
    [](HArray * arr) { return arr->key==""?"root":arr->key; },
    [](HSimpleValue * val) { return val->key; },
    [](HSubstitution * sub) { return sub->key; }
};

HTree::HTree() : members(std::unordered_map<std::string, std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
    }
}

void HTree::addMember(std::string key, std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> value) {
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
    } else if (std::holds_alternative<HSimpleValue*>(value)){
        HSimpleValue * val = std::get<HSimpleValue*>(value);
        val->parent = this;
        val->key = key;
    } else {
        HSubstitution * sub = std::get<HSubstitution*>(value);
        sub->parent = this;
        sub->key = key;
    }
    if(members.count(key) == 0) { // new key case
        //std::cout << "added key " << key << " with value " << std::visit(stringify, value) << std::endl;
        memberOrder.push_back(key);
        members.insert(std::make_pair(key, value));
    } else if (std::holds_alternative<HTree*>(members[key]) && std::holds_alternative<HTree*>(value)) { // object merge case
        std::get<HTree*>(members[key])->mergeTrees(std::get<HTree*>(value));
        //std::cout << "merged object key " << key << " with object " << std::visit(stringify, value) << std::endl;
    } else if (std::holds_alternative<HSubstitution*>(members[key]) && std::holds_alternative<HTree*>(value)) { // substitution expecting tree or pure substitution case.
        HSubstitution * sub = std::get<HSubstitution*>(members[key]);
        if (sub->substitutionType == 0 || sub->substitutionType == 3) {
            sub->substitutionType = 0;
            sub->values.push_back(std::get<HTree*>(value));
        } // add else case here.
    } else if (std::holds_alternative<HTree*>(members[key]) && std::holds_alternative<HSubstitution*>(value)) { // subsitution onto tree case.
        HSubstitution * sub = std::get<HSubstitution*>(value);
        sub->values.insert(sub->values.begin(), std::get<HTree*>(members[key]));
        if (sub->substitutionType == 3) {
            sub->substitutionType = 0;
        }
        members[key] = value;
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
        std::vector<std::string> test = getPath();
        std::string a = "";
        for( auto s : test) {
            a += s + ".";
        }
        if(!root) {
            out = "{ ... path = " + a + "\n";
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

/*
    Returns a global path to the current HTree. NOT IMPLEMENTED : If object is in an array, return empty string
*/
std::vector<std::string> HTree::getPath() {
    if (root == true) {
        return std::vector<std::string>();
    } else {
        std::vector<std::string> parentPath = std::visit(getPathStr, parent);
        parentPath.push_back(key);
        return parentPath;
    }
}

HArray::HArray() : elements(std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*>>()) {}

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
        std::vector<std::string> test = getPath();
        std::string a = "";
        for( auto s : test) {
            a += s + ".";
        }
        if(!root) {
            out = "{ ... path = " + a + "\n";
        } else {
            out = "{ ... root obj \n";
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

void HArray::addElement(std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> val) {
    if(std::holds_alternative<HTree*>(val)) {
        HTree * obj = std::get<HTree*>(val);
        obj->parent = this;
        obj->key = std::to_string(elements.size());
        obj->root = false;
    } else if (std::holds_alternative<HArray*>(val)) {
        HArray * arr = std::get<HArray*>(val);
        arr->parent = this;
        arr->key = std::to_string(elements.size());
        arr->root = false;
    } else if (std::holds_alternative<HSimpleValue*>(val)) {
        HSimpleValue * value = std::get<HSimpleValue*>(val);
        value->parent = this;
        value->key = std::to_string(elements.size());
    } else {
        HSubstitution * sub = std::get<HSubstitution*>(val);
        sub->parent = this;
        sub->key = std::to_string(elements.size());
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
    if (std::holds_alternative<std::string>(svalue)) {
        output = (std::get<std::string>(svalue)[0] == '"') ? std::get<std::string>(svalue) : "\"" + std::get<std::string>(svalue) + "\"";
    } else {
        for (auto t : tokenParts) {
            output += t.lexeme;
        }
    }
    if (debug) {
        std::vector<std::string> path = getPath();
        output += " ";
        for(auto str: path) {
            output += str + ".";
        }
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

HPath::HPath(std::vector<std::string> s, bool optional) : path(s), optional(optional) {}

HPath::HPath(Token t) {
    if(t.type == SUB || t.type == SUB_OPTIONAL) {
        path = HParser::splitPath(std::get<std::string>(t.literal));
        optional = t.type == SUB_OPTIONAL;
    } else {
        path = std::vector<std::string>();
        optional = true;
    }
}

std::string HPath::str() {
    std::string out;
    out += (optional ? "${?" : "${");
    if (path.size() > 0) {
        out += path[0];
        for (auto iter = path.begin() + 1; iter != path.end(); iter++) {
            out += "." + *iter;
        }
    }
    out += "}";
    return out;
}

HPath * HPath::deepCopy() {
    return new HPath(path, optional);
}

HSubstitution::HSubstitution(std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HPath*>> v) : values(v) {}

HSubstitution::~HSubstitution() {
    for (auto obj : values) {
        std::visit(deleteHObj, obj);
    }
}

std::string HSubstitution::str() {
    std::string out;
    if (values.size() > 0) {
        if (std::holds_alternative<HTree*>(values[0])) {
            out = "{...}";
        } else if (std::holds_alternative<HArray*>(values[0])) {
            out = "[...]";
        } else if (std::holds_alternative<HSimpleValue*>(values[0])){ 
            out = std::visit(stringify, values[0]);
        } else {
            out = (substitutionType == 2 || (substitutionType == 3 && values.size() > 1)) ? "'" + std::visit(stringify, values[0]) + std::get<HPath*>(values[0])->suffixWhitespace + "'" : "'" + std::visit(stringify, values[0]) + "'"; 
        }
        switch (substitutionType) {
            case 0:
            case 1:
                for(auto iter = values.begin() +1 ; iter != values.end(); iter++) {
                    if (std::holds_alternative<HTree*>(*iter)) {
                        out += " {...}";
                    } else if (std::holds_alternative<HArray*>(*iter)) {
                        out += " [...]";
                    } else { 
                        out += " " + std::visit(stringify, *iter);
                    }
                }
                break;
            case 2:
            case 3:
                for(auto iter = values.begin() +1 ; iter != values.end(); iter++) {
                    if (std::holds_alternative<HTree*>(*iter)) {
                        out += " {...}";
                    } else if (std::holds_alternative<HArray*>(*iter)) {
                        out += " [...]";
                    } else if (std::holds_alternative<HPath*>(*iter)) {
                        out += "'" + std::visit(stringify, *iter) + std::get<HPath*>(*iter)->suffixWhitespace + "'";
                    } else { 
                        out += "'" + std::visit(stringify, *iter) + "'";
                    }
                }
                break;
        }
    } else {
        out = "";
    }
    if (debug) {
        std::vector<std::string> path = getPath();
        out += " ";
        for(auto str : path) {
            out += str + ".";
        }
    }
    return out;
}

HSubstitution * HSubstitution::deepCopy() { // to do
    //HArray * copy = new HArray();
    std::vector<std::variant<HTree*,HArray*,HSimpleValue*, HPath*>> copies;
    for (auto obj : values) {
        copies.push_back(std::visit(subDeepCopy, obj));
    }
    HSubstitution * copy = new HSubstitution(copies);
    return copy;
}

std::vector<std::string> HSubstitution::getPath() {
    std::vector<std::string> parentPath = std::visit(getPathStr, parent);
    parentPath.push_back(key);
    return parentPath;
}


HParser::~HParser() {
    std::visit(deleteHObj, rootObject);
    for(auto pair : stack) {
        std::visit(deleteHObj, pair.second);
    }
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

void HParser::getStack() {

    for(auto s : stack) {
        std::string out = "";
        if (s.first.size() > 0) {
            for(auto str : s.first) {
                out += str;
            }
        }
        std::cout << out;
        std::cout << std::visit(stringify, s.second) << std::endl;
    }
}

void HParser::pushStack(std::variant<HTree*,HArray*,HSimpleValue*,HSubstitution*> value) {
    stack.push_back(std::make_pair(std::visit(getPathStr, value), new HTree()));
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
            delete hoconTree();
        } else if (match(LEFT_BRACKET)) {
            delete hoconArray();
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
            delete hoconTree();
        } else if (match(LEFT_BRACKET)) {
            delete hoconArray();
        }
        else advance();
    }
    match(std::vector<TokenType>{NEWLINE, COMMA});
    ignoreAllWhitespace();
}

/*
    panic mode method to consume until the next separator for a substitution.
*/
void HParser::consumeSubstitution() {
    while(!check(std::vector<TokenType>{NEWLINE, COMMA, RIGHT_BRACKET, RIGHT_BRACE}) && !atEnd()) {
        if(previous().type == LEFT_BRACE) {
            delete hoconTree();
        } else if (previous().type == LEFT_BRACKET) {
            delete hoconArray();
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
        error(peek().line, "Unexpected symbol " + peek().lexeme + " after root object member");
        consumeMember();
        match(RIGHT_BRACE); // preventing infinite loop from too many closing braces.
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
        std::string keyValue = path.size() > 0 ? path[path.size()-1] : "";
        if (path.size() > 1) {
            target = findOrCreatePath(path, output);
        } else {
            target = output;
        }
        if (match(LEFT_BRACE)) { // implied separator case. ex: foo {}
            HTree * obj = mergeAdjacentTrees();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj);
                target->addMember(keyValue, sub);
                pushStack(sub);
            } else {
                target->addMember(keyValue, obj);
                pushStack(obj);
            } 
            consumeToNextRootMember();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                HTree * obj = mergeAdjacentTrees();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(obj);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, obj);
                    pushStack(obj);
                }
            } else if (match(LEFT_BRACKET)) {
                HArray * arr = concatAdjacentArrays();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(arr);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, arr);
                    pushStack(arr);
                }
            } else if (check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution();
                pushStack(sub);
                target->addMember(keyValue, sub);
            } else {   
                HSimpleValue * val = hoconSimpleValue();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(val);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, val);
                    pushStack(val);
                }
            }
            consumeToNextRootMember();
        } else {
            error(peek().line, "Expected '=' or ':', got " + peek().lexeme + ", after the key '" + keyValue + "'");
            if (keyValue.size() == 0) {
                match(RIGHT_BRACE);
            }
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
            HTree * obj = mergeAdjacentTrees();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj);
                target->addMember(keyValue, sub);
                pushStack(sub);
            } else {
                target->addMember(keyValue, obj);
                pushStack(obj);
            }
            consumeToNextMember();
        } else if(match(KEY_VALUE_SEP)) {   // explicit separator ex: foo = {}
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                HTree * obj = mergeAdjacentTrees();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(obj);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, obj);
                    pushStack(obj);
                }
            } else if (match(LEFT_BRACKET)) {
                HArray * arr = concatAdjacentArrays();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(arr);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, arr);
                    pushStack(arr);
                }
            } else if (check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution();
                target->addMember(keyValue, sub);
                pushStack(sub);
            } else {   
                HSimpleValue * val = hoconSimpleValue();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(val);
                    target->addMember(keyValue, sub);
                    pushStack(sub);
                } else {
                    target->addMember(keyValue, val);
                    pushStack(val);
                }
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
        if (match(LEFT_BRACE)) {  // object case
            HTree * obj = mergeAdjacentTrees();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj);
                output->addElement(sub);
            } else {
                output->addElement(obj);
            }
            consumeToNextElement(); 
        } else if (match(LEFT_BRACKET)) { // array case
            HArray * arr = concatAdjacentArrays();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(arr);
                output->addElement(sub);
            } else {
                output->addElement(arr);
            }
            consumeToNextElement();
        } else if (check(SIMPLE_VALUES)) { // simple value case
            HSimpleValue * val = hoconSimpleValue();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(val);
                output->addElement(sub);
            } else {
                output->addElement(val);
            }
            consumeToNextElement();
        } else if (check(SUB) || check(SUB_OPTIONAL)) {
            HSubstitution* sub = parseSubstitution();
            output->addElement(sub);
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
        if (!check(SUB) && !check(SUB_OPTIONAL)) {
            while ((valTokens.end() - 1)->type == WHITESPACE) {
                valTokens.pop_back();
            }
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
                pushStack(obj);
                current = obj;
            }
        } else { // loop found a non-existent key in the past, no need to double check when there will never be a key.
            HTree * obj = new HTree();
            current->addMember(*iter, obj);
            pushStack(obj);
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

std::vector<std::string> HParser::splitPath(std::string path) {
    size_t start = 0;
    size_t current = 0;
    std::vector<std::string> out;
    while (current < path.size()) {
        if (path[current] == '.') {
            out.push_back(path.substr(start, current-start));
            start = ++current;
        } else if (path[current] == '"') {
            current++;
            while (path[current] != '"') {
                current++;
            }
            current++;
        } else {
            current++;
        }
    }
    if(start != current) {
        out.push_back(path.substr(start, current-start));
    }
    return out;
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

HSubstitution * HParser::parseSubstitution(std::variant<HTree*, HArray*, HSimpleValue*> prefix) {
    std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HPath*>> values;
    size_t subType = 3;
    switch(prefix.index()) {
        case 0:
            values.push_back(std::get<HTree*>(prefix));
            subType = 0;
            break;
        case 1:
            values.push_back(std::get<HArray*>(prefix));
            subType = 1;
            break;
        case 2:
            values.push_back(std::get<HSimpleValue*>(prefix));
            subType = 2;
            break;
    } 
    while(!match(NEWLINE) && !check(COMMA) && !atEnd()) {
        if(match(LEFT_BRACE)){
            if(subType == 3) {
                subType = 0;
            } else if (subType != 0) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + " got " + std::to_string(0));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconTree());
        } else if (match(LEFT_BRACKET)) {
            if(subType == 3) {
                subType = 1;
            } else if (subType != 1) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + " got " + std::to_string(1));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconArray());
        } else if (check(SUB) || check(SUB_OPTIONAL)) { 
            HPath * path = new HPath(advance());
            if (subType < 2) ignoreInlineWhitespace();
            else path->suffixWhitespace = check(WHITESPACE) ? advance().lexeme : "";
            values.push_back(path);
        } else {
            if(subType == 3) {
                subType = 2;
            } else if (subType != 2) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + "got " + std::to_string(2));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconSimpleValue());
        }
    }
    HSubstitution * out = new HSubstitution(values);
    out->substitutionType = subType;
    return out;
}

HSubstitution * HParser::parseSubstitution() {
    std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HPath*>> values;
    size_t subType = 3;
    while(!match(NEWLINE) && !check(std::vector<TokenType>{COMMA, RIGHT_BRACE, RIGHT_BRACKET}) && !atEnd()) {
        if(match(LEFT_BRACE)){
            if(subType == 3) {
                subType = 0;
            } else if (subType != 0) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + " got " + std::to_string(0));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconTree());
        } else if (match(LEFT_BRACKET)) {
            if(subType == 3) {
                subType = 1;
            } else if (subType != 1) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + " got " + std::to_string(1));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconArray());
        } else if (check(SUB) || check(SUB_OPTIONAL)) { 
            HPath * path = new HPath(advance());
            if (subType < 2) ignoreInlineWhitespace();
            else path->suffixWhitespace = check(WHITESPACE) ? advance().lexeme : "";
            values.push_back(path);
        } else {
            if(subType == 3) {
                subType = 2;
            } else if (subType != 2) {
                error(peek().line, "substitution mismatched types, expected type " + std::to_string(subType) + " got " + std::to_string(2));
                consumeSubstitution();
                return new HSubstitution(values);
            }
            values.push_back(hoconSimpleValue());
        }
    }
    HSubstitution * out = new HSubstitution(values);
    out->substitutionType = subType;
    return out;
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
