#include <lexer.hpp>
#include <reader.hpp>
#include <hocon-p.hpp>

#define IS_TRUE(x) { if (!(x)) std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; }

bool ASSERT_STRING(std::string str, std::string expect) {
    if (str == expect) {
        return true;
    } return false;
}

bool ASSERT(std::string str, std::string expect) {
    if (str == expect) {
        return true;
    } return false;
}

void test_parser_hoconKey() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("test string 1 :", tokens);
    lexer.run();
    HParser parser = HParser(tokens);
    HKey * k = parser.hoconKey();
    bool succeed = ASSERT_STRING(k->key, "test string 1");
    if (succeed) {
        std::cout << "hoconKey() succeeded" << std::endl;
    } else {
        std::cout << "hoconKey() failed" << std::endl;
    }
}

int main() {
    std::cout << "starting tests" << std::endl;
    test_parser_hoconKey();
}

