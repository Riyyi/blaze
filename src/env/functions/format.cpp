/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::next
#include <string>

#include "ruc/format/print.h"

#include "ast.h"
#include "env/macro.h"
#include "printer.h"
#include "util.h"

namespace blaze {

#define PRINTER_STRING(print_readably, concatenation)                                 \
	{                                                                                 \
		std::string result;                                                           \
                                                                                      \
		Printer printer;                                                              \
		for (auto it = begin; it != end; ++it) {                                      \
			result += ::format("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                                      \
			if (it != end && std::next(it) != end) {                                  \
				result += concatenation;                                              \
			}                                                                         \
		}                                                                             \
                                                                                      \
		return makePtr<String>(result);                                               \
	}

ADD_FUNCTION("str", PRINTER_STRING(false, ""));
ADD_FUNCTION("pr-str", PRINTER_STRING(true, " "));

#define PRINTER_PRINT(print_readably)                                    \
	{                                                                    \
		Printer printer;                                                 \
		for (auto it = begin; it != end; ++it) {                         \
			print("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                         \
			if (it != end && std::next(it) != end) {                     \
				print(" ");                                              \
			}                                                            \
		}                                                                \
		print("\n");                                                     \
                                                                         \
		return makePtr<Constant>();                                      \
	}

ADD_FUNCTION("prn", PRINTER_PRINT(true));
ADD_FUNCTION("println", PRINTER_PRINT(false));

} // namespace blaze
