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

namespace blaze {

// Parsing -> creates AST
class Reader {
public:
	Reader(std::vector<Token>&& tokens) noexcept;
	virtual ~Reader();

	void read();

	void dump();

	ASTNodePtr node() { return m_node; }

private:
	bool isEOF() const;
	Token peek() const;
	Token consume();
	bool consumeSpecific(Token token);
	void ignore();
	void retreat();

	ASTNodePtr readImpl();
	ASTNodePtr readSpliceUnquote(); // ~@
	ASTNodePtr readList();          // ()
	ASTNodePtr readVector();        // []
	ASTNodePtr readHashMap();       // {}
	ASTNodePtr readQuote();         // '
	ASTNodePtr readQuasiQuote();    // `
	ASTNodePtr readUnquote();       // ~
	ASTNodePtr readWithMeta();      // ^
	ASTNodePtr readDeref();         // @
	ASTNodePtr readString();        // "foobar"
	ASTNodePtr readKeyword();       // :keyword
	ASTNodePtr readValue();         // number, "nil", "true", "false", symbol

	void dumpImpl(ASTNodePtr node);

	size_t m_index { 0 };
	size_t m_indentation { 0 };
	std::vector<Token> m_tokens;

	char m_error_character { 0 };
	bool m_invalid_syntax { false };
	bool m_is_unbalanced { false };

	ASTNodePtr m_node { nullptr };
};

} // namespace blaze
