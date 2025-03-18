#include <doctest/doctest.h>

#include "helper.h"
#include "../library/Project.h"

using namespace Wizard;

extern GlobalFixture fixture;

TEST_CASE("Project load test") {
    auto projectFile = fixture.dataDir / "project.json";
    Project project;
    project.init(projectFile);
    CHECK(project.name == "WebProject");
    CHECK(project.description == "Test web project");
    CHECK(project.info == "info.json");
    CHECK(project.modules.size() == 1);
    CHECK(project.modules[0].name == "DatabaseSchema");
    std::vector<JsonTransformer::Rule> jtransform{{
                                                    {"name", Template{}, Template{}, {}, {}},
                                                    {"host", Template{}, Template{}, {}, {}},
                                                    {"models", Template{"{{id}}"}, Template{}, "idtables", {}},
                                                    {"models", Template{"{{not id}}"}, Template{}, "tables", {}}
                                                  }};
    CHECK(project.modules[0].transformer.rules() == jtransform);
}

TEST_CASE("Project transform test") {
    auto projectFile = fixture.dataDir / "project.json";
    auto dataFile = fixture.dataDir / "books.json";
    auto data = fixture.parse(dataFile);
    Project project;
    project.init(projectFile);
    auto newdata = project.modules[0].transformer.transform(data);
     CHECK(newdata.is_object());
     auto newdata_obj = newdata.as_object();
     CHECK(newdata_obj.if_contains("name"));
     CHECK(newdata_obj.at("name").is_string());
     CHECK(newdata_obj.if_contains("host"));
     CHECK(newdata_obj.at("host").is_string());
     CHECK(newdata_obj.if_contains("idtables"));
     CHECK(newdata_obj.at("idtables").is_array());
     CHECK(newdata_obj.if_contains("tables"));
     CHECK(newdata_obj.at("tables").is_array());
}
