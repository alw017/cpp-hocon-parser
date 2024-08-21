#include "reader.hpp"
//#include <parser.hpp>
#include <hocon-p.hpp>

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
    std::vector<Token> tokens;
    Lexer lexer = Lexer(file);
    tokens = lexer.run();
    if (lexer.hasError) {
        std::cerr << "Lexer Error occurred. Terminating program." << endl;
        exit(1);
    }
    //std::cout << "lexer finished." << std::endl;
    

    HParser parser = HParser(tokens);
    parser.lexer = &lexer;
    parser.parseTokens();
    if (!parser.validConf) {
        std::cout << "Invalid Configu ration, Aborted" << std::endl;
        std::cout << "Dumping tokens..." << std::endl;
        for (Token t : tokens) {
            std::cout << t.str() << std::endl;
        }
        return;
    }
    for (Token t : tokens) {
        //std::cout << t.str() << std::endl;
    }
    if(std::holds_alternative<HTree*>(parser.rootObject)) {
        HTree* p = std::get<HTree*>(parser.rootObject);
        std::cout << "Root Object String: \n" << p->str() << std::endl;
        HTree * deepCopy = p->deepCopy();
        //std::cout << "Deep Copy Object String: \n" << deepCopy->str() << std::endl;
        delete deepCopy;
    } else if (std::holds_alternative<HArray*>(parser.rootObject)) {
        HArray * p = std::get<HArray*>(parser.rootObject);
        std::cout << "Root Array String: \n" << p->str() << std::endl;
    }

    parser.getStack();
    /*
    std::unordered_set<HSubstitution*> unresolvedSubs = parser.getUnresolvedSubs();

    for (auto sub : unresolvedSubs) {
        std::string handles = "";
        for (auto unresolvedPath : sub->paths) {
            handles += (unresolvedPath->isSelfReference()?"selfref[":"rootref[") + std::to_string(unresolvedPath->counter) + "] ";
        }
        std::cout << sub->str();
        std::cout << ((sub->paths.size() > 1) ? ", with handles " : ", with handle ") << handles << std::endl;
    }
    */
    parser.resolveSubstitutions();
    std::cout << "\nResolved Object String: \n" << std::get<HTree*>(parser.rootObject)->str() << std::endl;
    if (std::holds_alternative<HTree*>(parser.rootObject)) {
        delete std::get<HTree*>(parser.rootObject);
    } else {
        delete std::get<HArray*>(parser.rootObject);
    }
    /*
    for (auto t : tree->members) {
            std::cout << t.first->key << " : " << std::endl;
    }
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