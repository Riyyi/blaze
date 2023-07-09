/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm> // std::copy, std::reverse_copy
#include <chrono>    // std::chrono::sytem_clock
#include <cstdint>   // int64_t
#include <iterator>  // std::advance, std::distance, std::next, std::prev
#include <memory>    // std::static_pointer_cast
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
			[](ValueVectorIt begin, ValueVectorIt end) -> ValuePtr lambda);

#define ADD_FUNCTION(symbol, lambda) ADD_FUNCTION_IMPL(__LINE__, symbol, lambda);

#define SIZE() std::distance(begin, end)

namespace blaze {

static std::unordered_map<std::string, FunctionType> s_functions;

ADD_FUNCTION(
	"+",
	{
		int64_t result = 0;

		for (auto it = begin; it != end; ++it) {
			VALUE_CAST(number, Number, (*it));
			result += number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"-",
	{
		if (SIZE() == 0) {
			return makePtr<Number>(0);
		}

		// Start with the first number
		VALUE_CAST(number, Number, (*begin));
		int64_t result = number->number();

		// Skip the first node
		for (auto it = begin + 1; it != end; ++it) {
			VALUE_CAST(number, Number, (*it));
			result -= number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"*",
	{
		int64_t result = 1;

		for (auto it = begin; it != end; ++it) {
			VALUE_CAST(number, Number, (*it));
			result *= number->number();
		}

		return makePtr<Number>(result);
	});

ADD_FUNCTION(
	"/",
	{
		CHECK_ARG_COUNT_AT_LEAST("/", SIZE(), 1);

		// Start with the first number
		VALUE_CAST(number, Number, (*begin));
		double result = number->number();

		// Skip the first node
		for (auto it = begin + 1; it != end; ++it) {
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
		CHECK_ARG_COUNT_AT_LEAST(#operator, SIZE(), 2);                        \
                                                                               \
		/* Start with the first number */                                      \
		VALUE_CAST(number_node, Number, (*begin));                             \
		int64_t number = number_node->number();                                \
                                                                               \
		/* Skip the first node */                                              \
		for (auto it = begin + 1; it != end; ++it) {                           \
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
		return makePtr<List>(begin, end);
	});

ADD_FUNCTION(
	"empty?",
	{
		bool result = true;

		for (auto it = begin; it != end; ++it) {
			VALUE_CAST(collection, Collection, (*it));
			if (!collection->empty()) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>((result) ? Constant::True : Constant::False);
	});

ADD_FUNCTION(
	"count",
	{
		CHECK_ARG_COUNT_IS("count", SIZE(), 1);

		size_t result = 0;
		if (is<Constant>(begin->get()) && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
			// result = 0
		}
		else if (is<Collection>(begin->get())) {
			result = std::static_pointer_cast<Collection>(*begin)->size();
		}
		else {
			Error::the().add(::format("wrong argument type: Collection, '{}'", *begin));
			return nullptr;
		}

		// FIXME: Add numeric_limits check for implicit cast: size_t > int64_t
		return makePtr<Number>((int64_t)result);
	});

// -----------------------------------------

#define PRINTER_STRING(print_readably, concatenation)                                 \
	{                                                                                 \
		std::string result;                                                           \
                                                                                      \
		Printer printer;                                                              \
		for (auto it = begin; it != end; ++it) {                                      \
			result += ::format("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                                      \
			if (it != end && std::next(it) != end) {                                  \
				result += concatenation;                                              \
			}                                                                         \
		}                                                                             \
                                                                                      \
		return makePtr<String>(result);                                               \
	}

ADD_FUNCTION("str", PRINTER_STRING(false, ""));
ADD_FUNCTION("pr-str", PRINTER_STRING(true, " "));

#define PRINTER_PRINT(print_readably)                                    \
	{                                                                    \
		Printer printer;                                                 \
		for (auto it = begin; it != end; ++it) {                         \
			print("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                         \
			if (it != end && std::next(it) != end) {                     \
				print(" ");                                              \
			}                                                            \
		}                                                                \
		print("\n");                                                     \
                                                                         \
		return makePtr<Constant>();                                      \
	}

ADD_FUNCTION("prn", PRINTER_PRINT(true));
ADD_FUNCTION("println", PRINTER_PRINT(false));

// -----------------------------------------

ADD_FUNCTION(
	"=",
	{
		CHECK_ARG_COUNT_AT_LEAST("=", SIZE(), 2);

		std::function<bool(ValuePtr, ValuePtr)> equal =
			[&equal](ValuePtr lhs, ValuePtr rhs) -> bool {
			if ((is<List>(lhs.get()) || is<Vector>(lhs.get()))
		        && (is<List>(rhs.get()) || is<Vector>(rhs.get()))) {
				const auto& lhs_nodes = std::static_pointer_cast<Collection>(lhs)->nodes();
				const auto& rhs_nodes = std::static_pointer_cast<Collection>(rhs)->nodes();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				auto lhs_it = lhs_nodes.cbegin();
				auto rhs_it = rhs_nodes.cbegin();
				for (; lhs_it != lhs_nodes.end(); ++lhs_it, ++rhs_it) {
					if (!equal(*lhs_it, *rhs_it)) {
						return false;
					}
				}

				return true;
			}

			if (is<HashMap>(lhs.get()) && is<HashMap>(rhs.get())) {
				const auto& lhs_nodes = std::static_pointer_cast<HashMap>(lhs)->elements();
				const auto& rhs_nodes = std::static_pointer_cast<HashMap>(rhs)->elements();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				for (const auto& [key, value] : lhs_nodes) {
					auto it = rhs_nodes.find(key);
					if (it == rhs_nodes.cend() || !equal(value, it->second)) {
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
		auto it = begin;
		auto it_next = begin + 1;
		for (; it_next != end; ++it, ++it_next) {
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
		CHECK_ARG_COUNT_IS("read-string", SIZE(), 1);

		VALUE_CAST(node, String, (*begin));
		std::string input = node->data();

		return read(input);
	});

ADD_FUNCTION(
	"slurp",
	{
		CHECK_ARG_COUNT_IS("slurp", SIZE(), 1);

		VALUE_CAST(node, String, (*begin));
		std::string path = node->data();

		auto file = ruc::File(path);

		return makePtr<String>(file.data());
	});

ADD_FUNCTION(
	"eval",
	{
		CHECK_ARG_COUNT_IS("eval", SIZE(), 1);

		return eval(*begin, nullptr);
	});

// (atom 1)
ADD_FUNCTION(
	"atom",
	{
		CHECK_ARG_COUNT_IS("atom", SIZE(), 1);

		return makePtr<Atom>(*begin);
	});

// (deref myatom)
ADD_FUNCTION(
	"deref",
	{
		CHECK_ARG_COUNT_IS("deref", SIZE(), 1);

		VALUE_CAST(atom, Atom, (*begin));

		return atom->deref();
	});

// (reset! myatom 2)
ADD_FUNCTION(
	"reset!",
	{
		CHECK_ARG_COUNT_IS("reset!", SIZE(), 2);

		VALUE_CAST(atom, Atom, (*begin));
		auto value = *(begin + 1);

		atom->reset(value);

		return value;
	});

// (swap! myatom (fn* [x y] (+ 1 x y)) 2) -> (deref (def! myatom (atom ((fn* [x y] (+ 1 x y)) (deref myatom) 2))))
ADD_FUNCTION(
	"swap!",
	{
		CHECK_ARG_COUNT_AT_LEAST("swap!", SIZE(), 2);

		VALUE_CAST(atom, Atom, (*begin));

		VALUE_CAST(callable, Callable, (*(begin + 1)));

		// Remove atom and function from the argument list, add atom value
		begin += 2;
		auto arguments = ValueVector(end - begin + 1);
		arguments[0] = atom->deref();
		std::copy(begin, end, arguments.begin() + 1);

		ValuePtr value = nullptr;
		if (is<Function>(callable.get())) {
			auto function = std::static_pointer_cast<Function>(callable)->function();
			value = function(arguments.begin(), arguments.end());
		}
		else {
			auto lambda = std::static_pointer_cast<Lambda>(callable);
			value = eval(lambda->body(), Environment::create(lambda, arguments));
		}

		return atom->reset(value);
	});

// (cons 1 (list 2 3))
ADD_FUNCTION(
	"cons",
	{
		CHECK_ARG_COUNT_IS("cons", SIZE(), 2);

		ValuePtr first = *begin;
		begin++;

		VALUE_CAST(collection, Collection, (*begin));
		const auto& collection_nodes = collection->nodes();

		ValueVector* result_nodes = new ValueVector(collection_nodes.size() + 1);
		result_nodes->at(0) = first;
		std::copy(collection_nodes.begin(), collection_nodes.end(), result_nodes->begin() + 1);

		return makePtr<List>(*result_nodes);
	});

// (concat (list 1) (list 2 3)) -> (1 2 3)
ADD_FUNCTION(
	"concat",
	{
		size_t count = 0;
		for (auto it = begin; it != end; ++it) {
			VALUE_CAST(collection, Collection, (*it));
			count += collection->size();
		}

		auto result_nodes = new ValueVector(count);
		size_t offset = 0;
		for (auto it = begin; it != end; ++it) {
			const auto& collection_nodes = std::static_pointer_cast<Collection>(*it)->nodes();
			std::copy(collection_nodes.begin(), collection_nodes.end(), result_nodes->begin() + offset);
			offset += collection_nodes.size();
		}

		return makePtr<List>(*result_nodes);
	});

// (vec (list 1 2 3))
ADD_FUNCTION(
	"vec",
	{
		CHECK_ARG_COUNT_IS("vec", SIZE(), 1);

		if (is<Vector>(begin->get())) {
			return *begin;
		}

		VALUE_CAST(collection, Collection, (*begin));

		return makePtr<Vector>(collection->nodes());
	});

// (nth (list 1 2 3) 0)
ADD_FUNCTION(
	"nth",
	{
		CHECK_ARG_COUNT_IS("nth", SIZE(), 2);

		VALUE_CAST(collection, Collection, (*begin));
		VALUE_CAST(number_node, Number, (*(begin + 1)));
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

// (first (list 1 2 3)) -> 1
ADD_FUNCTION(
	"first",
	{
		CHECK_ARG_COUNT_IS("first", SIZE(), 1);

		if (is<Constant>(begin->get())
	        && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
			return makePtr<Constant>();
		}

		VALUE_CAST(collection, Collection, (*begin));

		return (collection->empty()) ? makePtr<Constant>() : collection->front();
	});

// (rest (list 1 2 3))
ADD_FUNCTION(
	"rest",
	{
		CHECK_ARG_COUNT_IS("rest", SIZE(), 1);

		if (is<Constant>(begin->get())
	        && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
			return makePtr<List>();
		}

		VALUE_CAST(collection, Collection, (*begin));

		return makePtr<List>(collection->rest());
	});

// (apply + 1 2 (list 3 4)) -> (+ 1 2 3 4)
ADD_FUNCTION(
	"apply",
	{
		CHECK_ARG_COUNT_AT_LEAST("apply", SIZE(), 2);

		auto callable = *begin;
		IS_VALUE(Callable, callable);

		VALUE_CAST(collection, Collection, (*std::prev(end)));

		ValueVector arguments(begin + 1, end - 1);
		arguments.reserve(arguments.size() + collection->size());

		// Append list nodes to the argument leftovers
		auto nodes = collection->nodes();
		for (const auto& node : nodes) {
			arguments.push_back(node);
		}

		ValuePtr value = nullptr;
		if (is<Function>(callable.get())) {
			auto function = std::static_pointer_cast<Function>(callable)->function();
			value = function(arguments.begin(), arguments.end());
		}
		else {
			auto lambda = std::static_pointer_cast<Lambda>(callable);
			value = eval(lambda->body(), Environment::create(lambda, arguments));
		}

		return value;
	});

// (map (fn* (x) (* x 2)) (list 1 2 3))
ADD_FUNCTION(
	"map",
	{
		CHECK_ARG_COUNT_IS("map", SIZE(), 2);

		VALUE_CAST(callable, Callable, (*begin));
		VALUE_CAST(collection, Collection, (*(begin + 1)));
		const auto& collection_nodes = collection->nodes();

		auto result = makePtr<List>();

		if (is<Function>(callable.get())) {
			auto function = std::static_pointer_cast<Function>(callable)->function();
			for (const auto& node : collection_nodes) {
				auto arguments = ValueVector { node };
				result->add(function(arguments.begin(), arguments.end()));
			}
		}
		else {
			auto lambda = std::static_pointer_cast<Lambda>(callable);
			for (const auto& node : collection_nodes) {
				result->add(eval(lambda->body(), Environment::create(lambda, { node })));
			}
		}

		return result;
	});

// (throw x)
ADD_FUNCTION(
	"throw",
	{
		CHECK_ARG_COUNT_IS("throw", SIZE(), 1);

		Error::the().add(*begin);

		return nullptr;
	})

// -----------------------------------------

#define IS_CONSTANT(name, constant)                                              \
	{                                                                            \
		CHECK_ARG_COUNT_IS(name, SIZE(), 1);                                     \
                                                                                 \
		return makePtr<Constant>(                                                \
			is<Constant>(begin->get())                                           \
			&& std::static_pointer_cast<Constant>(*begin)->state() == constant); \
	}

// (nil? nil)
ADD_FUNCTION("nil?", IS_CONSTANT("nil?", Constant::Nil));
ADD_FUNCTION("true?", IS_CONSTANT("true?", Constant::True));
ADD_FUNCTION("false?", IS_CONSTANT("false?", Constant::False));

// -----------------------------------------

#define IS_TYPE(type)                            \
	{                                            \
		bool result = true;                      \
                                                 \
		if (SIZE() == 0) {                       \
			result = false;                      \
		}                                        \
                                                 \
		for (auto it = begin; it != end; ++it) { \
			if (!is<type>(it->get())) {          \
				result = false;                  \
				break;                           \
			}                                    \
		}                                        \
                                                 \
		return makePtr<Constant>(result);        \
	}

// (symbol? 'foo)
ADD_FUNCTION("atom?", IS_TYPE(Atom));
ADD_FUNCTION("keyword?", IS_TYPE(Keyword));
ADD_FUNCTION("list?", IS_TYPE(List));
ADD_FUNCTION("map?", IS_TYPE(HashMap));
ADD_FUNCTION("number?", IS_TYPE(Number));
ADD_FUNCTION("sequential?", IS_TYPE(Collection));
ADD_FUNCTION("string?", IS_TYPE(String));
ADD_FUNCTION("symbol?", IS_TYPE(Symbol));
ADD_FUNCTION("vector?", IS_TYPE(Vector));

ADD_FUNCTION(
	"fn?",
	{
		bool result = true;

		if (SIZE() == 0) {
			result = false;
		}

		for (auto it = begin; it != end; ++it) {
			if (!is<Callable>(it->get()) || is<Macro>(it->get())) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>(result);
	});

ADD_FUNCTION(
	"macro?",
	{
		bool result = true;

		if (SIZE() == 0) {
			result = false;
		}

		for (auto it = begin; it != end; ++it) {
			if (!is<Macro>(it->get())) {
				result = false;
				break;
			}
		}

		return makePtr<Constant>(result);
	});

// -----------------------------------------

#define STRING_TO_TYPE(name, type)                 \
	{                                              \
		CHECK_ARG_COUNT_IS(name, SIZE(), 1);       \
                                                   \
		if (is<type>(begin->get())) {              \
			return *begin;                         \
		}                                          \
                                                   \
		VALUE_CAST(stringValue, String, (*begin)); \
                                                   \
		return makePtr<type>(stringValue->data()); \
	}

// (symbol "foo")
ADD_FUNCTION("symbol", STRING_TO_TYPE("symbol", Symbol));
ADD_FUNCTION("keyword", STRING_TO_TYPE("keyword", Keyword));

// -----------------------------------------

ADD_FUNCTION(
	"vector",
	{
		auto result = makePtr<Vector>();

		for (auto it = begin; it != end; ++it) {
			result->add(*it);
		}

		return result;
	});

ADD_FUNCTION(
	"hash-map",
	{
		CHECK_ARG_COUNT_EVEN("hash-map", SIZE());

		auto result = makePtr<HashMap>();

		for (auto it = begin; it != end; std::advance(it, 2)) {
			result->add(*it, *(std::next(it)));
		}

		return result;
	});

// (assoc {:a 1 :b 2} :a 3 :c 1)
ADD_FUNCTION(
	"assoc",
	{
		CHECK_ARG_COUNT_AT_LEAST("assoc", SIZE(), 1);

		VALUE_CAST(hash_map, HashMap, (*begin));
		begin++;

		CHECK_ARG_COUNT_EVEN("assoc", SIZE());

		auto result = makePtr<HashMap>(hash_map->elements());

		for (auto it = begin; it != end; std::advance(it, 2)) {
			result->add(*it, *(std::next(it)));
		}

		return result;
	});

ADD_FUNCTION(
	"dissoc",
	{
		CHECK_ARG_COUNT_AT_LEAST("dissoc", SIZE(), 1);

		VALUE_CAST(hash_map, HashMap, (*begin));
		begin++;

		auto result = makePtr<HashMap>(hash_map->elements());

		for (auto it = begin; it != end; ++it) {
			result->remove(*it);
		}

		return result;
	});

// (get {:kw "value"} :kw) -> "value"
ADD_FUNCTION(
	"get",
	{
		CHECK_ARG_COUNT_AT_LEAST("get", SIZE(), 1);

		if (is<Constant>(begin->get())
	        && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
			return makePtr<Constant>();
		}

		VALUE_CAST(hash_map, HashMap, (*begin));
		begin++;

		if (SIZE() == 0) {
			return makePtr<Constant>();
		}

		auto result = hash_map->get(*begin);
		return (result) ? result : makePtr<Constant>();
	});

ADD_FUNCTION(
	"contains?",
	{
		CHECK_ARG_COUNT_IS("contains?", SIZE(), 2);

		VALUE_CAST(hash_map, HashMap, (*begin));

		if (SIZE() == 0) {
			return makePtr<Constant>(false);
		}

		return makePtr<Constant>(hash_map->exists(*(begin + 1)));
	});

ADD_FUNCTION(
	"keys",
	{
		CHECK_ARG_COUNT_AT_LEAST("keys", SIZE(), 1);

		VALUE_CAST(hash_map, HashMap, (*begin));

		auto result = makePtr<List>();

		auto elements = hash_map->elements();
		for (auto pair : elements) {
			if (pair.first.front() == 0x7f) { // 127
				result->add(makePtr<Keyword>(pair.first.substr(1)));
			}
			else {
				result->add(makePtr<String>(pair.first));
			}
		}

		return result;
	});

ADD_FUNCTION(
	"vals",
	{
		CHECK_ARG_COUNT_AT_LEAST("vals", SIZE(), 1);

		VALUE_CAST(hash_map, HashMap, (*begin));

		auto result = makePtr<List>();

		auto elements = hash_map->elements();
		for (auto pair : elements) {
			result->add(pair.second);
		}

		return result;
	});

ADD_FUNCTION(
	"readline",
	{
		CHECK_ARG_COUNT_IS("readline", SIZE(), 1);

		VALUE_CAST(prompt, String, (*begin));

		return readline(prompt->data());
	});

ADD_FUNCTION(
	"time-ms",
	{
		CHECK_ARG_COUNT_IS("time-ms", SIZE(), 0);

		int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							  std::chrono::system_clock::now().time_since_epoch())
	                          .count();

		return makePtr<Number>(elapsed);
	});

// (meta [1 2 3])
ADD_FUNCTION(
	"meta",
	{
		CHECK_ARG_COUNT_IS("meta", SIZE(), 1);

		auto front = *begin;
		Value* front_raw_ptr = begin->get();

		if (!is<Collection>(front_raw_ptr) && // List / Vector
	        !is<HashMap>(front_raw_ptr) &&    // HashMap
	        !is<Callable>(front_raw_ptr)) {   // Function / Lambda
			Error::the().add(::format("wrong argument type: Collection, HashMap or Callable, {}", front));
			return nullptr;
		}

		return front->meta();
	});

// (with-meta [1 2 3] "some metadata")
ADD_FUNCTION(
	"with-meta",
	{
		CHECK_ARG_COUNT_IS("with-meta", SIZE(), 2);

		auto front = *begin;
		Value* front_raw_ptr = begin->get();

		if (!is<Collection>(front_raw_ptr) && // List / Vector
	        !is<HashMap>(front_raw_ptr) &&    // HashMap
	        !is<Callable>(front_raw_ptr)) {   // Function / Lambda
			Error::the().add(::format("wrong argument type: Collection, HashMap or Callable, {}", front));
			return nullptr;
		}

		return front->withMeta(*(begin + 1));
	});

// (conj '(1 2 3) 4 5 6) -> (6 5 4 1 2 3)
// (conj [1 2 3] 4 5 6)  -> [1 2 3 4 5 6]
ADD_FUNCTION(
	"conj",
	{
		CHECK_ARG_COUNT_AT_LEAST("conj", SIZE(), 1);

		VALUE_CAST(collection, Collection, (*begin));
		begin++;

		const auto& collection_nodes = collection->nodes();
		size_t collection_count = collection_nodes.size();
		size_t argument_count = SIZE();

		ValueVector* nodes = new ValueVector(argument_count + collection_count);

		if (is<List>(collection.get())) {
			std::reverse_copy(begin, end, nodes->begin());
			std::copy(collection_nodes.begin(), collection_nodes.end(), nodes->begin() + argument_count);

			return makePtr<List>(*nodes);
		}

		std::copy(collection_nodes.begin(), collection_nodes.end(), nodes->begin());
		std::copy(begin, end, nodes->begin() + collection_count);

		return makePtr<Vector>(*nodes);
	});

// (seq '(1 2 3)) -> (1 2 3)
// (seq [1 2 3])  -> (1 2 3)
// (seq "foo")    -> ("f" "o" "o")
ADD_FUNCTION(
	"seq",
	{
		CHECK_ARG_COUNT_IS("seq", SIZE(), 1);

		auto front = *begin;
		Value* front_raw_ptr = front.get();

		if (is<Constant>(front_raw_ptr) && std::static_pointer_cast<Constant>(front)->state() == Constant::Nil) {
			return makePtr<Constant>();
		}
		if (is<Collection>(front_raw_ptr)) {
			auto collection = std::static_pointer_cast<Collection>(front);

			if (collection->empty()) {
				return makePtr<Constant>();
			}

			if (is<List>(front_raw_ptr)) {
				return front;
			}

			return makePtr<List>(collection->nodes());
		}
		if (is<String>(front_raw_ptr)) {
			auto string = std::static_pointer_cast<String>(front);

			if (string->empty()) {
				return makePtr<Constant>();
			}

			auto result = makePtr<List>();

			const auto& data = string->data();
			for (const auto& character : data) {
				result->add(makePtr<String>(character));
			}

			return result;
		}

		Error::the().add(::format("wrong argument type: Collection or String, {}", front));

		return nullptr;
	});

// -----------------------------------------

void installFunctions(EnvironmentPtr env)
{
	for (const auto& [name, lambda] : s_functions) {
		env->set(name, makePtr<Function>(name, lambda));
	}
}

} // namespace blaze
