/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstddef> // size_t
#include <cstdint> // uint64_t
#include <cstdlib> // std::strtoll
#include <memory>  // std::static_pointer_cast
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
		Error::the().add("more than one sexp in input");
	}
}

ValuePtr Reader::readImpl()
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
		Error::the().add("invalid read syntax: ')'");
		return nullptr;
		break;
	case Token::Type::BracketOpen: // [
		return readVector();
		break;
	case Token::Type::BracketClose: // ]
		Error::the().add("invalid read syntax: ']'");
		return nullptr;
		break;
	case Token::Type::BraceOpen: // {
		return readHashMap();
		break;
	case Token::Type::BraceClose: // }
		Error::the().add("invalid read syntax: '}'");
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

ValuePtr Reader::readSpliceUnquote()
{
	ignore(); // ~@

	if (isEOF()) {
		Error::the().add("expected form, got EOF");
		return nullptr;
	}

	auto nodes = ValueVector(2);
	nodes.at(0) = makePtr<Symbol>("splice-unquote");
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readList()
{
	ignore(); // (

	auto nodes = ValueVector();
	while (!isEOF() && peek().type != Token::Type::ParenClose) {
		auto node = readImpl();
		if (node == nullptr) {
			return nullptr;
		}
		nodes.push_back(node);
	}

	if (!consumeSpecific(Token { .type = Token::Type::ParenClose })) { // )
		Error::the().add("expected ')', got EOF");
		return nullptr;
	}

	return makePtr<List>(nodes);
}

ValuePtr Reader::readVector()
{
	ignore(); // [

	auto nodes = ValueVector();
	while (!isEOF() && peek().type != Token::Type::BracketClose) {
		auto node = readImpl();
		if (node == nullptr) {
			return nullptr;
		}
		nodes.push_back(node);
	}

	if (!consumeSpecific(Token { .type = Token::Type::BracketClose })) { // ]
		Error::the().add("expected ']', got EOF");
	}

	return makePtr<Vector>(nodes);
}

ValuePtr Reader::readHashMap()
{
	ignore(); // {

	auto hash_map = makePtr<HashMap>();
	while (!isEOF() && peek().type != Token::Type::BraceClose) {
		auto key = readImpl();

		if (key == nullptr || isEOF()) {
			break;
		}

		if (peek().type == Token::Type::BraceClose) {
			Error::the().add("hash-map requires an even-sized list");
			return nullptr;
		}

		if (!is<String>(key.get()) && !is<Keyword>(key.get())) {
			Error::the().add(::format("wrong argument type: string or keyword, {}", key));
			return nullptr;
		}

		auto value = readImpl();
		hash_map->add(key, value);
	}

	if (!consumeSpecific(Token { .type = Token::Type::BraceClose })) { // }
		Error::the().add("expected '}', got EOF");
		return nullptr;
	}

	return hash_map;
}

ValuePtr Reader::readQuote()
{
	ignore(); // '

	if (isEOF()) {
		Error::the().add("expected form, got EOF");
		return nullptr;
	}

	auto nodes = ValueVector(2);
	nodes.at(0) = makePtr<Symbol>("quote");
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readQuasiQuote()
{
	ignore(); // `

	if (isEOF()) {
		Error::the().add("expected form, got EOF");
		return nullptr;
	}

	auto nodes = ValueVector(2);
	nodes.at(0) = makePtr<Symbol>("quasiquote");
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readUnquote()
{
	ignore(); // ~

	if (isEOF()) {
		Error::the().add("expected form, got EOF");
		return nullptr;
	}

	auto nodes = ValueVector(2);
	nodes.at(0) = makePtr<Symbol>("unquote");
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readWithMeta()
{
	ignore(); // ^

	ignore();      // first token
	if (isEOF()) { // second token
		Error::the().add("expected form, got EOF");
		return nullptr;
	}
	retreat();

	auto nodes = ValueVector(3);
	nodes.at(0) = makePtr<Symbol>("with-meta");
	nodes.at(2) = readImpl(); // Note: second Value is read first
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readDeref()
{
	ignore(); // @

	if (isEOF()) {
		Error::the().add("expected form, got EOF");
		return nullptr;
	}

	auto nodes = ValueVector(2);
	nodes.at(0) = makePtr<Symbol>("deref");
	nodes.at(1) = readImpl();

	return makePtr<List>(nodes);
}

ValuePtr Reader::readString()
{
	std::string symbol = consume().symbol;

	return makePtr<String>(symbol);
}

ValuePtr Reader::readKeyword()
{
	return makePtr<Keyword>(consume().symbol);
}

ValuePtr Reader::readValue()
{
	Token token = consume();
	char* end_ptr = nullptr;
	int64_t result = std::strtoll(token.symbol.c_str(), &end_ptr, 10);
	if (end_ptr == token.symbol.c_str() + token.symbol.size()) {
		return makePtr<Number>(result);
	}

	if (token.symbol == "nil") {
		return makePtr<Constant>(Constant::Nil);
	}
	else if (token.symbol == "true") {
		return makePtr<Constant>(Constant::True);
	}
	else if (token.symbol == "false") {
		return makePtr<Constant>(Constant::False);
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

void Reader::dumpImpl(ValuePtr node)
{
	std::string indentation = std::string(m_indentation * 2, ' ');

	Value* node_raw_ptr = node.get();
	if (is<Collection>(node_raw_ptr)) {
		auto nodes = std::static_pointer_cast<List>(node)->nodes();
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Blue), "{}Container", (is<List>(node_raw_ptr)) ? "List" : "Vector");
		print(" <");
		print(fg(ruc::format::TerminalColor::Blue), "{}", (is<List>(node_raw_ptr)) ? "()" : "{}");
		print(">\n");
		m_indentation++;
		for (auto node : nodes) {
			dumpImpl(node);
		}
		m_indentation--;
		return;
	}
	else if (is<String>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "StringNode");
		print(" <{}>", node);
	}
	else if (is<Keyword>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "KeywordNode");
		print(" <{}>", node);
	}
	else if (is<Number>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "NumberNode");
		print(" <{}>", node);
	}
	else if (is<Constant>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "ValueNode");
		print(" <{}>", node);
	}
	else if (is<Symbol>(node_raw_ptr)) {
		print("{}", indentation);
		print(fg(ruc::format::TerminalColor::Yellow), "SymbolNode");
		print(" <{}>", node);
	}
	print("\n");
}

} // namespace blaze
