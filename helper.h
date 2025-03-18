#pragma once
#include <ostream>
#include <string>
#include <boost/json/parse.hpp>
namespace json = boost::json;

void pretty_print(std::ostream& os, json::value const& jv, std::string* indent = nullptr);
