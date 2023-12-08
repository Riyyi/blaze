/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <chrono>     // std::chrono::sytem_clock
#include <cstdint>    // int64_t
#include <filesystem> // std::filesystem::current_path

#include "ruc/file.h"

#include "blaze/ast.h"
#include "blaze/env/macro.h"
#include "blaze/error.h"
#include "blaze/forward.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadOther()
{
	ADD_FUNCTION(
		"pwd", "",
		"Return the full filename of the current working directory.",
		{
			CHECK_ARG_COUNT_IS("pwd", SIZE(), 0);

			auto path = std::filesystem::current_path().string();
			return makePtr<String>(path);
		});

	ADD_FUNCTION(
		"slurp", "",
		"Read file contents",
		{
			CHECK_ARG_COUNT_IS("slurp", SIZE(), 1);

			VALUE_CAST(node, String, (*begin));
			std::string path = node->data();

			auto file = ruc::File(path);

			return makePtr<String>(file.data());
		});

	// -----------------------------------------

	// (throw x)
	ADD_FUNCTION(
		"throw", "",
		"",
		{
			CHECK_ARG_COUNT_IS("throw", SIZE(), 1);

			Error::the().add(*begin);

			return nullptr;
		})

	// -----------------------------------------

	// (time-ms)
	ADD_FUNCTION(
		"time-ms", "",
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
