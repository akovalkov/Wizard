#include <array>
#include <doctest/doctest.h>

#include "helper.h"
#include "../library/JsonTransformer.h"

using namespace Wizard;

extern GlobalFixture fixture;

TEST_CASE("Load json rules") {
    std::string jrules = R"([{"from": "person", "filter": "age <= 25", "rules": [
                             {"expr": "at(split(fullname, \" \"), 0) ", "to": "first_name"}, 
                             {"expr": "at(split(fullname, \" \"), 1) ", "to": "last_name"}, 
                             {"from": "age"}
                            ]}])";
    JsonTransformer jt;
    jt.init(jrules);
    auto rules = jt.rules();
    std::vector<JsonTransformer::Rule> expected{{
                                                 {"person", Template{"{{age <= 25}}"}, Template{}, {}, {
                                                 {{}, Template{}, Template{"{{at(split(fullname, \" \"), 0) }}"}, "first_name", {}},
                                                 {{}, Template{}, Template{"{{at(split(fullname, \" \"), 1) }}"}, "last_name", {}},
                                                 {"age", Template{""}, Template{}, {}, {}}}}
                                                }};
    CHECK(rules == expected);
}

 TEST_CASE("Simple json transformation") {
     std::string jrules = R"([{"from": "person.fullname", "to": "fullname"}, {"from": "person.age", "to": "age"}])";
     JsonTransformer jt;
     jt.init(jrules);
     std::string json = R"([{"person":[
                            {"fullname":"John Doe", "age": 25},
                            {"fullname":"Alexander Kovalkov", "age": 50}
                        ]}])";
     std::string expected = R"({"fullname":["John Doe","Alexander Kovalkov"],"age":[25,50]})";
     auto result = jt.transform(json);
     CHECK(result == expected);
}

TEST_CASE("Simple json transformation 2") {
    std::string jrules = R"([{"from": "name", "to": "person.name"}, {"from": "age", "to": "person.age"}])";
    JsonTransformer jt;
    jt.init(jrules);
    std::string json = R"([{"name":"John Doe","age":25},{"name":"Alexander Kovalkov", "age": 50}])";
    std::string expected = R"({"person":{"name":["John Doe","Alexander Kovalkov"],"age":[25,50]}})";
    auto result = jt.transform(json);
    CHECK(result == expected);
}

TEST_CASE("Complex json transformation") {
    std::string jrules = R"([{"from": "person", "filter": "age <= 25", "rules": [
                            {"expr": "at(split(fullname, \" \"), 0) ", "to": "first_name"}, 
                            {"expr": "at(split(fullname, \" \"), 1) ", "to": "last_name"}, 
                            {"from": "age"}
                        ]}])";
    JsonTransformer jt;
    jt.init(jrules);
    std::string json = R"([{"person":[
                            {"fullname":"John Doe", "age": 25},
                            {"fullname":"Alexander Kovalkov", "age": 50}
                        ]}])";
    std::string expected = R"({"person":[{"first_name":"John","last_name":"Doe","age":25}]})";
    auto result = jt.transform(json);
    CHECK(result == expected);
}

 