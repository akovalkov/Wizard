//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <vector>
#include <algorithm>
#include <iterator>
#include <doctest/doctest.h>
#include "helper.h"
#include "TestVisitor.h"
#include "../library/Parser.h"

using namespace Wizard;

extern GlobalFixture fixture;

TEST_CASE("Lexer empty") {
    std::vector<Token> test_tokens{
        Token{Token::Kind::Eof, {}}
    };

    std::vector<Token> tokens;
    LexerConfig config;
    auto scanner = make_scanner(config);
    auto sequence = scanner("");
    while (!sequence.done()) {
        auto token = sequence.next();
        tokens.push_back(token);
    }
    CHECK(tokens == test_tokens);
}


TEST_CASE("Lexer DatabaseSchema.tpl") {
    
    auto filepath = fixture.templatesDir;
    filepath /= "sql/DatabaseSchema.tpl";
    std::string str = read_file(filepath);

    std::vector<Token> test_tokens{
        // Token{Token::Kind::LineStatementOpen, "##"},
        // Token{Token::Kind::Id, "template"},
        // Token{Token::Kind::Id, "DatabaseSchema"},
        // Token{Token::Kind::LineStatementClose, "\n"},
        Token{Token::Kind::CommentOpen, "{#"},
        Token{Token::Kind::CommentClose, "\n  Template for db.sql, mysql dump\"\n  - name\n  - host\n  - tables\n  - idtables\n-#}"},
        Token{Token::Kind::LineStatementOpen, "##"},
        Token{Token::Kind::Id, "file"},
        Token{Token::Kind::String, "\"db.sql\""},
        Token{Token::Kind::LineStatementClose, "\n"},
        Token{Token::Kind::Text, "-- phpMyAdmin SQL Dump\n"
        "-- version 3.3.8\n"
        "-- http://www.phpmyadmin.net\n"
        "--\n"
        "-- Host: "},
        Token{Token::Kind::ExpressionOpen, "{{"},
        Token{Token::Kind::Id, "host"},
        Token{Token::Kind::ExpressionClose, "}}"},
        Token{Token::Kind::Text, "\n"
        "\n"
        "SET SQL_MODE=\"NO_AUTO_VALUE_ON_ZERO\";\n"
        "\n"
        "--\n"
        "-- Database: `"},
        Token{Token::Kind::ExpressionOpen, "{{"},
        Token{Token::Kind::Id, "name"},
        Token{Token::Kind::ExpressionClose, "}}"},
        Token{Token::Kind::Text, "`\n"
        "--\n"
        "\n"
        "--\n"
        "-- Table `session`\n"
        "--\n"
        "\n"
        "CREATE TABLE IF NOT EXISTS `session` (\n"
        "  `id` char(32) NOT NULL DEFAULT '',\n"
        "  `name` char(32) NOT NULL DEFAULT '',\n"
        "  `modified` int(11) DEFAULT NULL,\n"
        "  `lifetime` int(11) DEFAULT NULL,\n"
        "  `data` text,\n"
        "  PRIMARY KEY (`id`,`name`)\n"
        ") ENGINE=MyISAM DEFAULT CHARSET=utf8;\n"
        "\n"},
        Token{Token::Kind::LineStatementOpen, "##"},
        Token{Token::Kind::Id, "apply-template"},
        Token{Token::Kind::Id, "TableSchema"},
        Token{Token::Kind::Id, "tables"},
        Token{Token::Kind::LineStatementClose, "\n"},
        Token{Token::Kind::LineStatementOpen, "##"},
        Token{Token::Kind::Id, "apply-template"},
        Token{Token::Kind::Id, "TableSchemaId"},
        Token{Token::Kind::Id, "idtables"},
        Token{Token::Kind::LineStatementClose, "\n"},
        Token{Token::Kind::LineStatementOpen, "##"},
        Token{Token::Kind::Id, "endfile"},
        Token{Token::Kind::LineStatementClose, "\n"},
        // Token{Token::Kind::LineStatementOpen, "##"},
        // Token{Token::Kind::Id, "endtemplate"},
        // Token{Token::Kind::LineStatementClose, "\n"},
        Token{Token::Kind::Eof, {}}
    };

    std::vector<Token> tokens;
    LexerConfig config;
    auto scanner = make_scanner(config);
    auto sequence = scanner(str);
    auto itTest = test_tokens.begin();
    while (!sequence.done()) {
        auto token = sequence.next();
        CHECK(token == *itTest++);
        tokens.push_back(token);
    }
    // for (auto&& token : tokens) {
    //     std::cout << enum_token_kind[(int)token.kind] << ", " << std::quoted(token.text) << std::endl;
    // }
    //CHECK(tokens.size() == 34);
    CHECK(tokens.size() == 29);
    CHECK(tokens == test_tokens);
}

TEST_CASE("Parser DatabaseSchema.tpl") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    lconfig.templates_dir = fixture.templatesDir;

    Parser parser(pconfig, lconfig, templates, functions);
    std::filesystem::path template_name = "sql/DatabaseSchema.tpl";
    Template root = parser.parse_file(template_name);
    TestVisitor rootVisitor;
    rootVisitor.process(root);
    // template "DatabaseSchema.tpl"
    std::vector<TestVisitor::NodeInfo> test_root_nodes{
        {"Block", 0},
        {"FileStatement", 1},
        {"ExpressionWrapper", 2},
        {"Literal", 3},
        {"Block", 2},
        {"Text", 3},
        {"ExpressionWrapper", 3},
        {"Data", 4},
        {"Text", 3},
        {"ExpressionWrapper", 3},
        {"Data", 4},
        {"Text", 3},
        {"ApplyTemplateStatement", 3},
        {"ApplyTemplateStatement", 3},
    };
    CHECK(test_root_nodes == rootVisitor.nodes);
    CHECK(templates.size() == 3);
    std::vector<std::vector<TestVisitor::NodeInfo>> test_nodes{
        {
            // template "FieldSchema.tpl"
            {"Block", 0},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Function", 3},
            {"Data", 4},
            {"Literal", 4},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Function", 4},
            {"Data", 5},
            {"Literal", 5},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 3},
            {"ExpressionWrapper", 4},
            {"Function", 5},
            {"Data", 6},
            {"Literal", 6},
            {"Block", 4},
            {"Text", 5},
            {"Block", 4},
            {"IfStatement", 5},
            {"ExpressionWrapper", 6},
            {"Function", 7},
            {"Data", 8},
            {"Literal", 8},
            {"Block", 6},
            {"Text", 7},
            {"Block", 6},
            {"IfStatement", 7},
            {"ExpressionWrapper", 8},
            {"Function", 9},
            {"Data", 10},
            {"Literal", 10},
            {"Block", 8},
            {"Text", 9},
            {"Block", 8},
            {"IfStatement", 9},
            {"ExpressionWrapper", 10},
            {"Function", 11},
            {"Data", 12},
            {"Literal", 12},
            {"Block", 10},
            {"Text", 11},
            {"Block", 10},
            {"IfStatement", 11},
            {"ExpressionWrapper", 12},
            {"Function", 13},
            {"Data", 14},
            {"Literal", 14},
            {"Block", 12},
            {"Text", 13},
            {"Block", 12},
            {"IfStatement", 13},
            {"ExpressionWrapper", 14},
            {"Function", 15},
            {"Data", 16},
            {"Literal", 16},
            {"Block", 14},
            {"Text", 15},
            {"ExpressionWrapper", 15},
            {"Data", 16},
            {"Text", 15},
            {"Block", 14},
            {"IfStatement", 15},
            {"ExpressionWrapper", 16},
            {"Function", 17},
            {"Data", 18},
            {"Literal", 18},
            {"Block", 16},
            {"Text", 17},
            {"Block", 16},
            {"IfStatement", 17},
            {"ExpressionWrapper", 18},
            {"Function", 19},
            {"Data", 20},
            {"Literal", 20},
            {"Block", 18},
            {"Text", 19},
            {"Block", 18},
            {"IfStatement", 19},
            {"ExpressionWrapper", 20},
            {"Function", 21},
            {"Data", 22},
            {"Literal", 22},
            {"Block", 20},
            {"Text", 21},
            {"Block", 20},
            {"IfStatement", 21},
            {"ExpressionWrapper", 22},
            {"Function", 23},
            {"Data", 24},
            {"Literal", 24},
            {"Block", 22},
            {"Text", 23},
            {"Block", 22},
            {"IfStatement", 23},
            {"ExpressionWrapper", 24},
            {"Function", 25},
            {"Data", 26},
            {"Literal", 26},
            {"Block", 24},
            {"Text", 25},
            {"Block", 24},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Function", 3},
            {"Data", 4},
            {"Function", 4},
            {"Data", 5},
            {"Block", 2},
            {"Text", 3},
            {"Block", 2},
            {"Text", 1}        },
        {
            // template "TableSchemaId.tpl"
            {"Block", 0},
            {"SetStatement", 1},
            {"ExpressionWrapper", 2},
            {"Literal", 3},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ApplyTemplateStatement", 1},
            {"Text", 1}
        },
        {
            // template "TableSchema.tpl"
            {"Block", 0},
            {"SetStatement", 1},
            {"ExpressionWrapper", 2},
            {"Literal", 3},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ApplyTemplateStatement", 1},
            {"Text", 1}
        }
    };
    auto index = 0;
    for (auto& [name, tpl] : templates) {
        TestVisitor tvisitor;
        tvisitor.process(tpl);
        /*for (auto& info : tvisitor.nodes) {
            std::cout << "{\"" << info.name << "\", " << info.indent << "}," << std::endl;
        }
        std::cout << std::endl;*/
        CHECK(test_nodes[index++] == tvisitor.nodes);
    }
}

TEST_CASE("Parser DatabaseSchema.tpl (with comments)") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    lconfig.templates_dir = fixture.templatesDir;
    pconfig.keep_comments = true; // add comments in result AST
    Parser parser(pconfig, lconfig, templates, functions);
    std::filesystem::path template_name = "sql\\DatabaseSchema.tpl";
    Template root = parser.parse_file(template_name);
    TestVisitor rootVisitor;
    rootVisitor.process(root);
    // template "DatabaseSchema.tpl"
    std::vector<TestVisitor::NodeInfo> test_root_nodes{
        {"Block", 0},
        {"Comment", 1},
        {"FileStatement", 1},
        {"ExpressionWrapper", 2},
        {"Literal", 3},
        {"Block", 2},
        {"Text", 3},
        {"ExpressionWrapper", 3},
        {"Data", 4},
        {"Text", 3},
        {"ExpressionWrapper", 3},
        {"Data", 4},
        {"Text", 3},
        {"ApplyTemplateStatement", 3},
        {"ApplyTemplateStatement", 3},
    };
    CHECK(test_root_nodes == rootVisitor.nodes);
    CHECK(templates.size() == 3);
    std::vector<std::vector<TestVisitor::NodeInfo>> test_nodes{
        {
            // template "FieldSchema.tpl"
            {"Block", 0},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"Comment", 1},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Function", 3},
            {"Data", 4},
            {"Literal", 4},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Function", 4},
            {"Data", 5},
            {"Literal", 5},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 3},
            {"ExpressionWrapper", 4},
            {"Function", 5},
            {"Data", 6},
            {"Literal", 6},
            {"Block", 4},
            {"Text", 5},
            {"Block", 4},
            {"IfStatement", 5},
            {"ExpressionWrapper", 6},
            {"Function", 7},
            {"Data", 8},
            {"Literal", 8},
            {"Block", 6},
            {"Text", 7},
            {"Block", 6},
            {"IfStatement", 7},
            {"ExpressionWrapper", 8},
            {"Function", 9},
            {"Data", 10},
            {"Literal", 10},
            {"Block", 8},
            {"Text", 9},
            {"Block", 8},
            {"IfStatement", 9},
            {"ExpressionWrapper", 10},
            {"Function", 11},
            {"Data", 12},
            {"Literal", 12},
            {"Block", 10},
            {"Text", 11},
            {"Block", 10},
            {"IfStatement", 11},
            {"ExpressionWrapper", 12},
            {"Function", 13},
            {"Data", 14},
            {"Literal", 14},
            {"Block", 12},
            {"Text", 13},
            {"Block", 12},
            {"IfStatement", 13},
            {"ExpressionWrapper", 14},
            {"Function", 15},
            {"Data", 16},
            {"Literal", 16},
            {"Block", 14},
            {"Text", 15},
            {"ExpressionWrapper", 15},
            {"Data", 16},
            {"Text", 15},
            {"Block", 14},
            {"IfStatement", 15},
            {"ExpressionWrapper", 16},
            {"Function", 17},
            {"Data", 18},
            {"Literal", 18},
            {"Block", 16},
            {"Text", 17},
            {"Block", 16},
            {"IfStatement", 17},
            {"ExpressionWrapper", 18},
            {"Function", 19},
            {"Data", 20},
            {"Literal", 20},
            {"Block", 18},
            {"Text", 19},
            {"Block", 18},
            {"IfStatement", 19},
            {"ExpressionWrapper", 20},
            {"Function", 21},
            {"Data", 22},
            {"Literal", 22},
            {"Block", 20},
            {"Text", 21},
            {"Block", 20},
            {"IfStatement", 21},
            {"ExpressionWrapper", 22},
            {"Function", 23},
            {"Data", 24},
            {"Literal", 24},
            {"Block", 22},
            {"Text", 23},
            {"Block", 22},
            {"IfStatement", 23},
            {"ExpressionWrapper", 24},
            {"Function", 25},
            {"Data", 26},
            {"Literal", 26},
            {"Block", 24},
            {"Text", 25},
            {"Block", 24},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Text", 3},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Data", 3},
            {"Block", 2},
            {"Text", 3},
            {"ExpressionWrapper", 3},
            {"Data", 4},
            {"Block", 2},
            {"IfStatement", 1},
            {"ExpressionWrapper", 2},
            {"Function", 3},
            {"Data", 4},
            {"Function", 4},
            {"Data", 5},
            {"Block", 2},
            {"Text", 3},
            {"Block", 2},
            {"Text", 1}        },
        {
            // template "TableSchemaId.tpl"
            {"Block", 0},
            {"SetStatement", 1},
            {"ExpressionWrapper", 2},
            {"Literal", 3},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ApplyTemplateStatement", 1},
            {"Text", 1}
        },
        {
            // template "TableSchema.tpl"
            {"Block", 0},
            {"SetStatement", 1},
            {"ExpressionWrapper", 2},
            {"Literal", 3},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ExpressionWrapper", 1},
            {"Data", 2},
            {"Text", 1},
            {"ApplyTemplateStatement", 1},
            {"Text", 1}
        }
    };
    auto index = 0;
    for (auto& [name, tpl] : templates) {
        TestVisitor tvisitor;
        tvisitor.process(tpl);
        /*for (auto& info : tvisitor.nodes) {
            std::cout << "{\"" << info.name << "\", " << info.indent << "}," << std::endl;
        }
        std::cout << std::endl;*/
        CHECK(test_nodes[index++] == tvisitor.nodes);
    }
}


TEST_CASE("Parser expresssion") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    Parser parser(pconfig, lconfig, templates, functions);
    std::string expression = R"(exists("tables.id") and tables.id)";
    Template root = parser.parse_expression(expression);
    TestVisitor tvisitor;
    tvisitor.process(root);
    // expression
    std::vector<TestVisitor::NodeInfo> test_nodes{
        {"Block", 0},
        {"ExpressionWrapper", 1},
        {"Function", 2},
        {"Function", 3},
        {"Literal", 4},
        {"Data", 3}
    };
    CHECK(test_nodes == tvisitor.nodes);
}

/*
TEST_CASE("Parser empty") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    auto parser = make_parser<true>(lconfig, pconfig);
    auto stm = parser(State::Text, "");
    CHECK(stm.state() == State::Eof);

    const std::vector<states_history<State, Token, true>::state_event> test_events{
        {State::Text, Token{Token::Kind::Eof, {}}, State::Eof}
    };
    const auto& events = stm.get_history();
    CHECK(events.size() == 1);
    CHECK(test_events == events);
}


TEST_CASE("Parser FieldSchema.tpl") {
    std::string str = read_file_relative("..\\test\\data\\FieldSchema.tpl");
    LexerConfig lconfig;
    ParserConfig pconfig;
    auto parser = make_parser<true>(lconfig, pconfig);
    auto stm = parser(State::Text, str);
    CHECK(stm.state() == State::Eof);
    const std::vector<states_history<State, Token, true>::state_event> test_events{
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "template"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "FieldSchema"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " `"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "`\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"string\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "varchar("}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "length"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, ")\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"text\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "text\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"integer\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "int(11) unsigned\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"object\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "int(11) unsigned\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"money\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "decimal(10,2)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"discount\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "decimal(10,3)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"enum\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "enum("}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "enumeration"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, ")\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"boolean\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "tinyint(4)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"date\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "date\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"time\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "time\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"datetime\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "datetime\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "else"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "type"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Equal, "=="}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::String, "\"timestamp\""}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "timestamp\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "required"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " NOT NULL\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "default"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " DEFAULT '"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "default"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "'\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "primary"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " AUTO_INCREMENT,\n PRIMARY KEY (`"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "`)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "unique"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " UNIQUE KEY `unique_"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "` (`"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "`)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "index"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " KEY `idx_"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "` (`"}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "name"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "`)\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "if"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "comment"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Text, " -- "}, State::Text},
        {State::Text, Token{Token::Kind::ExpressionOpen, "{{"}, State::Expression},
        {State::Expression, Token{Token::Kind::Id, "comment"}, State::Expression},
        {State::Expression, Token{Token::Kind::ExpressionClose, "}}"}, State::Text},
        {State::Text, Token{Token::Kind::Text, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endif"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::LineStatementOpen, "##"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::Id, "endtemplate"}, State::LineStatement},
        {State::LineStatement, Token{Token::Kind::LineStatementClose, "\n"}, State::Text},
        {State::Text, Token{Token::Kind::Eof, ""}, State::Eof}        
    };
    const auto& events = stm.get_history();
    CHECK(events.size() == 195);
    CHECK(test_events == events);
    // for (auto&& state : stm.get_history()) {
    //     std::cout << "{" << enum_states[(int)state.state] << ", Token{"
    //               << enum_token_kind[(int)state.symbol.kind] << ", " 
    //               << std::quoted(state.symbol.text) << "}, " << enum_states[(int)state.newstate] << "}," << std::endl;
    // }

}
*/