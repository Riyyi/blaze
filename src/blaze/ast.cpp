/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <memory>  // std::static_pointer_cast
#include <string>
#include <utility> // std::move
#include <vector>

#include "blaze/ast.h"
#include "blaze/env/environment.h"
#include "blaze/error.h"
#include "blaze/forward.h"
#include "blaze/printer.h"
#include "blaze/types.h"

namespace blaze {

ValuePtr Value::withMeta(ValuePtr meta) const
{
	return withMetaImpl(meta);
}

ValuePtr Value::meta() const
{
	return (m_meta == nullptr) ? makePtr<Constant>() : m_meta;
}

// -----------------------------------------

Collection::Collection(const ValueVector& nodes)
	: m_nodes(nodes)
{
}

Collection::Collection(ValueVector&& nodes) noexcept
	: m_nodes(std::move(nodes))
{
}

Collection::Collection(ValueVectorIt begin, ValueVectorIt end)
	: m_nodes(ValueVector(begin, end))
{
}

Collection::Collection(ValueVectorConstIt begin, ValueVectorConstIt end)
	: m_nodes(ValueVector(begin, end))
{
}

Collection::Collection(const Collection& that, ValuePtr meta)
	: Value(meta)
	, m_nodes(that.m_nodes)
{
}

ValueVector Collection::rest() const
{
	auto start = (m_nodes.size() > 0) ? m_nodes.begin() + 1 : m_nodes.end();
	return ValueVector(start, m_nodes.end());
}

// -----------------------------------------

List::List(const ValueVector& nodes)
	: Collection(nodes)
{
}

List::List(ValueVector&& nodes) noexcept
	: Collection(std::move(nodes))
{
}

List::List(ValueVectorIt begin, ValueVectorIt end)
	: Collection(begin, end)
{
}

List::List(ValueVectorConstIt begin, ValueVectorConstIt end)
	: Collection(begin, end)
{
}

List::List(const List& that, ValuePtr meta)
	: Collection(that, meta)
{
}

// -----------------------------------------

Vector::Vector(const ValueVector& nodes)
	: Collection(nodes)
{
}

Vector::Vector(ValueVector&& nodes) noexcept
	: Collection(std::move(nodes))
{
}

Vector::Vector(ValueVectorIt begin, ValueVectorIt end)
	: Collection(begin, end)
{
}

Vector::Vector(ValueVectorConstIt begin, ValueVectorConstIt end)
	: Collection(begin, end)
{
}

Vector::Vector(const Vector& that, ValuePtr meta)
	: Collection(that, meta)
{
}

// -----------------------------------------

HashMap::HashMap(const Elements& elements)
	: m_elements(elements)
{
}

HashMap::HashMap(const HashMap& that, ValuePtr meta)
	: Value(meta)
	, m_elements(that.m_elements)
{
}

std::string HashMap::getKeyString(ValuePtr key)
{
	if (!is<String>(key.get()) && !is<Keyword>(key.get())) {
		Error::the().add(::format("wrong argument type: string or keyword, {}", key));
		return {};
	}

	return is<String>(key.get())
	           ? std::static_pointer_cast<String>(key)->data()
	           : std::static_pointer_cast<Keyword>(key)->keyword();
}

bool HashMap::exists(const std::string& key)
{
	return m_elements.find(key) != m_elements.end();
}

bool HashMap::exists(ValuePtr key)
{
	return exists(getKeyString(key));
}

ValuePtr HashMap::get(const std::string& key)
{
	if (!exists(key)) {
		return nullptr;
	}

	return m_elements[key];
}

ValuePtr HashMap::get(ValuePtr key)
{
	return get(getKeyString(key));
}

// -----------------------------------------

String::String(const std::string& data)
	: m_data(data)
{
}

String::String(char character)
	: m_data(std::string(1, character))
{
}

// -----------------------------------------

Keyword::Keyword(const std::string& data)
	: m_data(std::string(1, 0x7f) + data) // 127
{
}

Keyword::Keyword(int64_t number)
	: m_data(std::string(1, 0x7f) + std::to_string(number)) // 127
{
}

// -----------------------------------------

Number::Number(int64_t number)
	: Numeric()
	, m_number(number)
{
}

Decimal::Decimal(double decimal)
	: Numeric()
	, m_decimal(decimal)
{
}

// -----------------------------------------

Constant::Constant(State state)
	: m_state(state)
{
}

Constant::Constant(bool state)
	: m_state(state ? Constant::True : Constant::False)
{
}

// -----------------------------------------

Symbol::Symbol(const std::string& symbol)
	: m_symbol(symbol)
{
}

// -----------------------------------------

Callable::Callable(ValuePtr meta)
	: Value(meta)
{
}

// -----------------------------------------

Function::Function(std::string_view name, std::string_view bindings, std::string_view documentation, FunctionType function)
	: Callable()
	, m_name(name)
	, m_bindings(bindings)
	, m_documentation(documentation)
	, m_function(function)
{
}

Function::Function(const Function& that, ValuePtr meta)
	: Callable(meta)
	, m_name(that.m_name)
	, m_function(that.m_function)
{
}

// -----------------------------------------

Lambda::Lambda(const std::vector<std::string>& bindings, ValuePtr body, EnvironmentPtr env)
	: Callable()
	, m_bindings(bindings)
	, m_body(body)
	, m_env(env)
{
}

Lambda::Lambda(const Lambda& that)
	: Callable()
	, m_bindings(that.m_bindings)
	, m_body(that.m_body)
	, m_env(that.m_env)
{
}

Lambda::Lambda(const Lambda& that, ValuePtr meta)
	: Callable(meta)
	, m_bindings(that.m_bindings)
	, m_body(that.m_body)
	, m_env(that.m_env)
{
}

// -----------------------------------------

Macro::Macro(const Lambda& that)
	: Lambda(that)
{
}

// -----------------------------------------

Atom::Atom(ValuePtr pointer)
	: m_value(pointer)
{
}

} // namespace blaze

// -----------------------------------------

void Formatter<blaze::ValuePtr>::format(Builder& builder, blaze::ValuePtr value) const
{
	blaze::Printer printer;
	return Formatter<std::string>::format(builder, printer.printNoErrorCheck(value));
}
