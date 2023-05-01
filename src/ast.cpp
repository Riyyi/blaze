/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <memory>  // std::static_pointer_cast
#include <string>
#include <vector>

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "forward.h"
#include "printer.h"
#include "types.h"

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

Collection::Collection(const ValueList& nodes)
	: m_nodes(nodes)
{
}

Collection::Collection(ValueListIt begin, ValueListIt end)
	: m_nodes(ValueList(begin, end))
{
}

Collection::Collection(ValueListConstIt begin, ValueListConstIt end)
	: m_nodes(ValueList(begin, end))
{
}

Collection::Collection(const Collection& that, ValuePtr meta)
	: Value(meta)
	, m_nodes(that.m_nodes)
{
}

void Collection::add(ValuePtr node)
{
	if (node == nullptr) {
		return;
	}

	m_nodes.push_back(node);
}

ValueList Collection::rest() const
{
	auto start = (m_nodes.size() > 0) ? m_nodes.begin() + 1 : m_nodes.end();
	return ValueList(start, m_nodes.end());
}

// -----------------------------------------

List::List(const ValueList& nodes)
	: Collection(nodes)
{
}

List::List(ValueListIt begin, ValueListIt end)
	: Collection(begin, end)
{
}

List::List(ValueListConstIt begin, ValueListConstIt end)
	: Collection(begin, end)
{
}

List::List(const List& that, ValuePtr meta)
	: Collection(that, meta)
{
}

// -----------------------------------------

Vector::Vector(const ValueList& nodes)
	: Collection(nodes)
{
}

Vector::Vector(ValueListIt begin, ValueListIt end)
	: Collection(begin, end)
{
}

Vector::Vector(ValueListConstIt begin, ValueListConstIt end)
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

void HashMap::add(const std::string& key, ValuePtr value)
{
	if (value == nullptr) {
		return;
	}

	m_elements.insert_or_assign(key, value);
}

void HashMap::add(ValuePtr key, ValuePtr value)
{
	if (key == nullptr || value == nullptr) {
		return;
	}

	m_elements.insert_or_assign(getKeyString(key), value);
}

void HashMap::remove(const std::string& key)
{
	m_elements.erase(key);
}

void HashMap::remove(ValuePtr key)
{
	if (key == nullptr) {
		return;
	}

	m_elements.erase(getKeyString(key));
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

std::string HashMap::getKeyString(ValuePtr key)
{
	if (!is<String>(key.get()) && !is<Keyword>(key.get())) {
		Error::the().add(format("wrong argument type: string or keyword, {}", key));
		return {};
	}

	return is<String>(key.get())
	           ? std::static_pointer_cast<String>(key)->data()
	           : std::static_pointer_cast<Keyword>(key)->keyword();
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

// -----------------------------------------

Number::Number(int64_t number)
	: m_number(number)
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

Function::Function(const std::string& name, FunctionType function)
	: m_name(name)
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
	: m_bindings(bindings)
	, m_body(body)
	, m_env(env)
{
}

Lambda::Lambda(const Lambda& that)
	: m_bindings(that.m_bindings)
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
