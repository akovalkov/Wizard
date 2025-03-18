#include <doctest/doctest.h>
#include <boost/json/value.hpp>
#include <algorithm>
namespace json = boost::json;

#include "helper.h"
#include "../library/Environment.h"

using namespace Wizard;

extern GlobalFixture fixture;


TEST_CASE("Simple environment") {
    
    Environment env;

    std::string template_text = "Simple text";
    json::value data = {};
    auto output = env.render(template_text, data);

    std::string test_output("Simple text");
    CHECK(output == test_output);
}


TEST_CASE("Environment with custom function") {
    
    Environment env;

    env.add_callback("argmax", [](Arguments& args) {
        if (args[0]->is_array()) {
            // numbers array 
            auto result = std::max_element(args[0]->as_array().begin(), args[0]->as_array().end(),
                [](const json::value& a, const json::value& b) { return a.as_int64() < b.as_int64(); });
            return std::distance(args[0]->as_array().begin(), result);
        } else {
            // list numbers
            auto result = std::max_element(args.begin(), args.end(),
                [](const json::value* a, const json::value* b) { return a->as_int64() < b->as_int64(); });
            return std::distance(args.begin(), result);
        }
    });

    std::string template_text = 
        "Max element: {{ argmax(array) }}\n"
        "Max element: {{ argmax([4, 2, 6]) }}\n"
        "Max element: {{ argmax(4, 2, 6) }}\n";
        json::value data = { {"array", {4,2,6}} };
    auto output = env.render(template_text, data);

    std::string test_output(
        "Max element: 2\n"
        "Max element: 2\n"
        "Max element: 2\n");
    CHECK(output == test_output);
}


TEST_CASE("Ealuate expression") {
    
    Environment env;
    json::value data = {
        {"host", "localhost"},
        {"name", "testdb"},
        {"tables", {
            {{"name", "country"}, {"id", true}, {"fields", {
                {{"name", "name"}, {"type", "string"}, {"required", true}, {"index", true}, {"unique", true}},
            }}},
            {{"name", "author"}, {"id", true}, {"fields", {
                {{"name", "first_name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "last_name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "birth_date"}, {"type", "date"}, {"required", true}, {"index", true}},
                {{"name", "country_id"}, {"type", "integer"}, {"required", true}, {"index", true}}
            }}},
            {{"name", "book"}, {"id", false}, {"fields", {
                {{"name", "name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "published"}, {"type", "date"}, {"required", true}, {"index", true}}
            }}},
            {{"name", "book_author"}, {"fields", {
                {{"name", "book_id"}, {"type", "integer"}, {"required", true}, {"index", true}},
                {{"name", "author_id"}, {"type", "integer"}, {"required", true}, {"index", true}}
            }}}
        }}
    };    

    std::string expression = R"(exists("id") and id)";
    std::vector<bool> tests{true, true, false, false};
    auto index = 0;
    for(const auto& table : data.at("tables").as_array()) {
        auto result = env.evaluate(expression, table);
        CHECK(result.is_bool());
        CHECK(result.as_bool() == tests[index++]);
    }

}
