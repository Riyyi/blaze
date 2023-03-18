/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint8_t
#include <string>
#include <vector>

#include "ruc/format/print.h"
#include "ruc/genericlexer.h"

namespace blaze {

struct Token {
	enum class Type : uint8_t {
		None,
		Special,      // ~@
		BracketOpen,  // [
		BracketClose, // ]
		BraceOpen,    // {
		BraceClose,   // }
		ParenOpen,    // (
		ParenClose,   // )
		Quote,        // '
		Backtick,     // `
		Tilde,        // ~
		Caret,        // ^
		At,           // @
		String,       // "foobar"
		Comment,      // ;
		Value,        // symbols, numbers, "true", "false", and "nil"
	};

	Type type { Type::None };
	size_t column { 0 };
	size_t line { 0 };
	std::string symbol;
};

// Lexical analyzer -> tokenizes
class Lexer final : public ruc::GenericLexer {
public:
	Lexer(std::string_view input);
	virtual ~Lexer();

	void tokenize();

	void dump() const;

	std::vector<Token>& tokens() { return m_tokens; }

private:
	bool consumeSpliceUnquoteOrUnquote(); // ~@ or ~
	bool consumeString();
	bool consumeComment();
	bool consumeValue();

	size_t m_column { 0 };
	size_t m_line { 0 };

	std::vector<Token> m_tokens;
};

} // namespace blaze

// ~^@
// (+ 2 (* 3 4))

// Lexing -> creates tokens
// Parsing -> creates AST

// class Thing1 {
// public:
// 	std::vector<int>& numbers() { return m_numbers; }

// private:
// 	std::vector<int> m_numbers;
// };

// class Thing2 {
// public:
// 	std::vector<int>&& numbers() { return std::move(m_numbers); }

// private:
// 	std::vector<int> m_numbers;
// };

// class OtherThing {
// public:
// 	OtherThing(std::vector<int>&& numbers) noexcept
// 		: m_numbers(std::move(numbers))
// 	{
// 	}

// private:
// 	std::vector<int> m_numbers;
// };

// int main()
// {
// 	Thing1 thing1;
// 	Thing2 thing2;
// 	OtherThing other_thing(std::move(thing1.numbers()));
// 	OtherThing other_thing2(thing2.numbers());
// }
