/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <chrono>  // std::chrono::sytem_clock
#include <cstddef> // size_t
#include <cstdint> // int64_t
#include <memory>  // std::static_pointer_cast

#include "ast.h"
#include "env/macro.h"
#include "error.h"
#include "forward.h"
#include "util.h"

namespace blaze {

void Environment::loadOther()
{
	// (count '(1 2 3)) -> 3
	// (count [1 2 3]) -> 3
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

	// (throw x)
	ADD_FUNCTION(
		"throw",
		{
			CHECK_ARG_COUNT_IS("throw", SIZE(), 1);

			Error::the().add(*begin);

			return nullptr;
		})

	// -----------------------------------------

	// (time-ms)
	ADD_FUNCTION(
		"time-ms",
		{
			CHECK_ARG_COUNT_IS("time-ms", SIZE(), 0);

			int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
								  std::chrono::system_clock::now().time_since_epoch())
		                          .count();

			return makePtr<Number>(elapsed);
		});
}

} // namespace blaze
