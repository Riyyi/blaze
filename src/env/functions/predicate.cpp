/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadPredicate()
{
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

	// (contains? {:foo 5} :foo)   -> true
	// (contains? {"bar" 5} "foo") -> false
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

	// (empty? '() '())       -> true
	// (empty? [] [1 2 3] []) -> false
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
}

} // namespace blaze
