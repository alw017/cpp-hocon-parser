#include "reader.hpp"
#include <parser.hpp>

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

ASTValue * ConfigFile::getByPath(ASTMapObject * root, std::string path) {
    auto members = root->members;
    std::vector<std::string> paths = splitString(path, '.');
    for(int i = 0; i < paths.size()-1; i++) {
        if (members.count(paths[i]) != 0) {
            if (std::holds_alternative<ASTMapObject *>(members[paths[i]]->value)) {
                members = std::get<ASTMapObject *>(members[paths[i]]->value)->members;
            }
        }
    }
    if (members.count(paths[paths.size()-1]) != 0) {
        return members[paths[paths.size()-1]];
    } else {
        return nullptr;
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
    //cout << file << endl;
    std::vector<Token> tokens = vector<Token>();
    Lexer lexer = Lexer(file, tokens);
    if (!lexer.run()) {
        std::cerr << "Error occurred. Terminating program." << endl;
        exit(1);
    }
    std::cout << "lexer finished." << std::endl;
    for (Token t : tokens) {
        std::cout << t.str() << std::endl;
    }
    /*
    Parser parser = Parser(tokens);
    if (parser.parseTokens()) {
            //std::cout << std::to_string(parser.valid) << std::endl;
        ASTValue * val = getByPath(parser.root, "testval");
        if (val != nullptr) std::cout << parse_util::to_string(val) << std::endl;
        //std::cout << parse_util::string(parser.root) << endl;
    } else {
        std::cerr << "Error occurred during parse. Terminating Program." << std::endl;
    };*/

}