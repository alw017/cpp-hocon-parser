#include <lexer.hpp>
#include <reader.hpp>
#include <hocon-p.hpp>

#define IS_TRUE(x) { if (!(x)) std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; }

bool ASSERT_STRING(std::string str, std::string expect) {
    if (str == expect) {
        return true;
    } else {
        std::cout << "expected " << expect << ", got " << str << std::endl;
        return false;
    }
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

void test_parser_hoconSimpleValue() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("testvalue 214 true false m\n,", tokens);
    lexer.run();
    HParser parser = HParser(tokens);
    HSimpleValue * v = parser.hoconSimpleValue();
    bool succeed = ASSERT_STRING(v->str(), "testvalue 214 true false m");
    if (succeed) {
        std::cout << "hoconSimpleValue() succeeded" << std::endl;
    } else {
        std::cout << "hoconSimpleValue() failed" << std::endl;
    }
}

void test_parser_hoconTree_simpleValuesOnly() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("test {val: 1, obj {val = 2}}", tokens);
    lexer.run();
    HParser parser = HParser(tokens);
}

void test_parser_hoconArray() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("val1, value 2, { val : 1 }, ]", tokens);
    lexer.run();
    HParser parser = HParser(tokens);
    HArray * arr = parser.hoconArray();
    bool succeed = true;
    if (succeed) {
        std::cout << "hoconSimpleValue() succeeded" << std::endl;
    } else {
        std::cout << "hoconSimpleValue() failed" << std::endl;
    }
}

int main() {
    std::cout << "starting tests" << std::endl;
    test_parser_hoconKey();
    test_parser_hoconSimpleValue();
    test_parser_hoconArray();
}

