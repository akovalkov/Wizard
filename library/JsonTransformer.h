#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <sstream>
#include <boost/json/parse.hpp>
#include <boost/json/string.hpp>
#include <boost/json/error.hpp>
#include <boost/json/serialize.hpp>
namespace json = boost::json;
#include "Environment.h"

namespace Wizard
{
    class JsonTransformer
    {
    public:
        struct Rule
        {
            std::string from; // original name
            Template filter; // filter expression (boolean)
            Template expr; //  transform expression
            std::string to; // new name
            std::vector<Rule> rules; // sub rules
            bool operator==(const Rule& other) const = default;
        };

        void init(const std::filesystem::path& jrules)
        {
            std::ifstream ifs(jrules);
            if (!ifs) {
                throw FileError("Cannot open file " + jrules.string());
          }
            std::string jrules_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            init(jrules_str);
        }
    
        void init(const std::string& jrules)
        {
            std::error_code ec;
            auto jvrules = json::parse(jrules, ec);
            if(ec) {
                throw ParserError(ec.message(), {});
            }
            // parse transform rules
            Environment env;
            rules_.clear();
            parse_rules(env, rules_, jvrules);
        }
    
        void init(const json::value& jvrules){
            Environment env;
            rules_.clear();
            parse_rules(env, rules_, jvrules);
        }
    
        json::value transform(const json::value& value) const
        {
            // process rules
            Environment env;
            json::value result(json::object_kind);
            transform_value(env, rules_, value, result);
            return result;
        }
    
        std::string transform(const std::string& jstr) const
        {
            std::error_code ec;
            auto jvstr = json::parse(jstr, ec);
            if(ec) {
                throw ParserError(ec.message(), {});
            }
            json::value jvnew = transform(jvstr);
            return json::serialize(jvnew);
        }

        const auto& rules() const { return rules_; }

    protected:
        std::vector<Rule> rules_;

        void transform_value(Environment& env, const Rule& rule, 
                             const json::value& value,
                             json::value& result) const
        {
            // check value
            if(!rule.filter.empty()) {
                auto filter_value = env.evaluate_expression(rule.filter, value);
                if(!Renderer::truthy(&filter_value)) {
                    return; // return empty json
                }
            }
            if(!rule.rules.empty()) {
                // sub rules
                transform_value(env, rule.rules, value, result);
            } else {
                // simple transform 
                result = value;
            }
        }                        

        void transform_value(Environment& env, const std::vector<Rule>& rules, 
                             const json::value& value, 
                             json::value& result) const
        {
            if(rules.empty()) {
                return;
            }

            json::value expr_value;
            for(const auto& rule : rules) {
                std::vector<const json::value*> old_values;
                if(!rule.expr.empty()) {
                    // evaluate the expression and use result as value
                    expr_value = env.evaluate_expression(rule.expr, value);
                    if(!expr_value.is_null()) {
                        old_values.push_back(&expr_value);
                    }
                } else {
                    // just find json values by the "from" field
                    old_values = boost::json::find_pointers(value, rule.from);
                }
                if(old_values.empty()) {
                    continue;
                }
                json::value new_values(json::array_kind);
                for(const auto& old_value : old_values) {
                    json::value new_value(json::array_kind);
                    if(old_value->is_array()) {
                        for(const auto& jv : old_value->as_array()) {
                            json::value new_value_obj(json::object_kind);
                            transform_value(env, rule, jv, new_value_obj);
                            if (!new_value_obj.as_object().empty()) {
                                new_value.as_array().push_back(new_value_obj);
                            }
                        }
                    } else {
                        json::value new_value_obj(json::object_kind);
                        transform_value(env, rule, *old_value, new_value_obj);
                        new_value = new_value_obj;
                    }
                    if(old_values.size() > 1) {
                        new_values.as_array().push_back(new_value);
                    } else {
                        new_values = new_value;
                    }
                }
                auto to_path = rule.to.empty() ? rule.from : rule.to;
                result.set_at_pointer(convert_dot_to_ptr(to_path), new_values);
            }
        }
    
        void parse_rules(Environment& env, std::vector<Rule>& rules, const json::value& jvrules)
        {
            if(!jvrules.is_array()) {
                throw ParserError("The json rules should be an array", {});
            }
            for(const auto& jvrule : jvrules.as_array()) {
                if(!jvrule.is_object()) {
                    throw ParserError("The json rule should be an object", {});
                }
                auto jvrule_obj = jvrule.as_object();
                if(!jvrule_obj.if_contains("from") && !jvrule_obj.if_contains("expr")) {
                    throw ParserError("The json rule should have 'from' or 'expr' fields", {});
                }
                if(!jvrule_obj.if_contains("to") && !jvrule_obj.if_contains("from")) {
                    throw ParserError("The json rule should have 'from' or 'to' fields", {});
                }
                Rule rule;
                if(jvrule_obj.if_contains("from")) {
                    rule.from = jvrule_obj.at("from").as_string().c_str();
                }
                if(jvrule_obj.if_contains("filter")) {
                    rule.filter = env.parse_expression(jvrule_obj.at("filter").as_string().c_str());
                }
                if(jvrule_obj.if_contains("expr")) {
                    rule.expr = env.parse_expression(jvrule_obj.at("expr").as_string().c_str());
                }
                if(jvrule_obj.if_contains("to")) {
                    rule.to = jvrule_obj.at("to").as_string().c_str();
                }
                if(jvrule_obj.if_contains("rules")) {
                    parse_rules(env, rule.rules, jvrule_obj.at("rules"));
                }
                rules.push_back(std::move(rule));
            }
        }
    };
};
