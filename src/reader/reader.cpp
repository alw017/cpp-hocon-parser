#include "reader.hpp"

using namespace std;

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

auto simpleValueAsString = Overload {
    [](int num) { return std::to_string(num); },
    [](double num) { return std::to_string(num); },
    [](bool val) { return (val? std::string("true") : std::string("false")); },
    [](std::string str) { return str; }
};

std::vector<std::string> splitString(std::string str, char delim) {
    size_t pos = 1;
    size_t start = 0;
    std::vector<std::string> strings;
    if(str.length() == 0 || str[0] == delim) {
        return strings;
    }
    while (pos < str.length()) {
        if (str[pos++] == delim) {
            strings.push_back(str.substr(start, pos-start-1));
            start = pos;
        }
    }
    if(start == pos) {
        return std::vector<std::string>();
    } else {
        strings.push_back(str.substr(start));
        return strings;
    }
}

ConfigFile::ConfigFile(char * filename) {
    string filename_str = string(filename);
    ifstream conf_file(filename_str);
    if (!conf_file.is_open()) {
        cerr << "ERROR: File " << filename_str << " failed to open." << endl;
        exit(1);
    } else {
        ostringstream stream;
        stream << conf_file.rdbuf();
        file = stream.str();
    }
}

void ConfigFile::runFile() {
    std::vector<Token> tokens;
    Lexer lexer = Lexer(file);
    tokens = lexer.run();
    if (lexer.hasError) {
        std::cerr << "Lexer Error occurred. Terminating program." << endl;
        exit(1);
    }

    HParser * parser = new HParser(tokens);
    parser->lexer = &lexer;
    parser->parseTokens();

    if(std::holds_alternative<HTree*>(parser->rootObject)) {
        HTree* p = std::get<HTree*>(parser->rootObject);
        std::cout << "Root Object String: \n" << p->str() << std::endl;
    } else if (std::holds_alternative<HArray*>(parser->rootObject)) {
        HArray * p = std::get<HArray*>(parser->rootObject);
        std::cout << "Root Array String: \n" << p->str() << std::endl;
    }

    //parser->getStack();

    parser->resolveSubstitutions();
    if (!parser->validConf) {
        std::cout << "Invalid Configu ration, Aborted" << std::endl;
        std::cout << "Dumping tokens..." << std::endl;
        for (Token t : tokens) {
            std::cout << t.str() << std::endl;
        }
        return;
    }
    std::cout << "\nResolved Object String: \n" << std::get<HTree*>(parser->rootObject)->str() << std::endl;
    parserPtr = parser;
    /*
    std::cout << getStringByPath("foo.a") << std::endl;
    std::cout << getStringByPath("foo.b") << std::endl;
    std::cout << getStringByPath("foo.g") << std::endl;
    std::cout << getStringByPath("foo.i") << std::endl;
    std::string out = getBoolByPath("foo.g")? "true" : "false";
    std::cout << out << std::endl;
    out = getBoolByPath("foo.i") ? "true" : "false";
    std::cout << out << std::endl;
    std::cout << std::to_string(getIntByPath("foo.d")) << std::endl;
    std::cout << std::to_string(getDoubleByPath("foo.c")) << std::endl;
    */
}

ConfigFile::~ConfigFile() {
    delete parserPtr;
}

std::string ConfigFile::getStringByPath(std::string const& str) {
    std::vector<std::string> path = HParser::splitPath(str);
    std::variant<HTree*,HArray*,HSimpleValue*> res = parserPtr->getByPath(path);
    if (std::holds_alternative<HSimpleValue*>(res)) {
        return std::visit(simpleValueAsString, std::get<HSimpleValue*>(res)->svalue);
    } else {
        throw std::runtime_error("Error: getStringByPath encountered an invalid value at path " + str);
    }
}

bool ConfigFile::getBoolByPath(std::string const& str) {
    std::variant<HTree*,HArray*,HSimpleValue*> res = parserPtr->getByPath(HParser::splitPath(str));
    if (std::holds_alternative<HSimpleValue*>(res)) {
        std::variant<int, double, bool, std::string> svalue = std::get<HSimpleValue*>(res)->svalue;
        if (std::holds_alternative<bool>(svalue)) {
            return std::get<bool>(svalue);
        } else if (std::holds_alternative<std::string>(svalue)) {
            std::string val = std::get<std::string>(svalue);
            if (val == "true" || val == "yes" || val == "on") {
                return true;
            } else if (val == "false" || val == "no" || val == "off") {
                return false;
            } else {
                throw std::runtime_error("Error: getBoolByPath encountered an invalid value '" + val + "' at path " + str);
            }
        } else {
            throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
        }
    } else {
        throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
    }
}

double ConfigFile::getDoubleByPath(std::string const& str) {
    std::variant<HTree*,HArray*,HSimpleValue*> res = parserPtr->getByPath(HParser::splitPath(str));
    if (std::holds_alternative<HSimpleValue*>(res)) {
        std::variant<int, double, bool, std::string> svalue = std::get<HSimpleValue*>(res)->svalue;
        if (std::holds_alternative<std::string>(svalue)) {
            return std::strtod(str.c_str(), nullptr); // really should add a check for a valid string value here.
        } else if (std::holds_alternative<int>(svalue)) {
            return (double) std::get<int>(svalue);
        } else if (std::holds_alternative<double>(svalue)) {
            return std::get<double>(svalue);
        } else {
            throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
        }
    } else {
        throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
    }
}

int ConfigFile::getIntByPath(std::string const& str) {
    std::variant<HTree*,HArray*,HSimpleValue*> res = parserPtr->getByPath(HParser::splitPath(str));
    if (std::holds_alternative<HSimpleValue*>(res)) {
        std::variant<int, double, bool, std::string> svalue = std::get<HSimpleValue*>(res)->svalue;
        if (std::holds_alternative<std::string>(svalue)) {
            return std::stoi(str); // really should add a check for a valid string value here.
        } else if (std::holds_alternative<int>(svalue)) {
            return std::get<int>(svalue);
        } else if (std::holds_alternative<double>(svalue)) {
            return (int) std::get<double>(svalue);
        } else {
            throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
        }
    } else {
        throw std::runtime_error("Error: getBoolByPath encountered an invalid value at path " + str);
    }
}

bool ConfigFile::pathExists(std::string const& str) {
    std::variant<HTree*,HArray*,HSimpleValue*> res = parserPtr->getByPath(HParser::splitPath(str));
    if (std::holds_alternative<HTree*>(res) && !(std::get<HTree*>(res))) {
        return false;
    } else {
        return true;
    }
}