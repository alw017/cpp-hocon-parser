#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <lexer.hpp>
#include <parser.hpp>

class ConfigFile {
    private:
        std::string file;
        ASTMapObject * obj_tree;
    public:
        ConfigFile(char * filename);
        //~ConfigFile();

        ASTValue * getByPath(ASTMapObject * root, std::string path);
        
        void runFile(); // void for now but later it will return a map of relevant key/value pairs.
};

