#pragma once
#include <string>
#include <functional>
#include <filesystem>

namespace Wizard {
    
    struct Template;

    struct LexerConfig {
        std::string statement_open {"{%"};
        std::string statement_open_force_lstrip {"{%-"};
        std::string statement_close {"%}"};
        std::string statement_close_force_rstrip {"-%}"};
        std::string line_statement {"##"};
        std::string expression_open {"{{"};
        std::string expression_open_force_lstrip {"{{-"};
        std::string expression_close {"}}"};
        std::string expression_close_force_rstrip {"-}}"};
        std::string comment_open {"{#"};
        std::string comment_open_force_lstrip {"{#-"};
        std::string comment_close {"#}"};
        std::string comment_close_force_rstrip {"-#}"};

        std::filesystem::path templates_dir;
    };

    struct ParserConfig {
        bool parse_nested_template{true};
        bool keep_comments{false}; // add comments in AST

        std::function<Template(const std::filesystem::path&, const std::string&)> include_callback;
    };

    struct RenderConfig {
        std::filesystem::path output_dir;
        bool dry_run{false}; // only cout output
        bool strict{false}; // json variable must exists or not
        bool throw_at_missing_includes{true};

        std::string loop_variable_name{"loop"};
    };

}
