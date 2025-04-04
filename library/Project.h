#pragma once
#include <string>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
namespace json = boost::json;
#include "Environment.h"
#include "JsonTransformer.h"

namespace Wizard
{
    struct Module {
        std::string name;            // template name   
        std::filesystem::path info;  // fields template decription file (optional)
        JsonTransformer transformer; // json transformation rules (data json => template json)

        void init(const json::object& jmodule)
        {
            name = jmodule.at("template").as_string().c_str();
            info.clear();
            if(jmodule.if_contains("info")) {
                info = jmodule.at("info").as_string().c_str();
            }
            transformer.init(jmodule.at("rules"));
        }

        json::value transform(const json::value& data) const
        {
            if(transformer.rules().empty()) {
                return data;
            }
            // transform json data
            return transformer.transform(data);
        }
    };

    struct Project {
        std::string name;               // project name    
        std::string description;        // project description
        std::filesystem::path info;     // fields template decription file for all templates (optional)
        std::vector<Module> modules;    // templates

        std::string render(Environment& env, const json::value& data, 
                           const std::filesystem::path& infofile = "")
        {
            std::string result;
            for(const auto& module : modules) {
                auto mdata = module.transform(data);
                auto ifile = !infofile.empty() ? infofile : (!module.info.empty() ? module.info : info);
                result += env.render_file(module.name, mdata, ifile);
            }
            return result;
        }

        void init(const std::filesystem::path& path)
        {
            std::error_code ec;
            std::ifstream infile;
            infile.open(path);
            if(infile.fail()) {
                throw FileError("Couldn't open file: \"" + path.string() + "\"");
            }
            auto project = json::parse(infile, ec);
            if(ec) {
                throw FileError(ec.message());
            }
            init(project.as_object());
        }

        void init(const json::object& jproject)
        {
            name = jproject.at("name").as_string().c_str();
            description = jproject.at("description").as_string().c_str();
            info.clear();
            if(jproject.if_contains("info")) {
                info = jproject.at("info").as_string().c_str();
            }
            for(const auto& mod : jproject.at("modules").as_array()) {
                const auto& modobj = mod.as_object();
                Module module;
                module.init(modobj);
                modules.push_back(std::move(module));
            }	
        }
    };
}