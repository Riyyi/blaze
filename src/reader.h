/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef> // size_t
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

	ASTNode* node() { return m_node; }

private:
	bool isEOF() const;
	Token peek() const;
	Token consume();
	bool consumeSpecific(Token token);
	void ignore();
	void retreat();

	ASTNode* readImpl();
	ASTNode* readSpliceUnquote(); // ~@
	ASTNode* readVector();        // []
	ASTNode* readHashMap();       // {}
	ASTNode* readList();          // ()
	ASTNode* readQuote();         // '
	ASTNode* readQuasiQuote();    // `
	ASTNode* readUnquote();       // ~
	ASTNode* readWithMeta();      // ^
	ASTNode* readDeref();         // @
	ASTNode* readString();        // "foobar"
	ASTNode* readKeyword();       // :keyword
	ASTNode* readValue();         // true, false, nil

	void dumpImpl(ASTNode* node);

	size_t m_index { 0 };
	size_t m_indentation { 0 };
	std::vector<Token> m_tokens;

	char m_error_character { 0 };
	bool m_invalid_syntax { false };
	bool m_is_unbalanced { false };

	ASTNode* m_node { nullptr };
};

} // namespace blaze
