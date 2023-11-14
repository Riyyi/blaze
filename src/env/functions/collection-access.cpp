/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstddef> // size_t
#include <memory>  // std:static_pointer_cast

#include "ast.h"
#include "env/macro.h"
#include "forward.h"
#include "util.h"

namespace blaze {

void Environment::loadCollectionAccess()
{
	// (count '(1 2 3))        -> 3
	// (count [1 2 3])         -> 3
	// (count {:foo 2 :bar 3}) -> 2
	ADD_FUNCTION(
		"count",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("count", SIZE(), 1);

			size_t result = 0;
			if (is<Constant>(begin->get()) && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
				// result = 0
			}
			else if (is<Collection>(begin->get())) {
				result = std::static_pointer_cast<Collection>(*begin)->size();
			}
			else if (is<HashMap>(begin->get())) {
				result = std::static_pointer_cast<HashMap>(*begin)->size();
			}
			else {
				Error::the().add(::format("wrong argument type: Collection, '{}'", *begin));
				return nullptr;
			}

			// FIXME: Add numeric_limits check for implicit cast: size_t > int64_t
			return makePtr<Number>((int64_t)result);
		});

	// -----------------------------------------

	// (first (list 1 2 3)) -> 1
	ADD_FUNCTION(
		"first",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("first", SIZE(), 1);

			if (is<Constant>(begin->get())
		        && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
				return makePtr<Constant>();
			}

			VALUE_CAST(collection, Collection, (*begin));

			return (collection->empty()) ? makePtr<Constant>() : collection->front();
		});

	// (nth (list 1 2 3) 0) -> 1
	ADD_FUNCTION(
		"nth",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("nth", SIZE(), 2);

			VALUE_CAST(collection, Collection, (*begin));
			VALUE_CAST(number_node, Number, (*(begin + 1)));
			auto collection_nodes = collection->nodesRead();
			auto index = static_cast<size_t>(number_node->number());

			if (number_node->number() < 0 || index >= collection_nodes.size()) {
				Error::the().add("index is out of range");
				return nullptr;
			}

			return collection_nodes[index];
		});

	// (rest (list 1 2 3)) -> (2 3)
	ADD_FUNCTION(
		"rest",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("rest", SIZE(), 1);

			if (is<Constant>(begin->get())
		        && std::static_pointer_cast<Constant>(*begin)->state() == Constant::Nil) {
				return makePtr<List>();
			}

			VALUE_CAST(collection, Collection, (*begin));

			return makePtr<List>(collection->rest());
		});

	// -----------------------------------------

	// (get {:kw "value"} :kw) -> "value"
	ADD_FUNCTION(
		"get",
		"",
		"",
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

	// (keys {"foo" 3 :bar 5}) -> ("foo" :bar)
	ADD_FUNCTION(
		"keys",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("keys", SIZE(), 1);

			VALUE_CAST(hash_map, HashMap, (*begin));

			size_t count = hash_map->size();
			auto nodes = ValueVector(count);

			size_t i = 0;
			auto elements = hash_map->elements();
			for (auto pair : elements) {
				if (pair.first.front() == 0x7f) { // 127
					nodes.at(i) = makePtr<Keyword>(pair.first.substr(1));
				}
				else {
					nodes.at(i) = makePtr<String>(pair.first);
				}
				i++;
			}

			return makePtr<List>(nodes);
		});

	// (vals {"foo" 3 :bar 5}) -> (3 5)
	ADD_FUNCTION(
		"vals",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("vals", SIZE(), 1);

			VALUE_CAST(hash_map, HashMap, (*begin));

			size_t count = hash_map->size();
			auto nodes = ValueVector(count);

			size_t i = 0;
			auto elements = hash_map->elements();
			for (auto pair : elements) {
				nodes.at(i) = pair.second;
				i++;
			}

			return makePtr<List>(nodes);
		});
}

} // namespace blaze
