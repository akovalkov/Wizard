#pragma once
#include <filesystem>
#include <fstream>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
namespace json = boost::json;

struct GlobalFixture
{
    std::filesystem::path templatesDir; 
    std::filesystem::path dataDir; 
    GlobalFixture() {
    }
    void setDirectories(std::filesystem::path path = std::filesystem::current_path()) {
        templatesDir = path;  
        templatesDir /= "templates";
        dataDir = path;
        dataDir /= "data";
    }

    json::value parse(const std::filesystem::path& path)
    {
        std::error_code ec;
        std::ifstream infile;
        infile.open(path);
        if(infile.fail()) {
            throw std::runtime_error("Couldn't open file: \"" + path.string() + "\"");
        }
        auto data = json::parse(infile, ec);
        if(ec) {
            throw std::runtime_error(ec.message());
        }
        return data;
    }

};

static const char* enum_token_kind[] = {
        "Token::Kind::Text",
        "Token::Kind::ExpressionOpen",     // {{
        "Token::Kind::ExpressionClose",    // }}
        "Token::Kind::LineStatementOpen",  // ##
        "Token::Kind::LineStatementClose", // \n
        "Token::Kind::StatementOpen",      // {%
        "Token::Kind::StatementClose",     // %}
        "Token::Kind::CommentOpen",        // {#
        "Token::Kind::CommentClose",       // #}
        "Token::Kind::Id",                 // this, this.foo
        "Token::Kind::Number",             // 1, 2, -1, 5.2, -5.3
        "Token::Kind::String",             // "this"
        "Token::Kind::Plus",               // +
        "Token::Kind::Minus",              // -
        "Token::Kind::Times",              // *
        "Token::Kind::Slash",              // /
        "Token::Kind::Percent",            // %
        "Token::Kind::Power",              // ^
        "Token::Kind::Comma",              // ,
        "Token::Kind::Dot",                // .
        "Token::Kind::Colon",              // :
        "Token::Kind::LeftParen",          // (
        "Token::Kind::RightParen",         // )
        "Token::Kind::LeftBracket",        // [
        "Token::Kind::RightBracket",       // ]
        "Token::Kind::LeftBrace",          // {
        "Token::Kind::RightBrace",         // }
        "Token::Kind::Equal",               // ==
        "Token::Kind::NotEqual",           // !=
        "Token::Kind::GreaterThan",        // >
        "Token::Kind::GreaterEqual",       // >=
        "Token::Kind::LessThan",           // <
        "Token::Kind::LessEqual",          // <=
        "Token::Kind::Unknown",
        "Token::Kind::Eof"
};


static const char* enum_states[] = {
        "State::Text", 
        "State::Statement", 
        "State::LineStatement", 
        "State::Expression", 
        "State::Comment", 
        "State::Eof"    
};