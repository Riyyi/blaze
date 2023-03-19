/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstddef> // size_t
#include <cstdint> // uint64_t
#include <cstdlib> // std::strtoll
#include <utility> // std::move

#include "error.h"
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
	if (Error::the().hasAnyError() || m_node) {
		return;
	}

	m_node = readImpl();

	// Error checking

	if (m_invalid_syntax) {
		Error::the().addError("invalid read syntax: '" + std::string(1, m_error_character) + "'");
		return;
	}

	if (m_is_unbalanced) {
		Error::the().addError("expected '" + std::string(1, m_error_character) + "', got EOF");
		return;
	}

	if (!isEOF()) {
		Token::Type type = peek().type;
		switch (type) {
		case Token::Type::ParenOpen:  // (
		case Token::Type::ParenClose: // )
		case Token::Type::String:
		case Token::Type::Value:
			Error::the().addError("more than one sexp in input");
			break;
		default:
			Error::the().addError("unknown error");
			break;
		};
	}
}

ASTNode* Reader::readImpl()
{
	if (m_tokens.size() == 0) {
		return nullptr;
	}

	switch (peek().type) {
	case Token::Type::ParenOpen: // (
		return readList();
		break;
	case Token::Type::ParenClose: // )
		m_invalid_syntax = true;
		m_error_character = ')';
		return nullptr;
		break;
	case Token::Type::String:
		return readString();
		break;
	case Token::Type::Value:
		return readValue();
		break;
	default:
		// Unimplemented token
		VERIFY_NOT_REACHED();
		return nullptr;
	};
}

ASTNode* Reader::readList()
{
	ignore(); // (

	List* list = new List();
	while (!isEOF() && peek().type != Token::Type::ParenClose) {
		list->addNode(readImpl());
	}

	if (!consumeSpecific(Token { .type = Token::Type::ParenClose })) { // )
		m_error_character = ')';
		m_is_unbalanced = true;
	}

	return list;
}

ASTNode* Reader::readString()
{
	std::string symbol = consume().symbol;

	// Unbalanced string
	if (symbol.size() < 2 || symbol.front() != '"' || symbol.back() != '"') {
		m_error_character = '"';
		m_is_unbalanced = true;
	}

	return new String(symbol);
}

ASTNode* Reader::readValue()
{
	Token token = consume();
	char* end_ptr = nullptr;
	int64_t result = std::strtoll(token.symbol.c_str(), &end_ptr, 10);
	if (end_ptr == token.symbol.c_str() + token.symbol.size()) {
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

bool Reader::consumeSpecific(Token token)
{
	if (isEOF() || peek().type != token.type) {
		return false;
	}

	ignore();
	return true;
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
