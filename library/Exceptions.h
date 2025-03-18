#pragma once
#include <string>
#include <stdexcept>

namespace Wizard
{
	// Error handling staff
	struct SourceLocation {
		size_t line{0};
		size_t column{0};
	};

    struct BaseError : public std::runtime_error
    {
        const std::string type;
        const std::string message;

        const SourceLocation location;

        explicit BaseError(const std::string &type, const std::string &message)
            : std::runtime_error("[base.exception." + type + "] " + message), type(type), message(message), location({0, 0}) {}

        explicit BaseError(const std::string &type, const std::string &message, SourceLocation location)
            : std::runtime_error("[base.exception." + type + "] (at " + std::to_string(location.line) + ":" + std::to_string(location.column) + ") " + message),
              type(type), message(message), location(location) {}
    };

    struct ParserError : public BaseError
    {
        explicit ParserError(const std::string &message, SourceLocation location) : BaseError("parser_error", message, location) {}
    };

    struct RenderError : public BaseError
    {
        explicit RenderError(const std::string &message, SourceLocation location) : BaseError("render_error", message, location) {}
    };

    struct FileError : public BaseError
    {
        explicit FileError(const std::string &message) : BaseError("file_error", message) {}
        explicit FileError(const std::string &message, SourceLocation location) : BaseError("file_error", message, location) {}
    };

    struct DataError : public BaseError
    {
        explicit DataError(const std::string &message, SourceLocation location) : BaseError("data_error", message, location) {}
    };
}