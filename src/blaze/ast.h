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
#include <map>
#include <memory> // std::shared_ptr
#include <span>
#include <string>
#include <string_view>
#include <typeinfo> // typeid
#include <vector>

#include "ruc/format/formatter.h"

#include "blaze/forward.h"

namespace blaze {

template<typename T, typename... Args>
std::shared_ptr<T> makePtr(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

// -----------------------------------------

class Value {
public:
	virtual ~Value() = default;

	virtual ValuePtr withMetaImpl(ValuePtr meta) const = 0;

	ValuePtr withMeta(ValuePtr meta) const;
	ValuePtr meta() const;

	std::string className() const { return typeid(*this).name(); }

	template<typename T>
	bool fastIs() const = delete;

	virtual bool isCollection() const { return false; }
	virtual bool isList() const { return false; }
	virtual bool isVector() const { return false; }
	virtual bool isHashMap() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isKeyword() const { return false; }
	virtual bool isNumeric() const { return false; }
	virtual bool isNumber() const { return false; }
	virtual bool isDecimal() const { return false; }
	virtual bool isConstant() const { return false; }
	virtual bool isSymbol() const { return false; }
	virtual bool isCallable() const { return false; }
	virtual bool isFunction() const { return false; }
	virtual bool isLambda() const { return false; }
	virtual bool isMacro() const { return false; }
	virtual bool isAtom() const { return false; }

protected:
	Value() {}

	Value(ValuePtr meta)
		: m_meta(meta)
	{
	}

	ValuePtr m_meta;
};

#define WITH_META(Type)                                         \
	virtual ValuePtr withMetaImpl(ValuePtr meta) const override \
	{                                                           \
		return makePtr<Type>(*this, meta);                      \
	}

#define WITH_NO_META()                                          \
	virtual ValuePtr withMetaImpl(ValuePtr meta) const override \
	{                                                           \
		(void)meta;                                             \
		return nullptr;                                         \
	}

// -----------------------------------------

template<typename T>
concept IsValue = std::same_as<Value, T> || std::derived_from<T, Value>;

class Collection : public Value {
public:
	virtual ~Collection() = default;

	// TODO: rename size -> count
	size_t size() const { return m_nodes.size(); }
	bool empty() const { return m_nodes.size() == 0; }

	ValuePtr front() const { return m_nodes.front(); }
	ValueVector rest() const;

	ValueVectorConstIt begin() const { return m_nodes.cbegin(); }
	ValueVectorConstIt end() const { return m_nodes.cend(); }
	ValueVectorConstReverseIt beginReverse() const { return m_nodes.crbegin(); }
	ValueVectorConstReverseIt endReverse() const { return m_nodes.crend(); }

	const ValueVector& nodesCopy() const { return m_nodes; }
	std::span<const ValuePtr> nodesRead() const { return m_nodes; }

protected:
	Collection() = default;
	Collection(const ValueVector& nodes);
	Collection(ValueVector&& nodes) noexcept;
	Collection(ValueVectorIt begin, ValueVectorIt end);
	Collection(ValueVectorConstIt begin, ValueVectorConstIt end);
	Collection(const Collection& that, ValuePtr meta);

	template<IsValue... Ts>
	Collection(std::shared_ptr<Ts>... nodes)
	{
		m_nodes = { nodes... };
	}

private:
	virtual bool isCollection() const override { return true; }

	ValueVector m_nodes;
};

// -----------------------------------------

// ()
class List final : public Collection {
public:
	List() = default;
	List(const ValueVector& nodes);
	List(ValueVector&& nodes) noexcept;
	List(ValueVectorIt begin, ValueVectorIt end);
	List(ValueVectorConstIt begin, ValueVectorConstIt end);
	List(const List& that, ValuePtr meta);

	template<IsValue... Ts>
	List(std::shared_ptr<Ts>... nodes)
		: Collection(nodes...)
	{
	}

	virtual ~List() = default;

	WITH_META(List);

private:
	virtual bool isList() const override { return true; }
};

// -----------------------------------------

// []
class Vector final : public Collection {
public:
	Vector() = default;
	Vector(const ValueVector& nodes);
	Vector(ValueVector&& nodes) noexcept;
	Vector(ValueVectorIt begin, ValueVectorIt end);
	Vector(ValueVectorConstIt begin, ValueVectorConstIt end);
	Vector(const Vector& that, ValuePtr meta);

	template<IsValue... Ts>
	Vector(std::shared_ptr<Ts>... nodes)
		: Collection(nodes...)
	{
	}

	virtual ~Vector() = default;

	WITH_META(Vector);

private:
	virtual bool isVector() const override { return true; }
};

// -----------------------------------------

using Elements = std::map<std::string, ValuePtr>;

// {}
class HashMap final : public Value {
public:
	HashMap() = default;
	HashMap(const Elements& elements);
	HashMap(const HashMap& that, ValuePtr meta);
	virtual ~HashMap() = default;

	static std::string getKeyString(ValuePtr key);

	bool exists(const std::string& key);
	bool exists(ValuePtr key);
	ValuePtr get(const std::string& key);
	ValuePtr get(ValuePtr key);
	const Elements& elements() const { return m_elements; }
	size_t size() const { return m_elements.size(); }
	bool empty() const { return m_elements.size() == 0; }

	WITH_META(HashMap);

private:
	virtual bool isHashMap() const override { return true; }

	Elements m_elements;
};

// -----------------------------------------

// "string"
class String final : public Value {
public:
	String(const std::string& data);
	String(char character);
	virtual ~String() = default;

	const std::string& data() const { return m_data; }
	size_t size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }

	WITH_NO_META();

private:
	virtual bool isString() const override { return true; }

	const std::string m_data;
};

// -----------------------------------------

// :keyword
class Keyword final : public Value {
public:
	Keyword(const std::string& data);
	Keyword(int64_t number);
	virtual ~Keyword() = default;

	virtual bool isKeyword() const override { return true; }

	const std::string& keyword() const { return m_data; }

	WITH_NO_META();

private:
	const std::string m_data;
};

// -----------------------------------------

class Numeric : public Value {
public:
	virtual ~Numeric() = default;

protected:
	Numeric() = default;

	virtual bool isNumeric() const override { return true; }
};

// 123
class Number final : public Numeric {
public:
	Number(int64_t number);
	virtual ~Number() = default;

	int64_t number() const { return m_number; }

	WITH_NO_META();

private:
	virtual bool isNumber() const override { return true; }

	const int64_t m_number { 0 };
};

// 123.456
class Decimal final : public Numeric {
public:
	Decimal(double decimal);
	virtual ~Decimal() = default;

	double decimal() const { return m_decimal; }

	WITH_NO_META();

private:
	virtual bool isDecimal() const override { return true; }

	const double m_decimal { 0 };
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

	WITH_NO_META();

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

	WITH_NO_META();

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
	Callable(ValuePtr meta);

private:
	virtual bool isCallable() const override { return true; }
};

// -----------------------------------------

using FunctionType = std::function<ValuePtr(ValueVectorConstIt, ValueVectorConstIt)>;

class Function final : public Callable {
public:
	Function(std::string_view name, std::string_view bindings, std::string_view documentation, FunctionType function);
	Function(const Function& that, ValuePtr meta);
	virtual ~Function() = default;

	std::string_view name() const { return m_name; }
	std::string_view bindings() const { return m_bindings; }
	std::string_view documentation() const { return m_documentation; }
	FunctionType function() const { return m_function; }

	WITH_META(Function);

private:
	virtual bool isFunction() const override { return true; }

	std::string_view m_name;
	std::string_view m_bindings;
	std::string_view m_documentation;
	const FunctionType m_function;
};

// -----------------------------------------

class Lambda : public Callable {
public:
	Lambda(const std::vector<std::string>& bindings, ValuePtr body, EnvironmentPtr env);
	Lambda(const Lambda& that);
	Lambda(const Lambda& that, ValuePtr meta);
	virtual ~Lambda() = default;

	const std::vector<std::string>& bindings() const { return m_bindings; }
	ValuePtr body() const { return m_body; }
	EnvironmentPtr env() const { return m_env; }

	WITH_META(Lambda);

private:
	virtual bool isLambda() const override { return true; }

	const std::vector<std::string> m_bindings;
	const ValuePtr m_body;
	const EnvironmentPtr m_env;
};

// -----------------------------------------

class Macro final : public Lambda {
public:
	Macro(const Lambda& that);

	WITH_NO_META();

private:
	virtual bool isLambda() const override { return false; }
	virtual bool isMacro() const override { return true; }
};

// -----------------------------------------

class Atom final : public Value {
public:
	Atom() = default;
	Atom(ValuePtr pointer);
	virtual ~Atom() = default;

	ValuePtr reset(ValuePtr value) { return m_value = value; }
	ValuePtr deref() const { return m_value; }

	WITH_NO_META();

private:
	virtual bool isAtom() const override { return true; }

	ValuePtr m_value;
};

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
inline bool Value::fastIs<Numeric>() const { return isNumeric(); }

template<>
inline bool Value::fastIs<Number>() const { return isNumber(); }

template<>
inline bool Value::fastIs<Decimal>() const { return isDecimal(); }

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
inline bool Value::fastIs<Macro>() const { return isMacro(); }

template<>
inline bool Value::fastIs<Atom>() const { return isAtom(); }
// clang-format on

} // namespace blaze

// -----------------------------------------

template<>
struct ruc::format::Formatter<blaze::ValuePtr> : public Formatter<std::string> {
	void format(Builder& builder, blaze::ValuePtr value) const;
};
