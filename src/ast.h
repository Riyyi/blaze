/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>    // int64_t, uint8_t
#include <functional> // std::function
#include <list>
#include <memory> // std::shared_ptr
#include <span>
#include <string>
#include <string_view>
#include <typeinfo> // typeid
#include <unordered_map>
#include <vector>

#include "ruc/format/formatter.h"

#include "forward.h"

namespace blaze {

class ASTNode {
public:
	virtual ~ASTNode() = default;

	std::string className() const { return typeid(*this).name(); }

	template<typename T>
	bool fastIs() const = delete;

	virtual bool isCollection() const { return false; }
	virtual bool isList() const { return false; }
	virtual bool isVector() const { return false; }
	virtual bool isHashMap() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isKeyword() const { return false; }
	virtual bool isNumber() const { return false; }
	virtual bool isValue() const { return false; }
	virtual bool isSymbol() const { return false; }
	virtual bool isFunction() const { return false; }
	virtual bool isLambda() const { return false; }

protected:
	ASTNode() {}
};

// -----------------------------------------

class Collection : public ASTNode {
public:
	virtual ~Collection() = default;

	virtual bool isCollection() const override { return true; }

	void add(ASTNodePtr node);

	const std::list<ASTNodePtr>& nodes() const { return m_nodes; }
	size_t size() const { return m_nodes.size(); }
	bool empty() const { return m_nodes.size() == 0; }

protected:
	Collection() {}

private:
	std::list<ASTNodePtr> m_nodes;
};

// -----------------------------------------

// ()
class List final : public Collection {
public:
	List() = default;
	virtual ~List() = default;

	virtual bool isList() const override { return true; }
};

// -----------------------------------------

// []
class Vector final : public Collection {
public:
	Vector() = default;
	virtual ~Vector() = default;

	virtual bool isVector() const override { return true; }
};

// -----------------------------------------

// {}
class HashMap final : public ASTNode {
public:
	HashMap() = default;
	virtual ~HashMap() = default;

	virtual bool isHashMap() const override { return true; }

	void addElement(const std::string& key, ASTNodePtr value);

	const std::unordered_map<std::string, ASTNodePtr>& elements() const { return m_elements; }
	size_t size() const { return m_elements.size(); }
	bool empty() const { return m_elements.size() == 0; }

private:
	std::unordered_map<std::string, ASTNodePtr> m_elements;
};

// -----------------------------------------

// "string"
class String final : public ASTNode {
public:
	explicit String(const std::string& data);
	virtual ~String() = default;

	virtual bool isString() const override { return true; }

	const std::string& data() const { return m_data; }

private:
	const std::string m_data;
};

// -----------------------------------------

// :keyword
class Keyword final : public ASTNode {
public:
	explicit Keyword(const std::string& data);
	virtual ~Keyword() = default;

	virtual bool isKeyword() const override { return true; }

	const std::string& keyword() const { return m_data; }

private:
	const std::string m_data;
};

// -----------------------------------------
// 123
class Number final : public ASTNode {
public:
	explicit Number(int64_t number);
	virtual ~Number() = default;

	virtual bool isNumber() const override { return true; }

	int64_t number() const { return m_number; }

private:
	const int64_t m_number { 0 };
};

// -----------------------------------------

// true, false, nil
class Value final : public ASTNode {
public:
	enum State : uint8_t {
		Nil,
		True,
		False,
	};

	explicit Value(State state);
	virtual ~Value() = default;

	virtual bool isValue() const override { return true; }

	State state() const { return m_state; }

private:
	const State m_state;
};

// -----------------------------------------

// Symbols
class Symbol final : public ASTNode {
public:
	explicit Symbol(const std::string& symbol);
	virtual ~Symbol() = default;

	virtual bool isSymbol() const override { return true; }

	const std::string& symbol() const { return m_symbol; }

private:
	const std::string m_symbol;
};

// -----------------------------------------

using FunctionType = std::function<ASTNodePtr(std::list<ASTNodePtr>)>;

class Function final : public ASTNode {
public:
	explicit Function(const std::string& name, FunctionType function);
	virtual ~Function() = default;

	virtual bool isFunction() const override { return true; }

	const std::string& name() const { return m_name; }
	FunctionType function() const { return m_function; }

private:
	const std::string m_name;
	const FunctionType m_function;
};

// -----------------------------------------

class Lambda final : public ASTNode {
public:
	Lambda(std::vector<std::string> bindings, ASTNodePtr body, EnvironmentPtr env);
	virtual ~Lambda() = default;

	virtual bool isLambda() const override { return true; }

	std::vector<std::string> bindings() const { return m_bindings; }
	ASTNodePtr body() const { return m_body; }
	EnvironmentPtr env() const { return m_env; }

private:
	const std::vector<std::string> m_bindings;
	const ASTNodePtr m_body;
	const EnvironmentPtr m_env;
};

// -----------------------------------------

template<typename T, typename... Args>
std::shared_ptr<T> makePtr(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

// -----------------------------------------

// clang-format off
template<>
inline bool ASTNode::fastIs<Collection>() const { return isCollection(); }

template<>
inline bool ASTNode::fastIs<List>() const { return isList(); }

template<>
inline bool ASTNode::fastIs<Vector>() const { return isVector(); }

template<>
inline bool ASTNode::fastIs<HashMap>() const { return isHashMap(); }

template<>
inline bool ASTNode::fastIs<String>() const { return isString(); }

template<>
inline bool ASTNode::fastIs<Keyword>() const { return isKeyword(); }

template<>
inline bool ASTNode::fastIs<Number>() const { return isNumber(); }

template<>
inline bool ASTNode::fastIs<Value>() const { return isValue(); }

template<>
inline bool ASTNode::fastIs<Symbol>() const { return isSymbol(); }

template<>
inline bool ASTNode::fastIs<Function>() const { return isFunction(); }

template<>
inline bool ASTNode::fastIs<Lambda>() const { return isLambda(); }
// clang-format on

} // namespace blaze

// -----------------------------------------

template<>
struct ruc::format::Formatter<blaze::ASTNodePtr> : public Formatter<std::string> {
	void format(Builder& builder, blaze::ASTNodePtr value) const;
};
