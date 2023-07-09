/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory> // std::static_pointer_cast
#include <string>
#include <string_view>

#include "error.h"
#include "types.h"

// -----------------------------------------

// TODO: Move these ruc/test/macro.h -> ruc/src/meta/macro.h
#define GET_2TH_ARG(arg1, arg2, ...) arg2
#define GET_3TH_ARG(arg1, arg2, arg3, ...) arg3
#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define GET_5TH_ARG(arg1, arg2, arg3, arg4, arg5, ...) arg5
#define GET_6TH_ARG(arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6
#define MACRO_CHOOSER_1(macro, ...) \
	GET_2TH_ARG(__VA_ARGS__, macro##_1, )
#define MACRO_CHOOSER_2(macro, ...) \
	GET_3TH_ARG(__VA_ARGS__, macro##_2, macro##_1, )
#define MACRO_CHOOSER_3(macro, ...) \
	GET_4TH_ARG(__VA_ARGS__, macro##_3, macro##_2, macro##_1, )
#define MACRO_CHOOSER_4(macro, ...) \
	GET_5TH_ARG(__VA_ARGS__, macro##_4, macro##_3, macro##_2, macro##_1, )
#define MACRO_CHOOSER_5(macro, ...) \
	GET_6TH_ARG(__VA_ARGS__, macro##_5, macro##_4, macro##_3, macro##_2, macro##_1, )

// -----------------------------------------

#define CHECK_ARG_COUNT_IS_IMPL(name, size, expected, result)                        \
	if (size != expected) {                                                          \
		Error::the().add(::format("wrong number of arguments: {}, {}", name, size)); \
		return result;                                                               \
	}

#define CHECK_ARG_COUNT_IS_3(name, size, expected) \
	CHECK_ARG_COUNT_IS_IMPL(name, size, expected, nullptr)

#define CHECK_ARG_COUNT_IS_4(name, size, expected, result) \
	CHECK_ARG_COUNT_IS_IMPL(name, size, expected, result)

#define CHECK_ARG_COUNT_IS(...)                      \
	MACRO_CHOOSER_4(CHECK_ARG_COUNT_IS, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

#define CHECK_ARG_COUNT_AT_LEAST_IMPL(name, size, min, result)                       \
	if (size < min) {                                                                \
		Error::the().add(::format("wrong number of arguments: {}, {}", name, size)); \
		return result;                                                               \
	}

#define CHECK_ARG_COUNT_AT_LEAST_3(name, size, min) \
	CHECK_ARG_COUNT_AT_LEAST_IMPL(name, size, min, nullptr)

#define CHECK_ARG_COUNT_AT_LEAST_4(name, size, min, result) \
	CHECK_ARG_COUNT_AT_LEAST_IMPL(name, size, min, result)

#define CHECK_ARG_COUNT_AT_LEAST(...)                      \
	MACRO_CHOOSER_4(CHECK_ARG_COUNT_AT_LEAST, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

#define CHECK_ARG_COUNT_BETWEEN_IMPL(name, size, min, max, result)                   \
	if (size < min || size > max) {                                                  \
		Error::the().add(::format("wrong number of arguments: {}, {}", name, size)); \
		return result;                                                               \
	}

#define CHECK_ARG_COUNT_BETWEEN_4(name, size, min, max) \
	CHECK_ARG_COUNT_BETWEEN_IMPL(name, size, min, max, nullptr)

#define CHECK_ARG_COUNT_BETWEEN_5(name, size, min, max, result) \
	CHECK_ARG_COUNT_BETWEEN_IMPL(name, size, min, max, result)

#define CHECK_ARG_COUNT_BETWEEN(...)                      \
	MACRO_CHOOSER_5(CHECK_ARG_COUNT_BETWEEN, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

#define CHECK_ARG_COUNT_EVEN_IMPL(name, size, result)                                \
	if (size % 2 != 0) {                                                             \
		Error::the().add(::format("wrong number of arguments: {}, {}", name, size)); \
		return result;                                                               \
	}

#define CHECK_ARG_COUNT_EVEN_2(name, size) \
	CHECK_ARG_COUNT_EVEN_IMPL(name, size, nullptr)

#define CHECK_ARG_COUNT_EVEN_3(name, size, result) \
	CHECK_ARG_COUNT_EVEN_IMPL(name, size, result)

#define CHECK_ARG_COUNT_EVEN(...)                      \
	MACRO_CHOOSER_3(CHECK_ARG_COUNT_EVEN, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

#define IS_VALUE_IMPL(type, value, result)                                       \
	if (!is<type>(value.get())) {                                                \
		Error::the().add(::format("wrong argument type: {}, {}", #type, value)); \
		return result;                                                           \
	}

#define IS_VALUE_2(type, value) \
	IS_VALUE_IMPL(type, value, nullptr)

#define IS_VALUE_3(type, value, result) \
	IS_VALUE_IMPL(type, value, result)

#define IS_VALUE(...)                      \
	MACRO_CHOOSER_3(IS_VALUE, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

#define VALUE_CAST_IMPL(variable, type, value, result) \
	IS_VALUE(type, value, result);                     \
	auto variable = std::static_pointer_cast<type>(value);

#define VALUE_CAST_3(variable, type, value) \
	VALUE_CAST_IMPL(variable, type, value, nullptr)

#define VALUE_CAST_4(variable, type, value, result) \
	VALUE_CAST_IMPL(variable, type, value, result)

#define VALUE_CAST(...)                      \
	MACRO_CHOOSER_4(VALUE_CAST, __VA_ARGS__) \
	(__VA_ARGS__)

// -----------------------------------------

namespace blaze {

template<typename It, typename C>
inline bool isLast(It it, const C& container)
{
	return (it != container.end()) && (next(it) == container.end());
}

inline std::string replaceAll(std::string text, std::string_view search, std::string_view replace)
{
	size_t search_length = search.length();
	size_t replace_length = replace.length();
	size_t position = text.find(search, 0);
	while (position != std::string::npos) {
		text.replace(position, search_length, replace);
		position += replace_length;
		position = text.find(search, position);
	}

	return text;
}

} // namespace blaze
