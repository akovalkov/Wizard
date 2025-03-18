#pragma once
#include <string_view>

namespace Wizard {

    struct Token {
        enum class Kind : char {
            Text,
            ExpressionOpen,     // {{
            ExpressionClose,    // }}
            LineStatementOpen,  // ##
            LineStatementClose, // \n
            StatementOpen,      // {%
            StatementClose,     // %}
            CommentOpen,        // {#
            CommentClose,       // #}
            Id,                 // this, this.foo
            Number,             // 1, 2, -1, 5.2, -5.3
            String,             // "this"
            Plus,               // +
            Minus,              // -
            Times,              // *
            Slash,              // /
            Percent,            // %
            Power,              // ^
            Comma,              // ,
            Dot,                // .
            Colon,              // :
            LeftParen,          // (
            RightParen,         // )
            LeftBracket,        // [
            RightBracket,       // ]
            LeftBrace,          // {
            RightBrace,         // }
            Equal,              // ==
            NotEqual,           // !=
            GreaterThan,        // >
            GreaterEqual,       // >=
            LessThan,           // <
            LessEqual,          // <=
            Unknown,
            Eof,
        };

        Kind kind {Kind::Unknown};
        std::string_view text;
        std::size_t offset{std::string_view::npos};

        explicit constexpr Token() = default;
        explicit constexpr Token(Kind kind, std::string_view text, 
                                 std::size_t offset = std::string_view::npos)
                                    : kind(kind), text(text), offset(offset) {}
        constexpr bool operator ==(const Token& other) const {
            return kind == other.kind && text == other.text;
        }

        std::string describe() const {
            switch (kind) {
            case Kind::Text:
                return "<text>";
            case Kind::LineStatementClose:
                return "<eol>";
            case Kind::Eof:
                return "<eof>";
            default:
                return static_cast<std::string>(text);
            }
        }
    };

} // namespace Wizard

