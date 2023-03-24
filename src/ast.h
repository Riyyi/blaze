/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint> // int64_t
#include <string>
#include <string_view>
#include <typeinfo> // typeid
#include <vector>

#include "ruc/format/formatter.h"

namespace blaze {

class ASTNode {
public:
	virtual ~ASTNode() = default;

	std::string className() const { return typeid(*this).name(); }

	template<typename T>
	bool fastIs() const = delete;

	virtual bool isCollection() const { return false; }
	virtual bool isVector() const { return false; }
	virtual bool isHashMap() const { return false; }
	virtual bool isList() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isKeyword() const { return false; }
	virtual bool isNumber() const { return false; }
	virtual bool isValue() const { return false; }
	virtual bool isSymbol() const { return false; }
	virtual bool isFunction() const { return false; }

protected:
	ASTNode() {}
};

// -----------------------------------------

class Collection : public ASTNode {
public:
	virtual ~Collection() override;

	virtual bool isCollection() const override { return true; }

	void addNode(ASTNode* node);

	const std::vector<ASTNode*>& nodes() const { return m_nodes; }

protected:
	Collection() {}

private:
	std::vector<ASTNode*> m_nodes;
};

// -----------------------------------------

// []
class Vector final : public Collection {
public:
	Vector() = default;
	virtual ~Vector() = default;

	virtual bool isCollection() const override { return false; }
	virtual bool isVector() const override { return true; }
};

// -----------------------------------------

// {}
class HashMap final : public Collection {
public:
	HashMap() = default;
	virtual ~HashMap() = default;

	virtual bool isCollection() const override { return false; }
	virtual bool isHashMap() const override { return true; }
};

// -----------------------------------------

// ()
class List final : public Collection {
public:
	List() = default;
	virtual ~List() = default;

	virtual bool isCollection() const override { return false; }
	virtual bool isList() const override { return true; }
};

// -----------------------------------------

// "string"
class String final : public ASTNode {
public:
	String(const std::string& data);
	virtual ~String() = default;

	virtual bool isString() const override { return true; }

	const std::string& data() const { return m_data; }

private:
	std::string m_data;
};

// -----------------------------------------

// :keyword
class Keyword final : public ASTNode {
public:
	Keyword(const std::string& data);
	virtual ~Keyword() = default;

	virtual bool isKeyword() const override { return true; }

	const std::string& keyword() const { return m_data; }

private:
	std::string m_data;
};

// -----------------------------------------
// 123
class Number final : public ASTNode {
public:
	Number(int64_t number);
	virtual ~Number() = default;

	virtual bool isNumber() const override { return true; }

	int64_t number() const { return m_number; }

private:
	int64_t m_number { 0 };
};

// -----------------------------------------

// Symbols
class Symbol final : public ASTNode {
public:
	Symbol(const std::string& symbol);
	virtual ~Symbol() = default;

	virtual bool isSymbol() const override { return true; }

	const std::string& symbol() const { return m_symbol; }

private:
	std::string m_symbol;
};

// -----------------------------------------

// true, false, nil
class Value final : public ASTNode {
public:
	Value(const std::string& value);
	virtual ~Value() = default;

	virtual bool isValue() const override { return true; }

	const std::string& value() const { return m_value; }

private:
	std::string m_value;
};
};

// -----------------------------------------

// clang-format off
template<>
inline bool ASTNode::fastIs<Collection>() const { return isCollection(); }

template<>
inline bool ASTNode::fastIs<Vector>() const { return isVector(); }

template<>
inline bool ASTNode::fastIs<HashMap>() const { return isHashMap(); }

template<>
inline bool ASTNode::fastIs<List>() const { return isList(); }

template<>
inline bool ASTNode::fastIs<String>() const { return isString(); }

template<>
inline bool ASTNode::fastIs<Keyword>() const { return isKeyword(); }

template<>
inline bool ASTNode::fastIs<Number>() const { return isNumber(); }

template<>
inline bool ASTNode::fastIs<Symbol>() const { return isSymbol(); }

template<>
inline bool ASTNode::fastIs<Value>() const { return isValue(); }
// clang-format on

} // namespace blaze

// -----------------------------------------

template<>
struct ruc::format::Formatter<blaze::ASTNode*> : public Formatter<std::string> {
	void format(Builder& builder, blaze::ASTNode* value) const;
};
