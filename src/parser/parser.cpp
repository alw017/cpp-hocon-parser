#include "parser.hpp"

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

auto TypeOfASTValue = Overload {                                     
    [](double) { return "double"; },
    [](int) { return "int"; },
    [](ASTArray& arr) { return "Array"; },
    [](ASTObject& obj) { return "Object"; },
    [](bool) { return "bool"; },
    [](std::string) {return "string";},
    [](auto) { return "unknown type"; },
};

auto stringifyASTNode = Overload {
    [](double d) { return std::to_string(d); },
    [](int i) { return std::to_string(i); },
    [](ASTArray * arr) { 
        return parse_util::array(arr); 
    },
    [](ASTMapObject * obj) { 
        return parse_util::string(obj); 
    },
    [](ASTObject * ) { 
        return std::string("object");
        },
    [](bool b) { 
        return b ? std::string("true") : std::string("false"); 
        },
    [](std::string str) {
        return str;
        },
    [](auto) { 
        return std::string("unknown type"); 
        },
};

std::string parse_util::parseValues(std::vector<ASTValue> values) {
    std::string output = "";
    for (auto v : values) {
        output += std::visit(TypeOfASTValue, v.value);
    }
    return output;
}

std::string parse_util::string(ASTMapObject * obj) {
    std::string output = "\n { ";
    for (auto v : obj->members){
        output = output + v.first + ":" + std::visit(stringifyASTNode, v.second->value) + ", ";
    }
    return output + " } ";
}

std::string parse_util::array(ASTArray * arr) {
    std::string output = "[";
    for (auto v : arr->arr){
        output = output + std::visit(stringifyASTNode, v->value) + ", ";
    }
    return output + "]";
}

std::string parse_util::to_string(ASTValue * val) {
    return std::visit(stringifyASTNode, val->value);
}

ASTMember::~ASTMember() {
    delete value;
}

ASTArray::~ASTArray() {
    for (ASTValue * v : arr) {
        delete v;
    }
}

ASTObject::~ASTObject() {
    for (auto m : members) {
        delete m;
    }
}

ASTValue::~ASTValue() {
    if (std::holds_alternative<ASTObject *>(value)) {
        delete std::get<ASTObject*>(value);
    } else if (std::holds_alternative<ASTArray *>(value)) {
        delete std::get<ASTArray*>(value);
    } else if (std::holds_alternative<ASTMapObject *>(value)) {
        delete std::get<ASTMapObject*>(value);
    }
}

ASTMapObject::~ASTMapObject() {
    for (std::pair<std::string, ASTValue *> p : members) {
        delete p.second;
    }
}

Parser::~Parser() {
    delete root;
}

bool Parser::parseTokens() {
    ASTMapObject * output;
    if (match(LEFT_BRACE)) {
        output = mapObject();
    } else {
        error(peek().line, "expected {, got " + peek().lexeme);
        return false;
    }
    //std::cout << "end: " << peek().lexeme << std::endl;
    if (peek().type != ENDFILE){
        error(peek().line, "expected EOF, got " + peek().lexeme);
        return false;
    } else {
        root = output;
        return valid;
    }
}

bool Parser::atEnd() {
    return peek().type == ENDFILE;
}

bool Parser::match(std::vector<TokenType> types) {
    for(auto t : types) {
        if (check(t)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    if(atEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!atEnd()) return tokens[current++];
    else return tokens[current];
}

Token Parser::peek() {
    return tokens[current];
}

Token Parser::previous() {
    if(current > 1) return tokens[current - 1];
    else {
        return tokens[current];
    }
}

ASTObject* Parser::object() { //TODO Error Checking.
    std::vector<ASTMember*> members = std::vector<ASTMember*>();
    if (match(RIGHT_BRACE)) {
        //std::cout << advance().lexeme << " | end object";
        return new ASTObject();
    } else { // error check here for strings later.
        members.push_back(member());
        while (match(COMMA)) {
            members.push_back(member());
        }
        if (match(RIGHT_BRACE)) return new ASTObject(members);
        else {
            error(peek().line, "expected }, got " + peek().lexeme);
        }
        return new ASTObject(members);
    }
}

ASTMapObject * Parser::mapObject() { //TODO Error Checking.
    auto members = std::unordered_map<std::string, ASTValue *>();
    if (match(RIGHT_BRACE)) {
        return new ASTMapObject();
    } else {
        members.insert(mapMember());
        while (match(COMMA)) {
            members.insert(mapMember());
        }
        if (match(RIGHT_BRACE)) return new ASTMapObject(members);
        else {
            error(peek().line, "expected }, got " + peek().lexeme);
        }
        return new ASTMapObject();
    }
}

ASTMember* Parser::member() {
    if (check(QUOTED_STRING)) {
        Token keyToken = advance();
        std::string key = std::get<std::string>(keyToken.literal);
        advance(); // consume separator.
        return new ASTMember(key, value());
    } else {
        error(peek().line, "Expected string, got " + peek().lexeme);
    }
    return new ASTMember();
}

std::pair<std::string, ASTValue*> Parser::mapMember() {
    if (check(QUOTED_STRING)) {
        Token keyToken = advance();
        std::string key = std::get<std::string>(keyToken.literal);
        if (!match(COLON)) {
            error(peek().line, "Expected colon, got " + peek().lexeme);
        } // consume separator.
        return std::make_pair(key, value());
    } else {
        error(peek().line, "Expected string, got " + peek().lexeme);
    }
    return std::make_pair("", new ASTValue(0));
}

ASTArray * Parser::array() {
    if (match(RIGHT_BRACKET)) {
        return new ASTArray();
    } else {
        std::vector<ASTValue*> values = std::vector<ASTValue*>();
        values.push_back(value());
        while (match(COMMA)) {
            values.push_back(value());
        }
        if (match(RIGHT_BRACKET)) return new ASTArray(values);
        else {
            error(peek().line, "expected ], got " + peek().lexeme);
        }
        return new ASTArray();
    }
}

ASTValue * Parser::value() {
    Token t = advance();
    
    switch (t.type) {
        case NUMBER:
            return new ASTValue(std::get<double>(t.literal));
        case QUOTED_STRING:
            return new ASTValue(std::get<std::string>(t.literal));
        case TRUE:
            return new ASTValue(true);
        case FALSE:
            return new ASTValue(false);
        case NULLVALUE:
            return new ASTValue(0);
        case LEFT_BRACE: // {}
            return new ASTValue(mapObject());
        case LEFT_BRACKET: // []
            return new ASTValue(array());
        default:
            error(t.line, "Unexpected symbol " + t.lexeme);
            return new ASTValue(0);
            break;
    }
} 


void Parser::error(int line, std::string message) {
    report(line, "", message);
    valid = false;
    // add error recovery here.
}

void Parser::report(int line, std::string where, std::string message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
}