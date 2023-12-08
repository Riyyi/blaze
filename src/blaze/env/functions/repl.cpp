/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include "blaze/ast.h"
#include "blaze/env/macro.h"
#include "blaze/repl.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadRepl()
{
	// REPL reader
	ADD_FUNCTION(
		"read-string",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("read-string", SIZE(), 1);

			VALUE_CAST(node, String, (*begin));
			std::string input = node->data();

			return Repl::read(input);
		});

	// Prompt readline
	ADD_FUNCTION(
		"readline",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("readline", SIZE(), 1);

			VALUE_CAST(prompt, String, (*begin));

			return Repl::readline(prompt->data());
		});

	// REPL eval
	ADD_FUNCTION(
		"eval",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("eval", SIZE(), 1);

			return Repl::eval(*begin, nullptr);
		});
}

} // namespace blaze
