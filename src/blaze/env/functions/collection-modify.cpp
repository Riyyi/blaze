/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm> // std::copy, std::reverse_copy
#include <cstddef>   // size_t
#include <memory>    // std::static_pointer_cast

#include "blaze/ast.h"
#include "blaze/env/environment.h"
#include "blaze/env/macro.h"
#include "blaze/forward.h"
#include "blaze/repl.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadCollectionModify()
{
	// (apply + 1 2 (list 3 4)) -> (+ 1 2 3 4) -> 10
	ADD_FUNCTION(
		"apply",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("apply", SIZE(), 2);

			auto callable = *begin;
			IS_VALUE(Callable, callable);

			VALUE_CAST(collection, Collection, (*std::prev(end)));

			auto arguments = ValueVector(begin + 1, end - 1);
			arguments.reserve(arguments.size() + collection->size());

			// Append list nodes to the argument leftovers
			auto nodes = collection->nodesRead();
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
				value = Repl::eval(lambda->body(), Environment::create(lambda, std::move(arguments)));
			}

			return value;
		});

	// (cons 1 (list 2 3))
	ADD_FUNCTION(
		"cons",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("cons", SIZE(), 2);

			ValuePtr first = *begin;
			begin++;

			VALUE_CAST(collection, Collection, (*begin));
			const auto& collection_nodes = collection->nodesRead();

			auto result_nodes = ValueVector(collection_nodes.size() + 1);
			result_nodes.at(0) = first;
			std::copy(collection_nodes.begin(), collection_nodes.end(), result_nodes.begin() + 1);

			return makePtr<List>(result_nodes);
		});

	// (concat (list 1) (list 2 3)) -> (1 2 3)
	ADD_FUNCTION(
		"concat",
		"",
		"",
		{
			size_t count = 0;
			for (auto it = begin; it != end; ++it) {
				VALUE_CAST(collection, Collection, (*it));
				count += collection->size();
			}

			auto result_nodes = ValueVector(count);
			size_t offset = 0;
			for (auto it = begin; it != end; ++it) {
				const auto& collection_nodes = std::static_pointer_cast<Collection>(*it)->nodesRead();
				std::copy(collection_nodes.begin(), collection_nodes.end(), result_nodes.begin() + offset);
				offset += collection_nodes.size();
			}

			return makePtr<List>(result_nodes);
		});

	// (conj '(1 2 3) 4 5 6) -> (6 5 4 1 2 3)
	// (conj [1 2 3] 4 5 6)  -> [1 2 3 4 5 6]
	ADD_FUNCTION(
		"conj",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("conj", SIZE(), 1);

			VALUE_CAST(collection, Collection, (*begin));
			begin++;

			const auto& collection_nodes = collection->nodesRead();
			size_t collection_count = collection_nodes.size();
			size_t argument_count = SIZE();

			auto nodes = ValueVector(argument_count + collection_count);

			if (is<List>(collection.get())) {
				std::reverse_copy(begin, end, nodes.begin());
				std::copy(collection_nodes.begin(), collection_nodes.end(), nodes.begin() + argument_count);

				return makePtr<List>(nodes);
			}

			std::copy(collection_nodes.begin(), collection_nodes.end(), nodes.begin());
			std::copy(begin, end, nodes.begin() + collection_count);

			return makePtr<Vector>(nodes);
		});

	// (map (fn* (x) (* x 2)) (list 1 2 3)) -> (2 4 6)
	ADD_FUNCTION(
		"map",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("map", SIZE(), 2);

			VALUE_CAST(callable, Callable, (*begin));
			VALUE_CAST(collection, Collection, (*(begin + 1)));

			size_t count = collection->size();
			auto nodes = ValueVector(count);

			if (is<Function>(callable.get())) {
				auto function = std::static_pointer_cast<Function>(callable)->function();
				for (size_t i = 0; i < count; ++i) {
					nodes.at(i) = function(collection->begin() + i, collection->begin() + i + 1);
				}
			}
			else {
				auto lambda = std::static_pointer_cast<Lambda>(callable);
				auto collection_nodes = collection->nodesRead();
				for (size_t i = 0; i < count; ++i) {
					nodes.at(i) = (Repl::eval(lambda->body(), Environment::create(lambda, { collection_nodes[i] })));
				}
			}

			return makePtr<List>(nodes);
		});

	// (set-nth (list 1 2 3) 1 "foo") -> (1 "foo" 3)
	ADD_FUNCTION(
		"set-nth",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("set-nth-element", SIZE(), 3);

			VALUE_CAST(collection, Collection, (*begin));

			VALUE_CAST(number_node, Number, (*(begin + 1)));
			auto index = static_cast<size_t>(number_node->number() < 0 ? 0 : number_node->number());

			auto value = *(begin + 2);

			auto collection_nodes = collection->nodesCopy();
			if (index >= collection->size()) { // Enlarge list if index out of bounds
				collection_nodes.resize(index + 1, makePtr<Constant>());
			}
			collection_nodes[index] = value;

			if (is<Vector>(begin->get())) {
				return makePtr<Vector>(collection_nodes);
			}

			return makePtr<List>(collection_nodes);
		});

	// (seq '(1 2 3)) -> (1 2 3)
	// (seq [1 2 3])  -> (1 2 3)
	// (seq "foo")    -> ("f" "o" "o")
	ADD_FUNCTION(
		"seq",
		"",
		"",
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

				return makePtr<List>(collection->nodesCopy());
			}
			if (is<String>(front_raw_ptr)) {
				auto string = std::static_pointer_cast<String>(front);

				if (string->empty()) {
					return makePtr<Constant>();
				}

				size_t count = string->size();
				auto nodes = ValueVector(count);

				const auto& data = string->data();
				for (size_t i = 0; i < count; ++i) {
					nodes.at(i) = makePtr<String>(data[i]);
				}

				return makePtr<List>(nodes);
			}

			Error::the().add(::format("wrong argument type: Collection or String, {}", front));

			return nullptr;
		});

	// -----------------------------------------

	// (assoc {:a 1 :b 2} :a 3 :c 1) -> {:a 3 :b 2 :c 1}
	ADD_FUNCTION(
		"assoc",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("assoc", SIZE(), 1);

			VALUE_CAST(hash_map, HashMap, (*begin));
			begin++;

			CHECK_ARG_COUNT_EVEN("assoc", SIZE());

			Elements elements(hash_map->elements());
			for (auto it = begin; it != end; std::advance(it, 2)) {
				const ValuePtr& value = *(std::next(it)); // temporary instance to get around const
				elements.insert_or_assign(HashMap::getKeyString(*it), value);
			}

			return makePtr<HashMap>(elements);
		});

	// (dissoc {:a 1 :b 2 :c 3} :a :c :d) -> {:b 2}
	ADD_FUNCTION(
		"dissoc",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("dissoc", SIZE(), 1);

			VALUE_CAST(hash_map, HashMap, (*begin));
			begin++;

			Elements elements(hash_map->elements());
			for (auto it = begin; it != end; ++it) {
				elements.erase(HashMap::getKeyString(*it));
			}

			return makePtr<HashMap>(elements);
		});
}

} // namespace blaze
