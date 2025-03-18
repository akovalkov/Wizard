#pragma once
#include <string>
#include <filesystem>
#include "Config.h"
#include "Template.h"
#include "Parser.h"
#include "Renderer.h"
#include "DescVisitor.h"


namespace Wizard {

class Environment {
  LexerConfig lexer_config;
  ParserConfig parser_config;
  RenderConfig render_config;

  FunctionStorage function_storage;
  TemplateStorage template_storage;
public:

    // parse template (default configs)
    Template parse_file(const std::filesystem::path& path,
                        const std::filesystem::path& fileinfo = "")
    {
        // parse template
        Parser parser(parser_config, lexer_config, template_storage, function_storage);
        Template tpl = parser.parse_file(path);
        if(!fileinfo.empty()) {
            // parse template description
            auto name = path.stem().string();
            tpl.desc = Description::load_from_json(name, fileinfo);
        }
        return tpl;
    }

    Template parse(const std::string_view input)
    {
        Parser parser(parser_config, lexer_config, template_storage, function_storage);
        return parser.parse(input);
    }

    // parse template (custom configs)
    Template parse_file(const std::filesystem::path& path, 
                        const LexerConfig& lconfig, const ParserConfig& pconfig,
                        const std::filesystem::path& fileinfo = "")
    {
        // parse template
        Parser parser(pconfig, lconfig, template_storage, function_storage);
        Template tpl = parser.parse_file(path);
        if(!fileinfo.empty()) {
            // parse template description
            auto name = path.stem().string();
            tpl.desc = Description::load_from_json(name, fileinfo);
        }
        return tpl;
    }

    Template parse(const std::string_view input,
                  const LexerConfig& lconfig, const ParserConfig& pconfig)
    {
        Parser parser(pconfig, lconfig, template_storage, function_storage);
        return parser.parse(input);
    }

    // render template
    std::string render(const Template& tmpl, const json::value& data) {
    	Renderer renderer(render_config, template_storage, function_storage);
        std::stringstream os;
        renderer.render(os, tmpl, data);
        return os.str();
    }

    std::string render_file(const std::filesystem::path& filename, 
                            const json::value& data, 
                            const std::filesystem::path& infofile = "") {

        return render(parse_file(filename, infofile), data);
    }

    std::string render(const std::string& text, 
                       const json::value& data) {
        return render(parse(text), data);
    }

    // evaluate expression
    json::value evaluate_expression(const Template& tmpl, const json::value& data) {
    	Renderer renderer(render_config, template_storage, function_storage);
        return renderer.evaluate_expression(tmpl, data);
    }

    Template parse_expression(const std::string_view input)
    {
        Parser parser(parser_config, lexer_config, template_storage, function_storage);
        return parser.parse_expression(input);
    }

    json::value evaluate(const std::string_view expr, const json::value& data) {
        return evaluate_expression(parse_expression(expr), data);
    }

    // information about template
    Description description_from_file(const std::filesystem::path& filename) {
        ParserConfig pconfig = parser_config;
        pconfig.parse_nested_template = false; // don't parse child templates
        pconfig.keep_comments = true; // first comment is info about template
        return description(parse_file(filename, lexer_config, pconfig));
    }

    Description description(const std::string& text) {
        ParserConfig pconfig = parser_config;
        pconfig.parse_nested_template = false; // don't parse child templates
        pconfig.keep_comments = true; // first comment is info about template
        return description(parse(text, lexer_config, pconfig));
    }

    Description description(const Template& tmpl) {
    	DescriptionVisitor visitor(render_config);
        visitor.populate(tmpl);
        return visitor.description;
    }


    const TemplateStorage& get_templates() const { return template_storage; }
    const FunctionStorage& get_functions() const { return function_storage; }
    
    /// Sets the opener and closer for template statements
    void set_statement(const std::string& open, const std::string& close) {
        lexer_config.statement_open = open;
        lexer_config.statement_open_force_lstrip = open + "-";
        lexer_config.statement_close = close;
        lexer_config.statement_close_force_rstrip = "-" + close;
    }

    /// Sets the opener for template line statements
    void set_line_statement(const std::string& open) {
        lexer_config.line_statement = open;
    }

    /// Sets the opener and closer for template expressions
    void set_expression(const std::string& open, const std::string& close) {
        lexer_config.expression_open = open;
        lexer_config.expression_open_force_lstrip = open + "-";
        lexer_config.expression_close = close;
        lexer_config.expression_close_force_rstrip = "-" + close;
    }

    /// Sets the opener and closer for template comments
    void set_comment(const std::string& open, const std::string& close) {
        lexer_config.comment_open = open;
        lexer_config.comment_open_force_lstrip = open + "-";
        lexer_config.comment_close = close;
        lexer_config.comment_close_force_rstrip = "-" + close;
    }

    /// Sets templates directory
    void set_template_directory(const std::filesystem::path& dir) {
        lexer_config.templates_dir = dir;
    }

    // Set dry run (all output in returned string)
    void set_dry_run(bool dry) {
        render_config.dry_run = dry;
    }

    // Is dry run (all output in returned string)
    bool is_dry_run() {
        return render_config.dry_run;
    }

    // set output directory
    void set_output_dir(const std::filesystem::path& output) {
        render_config.output_dir = output;
    }

    /*!
    @brief Adds a variadic callback
    */
    void add_callback(const std::string& name, const CallbackFunction& callback) {
        add_callback(name, -1, callback);
    }

    /*!
    @brief Adds a variadic void callback
    */
    void add_void_callback(const std::string& name, const VoidCallbackFunction& callback) {
        add_void_callback(name, -1, callback);
    }

    /*!
    @brief Adds a callback with given number or arguments
    */
    void add_callback(const std::string& name, int num_args, const CallbackFunction& callback) {
        function_storage.add_callback(name, num_args, callback);
    }

    /*!
    @brief Adds a void callback with given number or arguments
    */
    void add_void_callback(const std::string& name, int num_args, const VoidCallbackFunction& callback) {
        function_storage.add_callback(name, num_args, [callback](Arguments& args) {
            callback(args);
            return json::value();
        });
    }

};

}