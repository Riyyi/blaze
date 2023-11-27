/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <chrono>  // std::chrono::sytem_clock
#include <cstdint> // int64_t

#include "blaze/ast.h"
#include "blaze/env/macro.h"
#include "blaze/error.h"
#include "blaze/forward.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadOther()
{
	// (throw x)
	ADD_FUNCTION(
		"throw",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("throw", SIZE(), 1);

			Error::the().add(*begin);

			return nullptr;
		})

	// -----------------------------------------

	// (time-ms)
	ADD_FUNCTION(
		"time-ms",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("time-ms", SIZE(), 0);

			int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
								  std::chrono::system_clock::now().time_since_epoch())
		                          .count();

			return makePtr<Number>(elapsed);
		});
}

} // namespace blaze
