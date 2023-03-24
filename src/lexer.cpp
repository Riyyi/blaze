/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <string> // std::to_string
#include <unordered_set>

#include "ruc/format/print.h"
#include "ruc/genericlexer.h"

#include "error.h"
#include "lexer.h"

namespace blaze {

Lexer::Lexer(std::string_view input)
	: ruc::GenericLexer(input)
{
}

Lexer::~Lexer()
{
}

// -----------------------------------------

void Lexer::tokenize()
{
	if (Error::the().hasAnyError() || m_tokens.size() > 0) {
		return;
	}

	while (m_index < m_input.length()) {
		switch (peek()) {
		case '~': // ~@ or ~
			consumeSpliceUnquoteOrUnquote();
			break;
		case '[':
			m_tokens.push_back({ Token::Type::BracketOpen, m_line, m_column, "[" });
			break;
		case ']':
			m_tokens.push_back({ Token::Type::BracketClose, m_line, m_column, "]" });
			break;
		case '{':
			m_tokens.push_back({ Token::Type::BraceOpen, m_line, m_column, "{" });
			break;
		case '}':
			m_tokens.push_back({ Token::Type::BraceClose, m_line, m_column, "}" });
			break;
		case '(':
			m_tokens.push_back({ Token::Type::ParenOpen, m_line, m_column, "(" });
			break;
		case ')':
			m_tokens.push_back({ Token::Type::ParenClose, m_line, m_column, ")" });
			break;
		case '\'':
			m_tokens.push_back({ Token::Type::Quote, m_line, m_column, "'" });
			break;
		case '`':
			m_tokens.push_back({ Token::Type::Backtick, m_line, m_column, "`" });
			break;
		case '^':
			m_tokens.push_back({ Token::Type::Caret, m_line, m_column, "^" });
			break;
		case '@':
			m_tokens.push_back({ Token::Type::At, m_line, m_column, "@" });
			break;
		case '"':
			if (!consumeString()) {
				return;
			}
			break;
		case ':':
			if (!consumeKeyword()) {
				return;
			}
			break;
		case ';':
			consumeComment();
			break;
		case ' ':
		case '\t':
		case ',':
			break;
		case '\r':
			if (peek(1) == '\n') { // CRLF \r\n
				break;
			}
			m_column = -1;
			m_line++;
			break;
		case '\n':
			m_column = -1;
			m_line++;
			break;
		default:
			consumeValue();
			break;
		}

		ignore();
		m_column++;
	}
}

bool Lexer::consumeSpliceUnquoteOrUnquote()
{
	size_t column = m_column;

	if (peek(1) == '@') {
		ignore(); // ~
		m_tokens.push_back({ Token::Type::Special, m_line, column, "~@" });
	}
	else {
		m_tokens.push_back({ Token::Type::Tilde, m_line, column, "~" });
	}

	return true;
}

bool Lexer::consumeString()
{
	size_t column = m_column;
	std::string text = "\"";

	static std::unordered_set<char> exit = {
		'"',
		'\r',
		'\n',
		'\0',
	};

	bool escape = false;
	char character = consume();
	for (;;) {
		character = peek();

		if (!escape && character == '\\') {
			text += '\\';
			ignore();
			escape = true;
			continue;
		}

		if (!escape && exit.find(character) != exit.end()) {
			break;
		}

		text += character;
		ignore();

		escape = false;
	}

	if (character == '"') {
		text += character;
	}
	else {
		Error::the().addError({ Token::Type::Error, m_line, column, "expected '\"', got EOF" });
	}

	m_tokens.push_back({ Token::Type::String, m_line, column, text });

	return true;
}

bool Lexer::consumeKeyword()
{
	size_t column = m_column;
	std::string keyword;
	keyword += 0x7f; // 127

	ignore(); // :

	static std::unordered_set<char> exit = {
		'[',
		']',
		'{',
		'}',
		'(',
		')',
		'\'',
		'`',
		',',
		'"',
		';',
		' ',
		'\t',
		'\r',
		'\n',
		'\0',
	};

	char character = 0;
	for (;;) {
		character = peek();

		if (exit.find(character) != exit.end()) {
			break;
		}

		keyword += character;
		ignore();
	}

	m_tokens.push_back({ Token::Type::Keyword, m_line, column, keyword });

	retreat();

	return true;
}

bool Lexer::consumeValue()
{
	size_t column = m_column;
	std::string value = "";

	static std::unordered_set<char> exit = {
		'[',
		']',
		'{',
		'}',
		'(',
		')',
		'\'',
		'`',
		',',
		'"',
		';',
		' ',
		'\t',
		'\r',
		'\n',
		'\0',
	};

	char character = 0;
	for (;;) {
		character = peek();

		if (exit.find(character) != exit.end()) {
			break;
		}

		value += character;
		ignore();
	}

	m_tokens.push_back({ Token::Type::Value, m_line, column, value });

	retreat();

	return true;
}

bool Lexer::consumeComment()
{
	size_t column = m_column;
	std::string comment = "";

	ignore(); // ;

	static std::unordered_set<char> exit = {
		'\r',
		'\n',
		'\0',
	};

	char character = 0;
	for (;;) {
		character = peek();

		if (exit.find(character) != exit.end()) {
			break;
		}

		comment += character;
		ignore();
	}

	// Trim comment
	comment.erase(comment.begin(),
	              std::find_if(comment.begin(), comment.end(), [](char c) { return !std::isspace(c); }));
	comment.erase(std::find_if(comment.rbegin(), comment.rend(), [](char c) { return !std::isspace(c); }).base(),
	              comment.end());

	m_tokens.push_back({ Token::Type::Comment, m_line, column, comment });

	return true;
}

void Lexer::dump() const
{
	print("tokens: {}\n", m_tokens.size());
	print("\"");
	for (auto& token : m_tokens) {
		print("{}", token.symbol);
	}
	print("\"\n");
}

} // namespace blaze
