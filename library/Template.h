#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <boost/json/object.hpp>
namespace json = boost::json;

#include "Node.h"
#include "Desc.h"

namespace Wizard
{
    struct Template
    {
        BlockNode root;
        std::string content;
        std::filesystem::path path;
        Description desc;
        
        explicit Template() {}
        explicit Template(const std::string& content, 
                          const std::filesystem::path& path = "") 
            : content(content), path(path) {
        }

        bool empty() const {return root.nodes.empty();}
        bool operator==(const Template& other) const {
            return content == other.content;
        }
    };

    using TemplateStorage = std::map<std::string, Template>;

} // namespace Wizard