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

HSimpleValue * debug_create_simple_string(std::string str) {
    Token t = Token(UNQUOTED_STRING, str, str, 0);
    return new HSimpleValue(str, std::vector<Token>{t});
}

void test_parser_hoconKey() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("test string 1 :", tokens);
    lexer.run();
    HParser parser = HParser(tokens);
    std::string k = parser.hoconKey()[0];
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
    std::cout << "hoconMergeTrees() ran" << std::endl;
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
    //std::cout << root->str() << std::endl;
    std::cout << "hoconConcatArray() ran" << std::endl;
    delete root;
}

void test_parser_splitString(std::string target) {
    std::vector<Token> tokens;
    Lexer lexer = Lexer(target, tokens);
    lexer.run();
    std::vector<std::string> out = HParser::splitPath(tokens);
    for(auto s : out) {
        //std::cout << s << std::endl;
    }
}

void test_parser_splitPath() {
    std::vector<Token> tokens;
    tokens.push_back(Token(UNQUOTED_STRING, "foo.", "foo.", 0));
    tokens.push_back(Token(QUOTED_STRING, "\"bar.baz\"", "bar.baz", 0));
    tokens.push_back(Token(UNQUOTED_STRING, ".gis", ".gis", 0));
    std::vector<std::string> out = HParser::splitPath(tokens);
    bool succeed = out.size() == 3;
    if (succeed) {
        succeed = ASSERT_STRING(out[0], "foo");
        succeed = ASSERT_STRING(out[1], "bar.baz");
        succeed = ASSERT_STRING(out[2], "gis");
    } else {
        std::cout << "Error, splitPath output unexpected number of strings, " << std::to_string(out.size()) << " instead of 3." << std::endl;
    }

    test_parser_splitString("10.0foo");
    test_parser_splitString("foo10.0");
    test_parser_splitString("foo\"10.0\"");
    test_parser_splitString("1.2.3");
    
    if (succeed) {
        std::cout << "splitPath() succeeded" << std::endl;
    } else {
        std::cout << "splitPath() failed" << std::endl;
    }
}

void test_parser_findCreate() {
    HTree * root = new HTree();
    root->addMember("foo", debug_create_simple_string("test"));
    HTree * second = new HTree();
    second->addMember("baz", debug_create_simple_string("value"));
    root->addMember("foo", second);
    std::cout << "Before FindCreate: \n" << root->str() << std::endl;
    HParser parser = HParser(std::vector<Token>());
    std::vector<Token> tokens;
    Lexer lexer = Lexer("foo.bar.baz", tokens);
    lexer.run();
    std::vector<std::string> out = HParser::splitPath(tokens);
    for(auto s : out) {
        //std::cout << s << std::endl;
    }
    std::cout << "Returned object: \n" << parser.findOrCreatePath(out, root)->str() << std::endl;
    std::cout << "After FindCreate: \n" << root->str() << std::endl;
}


int main() {
    std::cout << "starting tests" << std::endl;
    test_parser_hoconKey();
    test_parser_hoconSimpleValue();
    test_parser_hoconArray();
    test_parser_mergeTrees();
    test_parser_concatArray();
    test_parser_splitPath();
    test_parser_findCreate();
}

