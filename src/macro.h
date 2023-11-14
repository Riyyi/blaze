/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "eval.h"

#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b

#define EVAL_FUNCTION_IMPL(name, signature, documentation, struct_name)    \
	struct struct_name {                                                   \
		struct_name()                                                      \
		{                                                                  \
			Eval::registerSpecialForm({ name, signature, documentation }); \
		}                                                                  \
	};                                                                     \
	static struct struct_name struct_name; // NOLINT(clang-diagnostic-unused-function)

#define EVAL_FUNCTION(name, signature, documentation) \
	EVAL_FUNCTION_IMPL(name, signature, documentation, CONCAT(__EVAL_STRUCT_, __COUNTER__))
