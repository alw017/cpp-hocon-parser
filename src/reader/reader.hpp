#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <lexer.hpp>
#include <hocon-p.hpp>
#include <vector>


class ConfigFile {
    private:
        std::string file;
    public:
        ConfigFile(char * filename);
        //~ConfigFile();        
        void runFile(); // void for now but later it will return a map of relevant key/value pairs.
};

