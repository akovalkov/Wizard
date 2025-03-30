#pragma once
#include <stack>
#include <filesystem>
#include <numeric>
#include <vector>
#include <algorithm>
#include <sstream>
#include <array>
#include <ranges>
#include <boost/json/parse.hpp>
#include <boost/json/string.hpp>
#include <boost/json/error.hpp>
namespace json = boost::json;

#include "Desc.h"
#include "Config.h"
#include "Node.h"
#include "Template.h"

namespace Wizard
{
    class Renderer : public NodeVisitor
    {
        using Op = FunctionStorage::Operation;

        const RenderConfig& config;
        const TemplateStorage& template_storage;
        const FunctionStorage& function_storage;

        const Template* current_template { nullptr };
        size_t current_level{0};

        std::ostream* output_stream;    // output stream
        const json::value* input_data {nullptr};  // user data

        json::value additional_data{json::object_kind};   // additional data

        std::vector<const Template*> template_stack;
        std::vector<std::shared_ptr<json::value>> data_tmp_stack; // created variables 
        std::stack<const json::value*> data_eval_stack; // pointers to variables (created or input data reference)
        std::stack<const DataNode*> not_found_stack; // undeclared variables
        std::stack<std::ostream*> file_stack; 

    public:

        Renderer(const RenderConfig& config, const TemplateStorage& template_storage, const FunctionStorage& function_storage)
            : config(config), template_storage(template_storage), function_storage(function_storage){}

        void render(std::ostream& os, const Template& tmpl, const json::value& data, json::value* loop_data = nullptr) {
            output_stream = &os;
            current_template = &tmpl;
            input_data = &data;
            if(loop_data) {
                additional_data = *loop_data;
            } else{
                additional_data = json::value(json::object_kind);
            }

            template_stack.emplace_back(current_template);
            current_template->root.accept(*this);

            data_tmp_stack.clear();
        }

        json::value evaluate_expression(const Template& tpl, const json::value& data)
        {
            if(tpl.root.nodes.empty()){
                throw_renderer_error("empty expression", tpl.root);
            }
            input_data = &data;
            current_template = &tpl;
            auto node = tpl.root.nodes.begin()->get();
            const ExpressionWrapperNode* pExpression = dynamic_cast<const ExpressionWrapperNode*>(node);
            if(!pExpression) {
                throw_renderer_error("Template doesn't contain a expression node", *node);
            }
            auto result = eval_expression(*pExpression);
            return *result.get();
        }

        static bool truthy(const json::value* data) {
            if (data->is_bool()){
                return data->get_bool();
            } else if (data->is_uint64()) {
                return data->get_uint64() != 0;
            } else if (data->is_int64()) {
                return data->get_int64() != 0;
            } else if (data->is_double()) {
                return data->get_double() != 0.0;
            } else if (data->is_double()) {
                return !data->get_string().empty();
            } else if (data->is_array()) {
                return !data->get_array().empty();
            } else if (data->is_object()) {
                return !data->get_object().empty();
            } else if (data->is_null()) {
                return false;
            }
            return false;
        }

    protected:

        void throw_renderer_error(const std::string& message, const AstNode& node) {
            SourceLocation loc = get_source_location(current_template->content, node.pos);
            throw RenderError(message, loc);
        }

        void make_result(const json::value && result) {
            auto result_ptr = std::make_shared<json::value>(result);
            data_tmp_stack.push_back(result_ptr);
            data_eval_stack.push(result_ptr.get());
        }

        void make_result(const json::value & result) {
            auto result_ptr = std::make_shared<json::value>(result);
            data_tmp_stack.push_back(result_ptr);
            data_eval_stack.push(result_ptr.get());
        }

        auto create_empty_variable() {
            auto result_ptr = std::make_shared<json::value>(nullptr);
            data_tmp_stack.push_back(result_ptr); // anchor created variable
            return result_ptr.get();
        }

        auto create_array_variable(const std::vector<const json::value*>& data) {
            auto result_ptr = std::make_shared<json::value>(json::array_kind);
            for(const auto& pvalue : data){
                result_ptr->as_array().push_back(*pvalue);
            }
            data_tmp_stack.push_back(result_ptr); // anchor created variable
            return result_ptr.get();
        }

        void add_checked_data(const DataNode& node, const json::value* data){
            const auto& datapath = node.name;
            const Description& tpldesc = current_template->desc;
            Variable var;
            if(!tpldesc.find_variable(datapath, var)) {
                // no description, nothing to do
                if(data) {
                    data_eval_stack.push(data);
                } else {
                    // not found
                    data_eval_stack.push(nullptr);
                    not_found_stack.emplace(&node);
                }
                return; 
            }
            // set default value
            if(!data && !var.defvalue.is_null()) {
                make_result(var.defvalue);
                return;
            }
            // check required
            if(!data && var.required) {
                std::string message = "The \"" + datapath + "\" variable should be set"; 
                throw_renderer_error(message, node);

            }
            // optional value
            if(!data) {
                create_empty_variable();
                // not found data
                // data_eval_stack.push(nullptr);
                // not_found_stack.emplace(&node);
                return;
            }
            // no type, nothing to do
            if(var.type == Variable::Type::Null) {
                data_eval_stack.push(data);
                return;
            }
            // check type and conversion
            make_result(convert_value(var.type, *data));
        }

        template <size_t N, size_t N_start = 0, bool throw_not_found = true>
        std::array<const json::value*, N> get_arguments(const FunctionNode &node) {
            if (node.arguments.size() < N_start + N) {
                throw_renderer_error("function needs " + std::to_string(N_start + N) + " variables, but has only found " + std::to_string(node.arguments.size()), node);
            }

            for(size_t i = N_start; i < N_start + N; i += 1) {
                node.arguments[i]->accept(*this);
            }

            if(data_eval_stack.size() < N) {
                throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(data_eval_stack.size()), node);
            }

            std::array<const json::value*, N> result;
            for(size_t i = 0; i < N; i += 1) {
                result[N - i - 1] = data_eval_stack.top();
                data_eval_stack.pop();

                if(!result[N - i - 1]) {
                    const auto data_node = not_found_stack.top();
                    not_found_stack.pop();
                    if (throw_not_found) {
                        if(config.strict) {
                            throw_renderer_error("variable '" + data_node->name + "' not found", *data_node);
                        } else {
                            result[N - i - 1] = create_empty_variable();
                        }
                    }
                }
            }
            return result;
        }

        template <bool throw_not_found = true>
        Arguments get_argument_vector(const FunctionNode &node) {
            const size_t N = node.arguments.size();
            for (auto a : node.arguments) {
                a->accept(*this);
            }

            if(data_eval_stack.size() < N) {
                throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(data_eval_stack.size()), node);
            }

            Arguments result{N};
            for(size_t i = 0; i < N; i += 1) {
                result[N - i - 1] = data_eval_stack.top();
                data_eval_stack.pop();

                if(!result[N - i - 1]) {
                    const auto data_node = not_found_stack.top();
                    not_found_stack.pop();
                    if (throw_not_found){
                        if(config.strict) {
                            throw_renderer_error("variable '" + data_node->name + "' not found", *data_node);
                        } else {
                            result[N - i - 1] = create_empty_variable();
                        }
                    }
                }
            }
            return result;
        }

        static void print_expression(std::ostream& os, const json::value& value) {
            if (value.is_bool()){
                os << value.as_bool();
            } else if (value.is_uint64()) {
                os << value.as_uint64();
            } else if (value.is_int64()) {
                os << value.as_int64();
            } else if (value.is_double()) {
                os << value.as_double();
            } else if (value.is_string()) {
                os << value.as_string().c_str(); // otherwise the value is surrounded with ""
            } else if (value.is_array()) {
                os << value;
                // os << "[";
                // for(const auto& svalue : value.as_array()) {
                //     print_expression(os, svalue);
                //     os << ", ";
                // }
                // os << "]";
            } else if (value.is_object()) {
                // os << "{";
                // for(const auto& [sname, svalue] : value.as_object()) {
                //     os  << std::quoted(static_cast<std::string>(sname)) << ": ";
                //     print_expression(os, svalue);
                //     os << ", ";
                // }
                // os << "}";
                os << value;
            } else if (value.is_null()) {
            }
        }

        auto make_json_comparer(const AstNode& node) {
            return [&](const auto& lhs, const auto& rhs){
                if(lhs.kind() != rhs.kind() || (!lhs.is_number() && !lhs.is_string())) {
                    throw_renderer_error("the compare operator works only with array of numbers or strings", node);
                } else if(lhs.is_string()) {
                    return lhs.as_string() < rhs.as_string();
                } else if(lhs.is_double()) {
                    return lhs.as_double() < rhs.as_double();
                } else if(lhs.as_int64()) {
                    return lhs.as_int64() < rhs.as_int64();
                } else {
                    return lhs.as_uint64() < rhs.as_uint64();
                }
                throw_renderer_error("Unknown json type", node);
                return false;
            };   
        }

        const std::shared_ptr<json::value> eval_expression(const ExpressionWrapperNode& expression){
            if(!expression.root){
                throw_renderer_error("empty expression", expression);
            }

            expression.root->accept(*this);

            if(data_eval_stack.empty()) {
                throw_renderer_error("empty expression", expression);
            } else if(data_eval_stack.size() != 1) {
                throw_renderer_error("malformed expression", expression);
            }

            const auto result = data_eval_stack.top();
            data_eval_stack.pop();

            if(!result) {
                if(not_found_stack.empty()) {
                    throw_renderer_error("expression could not be evaluated", expression);
                }
                auto node = not_found_stack.top();
                not_found_stack.pop();
                if(config.strict) {
                    throw_renderer_error("variable '" + node->name + "' not found", *node);
                } else {
                    return std::make_shared<json::value>(nullptr);
                }
            }
            return std::make_shared<json::value>(*result);
        }

        void visit(const BlockNode &node){
            for (auto& child : node.nodes){
                child->accept(*this);
            }
        }

        void visit(const ExpressionWrapperNode& node) {
            auto expr = eval_expression(node);
            print_expression(*output_stream, *expr);
        }

        void visit(const TextNode& node) {
            output_stream->write(current_template->content.c_str() + node.pos, node.length);
        }

        void visit(const CommentNode&) {}

        void visit(const ExpressionNode&) {}

        void visit(const LiteralNode& node) {
            data_eval_stack.push(&node.value);
        }

        void visit(const DataNode& node){
            // boost::system::error_code errcode;
            // const auto* data = additional_data.find_pointer(node.path, errcode);
            // if (!data){
            //     data = input_data->find_pointer(node.path, errcode);    
            // }
            // if (!data) {
            //     // Try to evaluate as a no-argument callback
            //     const auto function_data = function_storage.find_function(node.name, 0);
            //     if (function_data.operation == FunctionStorage::Operation::Callback) {
            //         Arguments empty_args{};
            //         const auto value = std::make_shared<json::value>(function_data.callback(empty_args));
            //         add_checked_data(node, value.get());
            //         return;
            //     }
            // } 
            // add_checked_data(node, data);
            auto data = boost::json::find_pointers(static_cast<const json::value&>(additional_data), node.name);
            if (data.empty()){
                data = boost::json::find_pointers(*input_data, node.name);    
            }
            if (data.empty()) {
                // Try to evaluate as a no-argument callback
                const auto function_data = function_storage.find_function(node.name, 0);
                if (function_data.operation == FunctionStorage::Operation::Callback) {
                    Arguments empty_args{};
                    const auto value = std::make_shared<json::value>(function_data.callback(empty_args));
                    add_checked_data(node, value.get());
                    return;
                }
            } 
            // empty data
            if(data.empty()) {
                // may be null is ok or default value is specified
                add_checked_data(node, nullptr);
            } else if(data.size() == 1) {
                // if result scalar then just use first value
                add_checked_data(node, data.front());
            } else {
                // array of json pointers needs to convert in new json array 
                // where each element is copy of original value (may be it's bad decision)
                auto jarray = create_array_variable(data);
                add_checked_data(node, jarray);
            }
        }

        void visit(const FunctionNode& node) {
            switch (node.operation){
            case Op::Not: 
                {
                    const auto args = get_arguments<1>(node);
                    make_result(!truthy(args[0]));
                }
                break;
            case Op::And:
                {
                    make_result(truthy(get_arguments<1, 0>(node)[0]) && truthy(get_arguments<1, 1>(node)[0]));
                }
                break;
            case Op::Or:
                {
                    make_result(truthy(get_arguments<1, 0>(node)[0]) || truthy(get_arguments<1, 1>(node)[0]));
                }
                break;
            case Op::In:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_array()) {
                        throw_renderer_error("The 'in' function works only with array", node);
                    }
                    const auto& arr = args[1]->as_array();
                    make_result(std::find(arr.begin(), arr.end(), *args[0]) != arr.end());
                }
                break;
            case Op::Equal:
                {
                    const auto args = get_arguments<2>(node);
                    make_result(*args[0] == *args[1]);
                }
                break;
            case Op::NotEqual:
                {
                    const auto args = get_arguments<2>(node);
                    make_result(*args[0] != *args[1]);
                }
                break;
            case Op::Greater:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_string() && args[1]->is_string()){
                        make_result(args[0]->as_string() > args[1]->as_string());
                    } else if(args[0]->is_number() && args[1]->is_number()){
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) > 
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '>' operator works only with string or numbers", node);
                    }
                }
                break;
            case Op::GreaterEqual:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_string() && args[1]->is_string()){
                        make_result(args[0]->as_string() >= args[1]->as_string());
                    } else if(args[0]->is_number() && args[1]->is_number()){
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) >= 
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '>=' operator works only with string or numbers", node);
                    }
                }
                break;
            case Op::Less:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_string() && args[1]->is_string()){
                        make_result(args[0]->as_string() < args[1]->as_string());
                    } else if(args[0]->is_number() && args[1]->is_number()){
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) <
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '<' operator works only with string or numbers", node);
                    }
                }
                break;
            case Op::LessEqual:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_string() && args[1]->is_string()){
                        make_result(args[0]->as_string() <= args[1]->as_string());
                    } else if(args[0]->is_number() && args[1]->is_number()){
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) <=
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '<=' operator works only with string or numbers", node);
                    }
                }
                break;
            case Op::Add:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_string() && args[1]->is_string()){
                        auto str = args[0]->as_string();
                        str += args[1]->as_string();
                        make_result(str);
                    } else if(args[0]->is_number() && args[1]->is_number()){
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) +
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '+' operator works only with string or numbers", node);
                    }
                }
                break;
            case Op::Subtract:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_number() && args[1]->is_number()) {
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) -
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '+' operator works only with numbers", node);
                    }
                }
                break;
            case Op::Multiplication:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_number() && args[1]->is_number()) {
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) *
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '*' operator works only with numbers", node);
                    }
                }
                break;
            case Op::Division:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_number() && args[1]->is_number()) {
                        if ((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) == 0) {
                            throw_renderer_error("division by zero", node);
                        }
                        make_result((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()) /
                                    (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                    } else {
                        throw_renderer_error("The '/' operator works only with numbers", node);
                    }
                }
                break;
            case Op::Power:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_number() && args[1]->is_number()) {
                        const auto result = std::pow((args[0]->is_double() ? args[0]->as_double() : args[0]->as_int64()), 
                                                     (args[1]->is_double() ? args[1]->as_double() : args[1]->as_int64()));
                        make_result(result);
                    } else {
                        throw_renderer_error("The '^' operator works only with numbers", node);
                    }
                }
                break;
            case Op::Modulo:
                {
                    const auto args = get_arguments<2>(node);
                    if(args[0]->is_int64() && args[1]->is_int64()) {
                        make_result(args[0]->as_int64() % args[1]->as_int64());
                    } else {
                        throw_renderer_error("The '%' operator works only with int numbers", node);
                    }
                }
                break;
            case Op::AtId: // through dot object.field
                {
                    const auto container = get_arguments<1, 0, false>(node)[0];
                    node.arguments[1]->accept(*this);
                    if(not_found_stack.empty()) {
                        throw_renderer_error("could not find element with given name", node);
                    }
                    const auto id_node = not_found_stack.top();
                    not_found_stack.pop();
                    data_eval_stack.pop();
                    data_eval_stack.push(&container->at(id_node->name));
                }
                break;
            case Op::At:
                {
                    const auto args = get_arguments<2>(node);
                    if (args[0]->is_object()){
                        data_eval_stack.push(&args[0]->at(args[1]->as_string()));
                    } else {
                        data_eval_stack.push(&args[0]->at(args[1]->as_int64()));
                    }
                }
                break;
            case Op::Default:
                {
                    const auto test_arg = get_arguments<1, 0, false>(node)[0];
                    data_eval_stack.push(test_arg ? test_arg : get_arguments<1, 1>(node)[0]);
                }
                break;
            case Op::DivisibleBy:
                {
                    const auto args = get_arguments<2>(node);
                    const auto divisor = args[1]->as_int64();
                    make_result((divisor != 0) && (args[0]->as_int64() % divisor == 0));
                }
                break;
            case Op::Even:
                {
                    make_result(get_arguments<1>(node)[0]->as_int64() % 2 == 0);
                }
                break;
            case Op::Exists:
                {
                    auto &&name = get_arguments<1>(node)[0]->as_string();
                    boost::system::error_code ec;
                    make_result(input_data->find_pointer(convert_dot_to_ptr(name), ec) != nullptr);
                }
                break;
            case Op::ExistsInObject:
                {
                    const auto args = get_arguments<2>(node);
                    auto& obj = args[0]->as_object();
                    auto&& name = args[1]->as_string();
                    make_result(obj.find(name) != obj.end());
                }
                break;
            case Op::First:
                {
                    const auto args = get_arguments<1>(node);
                    if(!args[0]->is_array()) {
                        throw_renderer_error("the 'first' function works only with array", node);
                    }
                    const auto result = args[0]->get_array().front();
                    data_eval_stack.push(&result);
                }
                break;
            case Op::Float:
                {
                    const std::string&& number = static_cast<std::string>(get_arguments<1>(node)[0]->as_string());
                    make_result(std::stod(number));
                }
                break;
            case Op::Int:
                {
                    const std::string&& number = static_cast<std::string>(get_arguments<1>(node)[0]->as_string());
                    make_result(std::stoi(number));
                }
                break;
            case Op::Last:
                {
                    const auto args = get_arguments<1>(node);
                    if(!args[0]->is_array()) {
                        throw_renderer_error("the 'last' function works only with array", node);
                    }
                    const auto result = args[0]->get_array().back();
                    data_eval_stack.push(&result);
                }
                break;
            case Op::Length:
                {
                    const auto val = get_arguments<1>(node)[0];
                    if (val->is_string()) {
                        make_result(val->as_string().size());
                    } else if (val->is_array()) {
                        make_result(val->as_array().size());
                    } else if (val->is_object()) {
                        make_result(val->as_object().size());
                    } else {
                        throw_renderer_error("the 'length' function works only with array, object, string", node);
                    }
                }
                break;
            case Op::Lower:
                {
                    auto result = get_arguments<1>(node)[0]->as_string();
                    std::transform(result.begin(), result.end(), result.begin(), [](char c)
                                { return static_cast<char>(::tolower(c)); });
                    make_result(std::move(result));
                }
                break;
            case Op::Max:
                {
                    const auto args = get_arguments<1>(node);
                    if(!args[0]->is_array()) {
                        throw_renderer_error("the 'first' function works only with array", node);
                    }
                    const auto& arr = args[0]->get_array();
                    const auto result = std::max_element(arr.begin(), arr.end(), make_json_comparer(node));
                    data_eval_stack.push(&(*result));
                }
                break;
            case Op::Min:
                {
                    const auto args = get_arguments<1>(node);
                    if(!args[0]->is_array()) {
                        throw_renderer_error("the 'first' function works only with array", node);
                    }
                    const auto& arr = args[0]->get_array();
                    const auto result = std::min_element(arr.begin(), arr.end(), make_json_comparer(node));
                    data_eval_stack.push(&(*result));
                }
                break;
            case Op::Odd:
                {
                    make_result(get_arguments<1>(node)[0]->as_int64() % 2 != 0);
                }
                break;
            case Op::Range:
                {
                    std::vector<int> result(get_arguments<1>(node)[0]->as_int64());
                    std::iota(result.begin(), result.end(), 0);
                    make_result(json::array(result.begin(), result.end()));
                }
                break;
            case Op::Round:
                {
                    const auto args = get_arguments<2>(node);
                    const auto precision = args[1]->as_int64();
                    const double result = std::round(args[0]->as_double() * std::pow(10.0, precision)) / std::pow(10.0, precision);
                    if(precision == 0) {
                        make_result(static_cast<int>(result));
                    } else {
                        make_result(result);
                    }
                }
                break;
            case Op::Sort:
                {
                    const auto args = get_arguments<1>(node);
                    if(!args[0]->is_array()) {
                        throw_renderer_error("The 'sort' function works only with array", node);
                    }
                    auto arr = args[0]->get_array();
                    std::sort(arr.begin(), arr.end(), make_json_comparer(node));
                    auto result_ptr = std::make_shared<json::value>(arr);
                    data_tmp_stack.push_back(result_ptr);
                    data_eval_stack.push(result_ptr.get());
                }
                break;
            case Op::Upper:
                {
                    auto result = get_arguments<1>(node)[0]->as_string();
                    std::transform(result.begin(), result.end(), result.begin(), [](char c)
                                { return static_cast<char>(::toupper(c)); });
                    make_result(std::move(result));
                }
                break;
            case Op::IsBoolean:
                {
                    make_result(get_arguments<1>(node)[0]->is_bool());
                }
                break;
            case Op::IsNumber:
                {
                    make_result(get_arguments<1>(node)[0]->is_number());
                }
                break;
            case Op::IsInteger:
                {
                    make_result(get_arguments<1>(node)[0]->is_int64());
                }
                break;
            case Op::IsFloat:
                {
                    make_result(get_arguments<1>(node)[0]->is_double());
                }
                break;
            case Op::IsObject:
                {
                    make_result(get_arguments<1>(node)[0]->is_object());
                }
                break;
            case Op::IsArray:
                {
                    make_result(get_arguments<1>(node)[0]->is_array());
                }
                break;
            case Op::IsString:
                {
                    make_result(get_arguments<1>(node)[0]->is_string());
                }
                break;
            case Op::Callback:
                {
                    auto args = get_argument_vector(node);
                    make_result(node.callback(args));
                }
                break;
            case Op::Join:
                {
                    const auto args = get_arguments<2>(node);
                    const auto& arr = args[0]->as_array();
                    const auto& separator = args[1]->as_string();
                    std::ostringstream os;
                    std::string sep;
                    for (const auto &value : arr) {
                        os << sep;
                        if (value.is_string()) {
                            os << value.as_string().c_str(); // otherwise the value is surrounded with ""
                        } else {
                            os << value;
                        }
                        sep = separator;
                    }
                    make_result(json::value(os.str()));
                }
                break;
            case Op::Split:
                {
                    const auto args = get_arguments<2>(node);
                    const auto& str = args[0]->as_string();
                    const auto& delim = args[1]->as_string();
                    auto parts = str | std::views::split(delim) 
                                     | std::views::transform([](auto r) {
                                        return std::string(r.data(), r.size());
                                       });
                      make_result(json::array(parts.begin(), parts.end()));
                }
                break;
            case Op::None:
                break;
            }
        }

        void visit(const StatementNode &) {
        }

        void visit(const ForStatementNode &) {
        }

        void visit(const ForArrayStatementNode& node){
            const auto result = eval_expression(node.condition);
            if (!result->is_array()){
                throw_renderer_error("object must be an array", node);
            }
            const auto array_result = result->as_array();

            json::object& data = additional_data.as_object();
            json::object loop_data;
            if(data.contains(config.loop_variable_name)){
                loop_data["parent"] = data[config.loop_variable_name];
            }

            size_t index = 0;
            loop_data["is_first"] = true;
            loop_data["is_last"] = array_result.size() <= 1;
            for(auto it = array_result.begin(); it != array_result.end(); ++it) {

                loop_data["index"] = index;
                loop_data["index1"] = index + 1;
                if(index == 1) {
                    loop_data["is_first"] = false;
                }
                if(index == array_result.size() - 1) {
                    loop_data["is_last"] = true;
                }
                data[node.value] = *it;
                data[config.loop_variable_name] = loop_data;
                node.body.accept(*this);
                ++index;
            }

            data.erase(node.value);
            if(loop_data.contains("parent")) {
                data[config.loop_variable_name] = loop_data["parent"];
            } else {
                data.erase(config.loop_variable_name);
            }
        }

        void visit(const ForObjectStatementNode& node)
        {
            const auto result = eval_expression(node.condition);
            if(!result->is_object()) {
                throw_renderer_error("object must be an object", node);
            }

            const auto object_result = result->as_object();

            json::object& data = additional_data.as_object();
            json::object loop_data;
            if(data.contains(config.loop_variable_name)){
                loop_data["parent"] = data[config.loop_variable_name];
            }

            size_t index = 0;
            loop_data["is_first"] = true;
            loop_data["is_last"] = object_result.size() <= 1;
            for (auto it = object_result.begin(); it != object_result.end(); ++it) {
                loop_data["index"] = index;
                loop_data["index1"] = index + 1;
                if(index == 1) {
                    loop_data["is_first"] = false;
                }
                if(index == object_result.size() - 1) {
                    loop_data["is_last"] = true;
                }
                data[node.key] = it->key();
                data[node.value] = it->value();
                data[config.loop_variable_name] = loop_data;
                node.body.accept(*this);
                ++index;
            }

            data.erase(node.key);
            data.erase(node.value);
            if(loop_data.contains("parent")) {
                data[config.loop_variable_name] = loop_data["parent"];
            } else {
                data.erase(config.loop_variable_name);
            }
        }

        void visit(const IfStatementNode& node)
        {
            const auto result = eval_expression(node.condition);
            if (truthy(result.get())){
                node.true_statement.accept(*this);
            }else if (node.has_false_statement) {
                node.false_statement.accept(*this);
            }
        }

        void visit(const FileStatementNode& node) {
            const auto filename = eval_expression(node.filename);
            if(!filename->is_string()) {
                throw_renderer_error("filename must be an string", node);
            }
            if(config.dry_run) {
                // debug output to console
                auto sfilename = static_cast<std::string>(filename->as_string());
                *output_stream << ">>>>>> Start file: " << std::quoted(sfilename) << std::endl;
                node.body.accept(*this);
                *output_stream << "<<<<<< End file: " << std::quoted(sfilename) << std::endl;
                return;
            }
            // real work
            // normalize path according OS (directory separator)
            std::filesystem::path pfilename = filesystem::path::normalize_separators(static_cast<std::string>(filename->as_string())); 
            std::filesystem::path filepath = config.output_dir;
            filepath /= pfilename;

            if(!std::filesystem::exists(filepath.parent_path()) && 
               !std::filesystem::create_directories(filepath.parent_path())) {
                throw_renderer_error("couldn't create output path", node);
            }
            std::ofstream ofile;
            ofile.open(filepath.c_str()); 
            if(ofile.fail()) {
                throw_renderer_error("couldn't create output file", node);
            }
            file_stack.push(output_stream);
            output_stream = &ofile;
            node.body.accept(*this);
            output_stream = file_stack.top();
            file_stack.pop();
        }

        void visit(const ApplyTemplateStatementNode& node) {
            // find data
            boost::system::error_code errcode;
            if(!input_data->find_pointer(node.field_path, errcode)) {
                return; // no field is OK ?????
            }

            // find template
            const auto template_it = template_storage.find(node.template_name);
            if (template_it != template_storage.end()){
                // find data
                auto& subdata = input_data->at_pointer(node.field_path);
                if(subdata.is_array()) {
                    json::object& data = additional_data.as_object();
                    json::object loop_data;
                    if (data.contains(config.loop_variable_name)) {
                        loop_data["parent"] = data[config.loop_variable_name];
                    }

                    // render templates
                    auto& subarr = subdata.get_array();
                    loop_data["is_first"] = true;
                    loop_data["is_last"] = subarr.size() <= 1;
                    for(auto i = 0ul; i != subarr.size(); ++i) {
                        loop_data["index"] = i;
                        loop_data["index1"] = i + 1;
                        if (i == 1) {
                            loop_data["is_first"] = false;
                        }
                        if (i == subarr.size() - 1) {
                            loop_data["is_last"] = true;
                        }
                        data[config.loop_variable_name] = loop_data;
                        auto sub_renderer = Renderer(config, template_storage, function_storage);
                        sub_renderer.render(*output_stream, template_it->second, subarr[i], &additional_data);
                    }
                    if (loop_data.contains("parent")) {
                        data[config.loop_variable_name] = loop_data["parent"];
                    } else {
                        data.erase(config.loop_variable_name);
                    }

                } else {
                    // render template
                    auto sub_renderer = Renderer(config, template_storage, function_storage);
                    sub_renderer.render(*output_stream, template_it->second, subdata, &additional_data);
                }
            } else if (config.throw_at_missing_includes) {
                throw_renderer_error("apply template '" + node.template_name + "' not found", node);
            }
        }

        void visit(const SetStatementNode &node){
            std::string ptr = convert_dot_to_ptr(node.key);
            additional_data.set_at_pointer(ptr, *eval_expression(node.expression));
        }
 
        json::value convert_value(const Variable::Type& type, const json::value& value)
        {
            switch(value.kind()) {
            case json::kind::null: {
                    // null
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(false);
                    case Variable::Type::Integer:
                        return json::value(0);
                    case Variable::Type::Double:
                        return json::value(0.0);
                    case Variable::Type::String:
                        return json::value("");
                    case Variable::Type::Array:
                        return json::value(json::array_kind);
                    case Variable::Type::Object:
                        return json::value(json::object_kind);
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::bool_: {
                    // boolean
                    auto bvalue = value.as_bool();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(bvalue);
                    case Variable::Type::Integer:
                        return json::value(bvalue ? 0 : 1);
                    case Variable::Type::Double: {
                            std::string message = "Cannot convert bool value to double";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::String:
                        return json::value(bvalue ? "true" : "false");
                    case Variable::Type::Array:
                        //return json::value(json::array_kind);
                        return json::value({{bvalue}});
                    case Variable::Type::Object: {
                            std::string message = "Cannot convert bool value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::int64: {
                    // signed int
                    auto ivalue = value.as_int64();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(ivalue != 0);
                    case Variable::Type::Integer:
                        return json::value(ivalue);
                    case Variable::Type::Double:
                        return json::value(ivalue * 1.0);
                    case Variable::Type::String:
                        return json::value(std::to_string(ivalue));
                    case Variable::Type::Array:
                        return json::value({{ivalue}});
                    case Variable::Type::Object: {
                            std::string message = "Cannot convert int value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::uint64: {
                    // unsgined int
                    auto ivalue = value.as_uint64();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(ivalue != 0);
                    case Variable::Type::Integer:
                        return json::value(ivalue);
                    case Variable::Type::Double:
                        return json::value(ivalue * 1.0);
                    case Variable::Type::String:
                        return json::value(std::to_string(ivalue));
                    case Variable::Type::Array:
                        return json::value({{ivalue}});
                    case Variable::Type::Object: {
                            std::string message = "Cannot convert unsigned int value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::double_: {
                    // double
                    auto dvalue = value.as_double();
                    switch(type){
                    case Variable::Type::Boolean: {
                            std::string message = "Cannot convert double value to bool";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Integer:
                        return json::value(static_cast<int>(dvalue));
                    case Variable::Type::Double:
                        return json::value(dvalue);
                    case Variable::Type::String:
                        return json::value(std::to_string(dvalue));
                    case Variable::Type::Array:
                        return json::value({{dvalue}});
                    case Variable::Type::Object:{
                            std::string message = "Cannot convert unsigned int value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::string: {
                    // string
                    std::string svalue = value.as_string().c_str();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(svalue == "true" ? true : false);
                    case Variable::Type::Integer:
                        return json::value(std::stoi(svalue));
                    case Variable::Type::Double:
                        return json::value(std::stod(svalue));
                    case Variable::Type::String:
                        return json::value(svalue);
                    case Variable::Type::Array:
                        return json::value({{svalue}});
                    case Variable::Type::Object: {
                            std::string message = "Cannot convert string value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::array: {
                    // array
                    auto& arr = value.as_array();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(!arr.empty());
                    case Variable::Type::Integer:{
                            std::string message = "Cannot convert array value to integer";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Double:{
                            std::string message = "Cannot convert array value to double";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::String:{
                            std::string str;
                            for(auto val: arr){
                                auto sval = convert_value(Variable::Type::String, val);
                                if(!str.empty()) {
                                    str += ", ";
                                }
                                str += "\"";
                                str += sval.as_string().c_str();
                                str += "\"";
                            }		
                            str = "[" + str + "]";
                            return json::value(str);
                    }
                    case Variable::Type::Array:
                        return arr;
                    case Variable::Type::Object:{
                            std::string message = "Cannot convert array value to object";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            case json::kind::object: {
                    // object
                    auto& obj = value.as_object();
                    switch(type){
                    case Variable::Type::Boolean:
                        return json::value(!obj.empty());
                    case Variable::Type::Integer:{
                            std::string message = "Cannot convert object value to integer";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::Double:{
                            std::string message = "Cannot convert object value to double";
                            throw BaseError("data_error", message);
                        }
                    case Variable::Type::String:{
                            std::string str;
                            for(const auto& [key, val]: obj){
                                auto sval = convert_value(Variable::Type::String, val);
                                if(!str.empty()) {
                                    str += ", ";
                                }
                                str += "\"";
                                str += key;
                                str += "\" : \"";
                                str += sval.as_string().c_str();
                                str += "\"";
                            }		
                            str = "{" + str + "}";
                            return json::value(str);
                        }
                    case Variable::Type::Array:
                        return json::value({{obj}});
                    case Variable::Type::Object:
                        return obj;
                    case Variable::Type::Null:
                        return {};
                    }
                }
                break;
            }
            std::string message = "Internal Error: Unknown json type";
            throw BaseError("data_error", message);
        }
   };
}