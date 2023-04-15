/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <memory>
#include <string>

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "forward.h"
#include "printer.h"
#include "types.h"

namespace blaze {

Collection::Collection(const std::list<ValuePtr>& nodes)
	: m_nodes(nodes)
{
}

void Collection::add(ValuePtr node)
{
	if (node == nullptr) {
		return;
	}

	m_nodes.push_back(node);
}

// -----------------------------------------

List::List(const std::list<ValuePtr>& nodes)
	: Collection(nodes)
{
}

// -----------------------------------------

Vector::Vector(const std::list<ValuePtr>& nodes)
	: Collection(nodes)
{
}

// -----------------------------------------

HashMap::HashMap(const Elements& elements)
	: m_elements(elements)
{
}

void HashMap::add(const std::string& key, ValuePtr value)
{
	if (value == nullptr) {
		return;
	}

	m_elements.emplace(key, value);
}

void HashMap::add(ValuePtr key, ValuePtr value)
{
	if (key == nullptr || value == nullptr) {
		return;
	}

	m_elements.emplace(getKeyString(key), value);
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

Function::Function(const std::string& name, FunctionType function)
	: m_name(name)
	, m_function(function)
{
}

// -----------------------------------------

Lambda::Lambda(const std::vector<std::string>& bindings, ValuePtr body, EnvironmentPtr env)
	: m_bindings(bindings)
	, m_body(body)
	, m_env(env)
{
}

Lambda::Lambda(std::shared_ptr<Lambda> that, bool is_macro)
	: m_bindings(that->m_bindings)
	, m_body(that->m_body)
	, m_env(that->m_env)
	, m_is_macro(is_macro)
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
