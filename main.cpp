
#include <filesystem>
#include <boost/program_options.hpp>
#include <clocale>
#include "library/Environment.h"
#include "library/Project.h"
#include "helper.h"
namespace po = boost::program_options;

//print raw template description
void print_description(const Wizard::Description& desc) {
	std::cout << desc.name << std::endl;
	std::cout << desc.description << std::endl;
	// variables
	std::cout << "Variables:" << std::endl;
	for(const auto& [name, var] : desc.variables) {
		std::cout << "\t" << var.name << std:: endl;;
		if(var.type != Wizard::Variable::Type::Null) {
			std::cout << "\t" << Wizard::Description::type_to_string(var.type) << std:: endl;;
		}
		if(var.required) {
			std::cout << "\t(required)" << std::endl;
		}
		std::cout << " - " << var.description;
		if(!var.defvalue.is_null()) {
			std::cout << " (default "<< var.defvalue.at("default").as_string().c_str() << ")";
		}
	}	
	// nested templates
	if(!desc.nested.empty()) {
		std::cout << "Nested templates:" << std::endl;
		for(const auto& tpl_name : desc.nested) {
			std::cout << "\t" << tpl_name << std::endl;
		}
	}
}

//show template description
int template_description(Wizard::Environment& env, 
	  				     const std::filesystem::path& filetpl,
						 const std::filesystem::path& descjson)
{
	// teamplate name
	auto name = filetpl.filename().stem().string();
	//load json file with template description
	auto jsonpath = Wizard::get_template_description_file(filetpl, descjson);
	try {
		Wizard::Description desc = Wizard::Description::load_from_json(name, filetpl);
		print_description(desc);
	} catch(Wizard::FileError& ferr) {
		// Show information from template
		std::cout << "Couldn't open template description file: " << std::quoted(jsonpath.string()) << std::endl;
		std::cout << "Show raw information from the " << std::quoted(name) << " template" << std::endl << std::endl;
		Wizard::Description desc = env.description_from_file(filetpl);
		print_description(desc);
	}
	return 0;
}

// input object field value
void input_property(json::object& object, const std::string& property, json::kind prtype)
{
	std::string new_value;
	json::value old_value;
	if(object.if_contains(property)) {
		old_value = object[property];
		std::cout << "Old " << property << ": " << old_value << std::endl;
	}
	std::cout << "Input " << property << "(" << json::to_string(prtype) << "): " << std::endl;
	std::getline(std::cin, new_value);
	if(new_value.empty()) {
		// TODO remove field or keep old value
		return;
	}
	switch (prtype)
	{
	case json::kind::bool_: {
		// boolean
			bool bvalue;
			std::istringstream(new_value) >> std::boolalpha >> bvalue;
			object[property] = bvalue;
		}
		break;
	case json::kind::int64: {
		// signed int
			long lvalue;
			std::istringstream(new_value) >> lvalue;
			object[property] = lvalue;
		}
		break;
	case json::kind::uint64: {
		// unsgined int
			unsigned long ulvalue;
			std::istringstream(new_value) >> ulvalue;
			object[property] = ulvalue;
		}
		break;
	case json::kind::double_: {
		// double
			double dvalue;
			std::istringstream(new_value) >> dvalue;
			object[property] = dvalue;
		}
		break;
	default: 
		// string 
		object[property] = new_value;
		break;
	}
}

// create template description
int create_template_description(Wizard::Environment& env, 
							    const std::filesystem::path& filetpl,
								const std::filesystem::path& infodat)
{
	// find template description
	auto name = filetpl.filename().stem().string();
	auto old_desc = json::value(json::array_kind);
	//load json file with template description
	auto filepath = Wizard::get_template_description_file(filetpl, infodat);
	Wizard::Description desc = Wizard::Description::load_from_json(name, filepath);
	// find template description
	json::object newtpl;
	auto& template_obj = Wizard::find_object(old_desc, "template", name, newtpl);
	bool is_new = template_obj.empty();

	Wizard::Description new_desc = env.description_from_file(filetpl);
	// name
	std::cout << "Template: " << name << std::endl;
	template_obj["template"] = name;
	// description
	input_property(template_obj, "description", json::kind::string);
	std::cout << std::endl;
	// variables
	json::array variables;
	// create/update variables
	for(const auto& [name, var] : new_desc.variables) {
		json::object newvar;
		auto& var_obj = Wizard::find_object(template_obj["variables"], "name", var.name, newvar);
		// name
		std::cout << "Variable: " << var.name << std::endl;
		var_obj["name"] = var.name;
		// description
		input_property(var_obj, "description", json::kind::string);
		// type
		input_property(var_obj, "type", json::kind::string);
		// required
		input_property(var_obj, "required", json::kind::bool_);
		// default
		input_property(var_obj, "default", json::kind::string);
		// add new variable
		variables.push_back(var_obj);
		std::cout << std::endl;
	}
	template_obj["variables"] = variables;
	// nested templates
	template_obj["templates"] = json::value(json::array_kind);
	for(const auto& tpl_name : new_desc.nested) {
		template_obj["templates"].as_array().push_back(tpl_name.c_str());
	}
	if(is_new) {
		old_desc.as_array().push_back(template_obj);
	}
	// write output json file
	std::ofstream ofile;
	ofile.open(filepath.string()); 
	if(ofile.fail()) {
		std::cerr << "Couldn't write output json file: " << std::quoted(filepath.string()) << std::endl;
		return 1;
	}
	pretty_print(ofile, old_desc);
	return 0;
}

// check data by template description

// render template
int render_template(Wizard::Environment& env,
				    const std::filesystem::path& filetpl,
				    const std::filesystem::path& fileinfo,
					const std::filesystem::path& filedata)
{
	// read json data
	std::ifstream jsonfile;
	jsonfile.open(filedata);
	if(jsonfile.fail()) {
		std::cerr << "Couldn't open JSON data file: " << std::quoted(filedata.string()) <<  std::endl;
		return 1;		
	}
	// parse json
	std::error_code ec;
	json::value data = json::parse(jsonfile, ec);
	if(ec) {
		std::cerr << ec.message()<<  std::endl;
		return 1;		
	}
	try{
		// set templates directory (search nested templates)
		std::filesystem::path tpldir = filetpl.parent_path();
		env.set_template_directory(tpldir);
		// render template
		auto result = env.render_file(filetpl.filename(), data, fileinfo);
		// output render result if the dry run is set
		if(env.is_dry_run()) {
			std::cout << result << std::endl;
		}
	} catch(Wizard::BaseError& err) {
		std::cerr << err.what() <<  std::endl;
		return 1;		
	}
	return 0;
}

int main(int argc, const char* argv[])
{
	std::setlocale(LC_ALL, "");
	auto cdir = std::filesystem::current_path();
	// Declare the supported options.
	std::string infopath;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("file,f", po::value<std::string>(), "input template file")
		("data,d", po::value<std::string>(), "input JSON data file")
		("info,i", po::value<std::string>()->implicit_value(""), "template description (from json file)")
		("create-info,c", po::value<std::string>()->implicit_value(""), "create/update template description (into json file)")
		("output,o", po::value<std::string>(), "output directory")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || !vm.count("file")) {
		std::cerr << desc << std::endl;
		return 1;
	}
	// template file
	std::filesystem::path filetpl = vm["file"].as<std::string>();
	Wizard::Environment env;
	// show template info
	if(vm.count("info") && !vm.count("data")) {
		std::filesystem::path infodat = vm["info"].as<std::string>();
		try{
			template_description(env, filetpl, infodat);
		} catch(Wizard::BaseError& err) {
			std::cerr << err.what() <<  std::endl;
			return 1;		
		}
		return 0;
	}
	// create template info
	if(vm.count("create-info")) {
		std::filesystem::path infodat = vm["create-info"].as<std::string>();
		try{
			create_template_description(env, filetpl, infodat);
		} catch(Wizard::BaseError& err) {
			std::cerr << err.what() <<  std::endl;
			return 1;		
		}
		return 0;
	}
	// json data file
	if(!vm.count("data")) {
		std::cerr << "Please specify JSON data file (-d,--data)" << std::endl;
		std::cerr << desc << std::endl;
		return 1;
	}
	// render template
	std::filesystem::path infodat;
	if(vm.count("info"))
	 	infodat = vm["info"].as<std::string>();
	std::filesystem::path filedata = vm["data"].as<std::string>();
	if(vm.count("output")) {
		std::filesystem::path outputdir = vm["output"].as<std::string>();
		env.set_output_dir(outputdir);
	} else {
		env.set_dry_run(true); // test rendering
	}
    return render_template(env, filetpl, infodat, filedata);
}
