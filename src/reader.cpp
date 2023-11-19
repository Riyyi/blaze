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
#include "settings.h"
#include "types.h"

namespace blaze {

Reader::Reader()
{
}

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

	if (!consumeSpecific(Token { .type = Token::Type::ParenClose, .symbol = "" })) { // )
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

	if (!consumeSpecific(Token { .type = Token::Type::BracketClose, .symbol = "" })) { // ]
		Error::the().add("expected ']', got EOF");
	}

	return makePtr<Vector>(nodes);
}

ValuePtr Reader::readHashMap()
{
	ignore(); // {

	Elements elements;
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
		elements.insert_or_assign(HashMap::getKeyString(key), value);
	}

	if (!consumeSpecific(Token { .type = Token::Type::BraceClose, .symbol = "" })) { // }
		Error::the().add("expected '}', got EOF");
		return nullptr;
	}

	return makePtr<HashMap>(elements);
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

void Reader::dump(ValuePtr node)
{
	m_indentation = 0;
	dumpImpl((node != nullptr) ? node : m_node);
}

void Reader::dumpImpl(ValuePtr node)
{
	std::string indentation = std::string(m_indentation * INDENTATION_WIDTH, ' ');
	print("{}", indentation);

	bool pretty_print = Settings::the().getEnvBool("*PRETTY-PRINT*");
	auto blue = fg(ruc::format::TerminalColor::BrightBlue);
	auto yellow = fg(ruc::format::TerminalColor::Yellow);

	Value* node_raw_ptr = node.get();
	if (is<Collection>(node_raw_ptr)) {
		auto container = (is<List>(node_raw_ptr)) ? "List" : "Vector";
		auto parens = (is<List>(node_raw_ptr)) ? "()" : "[]";
		pretty_print ? print(blue, "{}", container) : print("{}", container);
		print(" <");
		pretty_print ? print(blue, "{}", parens) : print("{}", parens);
		print(">\n");
		m_indentation++;
		auto nodes = std::static_pointer_cast<List>(node)->nodesRead();
		for (auto node : nodes) {
			dumpImpl(node);
		}
		m_indentation--;
		return;
	}
	else if (is<HashMap>(node_raw_ptr)) {
		auto hash_map = std::static_pointer_cast<HashMap>(node);
		auto elements = hash_map->elements();
		pretty_print ? print(blue, "HashMap") : print("HashMap");
		print(" <");
		pretty_print ? print(blue, "{{}}") : print("{{}}");
		print(">\n");
		m_indentation++;
		ValuePtr key_node = nullptr;
		for (auto element : elements) {
			bool is_keyword = element.first.front() == 0x7f; // 127
			is_keyword
				? dumpImpl(makePtr<Keyword>(element.first.substr(1)))
				: dumpImpl(makePtr<String>(element.first));
			m_indentation++;
			dumpImpl(element.second);
			m_indentation--;
		}
		m_indentation--;
		return;
	}
	else if (is<String>(node_raw_ptr)) {
		pretty_print ? print(yellow, "StringNode") : print("StringNode");
		print(" <{}>", node);
	}
	else if (is<Keyword>(node_raw_ptr)) {
		pretty_print ? print(yellow, "KeywordNode") : print("KeywordNode");
		print(" <{}>", node);
	}
	else if (is<Number>(node_raw_ptr)) {
		pretty_print ? print(yellow, "NumberNode") : print("NumberNode");
		print(" <{}>", node);
	}
	else if (is<Constant>(node_raw_ptr)) {
		pretty_print ? print(yellow, "ValueNode") : print("ValueNode");
		print(" <{}>", node);
	}
	else if (is<Symbol>(node_raw_ptr)) {
		pretty_print ? print(yellow, "SymbolNode") : print("SymbolNode");
		print(" <{}>", node);
	}
	else if (is<Function>(node_raw_ptr)) {
		auto function = std::static_pointer_cast<Function>(node);
		pretty_print ? print(blue, "Function") : print("Function");
		print(" <");
		pretty_print ? print(blue, "{}", function->name()) : print("{}", function->name());
		print(">\n");

		m_indentation++;
		indentation = std::string(m_indentation * INDENTATION_WIDTH, ' ');

		// bindings
		print("{}", indentation);
		pretty_print ? print(blue, "Bindings") : print("Bindings");
		print(" <{}>\n", function->bindings());

		m_indentation--;
		return;
	}
	else if (is<Lambda>(node_raw_ptr) || is<Macro>(node_raw_ptr)) {
		auto container = (is<Lambda>(node_raw_ptr)) ? "Lambda" : "Macro";
		auto lambda = std::static_pointer_cast<Lambda>(node);
		pretty_print ? print(blue, "{}", container) : print("{}", container);
		print(" <");
		pretty_print ? print(blue, "{:p}", node_raw_ptr) : print("{:p}", node_raw_ptr);
		print(">\n");

		m_indentation++;
		indentation = std::string(m_indentation * INDENTATION_WIDTH, ' ');

		// bindings
		print("{}", indentation);
		pretty_print ? print(blue, "Bindings") : print("Bindings");
		print(" <");
		const auto& bindings = lambda->bindings();
		for (size_t i = 0; i < bindings.size(); ++i) {
			print("{}{}", (i > 0) ? " " : "", bindings[i]);
		}
		print(">\n");

		// body
		dumpImpl(lambda->body());

		m_indentation--;
		return;
	}
	else if (is<Atom>(node_raw_ptr)) {
		pretty_print ? print(yellow, "AtomNode") : print("AtomNode");
		print(" <{}>", std::static_pointer_cast<Atom>(node)->deref());
	}
	print("\n");
}

} // namespace blaze
