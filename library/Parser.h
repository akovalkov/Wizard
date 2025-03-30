#pragma once
#include <vector>
#include <stack>
#include <filesystem>
#include "Config.h"
#include "Lexer.h"
#include "FunctionStorage.h"
#include "Template.h"

namespace Wizard
{
    class Parser
    {
    protected:    
        using Arguments = std::vector<std::shared_ptr<ExpressionNode>>;
        using OperatorStack = std::stack<std::shared_ptr<FunctionNode>>;

        const ParserConfig &pconfig;
        const LexerConfig &lconfig;

        TemplateStorage &template_storage;
        const FunctionStorage &function_storage;
        //Scanner<Token> sequence;
        Lexer lexer;


        struct ParserState
        {
            //Scanner<Token>& sequence;
            Lexer& lexer;
            Lexer::LexerState lstate;

            Token tok{}, peek_tok{};
            bool have_peek_tok{false};

            BlockNode* current_block{nullptr};

            //ParserState(Scanner<Token>& sequence) : sequence(sequence) {}
            ParserState(Lexer& lexer, BlockNode* block = nullptr) : lexer(lexer), current_block(block) {}

            FileStatementNode* current_file_statement{nullptr};
            std::stack<IfStatementNode*> if_statement_stack;
            std::stack<ForStatementNode*> for_statement_stack;

            inline void get_next_token() {
                if (have_peek_tok) {
                    tok = peek_tok;
                    have_peek_tok = false;
                } else {
                    lstate = lexer.scan(lstate);
                    tok = lstate.token;
                    //tok = sequence.next();
                }
            }

            inline void get_peek_token() {
                if (!have_peek_tok) {
                    lstate = lexer.scan(lstate);
                    peek_tok = lstate.token;
                    //peek_tok = sequence.next();
                    have_peek_tok = true;
                }
            }

        };

        inline void throw_parser_error(const std::string &message, ParserState& state) const {
            //throw ParserError(message, get_source_location());
            throw ParserError(message, state.lexer.current_position(state.lstate));
        }

        inline void add_literal(ParserState& state, const Token& literal_start, Arguments &arguments)
        {
            // string, number or json 
            std::string_view data_text(literal_start.text.data(), state.tok.text.data() - literal_start.text.data() + state.tok.text.size());
            arguments.emplace_back(std::make_shared<LiteralNode>(data_text, literal_start.offset));
        }

        inline void add_operator(ParserState& state, Arguments& arguments, OperatorStack& operator_stack)
        {
            // get operator with the best precedence
            auto function = operator_stack.top();
            operator_stack.pop();

            if(static_cast<int>(arguments.size()) < function->number_args) {
                throw_parser_error("too few arguments", state);
            }
            // get arguments for operator
            for(int i = 0; i < function->number_args; ++i) {
                function->arguments.insert(function->arguments.begin(), arguments.back());
                arguments.pop_back();
            }
            // and push back operator with arguments (now is function call) on parameters stack 
            arguments.emplace_back(function);
        }

        std::shared_ptr<FunctionNode> create_function(ParserState& state, Template &tmpl) {
            // create function node
            auto func = std::make_shared<FunctionNode>(state.tok.text, state.tok.offset);
            // expected Token::Kind::LeftParen (already checked)
            state.get_next_token();
            do
            {
                // function argument as expression (function nodes also)
                state.get_next_token();
                auto expr = parse_expression(state, tmpl);
                if (!expr) {
                    break;
                }
                func->number_args += 1;
                func->arguments.emplace_back(expr);
            } while (state.tok.kind == Token::Kind::Comma); // comma is function arguments separtor

            // expects Token::Kind::RightParen, otherwise is error 
            if (state.tok.kind != Token::Kind::RightParen) {
                throw_parser_error("expected right parenthesis, got '" + state.tok.describe() + "'", state);
            }
            // should be known function (predefined or user)
            auto function_data = function_storage.find_function(func->name, func->number_args);
            if (function_data.operation == FunctionStorage::Operation::None) {
                throw_parser_error("unknown function " + func->name, state);
            }
            // copy user function implementation (std::function) in node
            func->operation = function_data.operation;
            if (function_data.operation == FunctionStorage::Operation::Callback) {
                func->callback = function_data.callback;
            }
            return func;
        }

        // sub expression (something between Token::Kind::LeftParen and Token::Kind::RightParen)
        std::shared_ptr<ExpressionNode> create_sub_expression(ParserState& state, Template &tmpl) {
            // expected Token::Kind::LeftParen (already checked)
            state.get_next_token();
            auto expr = parse_expression(state, tmpl);
            // expects Token::Kind::RightParen, otherwise is error 
            if(state.tok.kind != Token::Kind::RightParen) {
                throw_parser_error("expected right parenthesis, got '" + state.tok.describe() + "'", state);
            }
            // no empty experssion
            if(!expr) {
                throw_parser_error("empty expression in parentheses", state);
            }
            return expr;
        }


        FunctionStorage::Operation get_operator_type(ParserState& state)
        {
            switch (state.tok.kind)
            {
            case Token::Kind::Id:
                {
                    if(state.tok.text == "and") {
                        return FunctionStorage::Operation::And;
                    } else if(state.tok.text == "or") {
                        return FunctionStorage::Operation::Or;
                    } else if(state.tok.text == "in") {
                        return FunctionStorage::Operation::In;
                    } else if(state.tok.text == "not") {
                        return FunctionStorage::Operation::Not;
                    }
                }
                break;
            case Token::Kind::Equal:
                return FunctionStorage::Operation::Equal;
            case Token::Kind::NotEqual:
                return FunctionStorage::Operation::NotEqual;
            case Token::Kind::GreaterThan:
                return FunctionStorage::Operation::Greater;
            case Token::Kind::GreaterEqual:
                return FunctionStorage::Operation::GreaterEqual;
            case Token::Kind::LessThan:
                return FunctionStorage::Operation::Less;
            case Token::Kind::LessEqual:
                return FunctionStorage::Operation::LessEqual;
            case Token::Kind::Plus:
                return FunctionStorage::Operation::Add;
            case Token::Kind::Minus:
                return FunctionStorage::Operation::Subtract;
            case Token::Kind::Times:
                return FunctionStorage::Operation::Multiplication;
            case Token::Kind::Slash:
                return FunctionStorage::Operation::Division;
            case Token::Kind::Power:
                return FunctionStorage::Operation::Power;
            case Token::Kind::Percent:
                return FunctionStorage::Operation::Modulo;
            // case Token::Kind::Dot:
            //     return FunctionStorage::Operation::AtId;
            default:
                break;
            }
            throw_parser_error("unknown operator in parser.", state);
            return  FunctionStorage::Operation::None;
        }

        std::shared_ptr<FunctionNode> create_operator(ParserState& state, Arguments& arguments, OperatorStack& operator_stack) {
            FunctionStorage::Operation operation = get_operator_type(state);
            auto operator_node = std::make_shared<FunctionNode>(operation, state.tok.offset);

            // check precedence operators
            // if current operator has low precedence then all operators with higher precedence are copied in argumentsof current function
            while (!operator_stack.empty() &&
                ((operator_stack.top()->precedence > operator_node->precedence) ||
                    (operator_stack.top()->precedence == operator_node->precedence && operator_node->associativity == FunctionStorage::Associativity::Left))) {
                add_operator(state, arguments, operator_stack);
            }
            return operator_node;
        }


        void add_to_template_storage(std::string template_name)
        {
            if (template_storage.find(template_name) != template_storage.end()) {
                return;
            }
            std::filesystem::path template_path = template_name + ".tpl";
            if (pconfig.parse_nested_template) {
                // Parse sub template
                auto sub_parser = Parser(pconfig, lconfig, template_storage, function_storage);
                template_storage.emplace(template_name, sub_parser.parse_file(template_path));
                return;
            }
            // Try include callback
            if(pconfig.include_callback) {
                auto include_template = pconfig.include_callback(template_path, template_name);
                template_storage.emplace(template_name, include_template);
            }
        }

        bool parse_expression(ParserState& state, Template &tmpl, Token::Kind closing,
                              ExpressionWrapperNode& current_expression)
        {
            current_expression.root = parse_expression(state, tmpl);
            return state.tok.kind == closing;
        }

        std::shared_ptr<ExpressionNode> parse_expression(ParserState& state, Template &tmpl)
        {
            size_t current_bracket_level{0};
            size_t current_brace_level{0};
            OperatorStack operator_stack; // operators stack (sorted by precedence and associativity)
            Arguments arguments; // arguments for operators
            Token literal_start; // start literal (can be json)

            while (state.tok.kind != Token::Kind::Eof)
            {
                // Literals
                switch (state.tok.kind)
                {
                    case Token::Kind::String:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            literal_start = state.tok;
                            add_literal(state, literal_start, arguments);
                        }
                    }
                    break;
                case Token::Kind::Number:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            literal_start = state.tok;
                            add_literal(state, literal_start, arguments);
                        }
                    }
                    break;
                case Token::Kind::LeftBracket:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            literal_start = state.tok;
                        }
                        current_bracket_level += 1;
                    }
                    break;
                case Token::Kind::LeftBrace:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            literal_start = state.tok;
                        }
                        current_brace_level += 1;
                    }
                    break;
                case Token::Kind::RightBracket:
                    {
                        if(current_bracket_level == 0) {
                            throw_parser_error("unexpected ']'", state);
                        }

                        current_bracket_level -= 1;
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            add_literal(state, literal_start, arguments);
                        }
                    }
                    break;
                case Token::Kind::RightBrace:
                    {
                        if(current_brace_level == 0) {
                            throw_parser_error("unexpected '}'", state);
                        }

                        current_brace_level -= 1;
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            add_literal(state, literal_start, arguments);
                        }
                    }
                    break;
                case Token::Kind::Id:
                    {
                        state.get_peek_token();
                        // Data Literal
                        if(state.tok.text == "true" || state.tok.text == "false" || state.tok.text == "null"){
                            if(current_brace_level == 0 && current_bracket_level == 0) {
                                literal_start = state.tok;
                                add_literal(state, literal_start, arguments);
                            }
                        // Operator
                        } else if(state.tok.text == "and" || state.tok.text == "or" || state.tok.text == "in" || state.tok.text == "not") {
                            // logical operators
                            goto parse_operator;
                        // Functions
                        } else if(state.peek_tok.kind == Token::Kind::LeftParen) {
                            auto func = create_function(state, tmpl);
                            arguments.emplace_back(func);
                        // Variables
                        } else {
                            arguments.emplace_back(std::make_shared<DataNode>(state.tok.text, state.tok.offset));
                        }

                    }
                    break;
                // Operators
                case Token::Kind::Equal:
                case Token::Kind::NotEqual:
                case Token::Kind::GreaterThan:
                case Token::Kind::GreaterEqual:
                case Token::Kind::LessThan:
                case Token::Kind::LessEqual:
                case Token::Kind::Plus:
                case Token::Kind::Minus:
                case Token::Kind::Times:
                case Token::Kind::Slash:
                case Token::Kind::Power:
                case Token::Kind::Percent:
                //case Token::Kind::Dot:
                    {
                    parse_operator:
                        auto operator_node = create_operator(state, arguments, operator_stack);
                        operator_stack.emplace(operator_node);
                    }
                    break;
                case Token::Kind::Comma:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            goto break_loop;
                        }
                    }
                    break;
                case Token::Kind::Colon:
                    {
                        if(current_brace_level == 0 && current_bracket_level == 0) {
                            throw_parser_error("unexpected ':'", state);
                        }
                    }
                    break;
                case Token::Kind::LeftParen:
                    {
                        // sub expression (something between Token::Kind::LeftParen and Token::Kind::RightParen)
                        auto expr = create_sub_expression(state, tmpl);
                        arguments.emplace_back(expr);
                    }
                    break;
                default:
                    goto break_loop;
                }
                state.get_next_token();
            }

        break_loop:
            // handle opertor stack
            while (!operator_stack.empty()) {
                add_operator(state, arguments, operator_stack);
            }
            // result must be alone expression (arguments.size() == 1), otherwise is error
            std::shared_ptr<ExpressionNode> expr;
            if (arguments.size() == 1) {
                expr = arguments[0];
            } else if (arguments.size() > 1) {
                throw_parser_error("malformed expression", state);
            }
            return expr;
        }

        
        bool parse_if_statement(bool is_nested, ParserState& state, Template &tmpl, Token::Kind closing) {
            // skip current token (keyword "if")
            state.get_next_token();
            // create "if" node
            auto if_statement_node = std::make_shared<IfStatementNode>(is_nested, state.current_block, state.tok.offset);
            // if nodes stack 
            state.if_statement_stack.emplace(if_statement_node.get()); // raw pointers
            // upate current block (true_statement)
            state.current_block->nodes.emplace_back(if_statement_node);
            state.current_block = &if_statement_node->true_statement;
            // parse if condition
            return parse_expression(state, tmpl, closing, if_statement_node->condition);
        }

        bool parse_else_statement(ParserState& state, Template &tmpl, Token::Kind closing) {
            // if nodes shouldn't be empty for "else"
            if (state.if_statement_stack.empty()) {
                throw_parser_error("else without matching if", state);
            }
            auto &if_statement_data = state.if_statement_stack.top();
            // skip current token (keyword "else")
            state.get_next_token();
            // "else" means if has "false" part
            if_statement_data->has_false_statement = true;
            state.current_block = &if_statement_data->false_statement;
            // chained else if
            if (state.tok.kind == Token::Kind::Id && state.tok.text == "if")  {
                return parse_if_statement(true, state, tmpl, closing);
            }
            return true;
        }

        bool parse_endif_statement(ParserState& state) {
            // "if" nodes stack shouldn't be empty for "endif"
            if (state.if_statement_stack.empty()) {
                throw_parser_error("endif without matching if", state);
            }
            // skip current token (keyword "endif")
            state.get_next_token();
            // Nested if statements
            while(state.if_statement_stack.top()->is_nested) {
                  state.if_statement_stack.pop();
            }
            // upate current block (parent the "if" statement)
            auto &if_statement_data = state.if_statement_stack.top();
            state.current_block = if_statement_data->parent;
            state.if_statement_stack.pop();
            return true;
        }

        bool parse_for_statement(ParserState& state, Template &tmpl, Token::Kind closing) {
            // skip current token (keyword "for")
            state.get_next_token();

            // options: for a in arr; for a, b in obj
            if (state.tok.kind != Token::Kind::Id) {
                throw_parser_error("expected id, got '" + state.tok.describe() + "'", state);
            }
            // for variable(s) (value or key, value)
            Token value_token = state.tok;
            // skip current token (the for variable)
            state.get_next_token();

            // for statement
            std::shared_ptr<ForStatementNode> for_statement_node;

            // if comma next then the for has two variables (key and value)
            if (state.tok.kind == Token::Kind::Comma) {
                // skip comma
                state.get_next_token();
                // current token should be variable (Token::Kind::Id)
                if (state.tok.kind != Token::Kind::Id) {
                    throw_parser_error("expected id, got '" + state.tok.describe() + "'", state);
                }

                Token key_token = std::move(value_token);
                value_token = state.tok;
                // skip second variable 
                state.get_next_token();
                // Object type
                for_statement_node = std::make_shared<ForObjectStatementNode>(static_cast<std::string>(key_token.text), 
                                                                              static_cast<std::string>(value_token.text),
                                                                              state.current_block, state.tok.offset);
            } else {
                // Array type
                for_statement_node =
                    std::make_shared<ForArrayStatementNode>(static_cast<std::string>(value_token.text), 
                                                            state.current_block, state.tok.offset);
            }

            // update current block
            state.current_block->nodes.emplace_back(for_statement_node);
            state.current_block = &for_statement_node->body;
            // for nodes stack 
            state.for_statement_stack.emplace(for_statement_node.get()); // raw pointer

            // next token should be the "in" keyword
            if (state.tok.kind != Token::Kind::Id || state.tok.text != "in") {
                throw_parser_error("expected 'in', got '" + state.tok.describe() + "'", state);
            }
            // skip the "in" keyword
            state.get_next_token();
            // parse array or object in the for
            return parse_expression(state, tmpl, closing, for_statement_node->condition);
        }

        bool parse_endfor_statement(ParserState& state) {
            // "for" nodes stack shouldn't be empty for "endif"
            if (state.for_statement_stack.empty()) {
                throw_parser_error("endfor without matching for", state);
            }
            // skip current token (keyword "endfor")
            state.get_next_token();

            // upate current block (parent the "for" statement)
            auto &for_statement_data = state.for_statement_stack.top();
            state.current_block = for_statement_data->parent;
            state.for_statement_stack.pop();
            return true;
        }

        bool parse_file_statement(ParserState& state, Template &tmpl, Token::Kind closing) {
            if(state.current_file_statement) {
                throw_parser_error("file statements cannot be nested", state);
            }
            // skip current token (keyword "file")
            state.get_next_token();
            // create "file" node
            auto file_statement_node = std::make_shared<FileStatementNode>(state.current_block, state.tok.offset);
            // if nodes stack 
            state.current_file_statement = file_statement_node.get();
            // upate current block (body)
            state.current_block->nodes.emplace_back(file_statement_node);
            state.current_block = &file_statement_node->body;
            return parse_expression(state, tmpl, closing, file_statement_node->filename);
        }

        bool parse_endfile_statement(ParserState& state) {
            // "file" nodes stack shouldn't be empty for "endfile"
            if (!state.current_file_statement) {
                throw_parser_error("endfile without matching file", state);
            }
            // skip current token (keyword "endfile")
            state.get_next_token();
            // upate current block (parent the "file" statement)
            state.current_block = state.current_file_statement->parent;
            state.current_file_statement = nullptr;
            return true;
        }

        bool parse_apply_template_statement(ParserState& state) {
            // skip current token (keyword "apply-template")
            state.get_next_token();
            // template name
            if (state.tok.kind != Token::Kind::Id) {
                throw_parser_error("expected template name, got '" + state.tok.describe() + "'", state);
            }
            auto name = state.tok.text;
            // field name
            state.get_next_token();
            if (state.tok.kind != Token::Kind::Id) {
                throw_parser_error("expected json field name, got '" + state.tok.describe() + "'", state);
            }
            auto field = state.tok.text;

            // create apply-template 
            state.current_block->nodes.emplace_back(std::make_shared<ApplyTemplateStatementNode>(static_cast<std::string>(name), 
                                                                                                 static_cast<std::string>(field),
                                                                                                 state.tok.offset));
            state.get_next_token();
            // add template (parse it)
            add_to_template_storage(static_cast<std::string>(name));
            return true;
        }

        bool parse_set_statement(ParserState& state, Template &tmpl, Token::Kind closing) {
            // skip current token (keyword "set")
            state.get_next_token();
            // next token should be variable name
            if (state.tok.kind != Token::Kind::Id) {
                throw_parser_error("expected variable name, got '" + state.tok.describe() + "'", state);
            }
            std::string key = static_cast<std::string>(state.tok.text);
            // create "set" statement 
            auto set_statement_node = std::make_shared<SetStatementNode>(key, state.tok.offset);
            state.current_block->nodes.emplace_back(set_statement_node);

            // next token should be "="
            state.get_next_token();
            if (state.tok.text != "=") {
                throw_parser_error("expected '=', got '" + state.tok.describe() + "'", state);
            }
            // and finaly variable value (as expression)
            state.get_next_token();
            return parse_expression(state, tmpl, closing, set_statement_node->expression);
        }

        bool parse_statement(ParserState& state, Template &tmpl, Token::Kind closing)
        {
            if (state.tok.kind != Token::Kind::Id) {
                return false;
            }

            if (state.tok.text == "if") {
                return parse_if_statement(false, state, tmpl, closing);
            } else if (state.tok.text == "else") {
                return parse_else_statement(state, tmpl, closing);
            } else if (state.tok.text == "endif") {
                return parse_endif_statement(state);
            } else if (state.tok.text == "for") {
                return parse_for_statement(state, tmpl, closing);
            } else if (state.tok.text == "endfor") {
                return parse_endfor_statement(state);
            } else if (state.tok.text == "file") {
                return parse_file_statement(state, tmpl, closing);
            } else if (state.tok.text == "endfile") {
                return parse_endfile_statement(state);
            } else if (state.tok.text == "apply-template") {
                return parse_apply_template_statement(state);
            } else if (state.tok.text == "set") {
                return parse_set_statement(state, tmpl, closing);
            }
           /* else if (state.tok.text == "template")
            {

                state.get_next_token();

                if (state.tok.kind != Token::Kind::Id)
                {
                    throw_parser_error("expected template name, got '" + tok.describe() + "'", state);
                }

                const std::string template_name = static_cast<std::string>(state.tok.text);

                auto template_statement_node = std::make_shared<TemplateStatementNode>(state.current_block, template_name, tok.offset);
                state.current_block->nodes.emplace_back(template_statement_node);
                template_statement_stack.emplace(template_statement_node.get());
                state.current_block = &template_statement_node->block;
                auto success = tmpl.block_storage.emplace(template_name, template_statement_node);
                if (!success.second)
                {
                    throw_parser_error("block with the name '" + template_name + "' does already exist", state);
                }

                state.get_next_token();
            }
            else if (state.tok.text == "endtemplate")
            {
                if (template_statement_stack.empty())
                {
                    throw_parser_error("endblock without matching block", state);
                }

                auto &template_statement_data = template_statement_stack.top();
                state.get_next_token();

                state.current_block = template_statement_data->parent;
                template_statement_stack.pop();
            }
            */
            return false;
        }

        void parse_into(Template& tmpl)
        {
            // auto scanner = make_scanner(lconfig);
            // sequence = scanner(tmpl.content);
            //ParserState state{sequence};
            
            ParserState state{lexer, &tmpl.root};
            state.lstate = lexer.start(tmpl.content);

            for (;;)
            {
                state.get_next_token();
                switch (state.tok.kind)
                {
                case Token::Kind::Eof:
                    {
                        if (!state.if_statement_stack.empty()) {
                            throw_parser_error("unmatched if", state);
                        }
                        if (!state.for_statement_stack.empty()) {
                            throw_parser_error("unmatched for", state);
                        }
                    }
                    return;
                case Token::Kind::Text:
                    {
                        state.current_block->nodes.emplace_back(std::make_shared<TextNode>(state.tok.offset, state.tok.text.size()));
                    }
                    break;
                case Token::Kind::StatementOpen: // {%
                    {
                        state.get_next_token();
                        if (!parse_statement(state, tmpl, Token::Kind::StatementClose)) {
                            throw_parser_error("expected statement, got '" + state.tok.describe() + "'", state);
                        }
                        if (state.tok.kind != Token::Kind::StatementClose) {
                            throw_parser_error("expected statement close, got '" + state.tok.describe() + "'", state);
                        }
                    }
                    break;
                case Token::Kind::LineStatementOpen: // ##
                    {
                        state.get_next_token();
                        if (!parse_statement(state, tmpl, Token::Kind::LineStatementClose)) {
                            throw_parser_error("expected statement, got '" + state.tok.describe() + "'", state);
                        }
                        if (state.tok.kind != Token::Kind::LineStatementClose && state.tok.kind != Token::Kind::Eof) {
                            throw_parser_error("expected line statement close, got '" + state.tok.describe() + "'", state);
                        }
                    }
                    break;
                case Token::Kind::ExpressionOpen: // {{
                    {
                        state.get_next_token();

                        auto expression_list_node = std::make_shared<ExpressionWrapperNode>(state.tok.offset);
                        state.current_block->nodes.emplace_back(expression_list_node);

                        if (!parse_expression(state, tmpl, Token::Kind::ExpressionClose, *expression_list_node.get())) {
                            throw_parser_error("expected expression close, got '" + state.tok.describe() + "'", state);
                        }
                    }
                    break;
                case Token::Kind::CommentOpen: // {#
                    {
                        state.get_next_token();
                        if (state.tok.kind != Token::Kind::CommentClose) {
                            throw_parser_error("expected comment close, got '" + state.tok.describe() + "'", state);
                        }
                        if(pconfig.keep_comments) {
                            auto comment_node = std::make_shared<CommentNode>(state.tok.offset, state.tok.text.size());
                            state.current_block->nodes.emplace_back(comment_node);
                        }
                    }
                    break;
                default:
                    {
                        throw_parser_error("unexpected token '" + state.tok.describe() + "'", state);
                    }
                    break;
                }
            }
        }

    public:
        explicit Parser(const ParserConfig &parser_config, const LexerConfig &lexer_config, TemplateStorage &template_storage,
                        const FunctionStorage &function_storage)
            : pconfig(parser_config), lconfig(lexer_config), template_storage(template_storage), function_storage(function_storage), lexer(lexer_config) {}


        Template parse_file(const std::filesystem::path& path)
        {
            auto input = load_file(path);
            auto result = Template(input, path);
            parse_into(result);
            return result;
        }

        Template parse(const std::string_view input)
        {
            auto result = Template(static_cast<std::string>(input));
            parse_into(result);
            return result;
        }

        std::string load_file(const std::filesystem::path& filename)
        {
            std::filesystem::path filepath = lconfig.templates_dir;
            filepath /= filename;
            std::ifstream file;
            file.open(filepath);
            if(file.fail()) {
                throw FileError("failed accessing file '" + filename.string() + "'");
            }
            std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return text;
        }

        Template parse_expression(const std::string_view input)
        {
            auto result = Template(static_cast<std::string>(input));
            // make expression from content string
            if (!result.content.starts_with(lconfig.expression_open)) {
                result.content = lconfig.expression_open + result.content + lconfig.expression_close;
            }
            parse_into(result);
            // keep original content?
            return result;
        }
    };

} // namespace Wizard