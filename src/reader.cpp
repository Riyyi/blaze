/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstddef> // size_t
#include <cstdint> // uint64_t
#include <cstdlib> // std::strtoll
#include <utility> // std::move

#include "ruc/format/color.h"
#include "ruc/meta/assert.h"

#include "ast.h"
#include "reader.h"
#include "types.h"

namespace blaze {

Reader::Reader(std::vector<Token>&& tokens) noexcept
	: m_tokens(std::move(tokens))
{
}

Reader::~Reader()
{
}

// -----------------------------------------

void Reader::read()
{
	if (m_node != nullptr) {
		return;
	}

	m_node = readImpl();
	VERIFY(m_index > m_tokens.size() - 1, "more than one sexp in input");
}

ASTNode* Reader::readImpl()
{
	switch (peek().type) {
	case Token::Type::ParenOpen:
		return readList();
		break;
	case Token::Type::String:
		return readString();
		break;
	case Token::Type::Value:
		return readValue();
	default:
		// Unimplemented token
		VERIFY_NOT_REACHED();
		return nullptr;
	}
}

ASTNode* Reader::readList()
{
	ignore(); // (

	List* list = new List();
	while (m_index < m_tokens.size() && peek().type != Token::Type::ParenClose) {
		list->addNode(readImpl());
	}

	VERIFY(m_index != m_tokens.size(), "missing closing ')'");

	ignore(); // )

	return list;
}

ASTNode* Reader::readString()
{
	Token token = consume();
	return new String(token.symbol);
}

ASTNode* Reader::readValue()
{
	Token token = consume();
	char* endPtr = nullptr;
	int64_t result = std::strtoll(token.symbol.c_str(), &endPtr, 10);
	if (endPtr == token.symbol.c_str() + token.symbol.size()) {
		return new Number(result);
	}

	return new Symbol(token.symbol);
}

// -----------------------------------------

bool Reader::isEOF() const
{
	return m_index >= m_tokens.size();
}

Token Reader::peek() const
{
	VERIFY(!isEOF());
	return m_tokens[m_index];
}

Token Reader::consume()
{
	VERIFY(!isEOF());
	return m_tokens[m_index++];
}

void Reader::ignore()
{
	m_index++;
}

// -----------------------------------------

void Reader::dump()
{
	dumpImpl(m_node);
}

void Reader::dumpImpl(ASTNode* node)
{
	std::string indentation = std::string(m_indentation * 2, ' ');

	if (is<List>(node)) {
		List* list = static_cast<List*>(node);
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Blue), "ListContainer");
		print(" <");
		print(fg(ruc::format::TerminalColor::Blue), "()");
		print(">\n");
		m_indentation++;
		for (size_t i = 0; i < list->nodes().size(); ++i) {
			dumpImpl(list->nodes()[i]);
		}
		m_indentation--;
		return;
	}
	else if (is<String>(node)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "StringNode");
		print(" <{}>", static_cast<String*>(node)->data());
	}
	else if (is<Number>(node)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "NumberNode");
		print(" <{}>", static_cast<Number*>(node)->number());
	}
	else if (is<Symbol>(node)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "SymbolNode");
		print(" <{}>", static_cast<Symbol*>(node)->symbol());
	}
	print("\n");
}

} // namespace blaze
