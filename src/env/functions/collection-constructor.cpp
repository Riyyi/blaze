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

void Environment::loadCollectionConstructor()
{
	// (list 1 2) -> (1 2)
	ADD_FUNCTION(
		"list",
		"",
		"",
		{
			return makePtr<List>(begin, end);
		});

	// (make-list 4 nil) -> (nil nil nil nil)
	ADD_FUNCTION(
		"make-list",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("make-list", SIZE(), 2);

			VALUE_CAST(number, Number, (*begin));
			auto count = static_cast<size_t>(number->number() < 0 ? 0 : number->number());
			auto value = *std::next(begin);

			auto nodes = ValueVector(count);
			if (is<Atom>(value.get())) {
				auto atom = std::static_pointer_cast<Atom>(value);
				for (size_t i = 0; i < count; ++i) {
					nodes[i] = makePtr<Atom>(atom);
				}
			}
			// else if (is<Collection>(value.get())) {
		    // 	for (size_t i = 0; i < count; ++i) {
		    // 		auto nodes = std::static_pointer_cast<Collection>(value)->nodesCopy();
		    // 		if (is<Vector>(value.get())) {
		    // 			makePtr<Vector>(nodes);
		    // 			continue;
		    // 		}
		    // 		nodes[i] = makePtr<List>(nodes);
		    // 	}
		    // }
		    // else if (is<Constant>(value.get())) {
		    // 	for (size_t i = 0; i < count; ++i) {
		    // 		auto constant = std::static_pointer_cast<Constant>(value);
		    // 		nodes[i] = makePtr<Constant>(constant);
		    // 	}
		    // }

			// TODO:
		    // Atom
		    // Collection
		    // Constant
		    // Function
		    // HashMap
		    // Keyword
		    // Lambda
		    // List
		    // Macro
		    // Number
		    // String
		    // Symbol
		    // Vector

			return makePtr<List>(std::move(nodes));
		});

	// -----------------------------------------

	// (vec (list 1 2 3))
	ADD_FUNCTION(
		"vec",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("vec", SIZE(), 1);

			if (is<Vector>(begin->get())) {
				return *begin;
			}

			VALUE_CAST(collection, Collection, (*begin));

			return makePtr<Vector>(collection->nodesCopy());
		});

	// (vector 1 2 3) -> [1 2 3]
	ADD_FUNCTION(
		"vector",
		"",
		"",
		{
			auto result = makePtr<Vector>();

			return makePtr<Vector>(begin, end);
		});

	// -----------------------------------------

	// (hash-map "foo" 5 :bar 10) -> {"foo" 5 :bar 10}
	ADD_FUNCTION(
		"hash-map",
		"",
		"",
		{
			CHECK_ARG_COUNT_EVEN("hash-map", SIZE());

			Elements elements;
			for (auto it = begin; it != end; std::advance(it, 2)) {
				const ValuePtr& value = *(std::next(it)); // temporary instance to get around const
				elements.insert_or_assign(HashMap::getKeyString(*it), value);
			}

			return makePtr<HashMap>(elements);
		});
}

} // namespace blaze
