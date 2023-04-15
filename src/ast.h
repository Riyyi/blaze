/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>   // std::derived_from, std::same_as
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

class Value {
public:
	virtual ~Value() = default;

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
	virtual bool isConstant() const { return false; }
	virtual bool isSymbol() const { return false; }
	virtual bool isCallable() const { return false; }
	virtual bool isFunction() const { return false; }
	virtual bool isLambda() const { return false; }
	virtual bool isAtom() const { return false; }

protected:
	Value() {}
};

// -----------------------------------------

template<typename T>
concept IsValue = std::same_as<Value, T> || std::derived_from<T, Value>;

class Collection : public Value {
public:
	virtual ~Collection() = default;

	void add(ValuePtr node);

	size_t size() const { return m_nodes.size(); }
	bool empty() const { return m_nodes.size() == 0; }

	const std::list<ValuePtr>& nodes() const { return m_nodes; }

protected:
	Collection() = default;
	Collection(const std::list<ValuePtr>& nodes);

	template<IsValue... Ts>
	Collection(std::shared_ptr<Ts>... nodes)
	{
		m_nodes = { nodes... };
	}

private:
	virtual bool isCollection() const override { return true; }

	std::list<ValuePtr> m_nodes;
};

// -----------------------------------------

// ()
class List final : public Collection {
public:
	List() = default;
	List(const std::list<ValuePtr>& nodes);

	template<IsValue... Ts>
	List(std::shared_ptr<Ts>... nodes)
		: Collection(nodes...)
	{
	}

	virtual ~List() = default;

private:
	virtual bool isList() const override { return true; }
};

// -----------------------------------------

// []
class Vector final : public Collection {
public:
	Vector() = default;
	Vector(const std::list<ValuePtr>& nodes);

	template<IsValue... Ts>
	Vector(std::shared_ptr<Ts>... nodes)
		: Collection(nodes...)
	{
	}

	virtual ~Vector() = default;

private:
	virtual bool isVector() const override { return true; }
};

// -----------------------------------------

// {}
class HashMap final : public Value {
public:
	HashMap() = default;
	virtual ~HashMap() = default;

	void add(const std::string& key, ValuePtr value);

	const std::unordered_map<std::string, ValuePtr>& elements() const { return m_elements; }
	size_t size() const { return m_elements.size(); }
	bool empty() const { return m_elements.size() == 0; }

private:
	virtual bool isHashMap() const override { return true; }

	std::unordered_map<std::string, ValuePtr> m_elements;
};

// -----------------------------------------

// "string"
class String final : public Value {
public:
	String(const std::string& data);
	virtual ~String() = default;

	const std::string& data() const { return m_data; }

private:
	virtual bool isString() const override { return true; }

	const std::string m_data;
};

// -----------------------------------------

// :keyword
class Keyword final : public Value {
public:
	Keyword(const std::string& data);
	virtual ~Keyword() = default;

	virtual bool isKeyword() const override { return true; }

	const std::string& keyword() const { return m_data; }

private:
	const std::string m_data;
};

// -----------------------------------------
// 123
class Number final : public Value {
public:
	Number(int64_t number);
	virtual ~Number() = default;

	int64_t number() const { return m_number; }

private:
	virtual bool isNumber() const override { return true; }

	const int64_t m_number { 0 };
};

// -----------------------------------------

// true, false, nil
class Constant final : public Value {
public:
	enum State : uint8_t {
		Nil,
		True,
		False,
	};

	Constant() = default;
	Constant(State state);
	Constant(bool state);
	virtual ~Constant() = default;

	State state() const { return m_state; }

private:
	virtual bool isConstant() const override { return true; }

	const State m_state { State::Nil };
};

// -----------------------------------------

// Symbols
class Symbol final : public Value {
public:
	Symbol(const std::string& symbol);
	virtual ~Symbol() = default;

	const std::string& symbol() const { return m_symbol; }

private:
	virtual bool isSymbol() const override { return true; }

	const std::string m_symbol;
};

// -----------------------------------------

class Callable : public Value {
public:
	virtual ~Callable() = default;

protected:
	Callable() = default;

private:
	virtual bool isCallable() const override { return true; }
};

// -----------------------------------------

using FunctionType = std::function<ValuePtr(std::list<ValuePtr>)>;

class Function final : public Callable {
public:
	Function(const std::string& name, FunctionType function);
	virtual ~Function() = default;

	const std::string& name() const { return m_name; }
	FunctionType function() const { return m_function; }

private:
	virtual bool isFunction() const override { return true; }

	const std::string m_name;
	const FunctionType m_function;
};

// -----------------------------------------

class Lambda final : public Callable {
public:
	Lambda(const std::vector<std::string>& bindings, ValuePtr body, EnvironmentPtr env);
	Lambda(std::shared_ptr<Lambda> that, bool is_macro);
	virtual ~Lambda() = default;

	const std::vector<std::string>& bindings() const { return m_bindings; }
	ValuePtr body() const { return m_body; }
	EnvironmentPtr env() const { return m_env; }
	bool isMacro() const { return m_is_macro; }

private:
	virtual bool isLambda() const override { return true; }

	const std::vector<std::string> m_bindings;
	const ValuePtr m_body;
	const EnvironmentPtr m_env;
	const bool m_is_macro { false };
};

// -----------------------------------------

class Atom final : public Value {
public:
	Atom() = default;
	Atom(ValuePtr pointer);
	virtual ~Atom() = default;

	ValuePtr reset(ValuePtr value) { return m_value = value; }
	ValuePtr deref() const { return m_value; }

private:
	virtual bool isAtom() const override { return true; }

	ValuePtr m_value;
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
inline bool Value::fastIs<Collection>() const { return isCollection(); }

template<>
inline bool Value::fastIs<List>() const { return isList(); }

template<>
inline bool Value::fastIs<Vector>() const { return isVector(); }

template<>
inline bool Value::fastIs<HashMap>() const { return isHashMap(); }

template<>
inline bool Value::fastIs<String>() const { return isString(); }

template<>
inline bool Value::fastIs<Keyword>() const { return isKeyword(); }

template<>
inline bool Value::fastIs<Number>() const { return isNumber(); }

template<>
inline bool Value::fastIs<Constant>() const { return isConstant(); }

template<>
inline bool Value::fastIs<Symbol>() const { return isSymbol(); }

template<>
inline bool Value::fastIs<Callable>() const { return isCallable(); }

template<>
inline bool Value::fastIs<Function>() const { return isFunction(); }

template<>
inline bool Value::fastIs<Lambda>() const { return isLambda(); }

template<>
inline bool Value::fastIs<Atom>() const { return isAtom(); }
// clang-format on

} // namespace blaze

// -----------------------------------------

template<>
struct ruc::format::Formatter<blaze::ValuePtr> : public Formatter<std::string> {
	void format(Builder& builder, blaze::ValuePtr value) const;
};
