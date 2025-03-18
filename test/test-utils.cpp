#include <array>
#include <ranges>
#include <iostream>
#include <doctest/doctest.h>

#include "helper.h"
#include "../library/Util.h"

using namespace Wizard;

extern GlobalFixture fixture;

TEST_CASE("Find simple json values") {
    const json::value persons = {{"persons", {{{"fullname","John Doe"}, {"age", 25}},
                                   {{"fullname","Alexander Kovalkov"}, {"age", 50}},
                                   {{"fullname","Ivan Ivanov"}, {"age", 30}},
                                   {{"fullname","Anna Sergeeva"}, {"age", 16}}}}};
    auto values = boost::json::find_pointers(persons, "persons.age");
    std::array<int, 4> ages{25, 50, 30, 16};
    CHECK(values.size() == ages.size());
    for(auto [value, age] : std::views::zip(values, ages)){
        CHECK(value->is_int64());
        CHECK(value->as_int64() == age);
    }
}

TEST_CASE("Find simple json array") {
    const json::value persons = {{"persons", {{{"fullname","John Doe"}, {"age", 25}},
                                   {{"fullname","Alexander Kovalkov"}, {"age", 50}},
                                   {{"fullname","Ivan Ivanov"}, {"age", 30}},
                                   {{"fullname","Anna Sergeeva"}, {"age", 16}}}}};
    auto values = boost::json::find_pointers(persons, "persons");
    CHECK(values.size() == 1);
    CHECK(values[0]->is_array());
}

TEST_CASE("Find json values") {
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
    auto values = boost::json::find_pointers(data, "tables.fields.name");
    std::array<std::string_view, 9> names{"name", "first_name", "last_name", "birth_date", 
                                          "country_id", "name", "published", "book_id", "author_id"};
    CHECK(values.size() == names.size());
    for(auto [value, name] : std::views::zip(values, names)){
        CHECK(value->is_string());
        CHECK(value->as_string() == name);
    }
}