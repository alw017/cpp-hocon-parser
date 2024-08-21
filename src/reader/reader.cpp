#include "reader.hpp"

using namespace std;

template<typename ... Ts>                                                 
struct Overload : Ts ... { 
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;


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

    HParser parser = HParser(tokens);
    parser.lexer = &lexer;
    parser.parseTokens();

    if(std::holds_alternative<HTree*>(parser.rootObject)) {
        HTree* p = std::get<HTree*>(parser.rootObject);
        std::cout << "Root Object String: \n" << p->str() << std::endl;
    } else if (std::holds_alternative<HArray*>(parser.rootObject)) {
        HArray * p = std::get<HArray*>(parser.rootObject);
        std::cout << "Root Array String: \n" << p->str() << std::endl;
    }

    parser.getStack();

    parser.resolveSubstitutions();
    if (!parser.validConf) {
        std::cout << "Invalid Configu ration, Aborted" << std::endl;
        std::cout << "Dumping tokens..." << std::endl;
        for (Token t : tokens) {
            std::cout << t.str() << std::endl;
        }
        return;
    }
    std::cout << "\nResolved Object String: \n" << std::get<HTree*>(parser.rootObject)->str() << std::endl;
    if (std::holds_alternative<HTree*>(parser.rootObject)) {
        delete std::get<HTree*>(parser.rootObject);
    } else {
        delete std::get<HArray*>(parser.rootObject);
    }
    //std::cout << parser.getValueString("simple.databases.active") << std::endl;
}