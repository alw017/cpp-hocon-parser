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
        REQUIRE(parser.peek().lexeme == ":");
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
        REQUIRE(parser.peek().lexeme == ":");       
    }
    
    SECTION("multiple path with quoted strings") {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("first.\"second.third\":");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        std::vector<std::string> k = parser.hoconKey();
        REQUIRE( k[0] == "first" );
        REQUIRE( k[1] == "second.third" );
        REQUIRE(parser.peek().lexeme == ":");
    }

    SECTION("whitespaces before and after") {
        std::vector<Token> tokens = std::vector<Token>();
        Lexer lexer = Lexer("    first    :");
        tokens = lexer.run();
        HParser parser = HParser(tokens);
        std::vector<std::string> k = parser.hoconKey();
        REQUIRE( k[0] == "first" );
        REQUIRE(parser.peek().lexeme == ":");       
    }
}

TEST_CASE( "hoconTree" ) {
    SECTION("simple case") {
        HParser parser = initWithString("a = b}");
        HTree * rootObj = parser.hoconTree(std::vector<std::string>()); 
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HSimpleValue*>(rootObj->members["a"])->svalue.index() == 3);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == "b");
        REQUIRE(parser.atEnd() == true);
    }

    SECTION("nested case") {
        HParser parser = initWithString("a = {a = {b = {d = 2}}}}");
        HTree * rootObj = parser.hoconTree(std::vector<std::string>());
        REQUIRE(rootObj->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(rootObj->members["a"])->members.count("a") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members.count("b") == 1);
        REQUIRE(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members.count("d") == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["a"])->members["a"])->members["b"])->members["d"])->svalue) == 2);
        REQUIRE(parser.atEnd() == true);
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

TEST_CASE( "Omitted root brace/rootTree" ) {
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

// ----------------------------------- integration tests -----------------------------------

TEST_CASE( "Test Include" ) {
    SECTION( "File Test" ) {
        HParser parser = initWithString("{a = 2, b = {include file(\"../tests/test_include_file.conf\")} }");
        parser.parseTokens();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["b"])->members["c"])->svalue) == 2);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["b"])->members["d"])->svalue) == "value");
    } 

    SECTION( "Required file doesn't exist" ) {
        HParser parser = initWithString("b = [a,{ include required(file(\"../tests/test_i\")) }]");
        parser.parseTokens();
        REQUIRE(parser.validConf == false);
    }

    SECTION( "not required file doesn't exist" ) {
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

    SECTION( "include file with obj substitution" ) {
        HParser parser = initWithString("{ refToOtherFile = { a = 2, b = ${otherValue} }, otherValue = 6, includedFile = { include file(\"../tests/test_include_file_with_sub.conf\") }}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(std::get<HTree*>(rootObj->members["includedFile"])->members["a"])->members["b"])->svalue) == 6);
    }


    SECTION( "include file resolves substitutions locally before looking in the global stack" ) {
        HParser parser = initWithString("b = 4, c = {include file(\"../tests/test_include_file_with_local_sub.conf\")}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["c"])->members["a"])->svalue) == 2);
    }

    SECTION( "include file with include" ) {
        // TODO unimplemented
    }
}

TEST_CASE( "Key-value separators" ) {
    SECTION( "Typical case" ) {
        HParser parser = initWithString("a = 2\nb:1\n c={d:2}\nd:{c=1}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(rootObj->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(rootObj->members["b"])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["c"])->members["d"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["d"])->members["c"])->svalue) == 1);
    }

    SECTION( "Implied Separator" ) {
        HParser parser = initWithString("c{d:2}\nd{c=1}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * rootObj = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["c"])->members["d"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(rootObj->members["d"])->members["c"])->svalue) == 1);
    }
}

TEST_CASE( "Test Comments" ) {
    SECTION( "Head Comment" ) {
        HParser parser1 = initWithString("// this is a comment before the root\ntest = 2");
        HParser parser2 = initWithString("# this is a comment before the root obj braces \n {a = 2, b = 3}");
        parser1.parseTokens();
        parser2.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        HTree * root2 = std::get<HTree*>(parser2.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root1->members["test"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root2->members["a"])->svalue) == 2);
        REQUIRE(parser1.validConf == true);
        REQUIRE(parser2.validConf == true);
    }

    SECTION( "Comment after member definition" ) {
        HParser parser = initWithString("{ // test \n c=2,//comment 2\n d = value # comment 3\n}");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root->members["c"])->svalue) == 2);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["d"])->svalue) == "value");
        REQUIRE(parser.validConf == true);
    }

    SECTION( "Comments between keyvalue separators" ) {
        HParser parser = initWithString("{\n c // comment here \n=2, \n d = //another comment \n 2}");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root->members["c"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root->members["d"])->svalue) == 2);
        REQUIRE(parser.validConf == true);
    }

    SECTION( "Comments between shorthand obj definition" ) {
        HParser parser = initWithString("{ obj #comment\n {a = 2}}");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["obj"])->members["a"])->svalue) == 2);
        REQUIRE(parser.validConf == true);
    }
}

TEST_CASE( "Commas" ) {
    SECTION( "Trailing Commas" ) {
        HParser parser = initWithString("arr = [1,2,3,]");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(parser.validConf == true);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["arr"])->elements[0])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["arr"])->elements[1])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["arr"])->elements[2])->svalue) == 3);
    }

    SECTION( "Array Double Commas" ) {
        HParser parser = initWithString("arr = [1,,2]");
        parser.parseTokens();
        REQUIRE(parser.validConf == false);
        HParser sec = initWithString("arr = [,1,2]");
        sec.parseTokens();
        REQUIRE(sec.validConf == false);
        HParser third = initWithString("arr = [1,2,,]");
        third.parseTokens();
        REQUIRE(third.validConf == false);
    }

    SECTION( "Object Double Commas" ) {
        HParser parser = initWithString("obj = {a=2,,b=3}");
        parser.parseTokens();
        REQUIRE(parser.validConf == false);
        HParser sec = initWithString("obj = {,a=2,b=2}");
        sec.parseTokens();
        REQUIRE(sec.validConf == false);
        HParser third = initWithString("obj = {a=1,b=2,,}");
        third.parseTokens();
        REQUIRE(third.validConf == false);
    }
}

TEST_CASE( "Duplicate Keys/object merging" ) {
    SECTION( "Duplicate Key overwrite simple -> simple" ) {
        HParser parser1 = initWithString("val = 1\nval =2");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root1->members["val"])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[0].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[1].second)->svalue) == 2);
    }

    SECTION( "Duplicate Key overwrite simple -> obj" ) {
        HParser parser1 = initWithString("val = 1\nval = {a = 2}");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["val"])->members["a"])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[0].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[1].second)->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser1.stack[2].second)->members["a"])->svalue) == 2);
    }

    SECTION( "Duplicate Key overwrite simple -> arr" ) {
        HParser parser1 = initWithString("val = 1\nval =[2]");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root1->members["val"])->elements[0])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[0].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[1].second)->elements[0])->svalue) == 2);
    }

    SECTION( "Duplicate Key overwrite arr -> simple" ) {
        HParser parser1 = initWithString("val = [2]\nval =1");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root1->members["val"])->svalue) == 1);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[0].second)->elements[0])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[1].second)->svalue) == 1);
    }

    SECTION( "Duplicate Key overwrite obj -> simple" ) {
        HParser parser1 = initWithString("val = {a = 1}\nval =2");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root1->members["val"])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[0].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser1.stack[1].second)->members["a"])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[2].second)->svalue) == 2);
    }

    SECTION( "Duplicate Key overwrite arr -> obj" ) {
        HParser parser1 = initWithString("val = [2]\nval = {a = 1}");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["val"])->members["a"])->svalue) == 1);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[0].second)->elements[0])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[1].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser1.stack[2].second)->members["a"])->svalue) == 1);
    }

    SECTION( "Duplicate Key overwrite obj -> arr" ) {
        HParser parser1 = initWithString("val = {a = 1}\nval =[2]");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root1->members["val"])->elements[0])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[2].second)->elements[0])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser1.stack[0].second)->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser1.stack[1].second)->members["a"])->svalue) == 1);
    }

    SECTION( "Duplicate Key overwrite arr -> arr" ) {
        HParser parser1 = initWithString("val = [1]\nval =[2]");
        parser1.parseTokens();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root1->members["val"])->elements[0])->svalue) == 2);
        REQUIRE(root1->members.size() == 1);
        REQUIRE(root1->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[0].second)->elements[0])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(parser1.stack[1].second)->elements[0])->svalue) == 2);
    }


    SECTION( "Object Merging" ) {
        HParser parser = initWithString("a = {b = 2, c = 3}\na = {b = 3, d = 10}");
        parser.parseTokens();
        HTree* root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 1);
        REQUIRE(root->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["b"])->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["c"])->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["d"])->svalue) == 10);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser.stack[0].second)->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser.stack[1].second)->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser.stack[2].second)->members["b"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser.stack[2].second)->members["c"])->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser.stack[3].second)->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(parser.stack[4].second)->svalue) == 10);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser.stack[5].second)->members["b"])->svalue) == 3);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(parser.stack[5].second)->members["d"])->svalue) == 10);
    }
}

TEST_CASE( "Array concatenation" ) {
    SECTION( "Simple concatenation" ) {
        HParser parser = initWithString("a = [1] [2] [3]");
        parser.parseTokens();
        HTree* root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 1);
        REQUIRE(root->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["a"])->elements[0])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["a"])->elements[1])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(root->members["a"])->elements[2])->svalue) == 3);
    }
}

TEST_CASE( "Paths as Keys" ) {
    SECTION ( "simple case" ) {
        HParser parser = initWithString("a.b = 2");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 1);
        REQUIRE(root->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["b"])->svalue) == 2);
    }

    SECTION( "add member to object" ) {
        HParser parser = initWithString("a = {b = 1}\n a.c = 2");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 1);
        REQUIRE(root->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["b"])->svalue) == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["c"])->svalue) == 2);
    }

    SECTION( "overwrite member in object" ) {
        HParser parser = initWithString("a = {b = 1}\n a.b = 2");
        parser.parseTokens();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 1);
        REQUIRE(root->memberOrder.size() == 1);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["a"])->members["b"])->svalue) == 2);
    }

    SECTION( "empty string as part of path is invalid" ) {
        HParser parser = initWithString("a.\"\".b = 2");
        parser.parseTokens();
        REQUIRE(parser.validConf == false);
    }
}

TEST_CASE( "Substitutions" ) {
    
    SECTION( "normal case substitutions" ) {
        HParser parser = initWithString("a = 2\n b = ${a}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(root->members["b"])->svalue) == 2);
    }

    SECTION( "cycles terminate instead of looping" ) {
        HParser parser = initWithString("a = ${b}\n b =${a}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        REQUIRE(parser.validConf == false);
        HParser parser1 = initWithString("b =${b}");
        parser1.parseTokens();
        parser1.resolveSubstitutions();
        REQUIRE(parser1.validConf == false);
        HParser parser2 = initWithString("a : { b : ${a} }");
        parser2.parseTokens();
        parser2.resolveSubstitutions();
        REQUIRE(parser2.validConf == false);
    }

    SECTION( "string substitutions preserve whitespace" ) {
        HParser parser = initWithString("a = 2 before \n b = ${a} ${c} word \n c = after");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["a"])->svalue) == "2 before");
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["b"])->svalue) == "2 before after word");
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["c"])->svalue) == "after");
    }
   
    SECTION( "self referential substitutions" ) {
        HParser parser = initWithString("foo : { a : { c : 1 } } \n foo : ${foo.a}\nfoo : { a : 2 }");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["foo"])->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["foo"])->members["c"])->svalue) == 1);

        HParser parser1 = initWithString("bar : { foo : 42, baz : ${bar.foo}} \nbar : { foo : 43 }");
        parser1.parseTokens();
        parser1.resolveSubstitutions();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["bar"])->members["baz"])->svalue) == 43);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["bar"])->members["foo"])->svalue) == 43);
    }
    
    SECTION( "Recursive object resolving" ) {
        HParser parser = initWithString("a = {b = 2, c = ${ref}} \n b = ${a} \n ref = test");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["b"])->members["c"])->svalue) == "test");
    }
    
    SECTION( "Optional Substitutions" ) {
        HParser parser = initWithString("a = ${?nonexistent.path}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 0);
    }
}

TEST_CASE( "Optional Substitutions" ) {
    
    SECTION( "Optional Substitutions normal case" ) {
        HParser parser = initWithString("a = ${?nonexistent.path}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(root->members.size() == 0);
    }

    SECTION( "cycles terminate instead of looping" ) {
        HParser parser = initWithString("a = ${?b}\n b =${?a}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        REQUIRE(parser.validConf == false);
    }

    SECTION( "string substitutions preserve whitespace" ) {
        HParser parser = initWithString("a = 2 before \n b = ${?a} ${?c} word \n c = after");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["a"])->svalue) == "2 before");
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["b"])->svalue) == "2 before after word");
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root->members["c"])->svalue) == "after");
    }
   
    SECTION( "self referential substitutions" ) {
        HParser parser = initWithString("foo : { a : { c : 1 } } \n foo : ${?foo.a}\nfoo : { a : 2 }");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["foo"])->members["a"])->svalue) == 2);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["foo"])->members["c"])->svalue) == 1);

        HParser parser1 = initWithString("bar : { foo : 42, baz : ${?bar.foo}} \nbar : { foo : 43 }");
        parser1.parseTokens();
        parser1.resolveSubstitutions();
        HTree * root1 = std::get<HTree*>(parser1.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["bar"])->members["baz"])->svalue) == 43);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HTree*>(root1->members["bar"])->members["foo"])->svalue) == 43);

        HParser parser2 = initWithString("a = ${?a} foo");
        parser2.parseTokens();
        parser2.resolveSubstitutions();
        HTree * root2 = std::get<HTree*>(parser2.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(root2->members["a"])->svalue) == "foo");
    }
    
    SECTION( "Recursive object resolving" ) {
        HParser parser = initWithString("a = {b = 2, c = ${?ref}} \n b = ${?a} \n ref = test");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<std::string>(std::get<HSimpleValue*>(std::get<HTree*>(root->members["b"])->members["c"])->svalue) == "test");
    }
}

TEST_CASE( "Merging Substitutions" ) {
    SECTION( "merging an object with a substitution into a substitution" ) {
        HParser parser = initWithString("base { }\nfoo = ${base} { a { b = 1 }, c = [${foo.a.b}]}");
        parser.parseTokens();
        parser.resolveSubstitutions();
        HTree * root = std::get<HTree*>(parser.rootObject);
        REQUIRE(std::get<int>(std::get<HSimpleValue*>(std::get<HArray*>(std::get<HTree*>(root->members["foo"])->members["c"])->elements[0])->svalue) == 1);
    }

    
}



// --------------------------------- configfile ---------------------------------

// api method testing.

