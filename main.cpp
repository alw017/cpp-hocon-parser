#include <lexer.hpp>
#include <reader.hpp>

int main(int argc, char * argv[]) {
    if (argc > 2) {
        std::cerr << "Expected: tester <scriptName>" << std::endl;
    } else if (argc == 2) {
        ConfigFile file = ConfigFile(argv[1]);
        file.runFile();
    } else {
        std::cout << "ran tester" << std::endl;
    }
}