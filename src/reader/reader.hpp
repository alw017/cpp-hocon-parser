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
        HParser * parserPtr;
    public:
        ConfigFile(char * filename);
        ~ConfigFile();        
        void runFile(); // void for now but later it will return a map of relevant key/value pairs.
        std::string getStringByPath(std::string const& str);
        bool getBoolByPath(std::string const& str);
        double getDoubleByPath(std::string const& str);
        int getIntByPath(std::string const& str);
        bool pathExists(std::string const& str);
};

