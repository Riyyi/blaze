/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <utility> // std::forward

#include "ruc/meta/odr.h"

namespace blaze {

namespace detail {

// struct hashMapConstructor {
// 	template<typename HashMap>
// 	static void construct(HashMap& hash_map, bool boolean)
// 	{
// 		//..
// 	}
// };

// template<typename HashMap, typename T>
// void toHashMap(HashMap& hash_map, const T& value)
// {
// 	hashMapConstructor::construct(hash_map, value);
// }

struct toHashMapFunction {
	template<typename HashMapPtr, typename T>
	auto operator()(HashMapPtr& hash_map, T&& value) const
	{
		return to_hash_map(hash_map, std::forward<T>(value));
	}
};

} // namespace detail

// Anonymous namespace prevents multiple definition of the reference
namespace {
// Function object
constexpr const auto& to_hash_map = ruc::detail::staticConst<detail::toHashMapFunction>; // NOLINT(misc-definitions-in-headers,clang-diagnostic-unused-variable)
} // namespace

} // namespace blaze

// Customization Points
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html

// blaze::to_hash_map is a function object, the type of which is
// blaze::detail::toHashMapFunction. In the blaze::detail namespace are the to_hash_map
// free functions. The function call operator of toHashMapFunction makes an
// unqualified call to to_hash_map which, since it shares the detail namespace with
// the to_hash_map free functions, will consider those in addition to any overloads
// that are found by argument-dependent lookup.

// Variable templates are linked externally, therefor every translation unit
// will see the same address for detail::staticConst<detail::toHashMapFunction>.
// Since blaze::to_hash_map is a reference to the variable template, it too will have
// the same address in all translation units.
