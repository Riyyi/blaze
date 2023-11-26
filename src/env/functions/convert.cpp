/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <charconv> // std::from_chars, std::to_chars
#include <memory>
#include <system_error> // std::errc

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadConvert()
{
	// (number-to-string 123) -> "123"
	ADD_FUNCTION(
		"number-to-string",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("number-to-string", SIZE(), 1);

			IS_VALUE(Numeric, (*begin));

			char result[32];
			auto conversion = std::to_chars(result,
		                                    result + sizeof(result),
		                                    is<Number>(begin->get())
		                                        ? std::static_pointer_cast<Number>(*begin)->number()
		                                        : std::static_pointer_cast<Decimal>(*begin)->decimal());
			if (conversion.ec != std::errc()) {
				return makePtr<Constant>(Constant::Nil);
			}

			return makePtr<String>(std::string(result, conversion.ptr - result));
		});

	// (string-to-char "123") -> 49
	ADD_FUNCTION(
		"string-to-char",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("string-to-char", SIZE(), 1);

			VALUE_CAST(string_value, String, (*begin));
			std::string data = string_value->data();

			return makePtr<Number>(data.c_str()[0]);
		});

	// (string-to-number "123") -> 123
	ADD_FUNCTION(
		"string-to-number",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("string-to-number", SIZE(), 1);

			VALUE_CAST(string_value, String, (*begin));
			std::string data = string_value->data();

			if (data.find('.') == std::string::npos) {
				int64_t number;
				auto conversion_number = std::from_chars(data.c_str(), data.c_str() + data.size(), number);
				if (conversion_number.ec != std::errc()) {
					return makePtr<Constant>(Constant::Nil);
				}

				return makePtr<Number>(number);
			}

			double decimal;
			auto conversion_decimal = std::from_chars(data.c_str(), data.c_str() + data.size(), decimal);
			if (conversion_decimal.ec != std::errc()) {
				return makePtr<Constant>(Constant::Nil);
			}

			return makePtr<Decimal>(decimal);
		});

#define STRING_TO_COLLECTION(name, type)                    \
	{                                                       \
		CHECK_ARG_COUNT_IS(name, SIZE(), 1);                \
                                                            \
		VALUE_CAST(string_value, String, (*begin));         \
		std::string data = string_value->data();            \
                                                            \
		ValueVector nodes(data.size());                     \
		for (size_t i = 0; i < data.size(); ++i) {          \
			nodes.at(i) = makePtr<Number>(data.c_str()[i]); \
		}                                                   \
                                                            \
		return makePtr<type>(nodes);                        \
	}

	// (string-to-list "foo")   -> (102 111 111)
	// (string-to-vector "foo") -> [102 111 111]
	ADD_FUNCTION("string-to-list", "", "", STRING_TO_COLLECTION("string-to-list", List));
	ADD_FUNCTION("string-to-vector", "", "", STRING_TO_COLLECTION("string-to-vector", Vector));

	// -------------------------------------

	// (symbol "foo")  -> foo
	ADD_FUNCTION(
		"symbol",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("symbol", SIZE(), 1);

			if (is<Symbol>(begin->get())) {
				return *begin;
			}

			VALUE_CAST(string_value, String, (*begin));

			return makePtr<Symbol>(string_value->data());
		});

	// (keyword "foo") -> :foo
	// (keyword 123)   -> :123
	ADD_FUNCTION(
		"keyword",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("keyword", SIZE(), 1);

			if (is<Keyword>(begin->get())) {
				return *begin;
			}
			else if (is<Number>(begin->get())) {
				VALUE_CAST(number_value, Number, (*begin));

				return makePtr<Keyword>(number_value->number());
			}

			VALUE_CAST(string_value, String, (*begin));

			return makePtr<Keyword>(string_value->data());
		});
}

} // namespace blaze
