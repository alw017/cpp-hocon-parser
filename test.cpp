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
    std::string k = parser.hoconKey();
    bool succeed = ASSERT_STRING(k, "test string 1");
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
    delete v;
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
        std::cout << "hoconArray() ran" << std::endl;
    } else {
        std::cout << "hoconArray() failed" << std::endl;
    }
    delete arr;
}

void test_parser_mergeTrees() {
    HTree * obj = new HTree();
    HTree * foo = new HTree();
    HTree * bar = new HTree();
    std::string s = "fooval1";
    std::string ov("bar overwrite");
    std::string ne("bar newvalue");
    foo->addMember("key1", new HSimpleValue(s,std::vector<Token>{Token(UNQUOTED_STRING, s, s, 0)}));
    obj->addMember("foo", foo);
    bar->addMember("key1", new HSimpleValue(ov,std::vector<Token>{Token(UNQUOTED_STRING, ov, ov, 0)}));
    bar->addMember("key2", new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}));
    obj->addMember("foo", bar);
    std::cout << obj->str() << std::endl;
    delete obj;
}

void test_parser_concatArray() {
    HArray * root = new HArray();
    HArray * next = new HArray();
    root->addElement(new HSimpleValue(10, std::vector<Token>{Token(NUMBER, "10", 10, 0)}));
    HTree * bar = new HTree();
    std::string ov("bar overwrite");
    std::string ne("bar newvalue");
    bar->addMember("key1", new HSimpleValue(ov,std::vector<Token>{Token(UNQUOTED_STRING, ov, ov, 0)}));
    bar->addMember("key2", new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}));
    root->addElement(bar);
    next->addElement(new HSimpleValue(1, std::vector<Token>{Token(NUMBER, "1", 1, 0)}));
    next->addElement(new HSimpleValue(2, std::vector<Token>{Token(NUMBER, "2", 2, 0)}));
    next->addElement(new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}));
    root->concatArrays(next);
    std::cout << root->str() << std::endl;
    delete root;
}

int main() {
    std::cout << "starting tests" << std::endl;
    test_parser_hoconKey();
    test_parser_hoconSimpleValue();
    test_parser_hoconArray();
    test_parser_mergeTrees();
    test_parser_concatArray();
}

