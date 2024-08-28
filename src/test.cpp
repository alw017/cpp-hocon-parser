#define CATCH_CONFIG_MAIN
#include <reader.hpp>
#include <catch2/catch_test_macros.hpp>

HParser initWithString(std::string str) {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer(str);
    tokens = lexer.run();
    return HParser(tokens);
}

// ---------------------------------- internal ----------------------------------

TEST_CASE("hoconSimpleValue") {
    SECTION( "Value concatenation case" ) {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("testvalue 214 true false m   \n,");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        HSimpleValue * v = parser.hoconSimpleValue();
        REQUIRE( std::get<std::string>(v->svalue) == "testvalue 214 true false m" );
        REQUIRE( parser.peek().lexeme == "\n");
        REQUIRE( parser.peek().type == NEWLINE);
        REQUIRE( v->defaultEnd == 9);
        REQUIRE( v->tokenParts.back().type == WHITESPACE);
        REQUIRE( v->tokenParts.size() == 10);
        delete v;
    }
    
    SECTION( "Single Value Case - string" ) {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("testvalue\n");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        HSimpleValue * v = parser.hoconSimpleValue();
        REQUIRE( std::get<std::string>(v->svalue) == "testvalue" );
        REQUIRE( parser.peek().lexeme == "\n");
        REQUIRE( parser.peek().type == NEWLINE);
        REQUIRE( v->defaultEnd == 1);
        REQUIRE( v->tokenParts.size() == 1);
        delete v;
    }

    SECTION( "Single Value Case - int" ) {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("1024\n");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        HSimpleValue * v = parser.hoconSimpleValue();
        REQUIRE( v->svalue.index() == 0 );
        REQUIRE( std::get<int>(v->svalue) == 1024 );
        REQUIRE( parser.peek().lexeme == "\n");
        REQUIRE( parser.peek().type == NEWLINE);
        REQUIRE( v->defaultEnd == 1);
        REQUIRE( v->tokenParts.size() == 1);
        delete v;
    }

    SECTION( "Single Value Case - double" ) {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("1024.192e-4\n");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        HSimpleValue * v = parser.hoconSimpleValue();
        REQUIRE( v->svalue.index() == 1 );
        double exp = std::strtod("1024.192e-4", nullptr);
        REQUIRE( std::get<double>(v->svalue) == exp );
        REQUIRE( parser.peek().lexeme == "\n");
        REQUIRE( parser.peek().type == NEWLINE);
        REQUIRE( v->defaultEnd == 1);
        REQUIRE( v->tokenParts.size() == 1);
        delete v;
    }

    SECTION( "Single Value Case - bool" ) {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("false\n");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        HSimpleValue * v = parser.hoconSimpleValue();
        REQUIRE( v->svalue.index() == 2 );
        REQUIRE( std::get<bool>(v->svalue) == false );
        REQUIRE( parser.peek().lexeme == "\n");
        REQUIRE( parser.peek().type == NEWLINE);
        REQUIRE( v->defaultEnd == 1);
        REQUIRE( v->tokenParts.size() == 1);
        delete v;
    }
}

TEST_CASE( "hoconKey" ) {

    SECTION("single path") {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("test string 1\n :");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        std::string k = parser.hoconKey()[0];
        REQUIRE( k == "test string 1");
    }

    SECTION("multiple path") {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("first.second.third:");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        std::vector<std::string> k = parser.hoconKey();
        REQUIRE( k[0] == "first" );
        REQUIRE( k[1] == "second" );
        REQUIRE( k[2] == "third" );        
    }
    
    SECTION("multiple path with quoted strings") {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("first.\"second.third\":");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        std::vector<std::string> k = parser.hoconKey();
        REQUIRE( k[0] == "first" );
        REQUIRE( k[1] == "second.third" );
    }
}

TEST_CASE( "hoconTree" ) {
    SECTION("simple case") {
        HParser parser = initWithString("{a = b}");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HSimpleValue*>(rootObj->members["a"])->svalue.index() == 3);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == "b");
    }

    SECTION("nested case") {
        HParser parser = initWithString("{a = {a = {b = {d = 2}}}}");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(rootObj->members["a"])->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members.count("b") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members.count("d") == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members["d"])->svalue) == 2);
    }

    SECTION("merged case") {
        HParser parser = initWithString("{a = {b = 2, c = 3} {c = 0, d = value}}");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["b"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["c"])->svalue) == 0);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["d"])->svalue) == "value");
    }
}

TEST_CASE( "rootTree" ) {
    SECTION("simple case") {
        HParser parser = initWithString("a = b");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HSimpleValue*>(rootObj->members["a"])->svalue.index() == 3);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == "b");
    }

    SECTION("nested case") {
        HParser parser = initWithString("a = {a = {b = {d = 2}}}");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(rootObj->members["a"])->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members.count("b") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members.count("d") == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members["d"])->svalue) == 2);
    }

    SECTION("merged case") {
        HParser parser = initWithString("a = {b = 2, c = 3} {c = 0, d = value}");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["b"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["c"])->svalue) == 0);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["a"])->members["d"])->svalue) == "value");
    }
}

TEST_CASE( "hoconArray" ) {
    HParser parser = initWithString("string, 1, {a = 2}, t]");
    HArray * arr = parser.hoconArray();
    REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(arr->elements[0])->svalue) == "string");
    REQUIRE(std::get<int>(std::get<HSimpleValue*>(arr->elements[1])->svalue) == 1);
    REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(arr->elements[2])->members["a"])->svalue) == 2);
    REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(arr->elements[3])->svalue) == "t");
}

TEST_CASE( "hoconArraySubTree" ) {
    HParser parser = initWithString("a = 1}");
    HTree * obj = parser.hoconArraySubTree();
    REQUIRE(parser.stack.size() == 0);
    REQUIRE(std::get<int>(std::get<HSimpleValue*>(obj->members["a"])->svalue) == 1);
}

// ----------------------------------- parser -----------------------------------

TEST_CASE( "Test Include" ) {
    SECTION( "File Test Required" ) {
        HParser parser = initWithString("{a = 2, b = {include file(\"../tests/test_include_file.conf\")} }");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["b"])->members["c"])->svalue) == 2);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["b"])->members["d"])->svalue) == "value");
    }

    SECTION( "File Test not required" ) {
        HParser parser = initWithString("b = [a,{ include file(\"../tests/test_i\") }]");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<HArray*>(rootObj->members["b"])->elements.size() == 1);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HArray*>(rootObj->members["b"])->elements[0])->svalue) == "a");
    }

    SECTION( "include file with substitution" ) {
        HParser parser = initWithString("{ refToOtherFile = achievedValue, includedFile = { include file(\"../tests/test_include_file_with_sub.conf\") }}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["includedFile"])->members["a"])->svalue) == "achievedValue");
    }
}

TEST_CASE( "Test Comments" ) {

}

TEST_CASE( "Omitted Root Brace" ) {

}

TEST_CASE( "Key-value separators" ) {

}

TEST_CASE( "Commas" ) {

}

TEST_CASE( "Duplicate Keys/object merging" ) {

}

TEST_CASE( "Array concatenation" ) {

}

TEST_CASE( "Paths as Keys" ) {

}

TEST_CASE( "Substitutions" ) {
    //normal
    //string preserve whitespace   
    //self-referential
    //recursive object
    //optional
    //cycle handling
}

// ----------------------------------- lexer ------------------------------------

// each token type

// --------------------------------- configfile ---------------------------------

// api method testing.

HSimpleValue * debug_create_simple_string(std::string str) {
    Token t = Token(UNQUOTED_STRING, str, str, 0);
    return new HSimpleValue(str, std::vector<Token>{t}, 0);
}

void test_parser_hoconTree_simpleValuesOnly() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("test {val: 1, obj {val = 2}}");
    tokens = lexer.run();
    HParser parser = HParser(tokens);
}

void test_parser_hoconArray() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("val1, value 2, { val : 1 }, ]");
    tokens = lexer.run();
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
    foo->addMember("key1", new HSimpleValue(s,std::vector<Token>{Token(UNQUOTED_STRING, s, s, 0)}, 0));
    obj->addMember("foo", foo);
    bar->addMember("key1", new HSimpleValue(ov,std::vector<Token>{Token(UNQUOTED_STRING, ov, ov, 0)}, 0));
    bar->addMember("key2", new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}, 0));
    obj->addMember("foo", bar);
    std::cout << "hoconMergeTrees() ran" << std::endl;
    delete obj;
}

void test_parser_concatArray() {
    HArray * root = new HArray();
    HArray * next = new HArray();
    root->addElement(new HSimpleValue(10, std::vector<Token>{Token(NUMBER, "10", 10, 0)}, 0));
    HTree * bar = new HTree();
    std::string ov("bar overwrite");
    std::string ne("bar newvalue");
    bar->addMember("key1", new HSimpleValue(ov,std::vector<Token>{Token(UNQUOTED_STRING, ov, ov, 0)}, 0));
    bar->addMember("key2", new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}, 0));
    root->addElement(bar);
    next->addElement(new HSimpleValue(1, std::vector<Token>{Token(NUMBER, "1", 1, 0)}, 0));
    next->addElement(new HSimpleValue(2, std::vector<Token>{Token(NUMBER, "2", 2, 0)}, 0));
    next->addElement(new HSimpleValue(ne,std::vector<Token>{Token(UNQUOTED_STRING, ne, ne, 0)}, 0));
    root->concatArrays(next);
    //std::cout << root->str() << std::endl;
    std::cout << "hoconConcatArray() ran" << std::endl;
    delete root;
}

void test_parser_splitString(std::string target) {
    std::vector<Token> tokens;
    Lexer lexer = Lexer(target);
    tokens = lexer.run();
    std::vector<std::string> out = HParser::splitPath(target);
    for(auto s : out) {
        std::cout << s << std::endl;
    }
}

void test_parser_splitPath() {
    std::vector<Token> tokens;
    tokens.push_back(Token(UNQUOTED_STRING, "foo.", "foo.", 0));
    tokens.push_back(Token(QUOTED_STRING, "\"bar.baz\"", "bar.baz", 0));
    tokens.push_back(Token(UNQUOTED_STRING, ".gis", ".gis", 0));
    std::vector<std::string> out = HParser::splitPath(tokens);
    test_parser_splitString("10.0foo");
    test_parser_splitString("foo10.0");
    test_parser_splitString("foo\"10.0\"");
    test_parser_splitString("1.2.3");
    
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
    Lexer lexer = Lexer("foo.bar.baz");
    tokens = lexer.run();
    std::vector<std::string> out = HParser::splitPath(tokens);
    for(auto s : out) {
        //std::cout << s << std::endl;
    }
    std::cout << "Returned object: \n" << parser.findOrCreatePath(out, root)->str() << std::endl;
    std::cout << "After FindCreate: \n" << root->str() << std::endl;
    delete root;
}

void test_parser_getPath() {
    HTree * root = new HTree();
    root->addMember("foo", debug_create_simple_string("test"));
    HTree * second = new HTree();
    second->addMember("baz", debug_create_simple_string("value"));
    root->addMember("bar", second);
    std::cout << "root obj str: \n" << root->str() << std::endl;
    std::cout << "get Path output: " << std::endl;
    for( auto s : std::get<HSimpleValue*>(second->members["baz"])->getPath()) {
        std::cout << s << std::endl;
    } 
    delete root;
}

void test_parser_substitution() {
    std::vector<std::variant<HTree*,HArray*, HSimpleValue*, HPath*>> values;
    values.push_back(new HTree());
    values.push_back(new HArray());
    values.push_back(new HPath(std::vector<std::string>{"test", "path", "boo"}, false));
    values.push_back(new HSimpleValue("unquotedstring", std::vector<Token>{Token(UNQUOTED_STRING, "unquotedstring", "unquotedstring", 0)}, 0));
    HSubstitution * sub = new HSubstitution(values);
    std::cout << sub->str() << std::endl;
    HTree * root = new HTree();
    root->addMember("foo", debug_create_simple_string("test"));
    root->addMember("bar", sub);
    std::cout << "root string: \n" << root->str() << std::endl;
    delete root;
}

void test_parser_includeFile() {
    std::vector<Token> tokens = std::vector<Token>();
    Lexer lexer = Lexer("include requid(url(\"test\"))");
    tokens = lexer.run();
    HParser parser = HParser(tokens);
    std::tuple<std::string, IncludeType, bool> out = parser.hoconInclude();
}

