/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm> // std::copy
#include <memory>    // std::static_pointer_cast

#include "ast.h"
#include "env/environment.h"
#include "env/macro.h"
#include "forward.h"
#include "repl.h"
#include "util.h"

namespace blaze {

void Environment::loadMutable()
{
	// (atom 1)
	ADD_FUNCTION(
		"atom",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("atom", SIZE(), 1);

			return makePtr<Atom>(*begin);
		});

	// (deref myatom)
	ADD_FUNCTION(
		"deref",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("deref", SIZE(), 1);

			VALUE_CAST(atom, Atom, (*begin));

			return atom->deref();
		});

	// (reset! myatom 2)
	ADD_FUNCTION(
		"reset!",
		"",
		"",
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
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("swap!", SIZE(), 2);

			VALUE_CAST(atom, Atom, (*begin));

			VALUE_CAST(callable, Callable, (*(begin + 1)));

			// Remove atom and function from the argument list, add atom value
			begin += 2;
			auto arguments = ValueVector(SIZE() + 1);
			arguments[0] = atom->deref();
			std::copy(begin, end, arguments.begin() + 1);

			ValuePtr value = nullptr;
			if (is<Function>(callable.get())) {
				auto function = std::static_pointer_cast<Function>(callable)->function();
				value = function(arguments.begin(), arguments.end());
			}
			else {
				auto lambda = std::static_pointer_cast<Lambda>(callable);
				value = Repl::eval(lambda->body(), Environment::create(lambda, std::move(arguments)));
			}

			return atom->reset(value);
		});
}

} // namespace blaze
