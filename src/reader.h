/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef> // size_t
#include <memory>  // std::shared_ptr
#include <vector>

#include "ast.h"
#include "lexer.h"

#define INDENTATION_WIDTH 2

namespace blaze {

// Parsing -> creates AST
class Reader {
public:
	Reader();
	Reader(std::vector<Token>&& tokens) noexcept;
	virtual ~Reader();

	void read();

	void dump(ValuePtr node = nullptr);

	ValuePtr node() { return m_node; }

private:
	bool isEOF() const;
	Token peek() const;
	Token consume();
	bool consumeSpecific(Token token);
	void ignore();
	void retreat();

	ValuePtr readImpl();
	ValuePtr readSpliceUnquote(); // ~@
	ValuePtr readList();          // ()
	ValuePtr readVector();        // []
	ValuePtr readHashMap();       // {}
	ValuePtr readQuote();         // '
	ValuePtr readQuasiQuote();    // `
	ValuePtr readUnquote();       // ~
	ValuePtr readWithMeta();      // ^
	ValuePtr readDeref();         // @
	ValuePtr readString();        // "foobar"
	ValuePtr readKeyword();       // :keyword
	ValuePtr readValue();         // number, "nil", "true", "false", symbol

	void dumpImpl(ValuePtr node);

	size_t m_index { 0 };
	size_t m_indentation { 0 };
	std::vector<Token> m_tokens;

	char m_error_character { 0 };
	bool m_invalid_syntax { false };
	bool m_is_unbalanced { false };

	ValuePtr m_node { nullptr };
};

} // namespace blaze
