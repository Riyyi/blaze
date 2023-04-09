/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast
#include <string>

#include "ruc/file.h"
#include "ruc/format/format.h"

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "forward.h"
#include "printer.h"
#include "types.h"
#include "util.h"

// At the top-level you cant invoke any function, but you can create variables.
// Using a struct's constructor you can work around this limitation.
// Also the line number in the file is used to make the struct names unique.

#define FUNCTION_STRUCT_NAME(unique) __functionStruct##unique

#define ADD_FUNCTION_IMPL(unique, symbol, lambda)     \
	struct FUNCTION_STRUCT_NAME(unique) {             \
		FUNCTION_STRUCT_NAME(unique)                  \
		(std::string __symbol, FunctionType __lambda) \
		{                                             \
			s_functions.emplace(__symbol, __lambda);  \
		}                                             \
	};                                                \
	static struct FUNCTION_STRUCT_NAME(unique)        \
		FUNCTION_STRUCT_NAME(unique)(                 \
			symbol,                                   \
			[](std::list<ValuePtr> nodes) -> ValuePtr lambda);

#define ADD_FUNCTION(symbol, lambda) ADD_FUNCTION_IMPL(__LINE__, symbol, lambda);

namespace blaze {

static std::unordered_map<std::string, FunctionType> s_functions;

ADD_FUNCTION(
	"+",
	{
		int64_t result = 0;

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().add(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}

			result += std::static_pointer_cast<Number>(node)->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"-",
	{
		if (nodes.size() == 0) {
			return makePtr<Number>(0);
		}

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().add(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}
		}

		// Start with the first number
		int64_t result = std::static_pointer_cast<Number>(nodes.front())->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			result -= std::static_pointer_cast<Number>(*it)->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"*",
	{
		int64_t result = 1;

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().add(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}

			result *= std::static_pointer_cast<Number>(node)->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"/",
	{
		if (nodes.size() == 0) {
			Error::the().add(format("wrong number of arguments: /, 0"));
			return nullptr;
		}

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().add(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}
		}

		// Start with the first number
		double result = std::static_pointer_cast<Number>(nodes.front())->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			result /= std::static_pointer_cast<Number>(*it)->number();
		}

		return makePtr<Number>((int64_t)result);
	});

// // -----------------------------------------

#define NUMBER_COMPARE(operator)                                                                    \
	{                                                                                               \
		bool result = true;                                                                         \
                                                                                                    \
		if (nodes.size() < 2) {                                                                     \
			Error::the().add(format("wrong number of arguments: {}, {}", #operator, nodes.size())); \
			return nullptr;                                                                         \
		}                                                                                           \
                                                                                                    \
		for (auto node : nodes) {                                                                   \
			if (!is<Number>(node.get())) {                                                          \
				Error::the().add(format("wrong argument type: number, '{}'", node));                \
				return nullptr;                                                                     \
			}                                                                                       \
		}                                                                                           \
                                                                                                    \
		/* Start with the first number */                                                           \
		int64_t number = std::static_pointer_cast<Number>(nodes.front())->number();                 \
                                                                                                    \
		/* Skip the first node */                                                                   \
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {                         \
			int64_t current_number = std::static_pointer_cast<Number>(*it)->number();               \
			if (!(number operator current_number)) {                                                \
				result = false;                                                                     \
				break;                                                                              \
			}                                                                                       \
			number = current_number;                                                                \
		}                                                                                           \
                                                                                                    \
		return makePtr<Constant>((result) ? Constant::True : Constant::False);                      \
	}

ADD_FUNCTION("<", NUMBER_COMPARE(<));
ADD_FUNCTION("<=", NUMBER_COMPARE(<=));
ADD_FUNCTION(">", NUMBER_COMPARE(>));
ADD_FUNCTION(">=", NUMBER_COMPARE(>=));

// -----------------------------------------

ADD_FUNCTION(
	"list",
	{
		return makePtr<List>(nodes);
	});

ADD_FUNCTION(
	"list?",
	{
		bool result = true;

		if (nodes.size() == 0) {
			result = false;
		}

		for (auto node : nodes) {
			if (!is<List>(node.get())) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

ADD_FUNCTION(
	"empty?",
	{
		bool result = true;

		for (auto node : nodes) {
			if (!is<Collection>(node.get())) {
				Error::the().add(format("wrong argument type: collection, '{}'", node));
				return nullptr;
			}

			if (!std::static_pointer_cast<Collection>(node)->empty()) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

ADD_FUNCTION(
	"count",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: count, {}", nodes.size()));
			return nullptr;
		}

		auto first_argument = nodes.front();

		size_t result = 0;
		if (is<Constant>(first_argument.get()) && std::static_pointer_cast<Constant>(nodes.front())->state() == Constant::Nil) {
			// result = 0
		}
		else if (is<Collection>(first_argument.get())) {
			result = std::static_pointer_cast<Collection>(first_argument)->size();
		}
		else {
			Error::the().add(format("wrong argument type: collection, '{}'", first_argument));
			return nullptr;
		}

		// FIXME: Add numeric_limits check for implicit cast: size_t > int64_t
		return makePtr<Number>((int64_t)result);
	});

// -----------------------------------------

#define PRINTER_STRING(print_readably, concatenation)                               \
	{                                                                               \
		std::string result;                                                         \
                                                                                    \
		Printer printer;                                                            \
		for (auto it = nodes.begin(); it != nodes.end(); ++it) {                    \
			result += format("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                                    \
			if (!isLast(it, nodes)) {                                               \
				result += concatenation;                                            \
			}                                                                       \
		}                                                                           \
                                                                                    \
		return makePtr<String>(result);                                             \
	}

ADD_FUNCTION("str", PRINTER_STRING(false, ""));
ADD_FUNCTION("pr-str", PRINTER_STRING(true, " "));

#define PRINTER_PRINT(print_readably)                                    \
	{                                                                    \
		Printer printer;                                                 \
		for (auto it = nodes.begin(); it != nodes.end(); ++it) {         \
			print("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                         \
			if (!isLast(it, nodes)) {                                    \
				print(" ");                                              \
			}                                                            \
		}                                                                \
		print("\n");                                                     \
                                                                         \
		return makePtr<Constant>(Constant::Nil);                         \
	}

ADD_FUNCTION("prn", PRINTER_PRINT(true));
ADD_FUNCTION("println", PRINTER_PRINT(false));

// -----------------------------------------

ADD_FUNCTION(
	"=",
	{
		if (nodes.size() < 2) {
			Error::the().add(format("wrong number of arguments: =, {}", nodes.size()));
			return nullptr;
		}

		std::function<bool(ValuePtr, ValuePtr)> equal =
			[&equal](ValuePtr lhs, ValuePtr rhs) -> bool {
			if ((is<List>(lhs.get()) || is<Vector>(lhs.get()))
		        && (is<List>(rhs.get()) || is<Vector>(rhs.get()))) {
				auto lhs_nodes = std::static_pointer_cast<Collection>(lhs)->nodes();
				auto rhs_nodes = std::static_pointer_cast<Collection>(rhs)->nodes();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				auto lhs_it = lhs_nodes.begin();
				auto rhs_it = rhs_nodes.begin();
				for (; lhs_it != lhs_nodes.end(); ++lhs_it, ++rhs_it) {
					if (!equal(*lhs_it, *rhs_it)) {
						return false;
					}
				}

				return true;
			}

			if (is<HashMap>(lhs.get()) && is<HashMap>(rhs.get())) {
				auto lhs_nodes = std::static_pointer_cast<HashMap>(lhs)->elements();
				auto rhs_nodes = std::static_pointer_cast<HashMap>(rhs)->elements();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				for (const auto& [key, value] : lhs_nodes) {
					auto it = rhs_nodes.find(key);
					if (it == rhs_nodes.end() || !equal(value, it->second)) {
						return false;
					}
				}

				return true;
			}

			if (is<String>(lhs.get()) && is<String>(rhs.get())
		        && std::static_pointer_cast<String>(lhs)->data() == std::static_pointer_cast<String>(rhs)->data()) {
				return true;
			}
			if (is<Keyword>(lhs.get()) && is<Keyword>(rhs.get())
		        && std::static_pointer_cast<Keyword>(lhs)->keyword() == std::static_pointer_cast<Keyword>(rhs)->keyword()) {
				return true;
			}
			if (is<Number>(lhs.get()) && is<Number>(rhs.get())
		        && std::static_pointer_cast<Number>(lhs)->number() == std::static_pointer_cast<Number>(rhs)->number()) {
				return true;
			}
			if (is<Constant>(lhs.get()) && is<Constant>(rhs.get())
		        && std::static_pointer_cast<Constant>(lhs)->state() == std::static_pointer_cast<Constant>(rhs)->state()) {
				return true;
			}
			if (is<Symbol>(lhs.get()) && is<Symbol>(rhs.get())
		        && std::static_pointer_cast<Symbol>(lhs)->symbol() == std::static_pointer_cast<Symbol>(rhs)->symbol()) {
				return true;
			}

			return false;
		};

		bool result = true;
		auto it = nodes.begin();
		auto it_next = std::next(nodes.begin());
		for (; it_next != nodes.end(); ++it, ++it_next) {
			if (!equal(*it, *it_next)) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

ADD_FUNCTION(
	"read-string",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: read-string, {}", nodes.size()));
			return nullptr;
		}

		if (!is<String>(nodes.front().get())) {
			Error::the().add(format("wrong argument type: string, '{}'", nodes.front()));
			return nullptr;
		}

		std::string input = std::static_pointer_cast<String>(nodes.front())->data();

		return read(input);
	});

ADD_FUNCTION(
	"slurp",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: slurp, {}", nodes.size()));
			return nullptr;
		}

		if (!is<String>(nodes.front().get())) {
			Error::the().add(format("wrong argument type: string, '{}'", nodes.front()));
			return nullptr;
		}

		std::string path = std::static_pointer_cast<String>(nodes.front())->data();

		auto file = ruc::File(path);

		return makePtr<String>(file.data());
	});

ADD_FUNCTION(
	"eval",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: eval, {}", nodes.size()));
			return nullptr;
		}

		return eval(nodes.front(), nullptr);
	});

// (atom 1)
ADD_FUNCTION(
	"atom",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: atom, {}", nodes.size()));
			return nullptr;
		}

		return makePtr<Atom>(nodes.front());
	});

// (atom? myatom 2 "foo")
ADD_FUNCTION(
	"atom?",
	{
		bool result = true;

		if (nodes.size() == 0) {
			result = false;
		}

		for (auto node : nodes) {
			if (!is<Atom>(node.get())) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

// (deref myatom)
ADD_FUNCTION(
	"deref",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: deref, {}", nodes.size()));
			return nullptr;
		}

		if (!is<Atom>(nodes.front().get())) {
			Error::the().add(format("wrong argument type: atom, '{}'", nodes.front()));
			return nullptr;
		}

		return std::static_pointer_cast<Atom>(nodes.front())->deref();
	});

// (reset! myatom 2)
ADD_FUNCTION(
	"reset!",
	{
		if (nodes.size() != 2) {
			Error::the().add(format("wrong number of arguments: reset!, {}", nodes.size()));
			return nullptr;
		}

		if (!is<Atom>(nodes.front().get())) {
			Error::the().add(format("wrong argument type: atom, '{}'", nodes.front()));
			return nullptr;
		}

		auto atom = std::static_pointer_cast<Atom>(*nodes.begin());
		auto value = *std::next(nodes.begin());

		atom->reset(value);

		return value;
	});

// (swap! myatom (fn* [x] (+ 1 x)))
ADD_FUNCTION(
	"swap!",
	{
		if (nodes.size() < 2) {
			Error::the().add(format("wrong number of arguments: swap!, {}", nodes.size()));
			return nullptr;
		}

		auto first_argument = *nodes.begin();
		auto second_argument = *std::next(nodes.begin());

		if (!is<Atom>(first_argument.get())) {
			Error::the().add(format("wrong argument type: atom, '{}'", first_argument));
			return nullptr;
		}

		if (!is<Callable>(second_argument.get())) {
			Error::the().add(format("wrong argument type: function, '{}'", second_argument));
			return nullptr;
		}

		auto atom = std::static_pointer_cast<Atom>(first_argument);

		// Remove atom and function from the argument list, add atom value
		nodes.pop_front();
		nodes.pop_front();
		nodes.push_front(atom->deref());

		ValuePtr value = nullptr;
		if (is<Function>(second_argument.get())) {
			auto function = std::static_pointer_cast<Function>(second_argument)->function();
			value = function(nodes);
		}
		else {
			auto lambda = std::static_pointer_cast<Lambda>(second_argument);
			value = eval(lambda->body(), Environment::create(lambda, nodes));
		}

		return atom->reset(value);
	});

// (cons 1 (list 2 3))
ADD_FUNCTION(
	"cons",
	{
		if (nodes.size() != 2) {
			Error::the().add(format("wrong number of arguments: cons, {}", nodes.size()));
			return nullptr;
		}

		auto first_argument = *nodes.begin();
		auto second_argument = *std::next(nodes.begin());

		if (!is<Collection>(second_argument.get())) {
			Error::the().add(format("wrong argument type: list, '{}'", second_argument));
			return nullptr;
		}

		auto result_nodes = std::static_pointer_cast<Collection>(second_argument)->nodes();
		result_nodes.push_front(first_argument);

		return makePtr<List>(result_nodes);
	});

// (concat (list 1) (list 2 3))
ADD_FUNCTION(
	"concat",
	{
		std::list<ValuePtr> result_nodes;

		for (auto node : nodes) {
			if (!is<Collection>(node.get())) {
				Error::the().add(format("wrong argument type: list, '{}'", node));
				return nullptr;
			}

			auto argument_nodes = std::static_pointer_cast<Collection>(node)->nodes();
			result_nodes.splice(result_nodes.end(), argument_nodes);
		}

		return makePtr<List>(result_nodes);
	});

// (vec (list 1 2 3))
ADD_FUNCTION(
	"vec",
	{
		if (nodes.size() != 1) {
			Error::the().add(format("wrong number of arguments: vec, {}", nodes.size()));
			return nullptr;
		}

		if (!is<Collection>(nodes.front().get())) {
			Error::the().add(format("wrong argument type: list, '{}'", nodes.front()));
			return nullptr;
		}

		auto result_nodes = std::static_pointer_cast<Collection>(nodes.front())->nodes();

		return makePtr<Vector>(result_nodes);
	});

// -----------------------------------------

void installFunctions(EnvironmentPtr env)
{
	for (const auto& [name, lambda] : s_functions) {
		env->set(name, makePtr<Function>(name, lambda));
	}
}

} // namespace blaze
