/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadConvert()
{
	// (symbol "foo")  -> foo
	ADD_FUNCTION(
		"symbol",
		{
			CHECK_ARG_COUNT_IS("symbol", SIZE(), 1);

			if (is<Symbol>(begin->get())) {
				return *begin;
			}

			VALUE_CAST(stringValue, String, (*begin));

			return makePtr<Symbol>(stringValue->data());
		});

	// (keyword "foo") -> :foo
	// (keyword 123)   -> :123
	ADD_FUNCTION(
		"keyword",
		{
			CHECK_ARG_COUNT_IS("symbol", SIZE(), 1);

			if (is<Keyword>(begin->get())) {
				return *begin;
			}
			else if (is<Number>(begin->get())) {
				VALUE_CAST(numberValue, Number, (*begin));

				return makePtr<Keyword>(numberValue->number());
			}

			VALUE_CAST(stringValue, String, (*begin));

			return makePtr<Keyword>(stringValue->data());
		});
}

} // namespace blaze
