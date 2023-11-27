/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "blaze/ast.h"
#include "blaze/env/macro.h"
#include "blaze/error.h"
#include "blaze/forward.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadMeta()
{
	// (meta [1 2 3])
	ADD_FUNCTION(
		"meta",
		"",
		"",
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
		"",
		"",
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
}

} // namespace blaze
