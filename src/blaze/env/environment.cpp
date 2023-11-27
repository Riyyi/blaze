/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <iterator> // std::distance
#include <memory>   // std::static_pointer_cast

#include "ruc/file.h"
#include "ruc/format/format.h"

#include "blaze/ast.h"
#include "blaze/env/environment.h"
#include "blaze/error.h"
#include "blaze/forward.h"
#include "blaze/repl.h"

namespace blaze {

std::vector<FunctionParts> Environment::s_function_parts;
std::vector<std::string> Environment::s_lambdas;

EnvironmentPtr Environment::create()
{
	return std::shared_ptr<Environment>(new Environment);
}

EnvironmentPtr Environment::create(EnvironmentPtr outer)
{
	auto env = create();

	env->m_outer = outer;

	return env;
}

EnvironmentPtr Environment::create(const ValuePtr lambda, ValueVector&& arguments)
{
	auto lambda_casted = std::static_pointer_cast<Lambda>(lambda);
	auto env = create(lambda_casted->env());
	auto bindings = lambda_casted->bindings();

	auto it = arguments.begin();
	for (size_t i = 0; i < bindings.size(); ++i, ++it) {
		if (bindings[i] == "&") {
			if (i + 2 != bindings.size()) {
				Error::the().add(::format("invalid function: {}", lambda));
				return nullptr;
			}

			auto nodes = ValueVector();
			for (; it != arguments.end(); ++it) {
				nodes.push_back(*it);
			}
			env->set(bindings[i + 1], makePtr<List>(nodes));

			return env;
		}

		if (it == arguments.end()) {
			Error::the().add(::format("wrong number of arguments: {}, {}", lambda, arguments.size()));
			return nullptr;
		}

		env->set(bindings[i], *it);
	}

	if (it != arguments.end()) {
		Error::the().add(::format("wrong number of arguments: {}, {}", lambda, arguments.size()));
		return nullptr;
	}

	return env;
}

// -----------------------------------------

void Environment::loadFunctions()
{
	s_function_parts.clear();
	s_lambdas.clear();

	loadCollectionAccess();
	loadCollectionConstructor();
	loadCollectionModify();
	loadCompare();
	loadConvert();
	loadFormat();
	loadMeta();
	loadMutable();
	loadOperators();
	loadOther();
	loadPredicate();
	loadRepl();

	// Load std files

	std::filesystem::path std = "./lisp";
	if (!std::filesystem::exists(std) || !std::filesystem::is_directory(std)) {
		return;
	}

	s_lambdas.reserve(std::distance(std::filesystem::directory_iterator(std), {}));
	for (const auto& entry : std::filesystem::directory_iterator(std)) {
		if (!std::filesystem::is_regular_file(entry.path())
		    || entry.path().extension().string() != ".bl") {
			continue;
		}

		std::filesystem::path filename = entry.path().filename();
		ruc::File file((std / filename).string());

		// The init will be added to the front and executed first
		if (filename.string() == "init.bl") {
			s_lambdas.emplace(s_lambdas.begin(), file.data());
		}
		else {
			s_lambdas.push_back(file.data());
		}
	}
}

void Environment::registerFunction(FunctionParts function_parts)
{
	s_function_parts.push_back(function_parts);
}

void Environment::installFunctions(EnvironmentPtr env)
{
	for (const auto& function_parts : s_function_parts) {
		env->set(std::string(function_parts.name),
		         makePtr<Function>(
					 function_parts.name,
					 function_parts.signature,
					 function_parts.documentation,
					 function_parts.function));
	}
	for (const auto& lambda : s_lambdas) {
		// Ensure all s-exprs are run with (do)
		Repl::eval(Repl::read("(do " + lambda + ")"), env);
	}
}

// -----------------------------------------

bool Environment::exists(std::string_view symbol)
{
	return m_values.find(std::string(symbol)) != m_values.end();
}

ValuePtr Environment::set(std::string_view symbol, ValuePtr value)
{
	if (exists(symbol)) {
		m_values.erase(std::string(symbol));
	}

	m_values.emplace(symbol, value);

	return value;
}

ValuePtr Environment::get(std::string_view symbol)
{
	if (exists(symbol)) {
		return m_values[std::string(symbol)];
	}

	if (m_outer) {
		return m_outer->get(symbol);
	}

	return nullptr;
}

} // namespace blaze
