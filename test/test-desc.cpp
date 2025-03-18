#include <array>
#include <doctest/doctest.h>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
namespace json = boost::json;

#include "helper.h"
#include "../library/Environment.h"

using namespace Wizard;

extern GlobalFixture fixture;


TEST_CASE("Description from json") {
    json::object data = {
        {"template", "DatabaseSchema"},
        {"description", "Database schema description"},
        {"variables", {
            {
                {"name", "host"},
                {"description", "hostname"},
                {"type", "string"},
                {"required", true},
                {"default", "localhost"}
            },
            {
                {"name", "name"},
                {"description", "Database name"},
                {"type", "string"},
                {"required", true}
            },
            {
                {"name", "tables"},
                {"description", "Tables in free form"},
                {"type", "array"},
                {"required", false}
            },
            {
                {"name", "idtables"},
                {"description", "Tables with id, date_created, date_updated fields"},
                {"type", "array"},
                {"required", false}
            },
            {
                {"name", "person"},
                {"description", "Person description"},
                {"type", "object"},
                {"required", false},
                {"variables", {
                    {
                        {"name", "firstname"},
                        {"description", "First name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "lastname"},
                        {"description", "Last name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "age"},
                        {"description", "Age"},
                        {"type", "integer"},
                        {"required", true},
                        {"default", 16}
                    },
                    {
                        {"name", "salary"},
                        {"description", "Salary"},
                        {"type", "double"},
                        {"required", false},
                        {"default", 42.42}
                    },
                    {
                        {"name", "hobby"},
                        {"description", "Hobbies list"},
                        {"type", "array"},
                        {"required", false}
                    }
                }}
            }
        }},
        {"templates", 
            {"TableSchema", "TableSchemaId"}
        }
    };
    Description desc = Description::load_from_json(data);
    CHECK(desc.name == "DatabaseSchema");
    CHECK(desc.description == "Database schema description");
    CHECK(desc.variables.size() == 5);
    std::array<Variable, 5> test_variables = {{
        {"host", "hostname", Variable::Type::String, true, json::value("localhost")},
        {"idtables", "Tables with id, date_created, date_updated fields", Variable::Type::Array, false},
        {"name", "Database name", Variable::Type::String},
        {"person", "Person description", Variable::Type::Object, false, json::value(nullptr), {
            {"age", {"age", "Age", Variable::Type::Integer, true, json::value(16)}},
            {"firstname", {"firstname", "First name", Variable::Type::String}},
            {"hobby", {"hobby", "Hobbies list", Variable::Type::Array, false}},
            {"lastname", {"lastname", "Last name", Variable::Type::String}},
            {"salary", {"salary", "Salary", Variable::Type::Double, false, json::value(42.42)}}
        }},
        {"tables", "Tables in free form", Variable::Type::Array, false}
    }};
    auto i = 0;
    for(const auto& [name, variable] : desc.variables) {
        CHECK(test_variables[i++] == variable);
    }
    CHECK(desc.nested.size() == 2);
    std::array<std::string, 2> test_nested = {"TableSchema", "TableSchemaId"}; 
    i = 0;
    for(const auto& name : desc.nested) {
        CHECK(name == test_nested[i++]);
    }

}


TEST_CASE("Json from description") {
    Description desc;
    desc.name = "DatabaseSchema";
    desc.description = "Database schema description";
    desc.variables = {{
        {"host", {"host", "hostname", Variable::Type::String, true, json::value("localhost")}},
        {"idtables", {"idtables", "Tables with id, date_created, date_updated fields", Variable::Type::Array, false}},
        {"name", {"name", "Database name", Variable::Type::String}},
        {"person", {"person", "Person description", Variable::Type::Object, false, json::value(nullptr), {
            {"age", {"age", "Age", Variable::Type::Integer, true, json::value(16)}},
            {"firstname", {"firstname", "First name", Variable::Type::String}},
            {"hobby", {"hobby", "Hobbies list", Variable::Type::Array, false}},
            {"lastname", {"lastname", "Last name", Variable::Type::String}},
            {"salary", {"salary", "Salary", Variable::Type::Double, false, json::value(42.42)}}
        }}},
        {"tables",{"tables", "Tables in free form", Variable::Type::Array, false}}
    }};
    desc.nested = {"TableSchema", "TableSchemaId"};
    auto data = desc.create_json_object();

    json::object test_data = {
        {"template", "DatabaseSchema"},
        {"description", "Database schema description"},
        {"variables", {
            {
                {"name", "host"},
                {"description", "hostname"},
                {"type", "string"},
                {"required", true},
                {"default", "localhost"}
            },
            {
                {"name", "idtables"},
                {"description", "Tables with id, date_created, date_updated fields"},
                {"type", "array"},
                {"required", false}
            },
            {
                {"name", "name"},
                {"description", "Database name"},
                {"type", "string"},
                {"required", true}
            },
            {
                {"name", "person"},
                {"description", "Person description"},
                {"type", "object"},
                {"required", false},
                {"variables", {
                    {
                        {"name", "age"},
                        {"description", "Age"},
                        {"type", "integer"},
                        {"required", true},
                        {"default", 16}
                    },
                    {
                        {"name", "firstname"},
                        {"description", "First name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "hobby"},
                        {"description", "Hobbies list"},
                        {"type", "array"},
                        {"required", false}
                    },
                    {
                        {"name", "lastname"},
                        {"description", "Last name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "salary"},
                        {"description", "Salary"},
                        {"type", "double"},
                        {"required", false},
                        {"default", 42.42}
                    }
                }}
            },
            {
                {"name", "tables"},
                {"description", "Tables in free form"},
                {"type", "array"},
                {"required", false}
            }
        }},
        {"templates", 
            {"TableSchema", "TableSchemaId"}
        }
    };

    CHECK(data == test_data);
}


TEST_CASE("Render with description") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text =
        "Information:\n"
        "{{person.firstname}} {{person.lastname}}\n"
        "{{person.address}}\n"
        "{{person.age}} year\n"
        "{{person.salary}}$\n"
        "## for hobby_name in person.hobby\n"
        "{{ hobby_name }}\n"
        "## endfor\n";

    json::object requirements = {
        {"template", "Test"},
        {"description", "Test requirements"},
        {"variables", {
            {
                {"name", "person"},
                {"description", "Person description"},
                {"type", "object"},
                {"required", false},
                {"variables", {
                    {
                        {"name", "firstname"},
                        {"description", "First name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "lastname"},
                        {"description", "Last name"},
                        {"type", "string"},
                        {"required", true}
                    },
                    {
                        {"name", "age"},
                        {"description", "Age"},
                        {"type", "integer"},
                        {"required", true},
                        {"default", 16}
                    },
                    {
                        {"name", "salary"},
                        {"description", "Salary"},
                        {"type", "double"},
                        {"required", false},
                        {"default", 42.42}
                    },
                    {
                        {"name", "hobby"},
                        {"description", "Hobbies list"},
                        {"type", "array"},
                        {"required", false}
                    }
                }}
            }
        }}
    };

    Template tpl = parser.parse(template_text);
    tpl.desc = Description::load_from_json(requirements);

    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer renderer(rconfig, templates, functions);

    std::stringstream ss;
    json::value data = {
            {"person", {
                {"firstname", "Ivan"},
                {"lastname", "Ivanov"},
                {"age", "50"},
                {"address", "World"},
                {"hobby", "Games"}
            }}
        };
    renderer.render(ss, tpl, data);
    auto output = ss.str();
    std::string test_output =
        "Information:\n"
        "Ivan Ivanov\n"
        "World\n"
        "50 year\n"
        "42.42$\n"
        "Games\n";
    //std::cout << output << std::endl;
    CHECK(output == test_output);
}