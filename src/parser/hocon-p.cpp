#include "hocon-p.hpp"

const std::string INDENT = "    "; 

bool debug = false;

std::string pathToString(std::vector<std::string> path) {
    std::string out = path.size() > 0 ? path[0] : "";
    for(size_t i = 1; i < path.size(); i++) {
        out += "." + path[i];
    }
    return out;
}

std::string getEnvVar( std::string const& key ) {
    char * val = std::getenv( key.c_str() );
    return val == NULL ? std::string("") : std::string(val);
}

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

auto getSubstitutions = Overload {
    [](HTree * obj) { return obj->getUnresolvedSubs(); },
    [](HArray * arr) { return arr->getUnresolvedSubs(); },
    [](HSimpleValue * val) { return std::unordered_set<HSubstitution*>(); },
    [](HSubstitution * sub) { return std::unordered_set<HSubstitution*>{sub}; }
};

auto linkResolvedSub = Overload {
    [](HTree * obj, std::string const& key, HTree* result) { 
        obj->members[key] = result; 
        },
    [](HTree * obj, std::string const& key, HArray* result) { 
        obj->members[key] = result; 
        },
    [](HTree * obj, std::string const& key, HSimpleValue* result) { 
        obj->members[key] = result; 
        },
    [](HTree * obj, std::string const& key, HSubstitution* result) { 
        obj->members[key] = result; 
        },
    [](HArray * arr, std::string const& key, HTree* result) { 
        arr->elements[std::stoi(key)] = result; 
        },
    [](HArray * arr, std::string const& key, HArray* result) { 
        arr->elements[std::stoi(key)] = result; 
        },
    [](HArray * arr, std::string const& key, HSimpleValue* result) { 
        arr->elements[std::stoi(key)] = result; 
        },
    [](HArray * arr, std::string const& key, HSubstitution* result) { 
        arr->elements[std::stoi(key)] = result; 
        },
};

auto deleteNullSub = Overload {
    [](HTree * obj, std::string const& key) {
        obj->removeMember(key);
    },
    [](HArray* arr, std::string const& key) {
        arr->removeElementAtIndex(std::stoi(key));
    },
};

auto valueExists = Overload {
    [](HTree * obj) { return obj != NULL; },
    [](HArray * arr) { return arr != NULL; },
    [](HSimpleValue * val) { return val != NULL; },
    [](HSubstitution * sub) { return sub != NULL; }
};

auto HArrayDecrementIndex = Overload {
    [](HTree * obj) { obj->key = std::to_string(std::stoi(obj->key) - 1); },
    [](HArray * arr) { arr->key = std::to_string(std::stoi(arr->key) - 1); },
    [](HSimpleValue * val) { val->key = std::to_string(std::stoi(val->key) - 1); },
    [](HSubstitution * sub) { sub->key = std::to_string(std::stoi(sub->key) - 1); }
};

HTree::HTree() : members(std::unordered_map<std::string, std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*>>()) {}

HTree::~HTree() {
    for(auto pair : members) {
        std::visit(deleteHObj, pair.second);
    }
}

bool HTree::addMember(std::string const& key, std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> value) {
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
        std::visit(deleteHObj,value);
        return true;
        //std::cout << "merged object key " << key << " with object " << std::visit(stringify, value) << std::endl;
    } else if (std::holds_alternative<HSubstitution*>(members[key]) && std::holds_alternative<HTree*>(value)) { // substitution expecting tree or pure substitution case.
        HSubstitution * sub = std::get<HSubstitution*>(members[key]);
        if (sub->substitutionType == 0 || sub->substitutionType == 3) {
            sub->substitutionType = 0;      
        } 
        sub->values.push_back(std::get<HTree*>(value));
        sub->interrupts.push_back(true);
    } else if (std::holds_alternative<HTree*>(members[key]) && std::holds_alternative<HSubstitution*>(value)) { // subsitution onto tree case.
        HSubstitution * sub = std::get<HSubstitution*>(value);
        sub->values.insert(sub->values.begin(), std::get<HTree*>(members[key]));
        if (sub->substitutionType == 3) {
            sub->substitutionType = 0;
        }
        members[key] = value;
    } else if (std::holds_alternative<HSubstitution*>(members[key]) && std::holds_alternative<HSubstitution*>(value)) { // double substitution case.
        HSubstitution * sub = std::get<HSubstitution*>(members[key]);
        HSubstitution * add = std::get<HSubstitution*>(value);
        for(auto iter = add->values.begin(); iter != add->values.end(); iter++) {
            std::variant<HTree*, HArray*, HSimpleValue*, HPath*> valueCopy = std::visit(subDeepCopy, *iter);
            sub->interrupts.push_back(iter == add->values.begin());
            sub->values.push_back(valueCopy);
            if (std::holds_alternative<HPath*>(valueCopy)) {
                std::get<HPath*>(valueCopy)->parent = sub;
                sub->paths.push_back(std::get<HPath*>(valueCopy));
            }
        }
        delete add;
        return true;
    } else { // duplicate key, override case
        std::visit(deleteHObj, members[key]);
        //std::cout << "overrided key " << key << " with value " << std::visit(stringify, value) << std::endl;
        members[key] = value;
    }
    return false;
}

bool HTree::memberExists(std::string const& key) {
    return members.count(key) != 0;
}

void HTree::removeMember(std::string const& key) {
    members.erase(key);
    for (auto iter = memberOrder.begin(); iter != memberOrder.end(); iter++) {
        if (*iter == key) {
            memberOrder.erase(iter);
            break;
        }
    }
}

HTree * HTree::deepCopy() {
    HTree * copy = new HTree();
    for (auto pair : members) {
        copy->addMember(pair.first, std::visit(getDeepCopy, pair.second));
    }
    copy->parent = this->parent;
    copy->key = this->key;
    copy->root = this->root;
    return copy;
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
    
    for(size_t i = 0; i < memberOrder.size(); i++) {
        std::string keyval = memberOrder[i];
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
                out += (word == "}" && i != memberOrder.size()-1) ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else if (std::holds_alternative<HArray*>(value)) {
            std::string string = std::get<HArray*>(value)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += (word == "]" && i != memberOrder.size()-1) ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else {
            out += std::visit(stringify, value) + ((i != memberOrder.size()-1) ? ",\n" : "\n");
        }
    }
    out += "}";
    return out;
}

/*
    Returns the absolute path to the current HTree.
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

/* 
    Merge method for trees. second parameter members take precedence (overwrite) the first.
    Remember to delete the second pointer after finishing.
*/
void HTree::mergeTrees(HTree * second) {
    for(auto pair : second->members) {
        if (members.count(pair.first) == 0) { // not exist case;
            addMember(pair.first, std::visit(getDeepCopy, pair.second));
        } else if (std::holds_alternative<HTree*>(members[pair.first]) && std::holds_alternative<HTree*>(pair.second)) { // exists, both objects
            std::get<HTree*>(members[pair.first])->mergeTrees(std::get<HTree*>(std::visit(getDeepCopy, pair.second)));
        } else { // exists but not both objects
            //std::visit(deleteHObj, members[pair.first]);
            addMember(pair.first, std::visit(getDeepCopy, pair.second));
        }
    }
}

std::unordered_set<HSubstitution*> HTree::getUnresolvedSubs() {
    std::unordered_set<HSubstitution*> out;
    for (auto pair : members) {
        std::unordered_set<HSubstitution*> curr = std::visit(getSubstitutions, pair.second);
        out.merge(curr);
    }
    return out;
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

    for(size_t i = 0; i < elements.size(); i++) {
        auto e = elements[i];
        out += INDENT;
        if (std::holds_alternative<HTree*>(e)) {
            std::string string = std::get<HTree*>(e)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += (word == "}" && i != elements.size()-1) ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else if (std::holds_alternative<HArray*>(e)) {
            std::string string = std::get<HArray*>(e)->str();
            std::stringstream ss(string);
            std::string word;
            std::getline(ss, word, '\n');
            out += word + "\n";
            while (!ss.eof()) {
                std::getline(ss, word, '\n');
                out += (word == "]" && i != elements.size()-1) ? INDENT + word + ",\n" : INDENT + word + "\n";
            }
        } else {
            out += std::visit(stringify, e) + ((i != elements.size()-1) ? ",\n" : "\n");
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

void HArray::removeElementAtIndex(size_t index) {
    // re-set all values of index keys to their proper value.
    for (size_t i = index + 1; i < elements.size(); i++) {
        std::visit(HArrayDecrementIndex, elements[i]);
    }
    elements.erase(elements.begin() + index);
    // remove the element at index.
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

std::unordered_set<HSubstitution*> HArray::getUnresolvedSubs() {
    std::unordered_set<HSubstitution*> out;
    for (auto e : elements) {
        std::unordered_set<HSubstitution*> curr = std::visit(getSubstitutions, e);
        out.merge(curr);
    }
    return out;
}

HSimpleValue::HSimpleValue(std::variant<int, double, bool, std::string> s, std::vector<Token> tokenParts, size_t end): svalue(s), tokenParts(tokenParts), defaultEnd(end) {}

std::string HSimpleValue::str() {
    std::string output;
    if (std::holds_alternative<std::string>(svalue)) {
        output = (std::get<std::string>(svalue)[0] == '"') ? std::get<std::string>(svalue) : "\"" + std::get<std::string>(svalue) + "\"";
    } else {
        for (size_t i = 0; i < defaultEnd; i++) {
            output += tokenParts[i].lexeme;
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
    return new HSimpleValue(svalue, tokenParts, defaultEnd);
}

void HSimpleValue::concatSimpleValues(HSimpleValue* second) {
    defaultEnd = tokenParts.size() + second->defaultEnd;
    //std::cout << "fusing [" << str() << "] and [" << second->str() << "]" << std::endl; 
    std::stringstream ss;
    for (auto t : second->tokenParts) {
        tokenParts.push_back(t);
    }
    for (size_t i = 0; i < defaultEnd; i++) {
        //std::cout << tokenParts[i].str() << std::endl;
        ss << ((tokenParts[i].type == QUOTED_STRING) ? std::get<std::string>(tokenParts[i].literal) : tokenParts[i].lexeme);
    }
    svalue = ss.str();
    delete second;
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
    out += "}";// + std::to_string(counter) + (isSelfReference()?" self":" root");
    if(debug && parent){
        out += " - path: " + pathToString(parent->getPath());
    }
    return out;
}

HPath * HPath::deepCopy() {
    HPath * out = new HPath(path, optional);
    out->counter = this->counter;
    out->parent = this->parent;
    out->suffixWhitespace = this->suffixWhitespace;
    return out;
}

/*
    Note: only works if called after the first pass parsing step. This will not work during parsing, because getPath will not work.
*/
bool HPath::isSelfReference() {
    if (!parent) return false;
    std::vector<std::string> parentPath = parent->getPath();
    std::stringstream ss{""}; 
    ss << pathToString(path) << " and " << pathToString(parentPath);
    std::string res = ss.str();
    //std::cout << res << std::endl;
    size_t indexMax = path.size() > parentPath.size() ? parentPath.size() : path.size(); 
    //std::cout << std::to_string(indexMax) << std::endl;
    for(size_t i = 0; i < indexMax; i++) {
        if (parentPath[i] == path[i]) continue;
        else return false;
    }
    return true;
}

HSubstitution::HSubstitution(std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HPath*>> v) {
    values = v;
    for(auto val : values) {
        if(std::holds_alternative<HPath*>(val)) {
            HPath * path = std::get<HPath*>(val);
            path->parent = this;
            paths.push_back(path);
        }
    }
}

HSubstitution::~HSubstitution() {
    for (auto obj : values) {
        std::visit(deleteHObj, obj);
    }
}

std::string HSubstitution::str() {
    std::string out;
    if (values.size() > 0) {
        if (std::holds_alternative<HTree*>(values[0])) {
            out = std::visit(stringify, values[0]);//"{...}";
        } else if (std::holds_alternative<HArray*>(values[0])) {
            out = std::visit(stringify, values[0]);//"[...]";
        } else if (std::holds_alternative<HSimpleValue*>(values[0])){ 
            out = std::visit(stringify, values[0]);
        } else {
            out = (substitutionType == 2 || (substitutionType == 3 && values.size() > 1)) ? std::visit(stringify, values[0]) + std::get<HPath*>(values[0])->suffixWhitespace : std::visit(stringify, values[0]); 
        }
        switch (substitutionType) {
            case 0:
            case 1:
                for(auto iter = values.begin() +1 ; iter != values.end(); iter++) {
                    if (std::holds_alternative<HTree*>(*iter)) {
                        out += std::visit(stringify, *iter);//" {...}";
                    } else if (std::holds_alternative<HArray*>(*iter)) {
                        out += std::visit(stringify, *iter);//" [...]";
                    } else { 
                        out += " " + std::visit(stringify, *iter);
                    }
                }
                break;
            case 2:
            case 3:
                for(auto iter = values.begin() +1 ; iter != values.end(); iter++) {
                    if (std::holds_alternative<HTree*>(*iter)) {
                        out += std::visit(stringify, *iter);//" {...}";
                    } else if (std::holds_alternative<HArray*>(*iter)) {
                        out += std::visit(stringify, *iter);//" [...]";
                    } else if (std::holds_alternative<HPath*>(*iter)) {
                        out += std::visit(stringify, *iter) + std::get<HPath*>(*iter)->suffixWhitespace;
                    } else { 
                        out += std::visit(stringify, *iter);
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
    //out += " stack counter = " + std::to_string(counter);
    
    if(debug) {
        out += "interrupts: ";
        for(auto b : interrupts) {
            out += b ? "true " : "false "; 
        }
    }
    return out;
}

HSubstitution * HSubstitution::deepCopy() {
    //HArray * copy = new HArray();
    std::vector<std::variant<HTree*,HArray*,HSimpleValue*, HPath*>> copies; 
    for (auto obj : values) {
        copies.push_back(std::visit(subDeepCopy, obj));
    }
    HSubstitution * copy = new HSubstitution(copies);
    copy->parent = this->parent;
    copy->key = this->key;
    copy->interrupts = this->interrupts;
    copy->substitutionType = this->substitutionType;
    copy->includePrefix = this->includePrefix;
    return copy;
}

std::vector<std::string> HSubstitution::getPath() {
    if (std::holds_alternative<HTree*>(parent)) {
        if(!std::get<HTree*>(parent)) return std::vector<std::string>{"getpath on sub failed..."};
    }
    std::vector<std::string> parentPath = std::visit(getPathStr, parent);
    parentPath.push_back(key);
    return parentPath;
}

HParser::HParser(HTree * newRoot) {
    rootObject = newRoot->deepCopy();
}

HParser::HParser(HArray * newRoot) {
    rootObject = newRoot->deepCopy();
}

HParser::~HParser() {
    //std::visit(deleteHObj, rootObject);
    for(auto pair : stack) {
        std::visit(deleteHObj, pair.second);
    }
    for(auto sub : unresolvedSubs) {
        delete sub;
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
    size_t index = 0;
    for(auto s : stack) {
        std::string out = pathToString(s.first);
        HTree * deb;
        if(std::holds_alternative<HTree*>(s.second)) {
            deb = std::get<HTree*>(s.second);
        }
        std::cout << " --------- [" << index++ << "] --------- \n" << out << " : " << std::visit(stringify, s.second) << std::endl;
    }
}

/*
    Takes a root path to the value, and a pointer to the value, and pushes it to the stack.
*/
void HParser::pushStack(std::vector<std::string> path, std::variant<HTree*,HArray*,HSimpleValue*,HSubstitution*> value) {
    if(std::holds_alternative<HSubstitution*>(value)) {
        HSubstitution* sub = std::get<HSubstitution*>(value); 
        HTree * handle = std::get<HTree*>(sub->parent);
        for(auto val : sub->values) { 
            if (std::holds_alternative<HPath*>(val)) {
                HPath* hpath = std::get<HPath*>(val);
                hpath->counter = hpath->counter == -1 ? stack.size() : hpath->counter; 
            }
        }
        //unresolvedSubs.push_back(sub->deepCopy());
    }
    std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> temp = std::visit(getDeepCopy, value);
    HSubstitution * handle;
    if (std::holds_alternative<HSubstitution*>(temp)) {
        handle = std::get<HSubstitution*>(temp);
    }
    stack.push_back(std::make_pair(path, temp)); // leave it for now, a potential fix if this causes memory issues is creating a new tree that points to earlier copies in the stack, instead of creating a new deep copy.
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
            delete hoconTree(std::vector<std::string>());
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
            delete hoconTree(std::vector<std::string>());
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
            delete hoconTree(std::vector<std::string>());
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
        std::vector<std::string> rootPath = path;
        if (path.size() > 1) {
            target = findOrCreatePath(path, output);
        } else {
            target = output;
        }
        if (match(LEFT_BRACE)) { // implied separator case. ex: foo {}
            HTree * obj = mergeAdjacentTrees(rootPath);
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj, rootPath, true);
                if (target->addMember(keyValue, sub)) {
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, sub);
                }
            } else {
                //target->addMember(keyValue, obj);
                if(target->addMember(keyValue, obj)) { // the method returns true if the passed pointer was deleted.
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, obj);
                }
            } 
            consumeToNextRootMember();
            // need to add object concatentation here.
        } else if(match(KEY_VALUE_SEP)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                HTree * obj = mergeAdjacentTrees(rootPath);
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(obj, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }   
                } else {
                    if(target->addMember(keyValue, obj)) { // the method returns true if the passed pointer was deleted.
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, obj);
                    }
                }
            } else if (match(LEFT_BRACKET)) {
                HArray * arr = concatAdjacentArrays();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(arr, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }
                } else {
                    target->addMember(keyValue, arr);
                    pushStack(rootPath, arr);
                }
            } else if (check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(rootPath, true);
                if (target->addMember(keyValue, sub)) {
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, sub);
                }
            } else {   
                HSimpleValue * val = hoconSimpleValue();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(val, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }
                } else {
                    target->addMember(keyValue, val);
                    pushStack(rootPath, val);
                }
            }
            if (path.size() > 1) {
                HTree* temp = target;
                size_t offset = 1;
                while (temp != output) {
                    pushStack(std::vector<std::string>(rootPath.begin(), rootPath.end() - offset), temp);
                    offset++;
                    temp = std::get<HTree*>(temp->parent);
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
HTree * HParser::hoconTree(std::vector<std::string> parentPath) { 
    HTree * output = new HTree();
    HTree * target = output;
    using std::get;
    while(!match(RIGHT_BRACE)) { // loop through members
        if(atEnd()) {
            error(peek().line, "Imbalanced {}");
            break;
        }
        std::vector<std::string> rootPath = parentPath;
        if (isInclude(peek())) {
            HTree * includedTree = parseInclude(rootPath);
            consumeToNextMember();
            match(RIGHT_BRACE);
            delete output;
            return includedTree;
        }
        std::vector<std::string> path = hoconKey(); 
        std::string keyValue = path[path.size()-1];
        rootPath.insert(std::end(rootPath), std::begin(path), std::end(path));
        if (path.size() > 1) {
            target = findOrCreatePath(path, output);
        } else {
            target = output;
        }
        
        if (match(LEFT_BRACE)) {            // implied separator case. ex: foo {}
            HTree * obj = mergeAdjacentTrees(rootPath);
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj, rootPath, true);
                if (target->addMember(keyValue, sub)) {
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, sub);
                }
            } else {
                //target->addMember(keyValue, obj);
                if(target->addMember(keyValue, obj)) { // the method returns true if the passed pointer was deleted.
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, obj);
                }
            }
            consumeToNextMember();
        } else if(match(KEY_VALUE_SEP)) {   // explicit separator ex: foo = {}
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                HTree * obj = mergeAdjacentTrees(rootPath);
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(obj, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }
                } else {
                    //target->addMember(keyValue, obj);
                    if(target->addMember(keyValue, obj)) { // the method returns true if the passed pointer was deleted.
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, obj);
                    }
                }
            } else if (match(LEFT_BRACKET)) {
                HArray * arr = concatAdjacentArrays();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(arr, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }
                } else {
                    target->addMember(keyValue, arr);
                    pushStack(rootPath, arr);
                }
            } else if (check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(rootPath, true);
                if (target->addMember(keyValue, sub)) {
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, sub);
                }
            } else {   
                HSimpleValue * val = hoconSimpleValue();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(val, rootPath, true);
                    if (target->addMember(keyValue, sub)) {
                        pushStack(rootPath, target->members[keyValue]);
                    } else {
                        pushStack(rootPath, sub);
                    }
                } else {
                    target->addMember(keyValue, val);
                    pushStack(rootPath, val);
                }
            }
            if (path.size() > 1) {
                HTree* temp = target;
                size_t offset = 1;
                while (temp != output) {
                    pushStack(std::vector<std::string>(rootPath.begin(), rootPath.end() - offset), temp);
                    offset++;
                    temp = std::get<HTree*>(temp->parent);
                }
            }
            consumeToNextMember();
        } else if (match(PLUS_EQUAL)) {
            ignoreAllWhitespace();
            if (match(LEFT_BRACKET)) {
                HArray * append = concatAdjacentArrays();
                std::vector<std::variant<HTree*,HArray*,HSimpleValue*,HPath*>> list;
                HPath * appendPath = new HPath(rootPath, true);
                list.push_back(appendPath);
                list.push_back(append);
                HSubstitution * sub = new HSubstitution(list);
                appendPath->parent = sub;
                sub->interrupts = std::vector<bool>{false, false};
                if (target->addMember(keyValue, sub)) {
                    pushStack(rootPath, target->members[keyValue]);
                } else {
                    pushStack(rootPath, sub);
                }
            } else {
                error(peek().line, "Expected an array, got " + peek().lexeme);
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
            HTree * obj = mergeAdjacentArraySubTrees(); // pass in empty path here because we are not pushing objects within arrays into the stack since they do not have an accessible path.
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj, std::vector<std::string>(), false);
                output->addElement(sub);
                for (auto val : sub->values) {
                    if (std::holds_alternative<HPath*>(val)) {
                        std::get<HPath*>(val)->counter = stack.size();
                    }
                }
                //unresolvedSubs.push_back(sub->deepCopy());
            } else {
                output->addElement(obj);
            }
            consumeToNextElement(); 
        } else if (match(LEFT_BRACKET)) { // array case
            HArray * arr = concatAdjacentArrays();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(arr, std::vector<std::string>(), false);
                output->addElement(sub);
                for (auto val : sub->values) {
                    if (std::holds_alternative<HPath*>(val)) {
                        std::get<HPath*>(val)->counter = stack.size();
                    }
                }
                //unresolvedSubs.push_back(sub->deepCopy());
            } else {
                output->addElement(arr);
            }
            consumeToNextElement();
        } else if (check(SIMPLE_VALUES)) { // simple value case
            HSimpleValue * val = hoconSimpleValue();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(val, std::vector<std::string>(), false);
                output->addElement(sub);
                for (auto val : sub->values) {
                    if (std::holds_alternative<HPath*>(val)) {
                        std::get<HPath*>(val)->counter = stack.size();
                    }
                }
                //unresolvedSubs.push_back(sub->deepCopy());
            } else {
                output->addElement(val);
            }
            consumeToNextElement();
        } else if (check(SUB) || check(SUB_OPTIONAL)) {
            HSubstitution* sub = parseSubstitution(std::vector<std::string>(), false);
            output->addElement(sub);
            for (auto val : sub->values) {
                if (std::holds_alternative<HPath*>(val)) {
                    std::get<HPath*>(val)->counter = stack.size();
                }
            }
            //unresolvedSubs.push_back(sub->deepCopy());
            consumeToNextElement();
        } else {
            error(peek().line, "Expected a {, [ or a simple value, got " + peek().lexeme + "', instead");
            consumeMember();
        }
    }
    return output;
}

/*
    Attempts to create a hocon object with the following tokens, consuming all tokens including the ending '}'.
    Assumes you are within the object, after the first {
*/
HTree * HParser::hoconArraySubTree() { 
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
            HTree * obj = mergeAdjacentArraySubTrees();
            if(check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(obj, std::vector<std::string>(), false);
                if (!target->addMember(keyValue, sub)) { // return false indicates there was no merge (obj or substitution)
                    //unresolvedSubs.push_back(sub->deepCopy());
                }
            } else {
                target->addMember(keyValue, obj); // the method returns true if the passed pointer was deleted.
            }
            consumeToNextMember();
        } else if(match(KEY_VALUE_SEP)) {   // explicit separator ex: foo = {}
            ignoreAllWhitespace();
            if (match(LEFT_BRACE)) {
                HTree * obj = mergeAdjacentArraySubTrees();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(obj, std::vector<std::string>(), false);
                    for(auto e : sub->values) { 
                        if (std::holds_alternative<HPath*>(e)) {
                            HPath* hpath = std::get<HPath*>(e);
                            hpath->counter = hpath->counter == -1 ? stack.size() : hpath->counter; 
                        }
                    }
                    if (!target->addMember(keyValue, sub)) { // return false indicates there was no merge (obj or substitution)
                        //unresolvedSubs.push_back(sub->deepCopy());
                    }
                } else {
                    //target->addMember(keyValue, obj);
                    target->addMember(keyValue, obj); // the method returns true if the passed pointer was deleted.
                }
            } else if (match(LEFT_BRACKET)) {
                HArray * arr = concatAdjacentArrays();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(arr, std::vector<std::string>(), false);
                    for(auto e : sub->values) { 
                        if (std::holds_alternative<HPath*>(e)) {
                            HPath* hpath = std::get<HPath*>(e);
                            hpath->counter = hpath->counter == -1 ? stack.size() : hpath->counter; 
                        }
                    }
                    if (!target->addMember(keyValue, sub)) { // return false indicates there was no merge (obj or substitution)
                        //unresolvedSubs.push_back(sub->deepCopy());
                    }
                } else {
                    target->addMember(keyValue, arr);
                }
            } else if (check(SUB) || check(SUB_OPTIONAL)) {
                HSubstitution* sub = parseSubstitution(std::vector<std::string>(), false);
                for(auto e : sub->values) { 
                    if (std::holds_alternative<HPath*>(e)) {
                        HPath* hpath = std::get<HPath*>(e);
                        hpath->counter = hpath->counter == -1 ? stack.size() : hpath->counter; 
                    }
                }
                if (!target->addMember(keyValue, sub)) { // return false indicates there was no merge (obj or substitution)
                    //unresolvedSubs.push_back(sub->deepCopy());
                }
            } else {   
                HSimpleValue * val = hoconSimpleValue();
                if(check(SUB) || check(SUB_OPTIONAL)) {
                    HSubstitution* sub = parseSubstitution(val, std::vector<std::string>(), false);
                    for(auto e : sub->values) { 
                        if (std::holds_alternative<HPath*>(e)) {
                            HPath* hpath = std::get<HPath*>(e);
                            hpath->counter = hpath->counter == -1 ? stack.size() : hpath->counter; 
                        }
                    }
                    if (!target->addMember(keyValue, sub)) { // return false indicates there was no merge (obj or substitution)
                        //unresolvedSubs.push_back(sub->deepCopy());
                    }
                } else {
                    target->addMember(keyValue, val);
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
    Assume you have just consumed the k-v separator (obj context), or peeked the first simple value token (array context). 
    Consume until non-simple value
*/
HSimpleValue * HParser::hoconSimpleValue() {
    std::vector<Token> valTokens = std::vector<Token>();
    while(check(SIMPLE_VALUES) || check(WHITESPACE)) { // note, the WHITESPACE TokenType differentiates between newlines and traditional whitespace.
        valTokens.push_back(advance());
    }
    size_t end = valTokens.size() - 1;
    if (!check(SUB) && !check(SUB_OPTIONAL)) {
        auto iter = valTokens.rbegin();
        while (iter->type == WHITESPACE) {
            end--;
            iter++;
        }
        end++;
    }
    if (end > 1) { // value concat, parse as string,
        std::stringstream ss {""};
        
        for (size_t i = 0; i < end; i++) {
            ss << valTokens[i].lexeme;
        }
        return new HSimpleValue(ss.str(), valTokens, end); // end index is exclusive; 
    } else if (end == 1) { // parse normally
        return new HSimpleValue(valTokens.begin()->literal, valTokens, 1);
    } else {
        error(peek().line, "Expected a value, got nothing");
        return new HSimpleValue(0, valTokens, 0);
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

/* 
    Include file will consume and parse a sequence of include tokens, returning a tuple
    of the link string, an IncludeType, and a boolean designating required status.
*/
std::tuple<std::string, IncludeType, bool> HParser::hoconInclude() {
    bool required = false;
    IncludeType type = HEURISTIC;
    std::string link = "";
    advance();
    match(WHITESPACE);
    if (check(QUOTED_STRING)) {
        link = std::get<std::string>(advance().literal);
    } else if (check(UNQUOTED_STRING)) {
        if (peek().lexeme == "required") {
            required = true;
            advance();
            if (match(LEFT_PAREN)) {
                match(WHITESPACE);
            } else {
                error(peek().line, "expected '(', got " + peek().lexeme);
                return std::make_tuple("", URL, false);
            }
        }
        if (peek().lexeme == "url") {
            type = URL;
        } else if (peek().lexeme == "file") {
            type = FILEPATH;
        } else {
            error(peek().line, "expected url or file, got " + peek().lexeme);
            return std::make_tuple("", URL, false);
        }
        
        advance(); // consume url/file/classpath
        if (match(LEFT_PAREN)) {
            match(WHITESPACE);
            if (check(QUOTED_STRING)) {
                link = std::get<std::string>(advance().literal);
                if (!match(RIGHT_PAREN)) {
                    error(peek().line, "unterminated ()");
                    return std::make_tuple("", URL, false);
                } 
            } else {
                error(peek().line, "expected a quoted string, got " + peek().lexeme);
                return std::make_tuple("", URL, false);
            }
        } else {
            error(peek().line, "expected '(', got " + peek().lexeme);
            return std::make_tuple("", URL, false);
        }
        if (required && !match(RIGHT_PAREN)) {
            error(peek().line, "unterminated ()");
            return std::make_tuple("", URL, false);
        }
    } else {
        error(peek().line, "expected a quoted string, or one of \"required\", \"url\", or \"file\", got " + peek().lexeme);
        return std::make_tuple("", URL, false);
    }
    return std::make_tuple(link, type, required);
}

HTree * HParser::parseInclude(std::vector<std::string> rootPath) {
    std::string keyValue = rootPath[rootPath.size()-1];
    std::tuple<std::string, IncludeType, bool> out = hoconInclude();
    HTree * res;
    if (std::get<0>(out) == "") {
        return res;
    } else {
        std::string content = getFileText(std::get<0>(out), std::get<1>(out));
        Lexer lexer = Lexer(content);
        std::vector<Token> tokens = lexer.run();
        HParser includeParser = HParser(tokens);
        includeParser.parseTokens();
        int stackOffset = stack.size();

        // need to set includePrefix for both the stack and the tree because they are separate objects representing the same data.
        // by the time that we set the data in the tree, the unset version of the object was already copied to the stack.
        for (auto pair : includeParser.stack) {
            std::vector<std::string> resolvedIncludePath = pair.first;
            resolvedIncludePath.insert(resolvedIncludePath.begin(), rootPath.begin(), rootPath.end());
            if(std::holds_alternative<HSubstitution*>(pair.second)) {
                std::get<HSubstitution*>(pair.second)->includePrefix = rootPath;
                for(auto path : std::get<HSubstitution*>(pair.second)->paths) {
                    path->counter += stackOffset;
                }
            } else if (std::holds_alternative<HTree*>(pair.second) || std::holds_alternative<HArray*>(pair.second)) {
                for(auto sub : std::visit(getSubstitutions, pair.second)) {
                    sub->includePrefix = rootPath;
                    for(auto path : sub->paths) {
                        path->counter += stackOffset;
                    }
                }
            }
            pushStack(resolvedIncludePath, pair.second);
        }
        if (std::holds_alternative<HArray*>(includeParser.rootObject)) {
            error(0, "cannot include a json file which contains an array as the root.");
            return new HTree();
        }
        res = std::get<HTree*>(includeParser.rootObject);
        for (HSubstitution * sub : res->getUnresolvedSubs()) {
            sub->includePrefix = rootPath;
            for(auto path : sub->values) {
                if (std::holds_alternative<HPath*>(path)) {
                    HPath* temp = std::get<HPath*>(path);
                    temp->counter += stackOffset;
                }
            }
        }
        if (!res && !std::get<2>(out)) {
            error(peek().line, "non optional include failed to evaluate.");
        }
        return res;
    }
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
    Takes a vector of tokens containing a path expression, and splits it into strings for each path section. allows for path segments with periods if they are quoted.
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

/*
    Helper method to split a string path delimited with "." into a vector of strings. allows for path segments with periods if they are quoted.
*/
std::vector<std::string> HParser::splitPath(std::string const& path) {
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

/*
    parses the next chain of adjacent arrays and their corresponding tokens, and returning the merged result. Adds the resulting elements to the stack history.
*/
HArray * HParser::concatAdjacentArrays() {
    HArray * curr = hoconArray();
    while (match(LEFT_BRACKET)) { // array concatenation here;
        curr->concatArrays(hoconArray());
    }
    return curr;
}

/*
    parses the next chain of adjacent objects and their corresponding tokens, and returning the merged result. Adds the resulting elements to the stack history.
*/
HTree * HParser::mergeAdjacentTrees(std::vector<std::string> path) {
    HTree * curr = hoconTree(path); // ends after consuming right brace.
    while (match(LEFT_BRACE)) { // obj concatenation here
        HTree * next = hoconTree(path);
        curr->mergeTrees(next);
        delete next;
    }
    return curr;
}

/*
    parses the next chain of adjacent objects and their corresponding tokens, and returning the merged result. Does not add resulting elements to the stack history.
*/
HTree * HParser::mergeAdjacentArraySubTrees() {
    HTree * curr = hoconArraySubTree(); // ends after consuming right brace.
    while (match(LEFT_BRACE)) { // obj concatenation here
        HTree * next = hoconArraySubTree();
        curr->mergeTrees(next);
        delete next;
    }
    return curr;
}

HSubstitution * HParser::parseSubstitution(std::variant<HTree*, HArray*, HSimpleValue*> prefix, std::vector<std::string> parentPath, bool addingToStack) {
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
            if (addingToStack) {
                values.push_back(hoconTree(parentPath));
            } else {
                values.push_back(hoconArraySubTree());
            }
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
    for (auto element : values) {
        out->interrupts.push_back(false);
    }
    out->substitutionType = subType;
    return out;
}

HSubstitution * HParser::parseSubstitution(std::vector<std::string> parentPath, bool addingToStack) {
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
            if (addingToStack) {
                values.push_back(hoconTree(parentPath));
            } else {
                values.push_back(hoconArraySubTree());
            }
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
    for (auto element : values) {
        out->interrupts.push_back(false);
    }
    out->substitutionType = subType;
    return out;
}

bool HParser::isInclude(Token t) {
    return (t.type == UNQUOTED_STRING && t.lexeme == "include");
}

// mini helper method for getFileText
size_t write_to_string(void *ptr, size_t size, size_t count, void *stream) {
  ((std::string*)stream)->append((char*)ptr, 0, size*count);
  return size*count;
}

std::string HParser::getFileText(std::string const& link, IncludeType type) {
    std::ifstream includedFile;
    std::string content;
    switch (type) {
        case URL:
            CURL *curl;
            CURLcode result;

            // Create our curl handle  
            curl = curl_easy_init();  
            curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

            result = curl_easy_perform(curl);
            if(result != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(result));
            curl_easy_cleanup(curl);
            break;
        case FILEPATH:
            includedFile = std::ifstream(link);
            if (!includedFile.is_open()) {
                std::cerr << "ERROR: File " << link << " failed to open." << std::endl;
                exit(1);
            } else {
                std::ostringstream stream;
                stream << includedFile.rdbuf();
                content = stream.str();
            }
            break;
        case HEURISTIC:
            break;
    }
    return content;
}

// parsing steps:

void HParser::parseTokens() {
    ignoreAllWhitespace();
    if (match(LEFT_BRACKET)) { // root array
        ignoreAllWhitespace();
        rootObject = hoconArray();
        ignoreAllWhitespace();
    } else { // if not array, implicitly assumed to be object.
        rootBrace = match(LEFT_BRACE);
        ignoreAllWhitespace();
        if (rootBrace) {
            rootObject = hoconTree(std::vector<std::string>());
        } else {
            rootObject = rootTree();
        }
        ignoreAllWhitespace();

    }
    ignoreAllWhitespace();
    if (!atEnd()){
        error(peek().line, "Expected EOF, got " + peek().lexeme);
    }
}

void HParser::resolveSubstitutions() {
    std::unordered_set<HSubstitution*> subs = getUnresolvedSubs();
    std::vector<std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*>> resolved; // temp
    while ( !subs.empty() ) { // temporary loop for testing resolveSub();
        HSubstitution* curr = *subs.begin();
        // destroy HSubstitution in root HTree structure at the original path.
        
        // take the result from resolveSub and set that as the value referred to by the key. 
        // set parent values for the resolve sub object.
        std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> result = resolveSub(curr, subs, std::unordered_set<HSubstitution*>());
        if (std::visit(valueExists, result)) {
            std::variant<std::string> keyStr = curr->key;
            std::visit(linkResolvedSub, curr->parent, keyStr, result);
            //pushStack(curr->getPath(), result);
        } else {
            std::variant<std::string> keyStr = curr->key;
            HTree * debugPar = std::get<HTree*>(curr->parent);
            std::visit(deleteNullSub, curr->parent, keyStr);
        }
        delete curr;
        //resolved.push_back(result);
    }
    for (auto res : resolved) {
        std::visit(deleteHObj, res);
    }
}

std::unordered_set<HSubstitution*> HParser::getUnresolvedSubs() {
    return std::visit(getSubstitutions, rootObject);
}

/*
    Still need to implement proper optional substitution handling (in the case that they don't resolve)
    implement resolving to environment variable in resolvePath();
    do more testing. write more tests.
*/
std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> HParser::resolveSub(HSubstitution* sub, std::unordered_set<HSubstitution*>& set, std::unordered_set<HSubstitution*> history) {
    std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> concatValue;
    for (size_t i = 0; i < sub->values.size(); i++) {
        std::variant<HTree *, HArray *, HSimpleValue*, HPath*> value = sub->values[i];
        std::string debug = std::visit(stringify, value);
        if (std::holds_alternative<HPath*>(value)) {
            HPath * path = std::get<HPath*>(value);
            std::vector<std::string> oldPath = path->path;
            path->path.insert(path->path.begin(), sub->includePrefix.begin(), sub->includePrefix.end());
            std::variant<HTree*,HArray*, HSimpleValue*, HSubstitution*> res = resolvePath(path);
            if (!std::visit(valueExists, res) && !sub->includePrefix.empty()) {
                path->path = oldPath;
                res = resolvePath(path);
            }
            std::string envVar = getEnvVar(pathToString(path->path));
            if (envVar != "" && !std::visit(valueExists, res)) { // if no path resolves, look in the environment variables.
                res = new HSimpleValue(envVar, std::vector<Token>{Token(UNQUOTED_STRING, envVar, envVar, 0)}, 1);
            }
            if (std::visit(valueExists, res)) {
                //std::cout << pathToString(std::get<HPath*>(value)->path) << " resolved to \"" << std::visit(stringify, res) << "\""<< std::endl;
                if (std::holds_alternative<HSubstitution*>(res)) {
                    HSubstitution * nextRes = std::get<HSubstitution*>(res);
                    if (history.count(nextRes)) {
                        error(0, "cycle detected, the substitution with path " + pathToString(nextRes->getPath()) + " was visited twice.");
                        break;
                    } else {
                        history.insert(sub);
                        res = resolveSub(nextRes, set, history);
                    }
                }

                std::unordered_set<HSubstitution*> subs = std::visit(getSubstitutions, res);
                if (!subs.empty()) {
                    HTree * t = std::get<HTree*>(res);
                    resolveObj(res, history);
                }
                // case where the path resolves to substitution which resolves in to a simple value
                if (std::holds_alternative<HSimpleValue*>(res)) { // delete existing trailing path whitespace and add the current path's interrim whitespace to the resolved Value.
                    HSimpleValue * curr = std::get<HSimpleValue*>(res);
                    for (size_t i = curr->tokenParts.size() -1; i >= curr->defaultEnd; i--) {
                        curr->tokenParts.pop_back();
                    }
                    curr->tokenParts.push_back(Token(WHITESPACE, path->suffixWhitespace, path->suffixWhitespace, 0));
                }
                //return res;
            } else if (path->optional) { // try to resolve to a previously defined value, otherwise do not add the value
                res = resolvePrevValue(path->counter, sub->getPath());
                if (std::visit(valueExists, res)) {
                    if (std::holds_alternative<HSubstitution*>(res)) {
                        HSubstitution * nextRes = std::get<HSubstitution*>(res);
                        if (history.count(nextRes)) {
                            error(0, "cycle detected, the substitution with path " + pathToString(nextRes->getPath()) + " was visited twice.");
                        } else {
                            history.insert(sub);
                            res = resolveSub(nextRes, set, history);
                        }
                    }
                } else {
                    continue; // skip concatenation if the value doesn't exist.
                }
            } else {
                error(0, "non-optional substitution with path \"" + pathToString(path->path) + "\" and counter=" + std::to_string(path->counter) + " failed to resolve" + (path->isSelfReference()? " selfref" : " rootref"));
                break;
                //return new HSimpleValue("resolve failed", std::vector<Token>{Token(UNQUOTED_STRING, "resolve failed", "resolve failed", 0)});
            }
            concatValue = concatSubValue(concatValue, res, sub->interrupts[i]);
        } else {
            std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> temp;
            switch(value.index()) {
                case 0:
                    temp = std::get<HTree*>(value);
                    temp = std::visit(getDeepCopy, temp);
                    break;
                case 1:
                    temp = std::get<HArray*>(value);
                    temp = std::visit(getDeepCopy, temp);
                    break;
                case 2:
                    temp = std::get<HSimpleValue*>(value);
                    temp = std::visit(getDeepCopy, temp);
                    break;
                case 3:
                    break;
            }
            if (std::visit(valueExists, temp)) {
                concatValue = concatSubValue(concatValue, temp, sub->interrupts[i]);
            }
        }
    }
    if (debug) {
        if (std::visit(valueExists, concatValue)) {
            std::cout << "Final constructed value: \n" << std::visit(stringify, concatValue) << std::endl;
        } else {
            std::cout << "substitution did not contain any resolved values" << std::endl;
        }
    }

    set.erase(sub);
    return concatValue;
}

void HParser::resolveObj(std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> obj, std::unordered_set<HSubstitution*> history) {
    std::unordered_set<HSubstitution*> subs = std::visit(getSubstitutions, obj);
    std::unordered_set<HSubstitution*> dummy;
    bool isTree = std::holds_alternative<HTree*>(obj);
    bool isArray = std::holds_alternative<HArray*>(obj);
    for(auto sub : subs) {
        history.insert(sub);
        if (isTree) {
            std::get<HTree*>(obj)->members[sub->key] = resolveSub(sub, dummy, history);
        } else {
            std::get<HArray*>(obj)->elements[std::stoi(sub->key)] = resolveSub(sub, dummy, history);
        }
        history.erase(sub);
        delete sub;
    }
}

std::variant<HTree*, HArray*, HSimpleValue*, HSubstitution*> HParser::resolvePath(HPath* path) {
    //std::cout << path->str() << std::endl;
    std::variant<HTree*,HArray*, HSimpleValue*, HSubstitution*> out;
    auto it = stack.rbegin();
    if (path->isSelfReference()) { // handle unset counter here.
        it = stack.rend() - path->counter;
    }
    for (it; it != stack.rend(); it++) {
        if(path->path == it->first) {
            if (std::holds_alternative<HSubstitution*>(it->second)) {
                out = it->second;
            } else {
                out = std::visit(getDeepCopy, it->second);
                if (std::holds_alternative<HSimpleValue*>(out)) { // string processing for simple value case.
                    HSimpleValue * temp = std::get<HSimpleValue*>(out);
                    // disregard old trailing whitespace after defaultEnd
                    for (size_t i = temp->tokenParts.size() -1; i >= temp->defaultEnd; i--) {
                        temp->tokenParts.pop_back();
                    }
                    // add new whitespace stored in HPath;
                    if (path->suffixWhitespace != "") temp->tokenParts.push_back(Token(WHITESPACE, path->suffixWhitespace, path->suffixWhitespace, 0)); // might not be correct formatting for whitespace tokens.
                }
            }
            return out;
        }
    }
    return out;
}

/*
    concatenates resolved values in a substitution. note that this function should never receive a substitution, and if it does, should notify an error.
    target should never be a null pointer or else a segfault will occur.
*/
std::variant<HTree *, HArray *, HSimpleValue*, HSubstitution*> HParser::concatSubValue(std::variant<HTree *, HArray *, HSimpleValue*, HSubstitution*> source, std::variant<HTree *, HArray *, HSimpleValue*, HSubstitution*> target, bool interrupt) {
    if (!std::visit(valueExists, source) && std::visit(valueExists, target)) {
        return target;
    } else if (std::visit(valueExists, source) && !std::visit(valueExists, target)){
        error(0, "concatSubValue encountered a null merge target");
        return source;
    } else if (!std::visit(valueExists, source) && !std::visit(valueExists, target)) {
        error(0, "tried to merge two uninitialized values (null pointers) in concatSubValue()");
    }
    switch(target.index()) {
        case 0:
            if(source.index() == 0) {
                std::cout << "---------------- DEBUG OUTPUT ----------------\n" << std::visit(stringify, target) << "\n---------------- END ------------------" << std::endl;
                std::get<HTree*>(source)->mergeTrees(std::get<HTree*>(target));
                std::visit(deleteHObj, target);
            } else {
                error(0, "tried to merge a tree with a nontree");
            }
            return source;
            break;
        case 1:
            if (interrupt) {
                std::visit(deleteHObj, source);
                return target;
            } else if (source.index() == 1) {
                std::get<HArray*>(source)->concatArrays(std::get<HArray*>(target));
                return source;
            } else {
                error(0, "tried to merge array into a nonarray");
            }
            break;
        case 2:
            if (interrupt) { // changeto interrupt later.
                std::visit(deleteHObj, source);
                return target;
            } else {
                std::get<HSimpleValue*>(source)->concatSimpleValues(std::get<HSimpleValue*>(target));
                return source;
            }
            break;
        case 3:
            error(0, "failed to resolve substitution before concatenating.");
            break;
    }
    std::variant<HTree *, HArray *, HSimpleValue*, HSubstitution*> n;
    return n;
}

std::variant<HTree *, HArray *, HSimpleValue*, HSubstitution*> HParser::resolvePrevValue(int counter, std::vector<std::string> path) {
    std::variant<HTree*,HArray*, HSimpleValue*, HSubstitution*> out;
    for (auto it = stack.rend() - counter; it != stack.rend(); it++) {
        if (path == it->first) {
            if (std::holds_alternative<HSubstitution*>(it->second)) {
                out = it->second;
            } else {
                out = std::visit(getDeepCopy, it->second);
            }
            return out;
        }
    }
    return out;
}

std::variant<HTree*, HArray*, HSimpleValue*> HParser::getByPath(std::vector<std::string> const& path) {
    if (std::holds_alternative<HArray*>(rootObject)) {
        throw std::runtime_error("Error: cannot use path expressions for a rooted array");
    }
    HTree * curr = std::get<HTree*>(rootObject);
    for (auto iter = path.begin(); iter != path.end()-1; iter++) {
        if (curr->memberExists(*iter)) {
            curr = std::get<HTree*>(curr->members[*iter]);
        } else {
            std::string out;
            while(iter != path.begin()) {
                out = "." + *iter + out;
            }
            out = *path.begin() + out;
            throw std::runtime_error("invalid path expression, " + out + " does not exist");
        }
    }
    std::string valueKey = *(path.end()-1);
    std::variant<HTree*, HArray*, HSimpleValue*> result;
    if (std::holds_alternative<HTree*>(curr->members[valueKey])) {
        result = std::get<HTree*>(curr->members[valueKey]);
    } else if (std::holds_alternative<HArray*>(curr->members[valueKey])) {
        result = std::get<HArray*>(curr->members[valueKey]);
    } else if (std::holds_alternative<HSimpleValue*>(curr->members[valueKey])) {
        result = std::get<HSimpleValue*>(curr->members[valueKey]);
    } else {
        throw std::runtime_error("unresolved substitution encountered after parsing.");
    }
    return result;
}

std::string HParser::getValueString(std::string const& path) {
    std::vector<std::string> splitPathStr = splitPath(path);
    return std::visit(stringify, getByPath(splitPathStr));
}

// split string by delimiter helper. 

// access methods:

// error reporting:

void HParser::error(int line, std::string const& message) {
    report(line, "", message);
    validConf = false;
}

void HParser::report(int line, std::string const& where, std::string const& message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
}

bool HParser::run() {
    parseTokens();
    resolveSubstitutions();
    if (!validConf) {
        std::cout << "Invalid Configu ration, Aborted" << std::endl;
        std::cout << "Dumping tokens..." << std::endl;
        for (Token t : tokenList) {
            std::cout << t.str() << std::endl;
        }
        return false;
    }
    return true;
}
