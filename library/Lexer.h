#pragma once
#include <unordered_map>
#include "Util.h"
#include "Token.h"
//#include "Parser.h"

namespace Wizard {


	class Lexer
	{
	public:
		enum class State
		{
			Text,
			ExpressionStart,
			ExpressionStartForceLstrip,
			ExpressionBody,
			LineStart,
			LineBody,
			StatementStart,
			StatementStartForceLstrip,
			StatementBody,
			CommentStart,
			CommentStartForceLstrip,
			CommentBody,
		};

		enum class MinusState
		{
			Operator,
			Number,
		};

		struct LexerState
		{
			std::string_view m_in;
			size_t tok_start = 0;
			size_t pos = 0;
			State state = State::Text;
			MinusState minus_state = MinusState::Number;
			// result
			Token token{};
		};
	protected:

		const LexerConfig &config;
		std::string open_chars;

		LexerState scan_body(LexerState& state, 
							 std::string_view close, Token::Kind closeKind, 
							 std::string_view close_rstrip = std::string_view())
		{
		again:
			// skip whitespace (except for \n as it might be a close)
			if (state.tok_start >= state.m_in.size()) {
				state.token = make_token(state, Token::Kind::Eof);
				return state;
			}
			const char ch = state.m_in[state.tok_start];
			if (ch == ' ' || ch == '\t' || ch == '\r') {
				state.tok_start += 1;
				goto again;
			}

			// check for close
			if (!close_rstrip.empty() && state.m_in.substr(state.tok_start).starts_with(close_rstrip)) {
				state.state = State::Text;
				state.pos = state.tok_start + close_rstrip.size();
				state.token = make_token(state, closeKind);
				rstrip(state);
				return state;
			}

			if (state.m_in.substr(state.tok_start).starts_with(close)) {
				state.state = State::Text;
				state.pos = state.tok_start + close.size();
				state.token = make_token(state, closeKind);
				return state;
			}

			// skip \n
			if (ch == '\n') {
				state.tok_start += 1;
				goto again;
			}

			state.pos = state.tok_start + 1;
			if (std::isalpha(ch) || ch == '.') {
				state.minus_state = MinusState::Operator;
				return scan_id(state);
			}

			const MinusState current_minus_state = state.minus_state;
			if (state.minus_state == MinusState::Operator){
				state.minus_state = MinusState::Number;
			}

			switch (ch)
			{
			case '+':
				state.token = make_token(state, Token::Kind::Plus);
				return state;
			case '-':
				if (current_minus_state == MinusState::Operator) {
					state.token = make_token(state, Token::Kind::Minus);
					return state;
				} else {
					return scan_number(state);
				}
			case '*':
				state.token = make_token(state, Token::Kind::Times);
				return state;
			case '/':
				state.token = make_token(state, Token::Kind::Slash);
				return state;
			case '^':
				state.token = make_token(state, Token::Kind::Power);
				return state;
			case '%':
				state.token = make_token(state, Token::Kind::Percent);
				return state;
			// case '.':
			// 	state.token = make_token(state, Token::Kind::Dot);
			// 	return state;
			case ',':
				state.token = make_token(state, Token::Kind::Comma);
				return state;
			case ':':
				state.token = make_token(state, Token::Kind::Colon);
				return state;
			case '(':
				state.token = make_token(state, Token::Kind::LeftParen);
				return state;
			case ')':
				state.minus_state = MinusState::Operator;
				state.token = make_token(state, Token::Kind::RightParen);
				return state;
			case '[':
			state.token = make_token(state, Token::Kind::LeftBracket);
				return state;
			case ']':
				state.minus_state = MinusState::Operator;
				state.token = make_token(state, Token::Kind::RightBracket);
				return state;
			case '{':
			state.token = make_token(state, Token::Kind::LeftBrace);
				return state;
			case '}':
				state.minus_state = MinusState::Operator;
				state.token = make_token(state, Token::Kind::RightBrace);
				return state;
			case '>':
				if (state.pos < state.m_in.size() && state.m_in[state.pos] == '='){
					state.pos += 1;
					state.token = make_token(state, Token::Kind::GreaterEqual);
					return state;
				} else {
					state.token = make_token(state, Token::Kind::GreaterThan);
					return state;
				}
			case '<':
				if (state.pos < state.m_in.size() && state.m_in[state.pos] == '='){
					state.pos += 1;
					state.token = make_token(state, Token::Kind::LessEqual);
					return state;
				} else {
					state.token = make_token(state, Token::Kind::LessThan);
					return state;
				}
			case '=':
				if (state.pos < state.m_in.size() && state.m_in[state.pos] == '='){
					state.pos += 1;
					state.token = make_token(state, Token::Kind::Equal);
					return state;
				} else {
					state.token = make_token(state, Token::Kind::Unknown);
					return state;
				}
			case '!':
				if (state.pos < state.m_in.size() && state.m_in[state.pos] == '='){
					state.pos += 1;
					state.token = make_token(state, Token::Kind::NotEqual); 
					return state;
				} else {
					state.token = make_token(state, Token::Kind::Unknown);
					return state;
				}
			case '\"':
				return scan_string(state);
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				state.minus_state = MinusState::Operator;
				return scan_number(state);
			case '_':
			case '@':
			case '$':
				state.minus_state = MinusState::Operator;
				return scan_id(state);
			default:
				state.token = make_token(state, Token::Kind::Unknown);
				return state;
			}
		}

		LexerState scan_id(LexerState& state)
		{
			for (;;)
			{
				if (state.pos >= state.m_in.size()) {
					break;
				}
				const char ch = state.m_in[state.pos];
				if (!std::isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') {
					break;
				}
				state.pos += 1;
			}
			state.token = make_token(state, Token::Kind::Id);
			return state;
		}

		LexerState scan_number(LexerState& state)
		{
			for (;;)
			{
				if (state.pos >= state.m_in.size())
				{
					break;
				}
				const char ch = state.m_in[state.pos];
				// be very permissive in lexer (we'll catch errors when conversion happens)
				if (!(std::isdigit(ch) || ch == '.' || ch == 'e' || ch == 'E' || 
					(ch == '+' && (state.pos == 0 || state.m_in[state.pos - 1] == 'e' || state.m_in[state.pos - 1] == 'E')) || 
					(ch == '-' && (state.pos == 0 || state.m_in[state.pos - 1] == 'e' || state.m_in[state.pos - 1] == 'E'))))
				{
					break;
				}
				state.pos += 1;
			}
			state.token = make_token(state, Token::Kind::Number);
			return state;
		}

		LexerState scan_string(LexerState& state)
		{
			bool escape{false};
			for (;;) {
				if (state.pos >= state.m_in.size()) {
					break;
				}
				const char ch = state.m_in[state.pos++];
				if (ch == '\\') {
					escape = !escape;
				} else if (!escape && ch == state.m_in[state.tok_start]) {
					break;
				} else {
					escape = false;
				}
			}
			state.token = make_token(state, Token::Kind::String);
			return state;
		}

		Token make_token(const LexerState& state, Token::Kind kind) const
		{
			return Token(kind, string_view::slice(state.m_in, state.tok_start, state.pos), state.tok_start);
		}

		void rstrip(LexerState& state)
		{
			// skip spaces, atbulation, CR, LF after
			while(state.pos < state.m_in.size() && 
			      (state.m_in[state.pos] == ' ' || 
				   state.m_in[state.pos] == '\t' || 
				   state.m_in[state.pos] == '\n' || 
				   state.m_in[state.pos] == '\r')) {
				++state.pos;
			}
		}

		std::string_view lstrip(std::string_view text)
		{
			// skip spaces, tabulation, CR, LF before
			auto result = text;
			while (!result.empty()) {
				const char ch = result.back();
				if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
					result.remove_suffix(1);
				} else {
					break;
				}
			}
			return result;
		}

        void set_open_chars()
        {
            open_chars = "";
            if (open_chars.find(config.line_statement[0]) == std::string::npos) {
                open_chars += config.line_statement[0];
            } if (open_chars.find(config.statement_open[0]) == std::string::npos) {
                open_chars += config.statement_open[0];
            } if (open_chars.find(config.statement_open_force_lstrip[0]) == std::string::npos) {
                open_chars += config.statement_open_force_lstrip[0];
            } if (open_chars.find(config.expression_open[0]) == std::string::npos) {
                open_chars += config.expression_open[0];
            } if (open_chars.find(config.expression_open_force_lstrip[0]) == std::string::npos) {
                open_chars += config.expression_open_force_lstrip[0];
            } if (open_chars.find(config.comment_open[0]) == std::string::npos) {
                open_chars += config.comment_open[0];
            } if (open_chars.find(config.comment_open_force_lstrip[0]) == std::string::npos) {
                open_chars += config.comment_open_force_lstrip[0];
            }
        }


		LexerState scan_text(LexerState& state)
		{
			state.tok_start = state.pos;
		again:
			if(state.tok_start >= state.m_in.size()) {
				state.token = make_token(state, Token::Kind::Eof);
				return state;
			}
			// fast-scan to first open character
			const size_t open_start = state.m_in.substr(state.pos).find_first_of(open_chars);
			if (open_start == std::string_view::npos) {
				// didn't find open, return remaining text as text token
				state.pos = state.m_in.size();
				state.token = make_token(state, Token::Kind::Text);
				return state;
			}
			state.pos += open_start;

			// try to match one of the opening sequences, and get the close
			std::string_view open_str = state.m_in.substr(state.pos);
			bool must_lstrip = false;
			if(open_str.starts_with(config.expression_open)) {
				if (open_str.starts_with(config.expression_open_force_lstrip)) {
					state.state = State::ExpressionStartForceLstrip;
					must_lstrip = true;
				} else {
					state.state = State::ExpressionStart;
				}
			} else if(open_str.starts_with(config.statement_open)) {
				if (open_str.starts_with(config.statement_open_force_lstrip)) {
					state.state = State::StatementStartForceLstrip;
					must_lstrip = true;
				} else {
					state.state = State::StatementStart;
				}
			} else if(open_str.starts_with(config.comment_open)) {
				if (open_str.starts_with(config.comment_open_force_lstrip)) {
					state.state = State::CommentStartForceLstrip;
					must_lstrip = true;
				} else {
					state.state = State::CommentStart;
				}
			} else if((state.pos == 0 || state.m_in[state.pos - 1] == '\n') && 
			           open_str.starts_with(config.line_statement)) {
				state.state = State::LineStart;
			} else {
				state.pos += 1; // wasn't actually an opening sequence
				goto again;
			}

			std::string_view text = string_view::slice(state.m_in, state.tok_start, state.pos);

			if (must_lstrip) {
				text = lstrip(text);
			}

			if(text.empty()) {
				// don't generate empty token
				return scan(state);
			}
			state.token = Token(Token::Kind::Text, text, state.tok_start);
			return state;

		}

		LexerState scan_comment(LexerState& state)
		{
			state.tok_start = state.pos;
			if (state.tok_start >= state.m_in.size()) {
				state.token = make_token(state, Token::Kind::Eof);
				return state;
			}
			// fast-scan to comment close
			const size_t end = state.m_in.substr(state.pos).find(config.comment_close);
			if (end == std::string_view::npos) {
				state.pos = state.m_in.size();
				state.token = make_token(state, Token::Kind::Eof);
				return state;
			}

			// Check for trim pattern
			const bool must_rstrip = state.m_in.substr(state.pos + end - 1).starts_with(config.comment_close_force_rstrip);

			// return the entire comment in the close token
			state.state = State::Text;
			state.pos += end + config.comment_close.size();
			state.token = make_token(state, Token::Kind::CommentClose);

			if (must_rstrip) {
				rstrip(state);
			}
			return state;

		}

		LexerState new_state(LexerState& state, State newstate, size_t length, Token::Kind ttype)
		{
			state.tok_start = state.pos;
			state.state = newstate;
			state.pos += length;
			state.token = make_token(state, ttype); 
			return state;
		}

	public:
		explicit Lexer(const LexerConfig &config) 
				: config(config) {}


		SourceLocation current_position(LexerState& state) const {
			return get_source_location(state.m_in, state.tok_start);
		}

		LexerState start(std::string_view input)
		{
			LexerState state{input};
			set_open_chars();
			// Consume byte order mark (BOM) for UTF-8
			if(state.m_in.starts_with("\xEF\xBB\xBF")) {
				state.m_in = state.m_in.substr(3);
			}
			return state;
		}

		LexerState scan(LexerState& state)
		{
			state.tok_start = state.pos;
			if (state.tok_start >= state.m_in.size()) {
				state.token = make_token(state, Token::Kind::Eof);
				return state;
			}

			switch (state.state)
			{
			default:
			case State::Text:
				return scan_text(state);
			case State::ExpressionStart: // {{
				return new_state(state, State::ExpressionBody, config.expression_open.size(), Token::Kind::ExpressionOpen);
			case State::ExpressionStartForceLstrip: // {{-
				return new_state(state, State::ExpressionBody, config.expression_open_force_lstrip.size(), Token::Kind::ExpressionOpen);
			case State::LineStart: // ##
				return new_state(state, State::LineBody, config.line_statement.size(), Token::Kind::LineStatementOpen);
			case State::StatementStart: // {%
				return new_state(state, State::StatementBody, config.statement_open.size(), Token::Kind::StatementOpen);
			case State::StatementStartForceLstrip: // {%-
				return new_state(state, State::StatementBody, config.statement_open_force_lstrip.size(), Token::Kind::StatementOpen);
			case State::CommentStart: // {#
				return new_state(state, State::CommentBody, config.comment_open.size(), Token::Kind::CommentOpen);
			case State::CommentStartForceLstrip: // {#-
				return new_state(state, State::CommentBody, config.comment_open_force_lstrip.size(), Token::Kind::CommentOpen);
			case State::ExpressionBody:
				return scan_body(state, config.expression_close, Token::Kind::ExpressionClose, config.expression_close_force_rstrip);
			case State::LineBody:
				return scan_body(state, "\n", Token::Kind::LineStatementClose);
			case State::StatementBody:
				return scan_body(state, config.statement_close, Token::Kind::StatementClose, config.statement_close_force_rstrip);
			case State::CommentBody:
				return scan_comment(state);
			}
		}

		const LexerConfig& get_config() const
		{
			return config;
		}
	
	
	};

    inline auto make_scanner(const LexerConfig& config) {
        auto scanner = [&config](std::string_view input) -> Scanner<Token> {
            Lexer lexer(config);
            auto state = lexer.start(input);
            while((state = lexer.scan(state)).token.kind != Token::Kind::Eof) {
                co_yield state.token;
            }
            co_return state.token;
            
        };
        return scanner;
    }

} // namespace Wizard