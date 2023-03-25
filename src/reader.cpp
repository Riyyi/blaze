/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstddef> // size_t
#include <cstdint> // uint64_t
#include <cstdlib> // std::strtoll
#include <memory>  // makePtr, std::shared_ptr
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

	if (Error::the().hasOtherError()) {
		return;
	}

	// Check for multiple expressions
	if (!isEOF()) {
		Token::Type type = peek().type;
		switch (type) {
		case Token::Type::Comment:
			break;
		default:
			Error::the().addError("more than one sexp in input");
			break;
		};
	}
}

ASTNodePtr Reader::readImpl()
{
	if (m_tokens.size() == 0) {
		return nullptr;
	}

	switch (peek().type) {
	case Token::Type::Special: // ~@
		return readSpliceUnquote();
		break;
	case Token::Type::ParenOpen: // (
		return readList();
		break;
	case Token::Type::ParenClose: // )
		Error::the().addError("invalid read syntax: ')'");
		return nullptr;
		break;
	case Token::Type::BracketOpen: // [
		return readVector();
		break;
	case Token::Type::BracketClose: // ]
		Error::the().addError("invalid read syntax: ']'");
		return nullptr;
		break;
	case Token::Type::BraceOpen: // {
		return readHashMap();
		break;
	case Token::Type::BraceClose: // }
		Error::the().addError("invalid read syntax: '}'");
		return nullptr;
		break;
	case Token::Type::Quote: // '
		return readQuote();
		break;
	case Token::Type::Backtick: // `
		return readQuasiQuote();
		break;
	case Token::Type::Tilde: // ~
		return readUnquote();
		break;
	case Token::Type::Caret: // ^
		return readWithMeta();
		break;
	case Token::Type::At: // @
		return readDeref();
		break;
	case Token::Type::String: // "foobar"
		return readString();
		break;
	case Token::Type::Keyword: // :keyword
		return readKeyword();
		break;
	case Token::Type::Comment: // ;
		ignore();
		return nullptr;
		break;
	case Token::Type::Value: // true, false, nil
		return readValue();
		break;
	default:
		break;
	};

	// Unimplemented token
	VERIFY_NOT_REACHED();
	return nullptr;
}

ASTNodePtr Reader::readSpliceUnquote()
{
	ignore(); // ~@

	if (isEOF()) {
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("splice-unquote"));
	list->addNode(readImpl());

	return list;
}

ASTNodePtr Reader::readList()
{
	ignore(); // (

	auto list = makePtr<List>();
	while (!isEOF() && peek().type != Token::Type::ParenClose) {
		list->addNode(readImpl());
	}

	if (!consumeSpecific(Token { .type = Token::Type::ParenClose })) { // )
		Error::the().addError("expected ')', got EOF");
		return nullptr;
	}

	return list;
}

ASTNodePtr Reader::readVector()
{
	ignore(); // [

	auto vector = makePtr<Vector>();
	while (!isEOF() && peek().type != Token::Type::BracketClose) {
		vector->addNode(readImpl());
	}

	if (!consumeSpecific(Token { .type = Token::Type::BracketClose })) { // ]
		Error::the().addError("expected ']', got EOF");
	}

	return vector;
}

ASTNodePtr Reader::readHashMap()
{
	ignore(); // {

	auto hash_map = makePtr<HashMap>();
	while (!isEOF() && peek().type != Token::Type::BraceClose) {
		auto key = readImpl();
		auto value = readImpl();

		if (key == nullptr && value == nullptr) {
			break;
		}

		if (key == nullptr || value == nullptr) {
			Error::the().addError("hash-map requires an even-sized list");
			return nullptr;
		}

		if (!is<String>(key.get()) && !is<Keyword>(key.get())) {
			Error::the().addError(format("{} is not a string or keyword", key));
			return nullptr;
		}

		std::string keyString = is<String>(key.get()) ? static_pointer_cast<String>(key)->data() : static_pointer_cast<Keyword>(key)->keyword();
		hash_map->addElement(keyString, value);
	}

	if (!consumeSpecific(Token { .type = Token::Type::BraceClose })) { // }
		Error::the().addError("expected '}', got EOF");
	}

	return hash_map;
}

ASTNodePtr Reader::readQuote()
{
	ignore(); // '

	if (isEOF()) {
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("quote"));
	list->addNode(readImpl());

	return list;
}

ASTNodePtr Reader::readQuasiQuote()
{
	ignore(); // `

	if (isEOF()) {
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("quasiquote"));
	list->addNode(readImpl());

	return list;
}

ASTNodePtr Reader::readUnquote()
{
	ignore(); // ~

	if (isEOF()) {
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("unquote"));
	list->addNode(readImpl());

	return list;
}

ASTNodePtr Reader::readWithMeta()
{
	ignore(); // ^

	ignore();      // first token
	if (isEOF()) { // second token
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}
	retreat();

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("with-meta"));
	ASTNodePtr first = readImpl();
	ASTNodePtr second = readImpl();
	list->addNode(second);
	list->addNode(first);

	return list;
}

ASTNodePtr Reader::readDeref()
{
	ignore(); // @

	if (isEOF()) {
		Error::the().addError("expected form, got EOF");
		return nullptr;
	}

	auto list = makePtr<List>();
	list->addNode(makePtr<Symbol>("deref"));
	list->addNode(readImpl());

	return list;
}

ASTNodePtr Reader::readString()
{
	std::string symbol = consume().symbol;

	// Unbalanced string
	if (symbol.size() < 2 || symbol.front() != '"' || symbol.back() != '"') {
		Error::the().addError("expected '\"', got EOF");
	}

	return makePtr<String>(symbol);
}

ASTNodePtr Reader::readKeyword()
{
	return makePtr<Keyword>(consume().symbol);
}

ASTNodePtr Reader::readValue()
{
	Token token = consume();
	char* end_ptr = nullptr;
	int64_t result = std::strtoll(token.symbol.c_str(), &end_ptr, 10);
	if (end_ptr == token.symbol.c_str() + token.symbol.size()) {
		return makePtr<Number>(result);
	}

	return makePtr<Symbol>(token.symbol);
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

void Reader::retreat()
{
	m_index--;
}

// -----------------------------------------

void Reader::dump()
{
	dumpImpl(m_node);
}

void Reader::dumpImpl(ASTNodePtr node)
{
	std::string indentation = std::string(m_indentation * 2, ' ');

	ASTNode* node_raw_ptr = node.get();
	if (is<List>(node_raw_ptr)) {
		List* list = static_cast<List*>(node_raw_ptr);
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
	else if (is<String>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "StringNode");
		print(" <{}>", static_cast<String*>(node_raw_ptr)->data());
	}
	else if (is<Keyword>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "KeywordNode");
		print(" <{}>", static_cast<Keyword*>(node_raw_ptr)->keyword());
	}
	else if (is<Number>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "NumberNode");
		print(" <{}>", static_cast<Number*>(node_raw_ptr)->number());
	}
	else if (is<Symbol>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "SymbolNode");
		print(" <{}>", static_cast<Symbol*>(node_raw_ptr)->symbol());
	}
	print("\n");
}

} // namespace blaze
