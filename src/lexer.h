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
		ParenOpen,    // (
		ParenClose,   // )
		BracketOpen,  // [
		BracketClose, // ]
		BraceOpen,    // {
		BraceClose,   // }
		Quote,        // '
		Backtick,     // `
		Tilde,        // ~
		Caret,        // ^
		At,           // @
		String,       // "foobar"
		Keyword,      // :keyword
		Value,        // numbers, "true", "false", and "nil", symbols
		Comment,      // ;
		Error,
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
	bool consumeKeyword();
	bool consumeValue();
	bool consumeComment();

	size_t m_column { 0 };
	size_t m_line { 0 };

	std::vector<Token> m_tokens;
};

} // namespace blaze
