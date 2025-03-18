#pragma once
#include <set>
#include <map>
#include <vector>
#include <fstream>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
namespace json = boost::json;

#include "Util.h"
#include "Exceptions.h"
namespace Wizard
{
    struct Variable;
    using Variables = std::map<std::string, Variable>;
    
    struct Variable
    {
        enum class Type : char {
            Null,
            Boolean,
            Integer,
            Double,
            String,
            Array,
            Object
        };
        std::string name;
        std::string description;
        Type type{Type::Null};
        bool required{true};
        json::value defvalue;
        Variables variables;
        bool operator==(const Variable &) const = default;
    };

    using Variables = std::map<std::string, Variable>;
    using Templates = std::set<std::string>;

    struct Description
    {
        std::string name;
        std::string description;
        Variables variables;
        Templates nested;

        void clear()
        {
            name.clear();
            description.clear();
            variables.clear();
            nested.clear();
        }

        static Description load_from_json(const std::string& name, const std::filesystem::path& path)
        {
            std::error_code ec;
            std::ifstream infile;
            infile.open(path);
            if(infile.fail()) {
                throw FileError("Couldn't open file: \"" + path.string() + "\"");
            }
            auto desc = json::parse(infile, ec);
            if(ec) {
                throw FileError(ec.message());
            }
            // find template description
            json::object newobj;
            auto& desc_obj = find_object(desc, "template", name, newobj);
            return load_from_json(desc_obj);
        }

        static Description load_from_json(const json::object& jdesc)
        {
            Description desc;
            desc.name = jdesc.at("template").as_string().c_str();
            desc.description = jdesc.at("description").as_string().c_str();
            load_variables(jdesc, desc.variables);
            if(jdesc.if_contains("templates") && jdesc.at("templates").is_array()) {
                for(const auto& tpl_name : jdesc.at("templates").as_array()) {
                    desc.nested.insert(tpl_name.as_string().c_str());
                }
            }
            return desc;
        }

        json::object create_json_object() const
        {
            json::object obj;
            obj["template"] = json::value(name);
            obj["description"] = json::value(description);
            obj["variables"] = json::value(json::array_kind);
            create_json_variables(variables, obj["variables"].as_array());
            if(!nested.empty()) {
                obj["templates"] = json::value(json::array_kind);
                for(const auto& tpl_name : nested) {
                    obj["templates"].as_array().emplace_back(tpl_name);
                }
            }
            return obj;
        }

        bool find_variable(const std::string_view& path, Variable& var) const
        {
            auto parts = split_path(path);
            const auto* pvars = &variables;
            const Variable* pvar = nullptr;
            for(auto& part : parts){
                auto it = pvars->find(part);
                if(it == pvars->end()) {
                    return false;
                }
                pvar = &it->second;
                pvars = &pvar->variables;
            }
            var = *pvar;
            return true;
        }

        static std::string type_to_string(Variable::Type vtype)
        {
            std::string stype;
            switch(vtype){
            case Variable::Type::Null:
                stype = "null";
                break;
            case Variable::Type::Boolean:
                stype = "bool";
                break;
            case Variable::Type::Integer:
                stype = "integer";
                break;
            case Variable::Type::Double:
                stype = "double";
                break;
            case Variable::Type::String:
                stype = "string";
                break;
            case Variable::Type::Array:
                stype = "array";
                break;
            case Variable::Type::Object:
                stype = "object";
                break;
            }
            return stype;
        }

    protected:        

        static auto split_path(std::string_view ptr_name) 
        {
            std::vector<std::string> parts;
            do {
                std::string_view part;
                std::tie(part, ptr_name) = string_view::split(ptr_name, '.');
                if (!part.empty()) {
                    parts.emplace_back(part.begin(), part.end());
                }
            } while (!ptr_name.empty());
            return parts;
        }

        void create_json_variables(const Variables& vars, json::array& jvars) const {
            for(const auto& [_, variable] : vars) {
                json::object vobj;
                vobj["name"] = json::value(variable.name);
                vobj["description"] = json::value(variable.description);
                auto stype = type_to_string(variable.type);
                vobj["type"] = json::value(stype);
                vobj["required"] = json::value(variable.required);
                if(!variable.defvalue.is_null()) {
                    vobj["default"] = variable.defvalue;
                }
                if(!variable.variables.empty()){
                    vobj["variables"] = json::value(json::array_kind);
                    create_json_variables(variable.variables, vobj["variables"].as_array());
                }
                jvars.push_back(vobj);
            }

        }

        static void load_variables(const json::object& jdesc, Variables& vars) {
            if(!jdesc.if_contains("variables")) {
                return;
            }
            for(const auto& var : jdesc.at("variables").as_array()) {
                const auto& varobj = var.as_object();
                Variable variable;
                variable.name = varobj.at("name").as_string().c_str();
                if (varobj.if_contains("description")) {
                    variable.description = varobj.at("description").as_string().c_str();
                }
                if (varobj.if_contains("type")) {
                    std::string stype = varobj.at("type").as_string().c_str();
                    if(stype == "null") {
                        variable.type = Variable::Type::Null;
                    } else if(stype == "bool") {
                        variable.type = Variable::Type::Boolean;
                    } else if(stype == "integer") {
                        variable.type = Variable::Type::Integer;
                    } else if(stype == "double") {
                        variable.type = Variable::Type::Double;
                    } else if(stype == "string") {
                        variable.type = Variable::Type::String;
                    } else if(stype == "array") {
                        variable.type = Variable::Type::Array;
                    } else if(stype == "object") {
                        variable.type = Variable::Type::Object;
                    } else {
                        throw DataError("Unknown variable type: \"" + stype + "\"", SourceLocation{});
                    }   
                }
                if (varobj.if_contains("required")) {
                    variable.required = varobj.at("required").as_bool();
                }
                if(varobj.if_contains("default")){
                    variable.defvalue = varobj.at("default");
                }
                load_variables(varobj, variable.variables);
                vars[variable.name] = variable;
            }	
        }
    };


};

