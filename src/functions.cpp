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

#define ADD_FUNCTION_IMPL(unique, symbol, lambda)            \
	struct FUNCTION_STRUCT_NAME(unique) {                    \
		FUNCTION_STRUCT_NAME(unique)                         \
		(const std::string& __symbol, FunctionType __lambda) \
		{                                                    \
			s_functions.emplace(__symbol, __lambda);         \
		}                                                    \
	};                                                       \
	static struct FUNCTION_STRUCT_NAME(unique)               \
		FUNCTION_STRUCT_NAME(unique)(                        \
			symbol,                                          \
			[](std::list<ValuePtr> nodes) -> ValuePtr lambda);

#define ADD_FUNCTION(symbol, lambda) ADD_FUNCTION_IMPL(__LINE__, symbol, lambda);

namespace blaze {

static std::unordered_map<std::string, FunctionType> s_functions;

ADD_FUNCTION(
	"+",
	{
		int64_t result = 0;

		for (auto node : nodes) {
			VALUE_CAST(number, Number, node);
			result += number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"-",
	{
		if (nodes.size() == 0) {
			return makePtr<Number>(0);
		}

		// Start with the first number
		VALUE_CAST(number, Number, nodes.front());
		int64_t result = number->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			VALUE_CAST(number, Number, (*it));
			result -= number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"*",
	{
		int64_t result = 1;

		for (auto node : nodes) {
			VALUE_CAST(number, Number, node);
			result *= number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"/",
	{
		CHECK_ARG_COUNT_AT_LEAST("/", nodes.size(), 1);

		// Start with the first number
		VALUE_CAST(number, Number, nodes.front());
		double result = number->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			VALUE_CAST(number, Number, (*it));
			result /= number->number();
		}

		return makePtr<Number>((int64_t)result);
	});

// // -----------------------------------------

#define NUMBER_COMPARE(operator)                                               \
	{                                                                          \
		bool result = true;                                                    \
                                                                               \
		CHECK_ARG_COUNT_AT_LEAST(#operator, nodes.size(), 2);                  \
                                                                               \
		/* Start with the first number */                                      \
		VALUE_CAST(number_node, Number, nodes.front());                        \
		int64_t number = number_node->number();                                \
                                                                               \
		/* Skip the first node */                                              \
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {    \
			VALUE_CAST(current_number_node, Number, (*it));                    \
			int64_t current_number = current_number_node->number();            \
			if (!(number operator current_number)) {                           \
				result = false;                                                \
				break;                                                         \
			}                                                                  \
			number = current_number;                                           \
		}                                                                      \
                                                                               \
		return makePtr<Constant>((result) ? Constant::True : Constant::False); \
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
			VALUE_CAST(collection, Collection, node);
			if (!collection->empty()) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

// FIXME: (count {1}) infinite loop
ADD_FUNCTION(
	"count",
	{
		CHECK_ARG_COUNT_IS("count", nodes.size(), 1);

		auto first_argument = nodes.front();

		size_t result = 0;
		if (is<Constant>(first_argument.get()) && std::static_pointer_cast<Constant>(nodes.front())->state() == Constant::Nil) {
			// result = 0
		}
		else if (is<Collection>(first_argument.get())) {
			result = std::static_pointer_cast<Collection>(first_argument)->size();
		}
		else {
			Error::the().add(format("wrong argument type: Collection, '{}'", first_argument));
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
		CHECK_ARG_COUNT_AT_LEAST("=", nodes.size(), 2);

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
		CHECK_ARG_COUNT_IS("read-string", nodes.size(), 1);

		VALUE_CAST(node, String, nodes.front());
		std::string input = node->data();

		return read(input);
	});

ADD_FUNCTION(
	"slurp",
	{
		CHECK_ARG_COUNT_IS("slurp", nodes.size(), 1);

		VALUE_CAST(node, String, nodes.front());
		std::string path = node->data();

		auto file = ruc::File(path);

		return makePtr<String>(file.data());
	});

ADD_FUNCTION(
	"eval",
	{
		CHECK_ARG_COUNT_IS("eval", nodes.size(), 1);

		return eval(nodes.front(), nullptr);
	});

// (atom 1)
ADD_FUNCTION(
	"atom",
	{
		CHECK_ARG_COUNT_IS("atom", nodes.size(), 1);

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
		CHECK_ARG_COUNT_IS("deref", nodes.size(), 1);

		VALUE_CAST(atom, Atom, nodes.front());

		return atom->deref();
	});

// (reset! myatom 2)
ADD_FUNCTION(
	"reset!",
	{
		CHECK_ARG_COUNT_IS("reset!", nodes.size(), 2);

		VALUE_CAST(atom, Atom, nodes.front());
		auto value = *std::next(nodes.begin());

		atom->reset(value);

		return value;
	});

// (swap! myatom (fn* [x] (+ 1 x)))
ADD_FUNCTION(
	"swap!",
	{
		CHECK_ARG_COUNT_AT_LEAST("swap!", nodes.size(), 2);

		VALUE_CAST(atom, Atom, nodes.front());

		auto second_argument = *std::next(nodes.begin());
		IS_VALUE(Callable, second_argument);

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
		CHECK_ARG_COUNT_IS("cons", nodes.size(), 2);

		VALUE_CAST(collection, Collection, (*std::next(nodes.begin())));

		auto result_nodes = collection->nodes();
		result_nodes.push_front(nodes.front());

		return makePtr<List>(result_nodes);
	});

// (concat (list 1) (list 2 3))
ADD_FUNCTION(
	"concat",
	{
		std::list<ValuePtr> result_nodes;

		for (auto node : nodes) {
			VALUE_CAST(collection, Collection, node);
			auto argument_nodes = collection->nodes();
			result_nodes.splice(result_nodes.end(), argument_nodes);
		}

		return makePtr<List>(result_nodes);
	});

// (vec (list 1 2 3))
ADD_FUNCTION(
	"vec",
	{
		CHECK_ARG_COUNT_IS("vec", nodes.size(), 1);

		VALUE_CAST(collection, Collection, nodes.front());

		return makePtr<Vector>(collection->nodes());
	});

// (nth (list 1 2 3) 0)
ADD_FUNCTION(
	"nth",
	{
		CHECK_ARG_COUNT_IS("nth", nodes.size(), 2);

		VALUE_CAST(collection, Collection, nodes.front());
		VALUE_CAST(number_node, Number, (*std::next(nodes.begin())));
		auto collection_nodes = collection->nodes();
		auto index = (size_t)number_node->number();

		if (number_node->number() < 0 || index >= collection_nodes.size()) {
			Error::the().add("index is out of range");
			return nullptr;
		}

		auto result = collection_nodes.begin();
		for (size_t i = 0; i < index; ++i) {
			result++;
		}

		return *result;
	});

// (first (list 1 2 3))
ADD_FUNCTION(
	"first",
	{
		CHECK_ARG_COUNT_IS("first", nodes.size(), 1);

		if (is<Constant>(nodes.front().get())
	        && std::static_pointer_cast<Constant>(nodes.front())->state() == Constant::Nil) {
			return makePtr<Constant>(Constant::Nil);
		}

		VALUE_CAST(collection, Collection, nodes.front());
		auto collection_nodes = collection->nodes();

		return (collection_nodes.empty()) ? makePtr<Constant>(Constant::Nil) : collection_nodes.front();
	});

// (rest (list 1 2 3))
ADD_FUNCTION(
	"rest",
	{
		CHECK_ARG_COUNT_IS("rest", nodes.size(), 1);

		if (is<Constant>(nodes.front().get())
	        && std::static_pointer_cast<Constant>(nodes.front())->state() == Constant::Nil) {
			return makePtr<List>();
		}

		VALUE_CAST(collection, Collection, nodes.front());
		auto collection_nodes = collection->nodes();
		if (collection_nodes.size() > 0) {
			collection_nodes.pop_front();
		}

		return makePtr<List>(collection_nodes);
	});

// -----------------------------------------

void installFunctions(EnvironmentPtr env)
{
	for (const auto& [name, lambda] : s_functions) {
		env->set(name, makePtr<Function>(name, lambda));
	}
}

} // namespace blaze
