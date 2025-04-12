#include <sstream>
#include <string>
#include <doctest/doctest.h>
#include <boost/json/value.hpp>
namespace json = boost::json;

#include "helper.h"
#include "../library/Parser.h"
#include "../library/Renderer.h"

using namespace Wizard;

extern GlobalFixture fixture;


TEST_CASE("Render literal") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;


    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text = "Simple text";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer render(rconfig, templates, functions);
    
    std::stringstream ss;
    json::value data = {};
    render.render(ss, tpl, data);

    std::string test_output("Simple text");
    auto output = ss.str();
    CHECK(output == test_output);
}

TEST_CASE("Render data") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;


    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text = "\n"
        "Title: {{ title }}\n"
        "Worker: {{ person.name }}\n"
        "Age: {{ person.age }}\n"
        "Salary: {{ salary }}\n"
        "Address: {{ person.address.zipcode}} {{ person.address.city }}, {{ person.address.country }}\n"
        "\n"
        "page {{ page }}\n"
        "\n";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer render(rconfig, templates, functions);
    
    std::stringstream ss;
    json::value data = {
        {"title", "Information"},
        {"page", 42},
        {"salary", 12.34},
        {"person", {
            {"name", "Alex"},
            {"age", 50},
            {"address", {
                {"country", "Georgia"},
                {"city", "Kutaisi"},
                {"zipcode", 123456}
            }}
        }}
    };
    render.render(ss, tpl, data);

    std::string test_output = "\n"
        "Title: Information\n"
        "Worker: Alex\n"
        "Age: 50\n"
        "Salary: 12.34\n"
        "Address: 123456 Kutaisi, Georgia\n"
        "\n"
        "page 42\n"
        "\n";
    auto output = ss.str();
    CHECK(output == test_output);
}


TEST_CASE("Render set statement") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;


    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text = 
        "{% set new_hour=23 %}{{ new_hour }}pm\n"
        "{% set time.start=18 %}{{ time.start }}pm\n";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer renderer(rconfig, templates, functions);
    
    std::stringstream ss;
    json::value data = {};
    renderer.render(ss, tpl, data);

    std::string test_output = 
        "23pm\n"
        "18pm\n";
    auto output = ss.str();
    CHECK(output == test_output);
}


TEST_CASE("Render array loop statement") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;


    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text = 
        "{{ at(guests, 1) }}\n"
        "(Guest List:\n"
        "## for guest in guests\n"
        "   {{ loop.index1 }}: {{ guest }}\n"
        "## endfor\n"
        ")\n";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer renderer(rconfig, templates, functions);
    
    std::stringstream ss;
    json::value data = { {"guests", {"Jeff", "Tom", "Patrick"}} };
    renderer.render(ss, tpl, data);

    std::string test_output = 
        "Tom\n"
        "(Guest List:\n"
        "   1: Jeff\n"
        "   2: Tom\n"
        "   3: Patrick\n"
        ")\n";
    auto output = ss.str();
    CHECK(output == test_output);
}

TEST_CASE("Render object loop statement") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text =
        "Information:\n"
        "## for field, value in person\n"
        "{{ field }}: {{ value}}\n"
        "## endfor\n"
        "\n";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer renderer(rconfig, templates, functions);

    std::stringstream ss;
    json::value data = {
            {"person", {
                {"name", "Alex"},
                {"nickname", "Merz"},
                {"age", 50},
                {"address", "World"}
            }}
        };
    renderer.render(ss, tpl, data);

    std::string test_output =
        "Information:\n"
        "name: Alex\n"
        "nickname: Merz\n"
        "age: 50\n"
        "address: World\n"
        "\n";
    auto output = ss.str();
    CHECK(output == test_output);
}

TEST_CASE("Render nested loops statement") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;

    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text =
        "## for country in countries\n"
        "##     for city in country.cities\n"
        "##         for person in city.people\n"
        "{{ loop.parent.parent.index1 }}.{{ loop.parent.index1 }}.{{ loop.index }} {{ country.name }} {{ city.name }} {{ person }}\n"
        "##         endfor\n"
        "##     endfor\n"
        "## endfor\n";
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    Renderer renderer(rconfig, templates, functions);

    std::stringstream ss;
    json::value data = {
            {"countries", {
                {{"name", "Russia"}, {"cities", {
                    {{"name", "St.Petersburg"}, {"people", {"Koly", "Sasha", "Anna"}}},
                    {{"name", "Moskow"}, {"people", {"Vova", "Katya"}}}
                }}},
                {{"name", "Georgia"}, {"cities", {
                    {{"name", "Tbilisi"}, {"people", {"Grigol", "Evgeniy"}}},
                    {{"name", "Kutaisi"}, {"people", {"Natia", "George"}}},
                    {{"name", "Batumi"}, {"people", {"George"}}}
                }}},
                {{"name", "USA"}, {"cities", {
                    {{"name", "Boston"}, {"people", {"Dima", "Alexey"}}},
                    {{"name", "Los Angeles"}, {"people", {"Anton", "Tatyana"}}},
                    {{"name", "New York"}, {"people", {"Andrey"}}}
                }}}
            }}
        };
    renderer.render(ss, tpl, data);

    std::string test_output =
        "1.1.0 Russia St.Petersburg Koly\n"
        "1.1.1 Russia St.Petersburg Sasha\n"
        "1.1.2 Russia St.Petersburg Anna\n"
        "1.2.0 Russia Moskow Vova\n"
        "1.2.1 Russia Moskow Katya\n"
        "2.1.0 Georgia Tbilisi Grigol\n"
        "2.1.1 Georgia Tbilisi Evgeniy\n"
        "2.2.0 Georgia Kutaisi Natia\n"
        "2.2.1 Georgia Kutaisi George\n"
        "2.3.0 Georgia Batumi George\n"
        "3.1.0 USA Boston Dima\n"
        "3.1.1 USA Boston Alexey\n"
        "3.2.0 USA Los Angeles Anton\n"
        "3.2.1 USA Los Angeles Tatyana\n"
        "3.3.0 USA New York Andrey\n";
    auto output = ss.str();
    CHECK(output == test_output);
}

TEST_CASE("Render if/else statement") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text =
        "{% set numbers = [42.23, 151, 125] %}\n"
        "## for number in numbers\n"
        "##     if number < 100\n"
        "{{number}} < 100\n"
        "##     else if number >= 150\n"
        "{{number}} >= 150.10\n"
        "##     else\n"
        "{{number}} >= 100 and {{number}} < 150.10\n"
        "##     endif\n"
        "## endfor\n"
        "## if person.name < title\n"
        "{{person.age}}\n"
        "## endif\n";
        
	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
		{"title", "Information"},
		{"page", 42},
		{"salary", 12.34},
		{"person", {
			{"name", "Alex"},
			{"age", 50}
		}}
	};
	renderer.render(ss, tpl, data);

	std::string test_output = "\n"
        "42.23 < 100\n"
        "151 >= 150.10\n"
        "125 >= 100 and 125 < 150.10\n"
        "50\n";
    auto output = ss.str();
	CHECK(output == test_output);

}

TEST_CASE("Render arithmetic") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;

	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
		"{% set number = 10 %}"
        "{{ number + 10 }}\n"
        "{{ number - 10 }}\n"
        "{{ number / 10 }}\n"
        "{{ number * 10 }}\n"
        "{{ number + 10 * 100 }}\n"
        "{{ number % 9 }}\n"
        "{{ number ^ 3 }}\n"
		"{% set number = 10.10 %}"
        "{{ number + 10 }}\n"
        "{{ number - 10 }}\n"
        "{{ number / 10 }}\n"
        "{{ number * 10 }}\n"
        "{{ number + 10 * 100 }}\n"
        "{% set string = \"Hello\" %}"
        "{{ string + \" World!\"}}\n"
        "{{ odd(42) }}\n"
        "{{ even(42) }}\n"
        "{{ divisibleBy(42, 7) }}\n"
        "{{ max([1, 2, 3]) }}\n"
        "{{ min([-2.4, -1.2, 4.5]) }}\n"
        "{{ round(3.1415, 0) }}\n"
        "{{ round(3.1415, 3) }}\n"
        "{{ int(\"2\") == 2 }}\n"
        "{{ float(\"1.8\") > 2 }}\n"
        ;
	Template tpl = parser.parse(template_text);

    RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {};
	renderer.render(ss, tpl, data);

	std::string test_output = 
        "20\n"
        "0\n"
        "1\n"
        "100\n"
        "1010\n"
        "1\n"
        "1000\n"
        "20.1\n"
        "0.1\n"
        "1.01\n"
        "101\n"
        "1010.1\n"
        "Hello World!\n"
        "0\n"
        "1\n"
        "1\n"
        "3\n"
        "-2.4\n"
        "3\n"
        "3.142\n"
        "1\n"
        "0\n";
    auto output = ss.str();
	CHECK(output == test_output);

}


TEST_CASE("Render access functions") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{% set field = \"name\" %}"
        "{{ at(company, field) }}\n"
        "{{ at(company, \"salary\") }}\n"
        "{% set index = 1 %}"
        "{{ at(company.persons, index) }}\n"
        "{{ exists(\"company.address\") }}\n"
        "{{ exists(\"company.name\") }}\n"
        "{{ existsIn(company, field) }}\n"
        "{{ existsIn(company, \"test\") }}\n"
        "{{ default(company.name, \"test\") }}\n"
        "{{ default(company.lastname, \"test\") }}\n"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"company", {
            {"name", "MASH"},
            {"page", 42},
            {"salary", 12.34},
            {"persons", {"Alex", "Dima", "Georgiy"}}
	    }}
    };
	renderer.render(ss, tpl, data);

	std::string test_output =
        "MASH\n"
        "12.34\n"
        "Dima\n"
        "0\n"
        "1\n"
        "1\n"
        "0\n"
        "MASH\n"
        "test\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);

}


TEST_CASE("Render array function") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{{ first(persons) }}\n"
        "{{ last(persons) }}\n"
        "{{ length(persons) }}\n"
        "{{ sort(persons) }}\n"
        "{{ join(persons, \" - \") }}\n"
        "{{ split(persons_str, \", \") }}\n"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"persons", {"Alex", "Dima", "Georgiy", "Misha", "Anna", "Tanya"}},
        {"persons_str", "Alex, Dima, Georgiy, Misha, Anna, Tanya"}
    };
	renderer.render(ss, tpl, data);

	std::string test_output =
        "Alex\n"
        "Tanya\n"
        "6\n"
        "[\"Alex\",\"Anna\",\"Dima\",\"Georgiy\",\"Misha\",\"Tanya\"]\n"
        "Alex - Dima - Georgiy - Misha - Anna - Tanya\n"
        "[\"Alex\",\"Dima\",\"Georgiy\",\"Misha\",\"Anna\",\"Tanya\"]\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);
}


TEST_CASE("Render range function") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{% for i in range(4) %}{{ loop.index1 }}:{{ i }}\n{% endfor %}"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {};
	renderer.render(ss, tpl, data);

	std::string test_output =
        "1:0\n"
        "2:1\n"
        "3:2\n"
        "4:3\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);

}


TEST_CASE("Render string functions") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{{ upper(at(persons, 0)) }}\n"
        "{{ lower(at(persons, 1)) }}\n"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"persons", {"Alex", "Dima", "georgiy"}}
    };
	renderer.render(ss, tpl, data);

	std::string test_output =
        "ALEX\n"
        "dima\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);

}

TEST_CASE("Render check type") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{{ isObject(company) }}\n"
        "{{ isString(company.name) }}\n"
        "{{ isNumber(company.name) }}\n"
        "{{ isNumber(company.price) }}\n"
        "{{ isFloat(company.price) }}\n"
        "{{ isBoolean(company.check) }}\n"
        "{{ isInteger(company.code) }}\n"
        "{{ isArray(company.persons) }}\n"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"company", {
            {"name", "Microsoft"},
            {"code", 123},
            {"price", 43.33},
            {"check", true},
            {"persons", {"Alex", "Dima", "georgiy"}}
        }}
    };
	renderer.render(ss, tpl, data);

	std::string test_output =
        "1\n"
        "1\n"
        "0\n"
        "1\n"
        "1\n"
        "1\n"
        "1\n"
        "1\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);
}

TEST_CASE("Render file structure (dry)") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;


	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{% file company.name + \".txt\" %}"
        "{{ company.name }}\n"
        "{% endfile %}"
        "{% for person in company.persons %}"
        "{% file company.name + \"\\\\\" + person + \".tbl\" %}"
        "{{ loop.index }}:{{person}}\n"
        "{% endfile %}"
        "{% endfor%}"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
	rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"company", {
            {"name", "Microsoft"},
            {"persons", {"Alex", "Dima", "Vitaly"}}
        }}
    };
	renderer.render(ss, tpl, data);

	std::string test_output =
        ">>>>>> Start file: \"Microsoft.txt\"\n"
        "Microsoft\n"
        "<<<<<< End file: \"Microsoft.txt\"\n"
        ">>>>>> Start file: \"Microsoft\\\\Alex.tbl\"\n"
        "0:Alex\n"
        "<<<<<< End file: \"Microsoft\\\\Alex.tbl\"\n"
        ">>>>>> Start file: \"Microsoft\\\\Dima.tbl\"\n"
        "1:Dima\n"
        "<<<<<< End file: \"Microsoft\\\\Dima.tbl\"\n"
        ">>>>>> Start file: \"Microsoft\\\\Vitaly.tbl\"\n"
        "2:Vitaly\n"
        "<<<<<< End file: \"Microsoft\\\\Vitaly.tbl\"\n"
        ;
	auto output = ss.str();
	CHECK(output == test_output);

}


TEST_CASE("Render file structure (real)") {
	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;

	Parser parser(pconfig, lconfig, templates, functions);
	std::string template_text =
        "{% file company.name + \".txt\" %}"
        "{{ company.name }}\n"
        "{% endfile %}"
        "{% for person in company.persons %}"
        "{% file company.name + \"\\\\\" + person + \".tbl\" %}"
        "{{ loop.index }}:{{person}}\n"
        "{% endfile %}"
        "{% endfor%}"
        ;

	Template tpl = parser.parse(template_text);


	RenderConfig rconfig;
    rconfig.output_dir = "output-dir";
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"company", {
            {"name", "Microsoft"},
            {"persons", {"Alex", "Dima", "Vitaly"}}
        }}
    };
	renderer.render(ss, tpl, data);

    // check output
	std::string test_output;
	auto output = ss.str();
	CHECK(output == test_output);
    // check files
    std::vector<std::string> files;
    for (const auto& dir_entry :
        std::filesystem::recursive_directory_iterator(rconfig.output_dir)) {
        files.push_back(dir_entry.path().string());
    }
    std::vector<std::string> test_files{
        filesystem::path::normalize_separators("output-dir\\Microsoft"),
        filesystem::path::normalize_separators("output-dir\\Microsoft\\Alex.tbl"),
        filesystem::path::normalize_separators("output-dir\\Microsoft\\Dima.tbl"),
        filesystem::path::normalize_separators("output-dir\\Microsoft\\Vitaly.tbl"),
        filesystem::path::normalize_separators("output-dir\\Microsoft.txt")
    };
    CHECK(files == test_files);

    std::filesystem::remove_all(rconfig.output_dir);

}

TEST_CASE("Render variable test") {
    LexerConfig lconfig;
    ParserConfig pconfig;
    TemplateStorage templates;
    FunctionStorage functions;


    Parser parser(pconfig, lconfig, templates, functions);
    std::string template_text = 
            "{{ variable }}\n"
            "{% if variable %}declared{% else %}undeclared{% endif %}\n"
            "{% if not variable %}undeclared{% else %}declared{% endif %}\n"
        ;
    Template tpl = parser.parse(template_text);


    RenderConfig rconfig;
    rconfig.dry_run = true;
    rconfig.strict = false;
    Renderer renderer(rconfig, templates, functions);

    // optional variable declaration    
    std::stringstream ss;
    json::value data = {};
    renderer.render(ss, tpl, data);

    std::string test_output = "\nundeclared\nundeclared\n";
    auto output = ss.str();
    CHECK(output == test_output);

    // required variable declaration    
    ss.str("");
    rconfig.strict = true;
    CHECK_THROWS_AS(renderer.render(ss, tpl, data), RenderError);

}


TEST_CASE("Render apply template") {

	LexerConfig lconfig;
	ParserConfig pconfig;
	TemplateStorage templates;
	FunctionStorage functions;

    lconfig.templates_dir = fixture.templatesDir;
    Parser parser(pconfig, lconfig, templates, functions);
    std::filesystem::path template_name = "sql\\DatabaseSchema.tpl";
	Template tpl = parser.parse_file(template_name);

	RenderConfig rconfig;
    rconfig.dry_run = true;
	Renderer renderer(rconfig, templates, functions);

	std::stringstream ss;
	json::value data = {
        {"host", "localhost"},
        {"name", "testdb"},
        {"idtables", {
            {{"name", "country"}, {"fields", {
                {{"name", "name"}, {"type", "string"}, {"required", true}, {"index", true}, {"unique", true}},
            }}},
            {{"name", "author"}, {"fields", {
                {{"name", "first_name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "last_name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "birth_date"}, {"type", "date"}, {"required", true}, {"index", true}},
                {{"name", "country_id"}, {"type", "integer"}, {"required", true}, {"index", true}}
            }}},
            {{"name", "book"}, {"fields", {
                {{"name", "name"}, {"type", "string"}, {"required", true}, {"index", true}},
                {{"name", "published"}, {"type", "date"}, {"required", true}, {"index", true}}
            }}}
        }},
        {"tables", {
            {{"name", "book_author"}, {"fields", {
                {{"name", "book_id"}, {"type", "integer"}, {"required", true}, {"index", true}},
                {{"name", "author_id"}, {"type", "integer"}, {"required", true}, {"index", true}}
            }}}
        }}
    };
	renderer.render(ss, tpl, data);

    // check output
	std::string test_output =
    R"(>>>>>> Start file: "db.sql"
-- phpMyAdmin SQL Dump
-- version 3.3.8
-- http://www.phpmyadmin.net
--
-- Host: localhost

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Database: `testdb`
--

--
-- Table `session`
--

CREATE TABLE IF NOT EXISTS `session` (
  `id` char(32) NOT NULL DEFAULT '',
  `name` char(32) NOT NULL DEFAULT '',
  `modified` int(11) DEFAULT NULL,
  `lifetime` int(11) DEFAULT NULL,
  `data` text,
  PRIMARY KEY (`id`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Table `book_author`
--
CREATE TABLE IF NOT EXISTS `book_author` (
  `book_id` int(11) unsigned NOT NULL KEY `idx_book_id` (`book_id`),
  `author_id` int(11) unsigned NOT NULL KEY `idx_author_id` (`author_id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------
--
-- Table `country`
--
CREATE TABLE IF NOT EXISTS `country` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL UNIQUE KEY `unique_name` (`name`) KEY `idx_name` (`name`),
  `date_created` datetime NOT NULL,
  `date_updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------

--
-- Table `author`
--
CREATE TABLE IF NOT EXISTS `author` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `first_name` varchar(255) NOT NULL KEY `idx_first_name` (`first_name`),
  `last_name` varchar(255) NOT NULL KEY `idx_last_name` (`last_name`),
  `birth_date` date NOT NULL KEY `idx_birth_date` (`birth_date`),
  `country_id` int(11) unsigned NOT NULL KEY `idx_country_id` (`country_id`),
  `date_created` datetime NOT NULL,
  `date_updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------

--
-- Table `book`
--
CREATE TABLE IF NOT EXISTS `book` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL KEY `idx_name` (`name`),
  `published` date NOT NULL KEY `idx_published` (`published`),
  `date_created` datetime NOT NULL,
  `date_updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------

<<<<<< End file: "db.sql"
)"; 
	auto output = ss.str();
	CHECK(output == test_output);

}

