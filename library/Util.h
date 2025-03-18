#pragma once
#include <coroutine>
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <queue>
#include <boost/json/parse.hpp>
namespace json = boost::json;
#include "Exceptions.h"

namespace Wizard {

	namespace string_view {
		inline std::string_view slice(std::string_view view, size_t start, size_t end) {
			start = std::min(start, view.size());
			end = std::min(std::max(start, end), view.size());
			return view.substr(start, end - start);
		}

		inline std::pair<std::string_view, std::string_view> split(std::string_view view, char Separator) {
			size_t idx = view.find(Separator);
			if (idx == std::string_view::npos) {
				return std::make_pair(view, std::string_view());
			}
			return std::make_pair(slice(view, 0, idx), slice(view, idx + 1, std::string_view::npos));
		}

	} // namespace string_view

	inline SourceLocation get_source_location(std::string_view content, size_t pos)	{
		// Get line and offset position (starts at 1:1)
		auto sliced = string_view::slice(content, 0, pos);
		std::size_t last_newline = sliced.rfind("\n");

		if (last_newline == std::string_view::npos) {
			return {1, sliced.length() + 1};
		}

		// Count newlines
		size_t count_lines = 0;
		size_t search_start = 0;
		while (search_start <= sliced.size()) {
			search_start = sliced.find("\n", search_start) + 1;
			if(search_start == 0) {
				break;
			}
			count_lines += 1;
		}
		return {count_lines + 1, sliced.length() - last_newline};
	}

	inline void replace_substring(std::string& str, const std::string& from, const std::string& to){
		if(from.empty()){
			return;
		}
		for (auto pos = str.find(from);			    // find first occurrence of "from"
			 pos != std::string::npos;		        // make sure "from" was found
			 str.replace(pos, from.size(), to),	    // replace with "to", and
			 pos = str.find(from, pos + to.size())) // find next occurrence of "from"
		{}
	}

	inline std::string convert_dot_to_ptr(std::string_view ptr_name) {
		std::string result;
		do {
			std::string_view part;
			std::tie(part, ptr_name) = string_view::split(ptr_name, '.');
			result.push_back('/');
			result.append(part.begin(), part.end());
		} while (!ptr_name.empty());
		return result;
	}

	template <typename promise_type>
	struct owning_handle {
		owning_handle() : handle_() {}
		owning_handle(std::nullptr_t) : handle_(nullptr) {}
		owning_handle(std::coroutine_handle<promise_type> handle) : handle_(std::move(handle)) {}

		owning_handle(const owning_handle<promise_type>&) = delete;
		owning_handle(owning_handle<promise_type>&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

		owning_handle<promise_type>& operator=(const owning_handle<promise_type>&) = delete;
		owning_handle<promise_type>& operator=(owning_handle<promise_type>&& other) {
			if (this != &other) {
				handle_ = std::exchange(other.handle_, nullptr);
			}
			return *this;
		}

		promise_type& promise() const {
			return handle_.promise();
		}

		bool done() const {
			assert(handle_ != nullptr);
			return handle_.done();
		}

		void resume() const {
			assert(handle_ != nullptr);
			return handle_.resume();
		}

		std::coroutine_handle<> raw_handle() const {
			return handle_;
		}

		std::coroutine_handle<> detach() {
    	    return std::exchange(handle_, {});
	    }


		~owning_handle() {
			if (handle_ != nullptr)
				handle_.destroy();
		}
	private:
		std::coroutine_handle<promise_type> handle_;
	};


	template<typename T>
	struct Scanner
	{
		struct promise_type // required
		{
			using handle_t = std::coroutine_handle<promise_type>;
			T value_;
			std::exception_ptr exception_;

			// promise interface
			auto get_return_object()
			{
				return Scanner<T>{handle_t::from_promise(*this)};
			}
			std::suspend_always initial_suspend() { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			// exception
			void unhandled_exception() { exception_ = std::current_exception(); } // saving

            // intermediate value
			template<std::convertible_to<T> Arg> // C++20 concept
			std::suspend_always yield_value(Arg&& value)
			{
				value_ = std::forward<Arg>(value); 
				return {};
			}
			template<std::convertible_to<T> Arg> // C++20 concept
			void return_value(Arg&& value) {
				value_ = std::forward<Arg>(value);
			}
		};


		explicit Scanner(promise_type::handle_t h) : handle_(h) {}

		bool done() const { return handle_.done(); }

		T next() {
    		handle_.resume();
			return handle_.promise().value_;;
		}

		T value() const {
			return handle_.promise().value_;
		}
    private:
		owning_handle<promise_type> handle_;
	};


	struct Resumable
	{
		struct promise_type
		{
			using handle_t = std::coroutine_handle<promise_type>;
			std::exception_ptr exception_;

			auto get_return_object() { return handle_t::from_promise(*this); }
			std::suspend_always initial_suspend() { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			void return_void() {}
			void unhandled_exception() { exception_ = std::current_exception(); }
		};

		Resumable(promise_type::handle_t h) : handle_(h) {}
		bool resume()
		{
			if (!handle_.done())
				handle_.resume();
			return !handle_.done();
		}

		std::coroutine_handle<> detach()
		{
			return handle_.detach();
		}

	private:
		owning_handle<promise_type> handle_;
	};


	inline std::string read_file(const std::filesystem::path& filepath)
	{
		std::ifstream ifs(filepath);
		if (ifs.fail()) {
			std::stringstream ss;
			ss << "Couldn't open file: " << filepath;
			throw std::runtime_error(ss.str());
		}
		return {std::istreambuf_iterator<char>{ifs}, {}};
	}

	// find template description file
	inline std::filesystem::path get_template_description_file(const std::filesystem::path& filetpl,
											        		   const std::filesystem::path& descjson)
	{
		if(!descjson.empty()) {
			return descjson;
		}				
		if(filetpl.empty()) {
			return filetpl;
		}
		auto filepath = filetpl.parent_path();
		filepath /= filetpl.stem();
		filepath += ".json"; 
		return filepath; 
	}

	// compare property with value
	inline bool equal_property(json::value& object, const std::string& property, const std::string& value)
	{
		if(!object.is_object()){
			return false;
		}
		if(!object.as_object().if_contains(property)) {
			return false;
		}
		return object.as_object().at(property).as_string() == value;
	}

	// find object with specific property value in json
	inline json::object& find_object(json::value& root, const std::string& property, const std::string& value, json::object& defobj)
	{
		if(root.is_array()) {
			for(auto& object : root.as_array()) {
				if(equal_property(object, property, value)) {
					return object.as_object();
				}
			}
		} else if(equal_property(root, property, value)) {
			return root.as_object();
		}	
		return defobj;
	}

	inline json::object find_template_description(const std::filesystem::path& path, 
						 						  const std::filesystem::path& info)
	{
		auto filepath = get_template_description_file(path, info);
		if(filepath.empty()) {
			return {};
		}
		// read template description
		std::ifstream infile;
		infile.open(filepath);
		if(infile.fail()) {
			return {};
		}
		std::error_code ec;
		auto tpldesc = json::parse(infile, ec);
		if(ec) {
			throw FileError("Couldn't parse template description file: '" + filepath.string() + "'");
		}
		// find description by template name
		auto name = filepath.filename().stem().string();
		json::object defobj;
		return find_object(tpldesc, "template", name, defobj);
	}

	inline const json::object* find_field_description(const json::object& info, const std::string& path) 
	{
		if(path.contains(".")) {
			throw BaseError("data_error", "Not supported the dots in variable name: \"" + path + "\"");
		}
		const auto* vars = info.if_contains("variables");
		if(!vars || vars->is_array()) {
			return nullptr;
		}
		for(const auto& var : vars->as_array()) {
			const auto* name = var.as_object().if_contains("name");
			if(!name || name->as_string() != path) {
				continue;
			}
			return &var.as_object();
		}
		return nullptr;
	}

} // namespace Wizard

namespace boost {
	namespace json {
		// return json value array by path
		inline std::vector<boost::json::value*> find_pointers(boost::json::value& root_value, std::string_view ptr_name)
		{
			std::queue<boost::json::value*> q({&root_value});
			while(!ptr_name.empty()){
				std::string_view part;
				std::tie(part, ptr_name) = Wizard::string_view::split(ptr_name, '.');
				auto size = q.size();
				for(auto i = 0; i != size; ++i) {
					auto pvalue = q.front();
					q.pop();
					if(pvalue->is_array()) {
						for(auto& cvalue : pvalue->as_array()) {	
							if(cvalue.is_object() && cvalue.as_object().if_contains(part)){
								q.push(&cvalue.as_object().at(part));
							}
						}
					} else if(pvalue->is_object()){
						if(pvalue->as_object().if_contains(part)) {
							q.push(&pvalue->as_object().at(part));
						}
					}
				}
			}
			std::vector<boost::json::value*> values;
			values.reserve(q.size());
			while(!q.empty()) {
				values.push_back(q.front());
				q.pop();
			}
			return values;
		}

		inline std::vector<const boost::json::value*> find_pointers(const boost::json::value& root_value, std::string_view ptr_name)
		{
			std::queue<const boost::json::value*> q({&root_value});
			while(!ptr_name.empty()){
				std::string_view part;
				std::tie(part, ptr_name) = Wizard::string_view::split(ptr_name, '.');
				if (part.empty()) {
					// reference to self
					break;
				}
				auto size = q.size();
				for(auto i = 0; i != size; ++i) {
					auto pvalue = q.front();
					q.pop();
					if(pvalue->is_array()) {
						for(auto& cvalue : pvalue->as_array()) {	
							if(cvalue.is_object() && cvalue.as_object().if_contains(part)){
								q.push(&cvalue.as_object().at(part));
							}
						}
					} else if(pvalue->is_object()){
						if(pvalue->as_object().if_contains(part)) {
							q.push(&pvalue->as_object().at(part));
						}
					}
				}
			}
			std::vector<const boost::json::value*> values;
			values.reserve(q.size());
			while(!q.empty()) {
				values.push_back(q.front());
				q.pop();
			}
			return values;
		}

	}
}
