/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::advance, std::next, std::prev
#include <list>
#include <memory> // std::static_pointer_cast
#include <span>   // std::span
#include <string>

#include "blaze/ast.h"
#include "blaze/env/environment.h"
#include "blaze/error.h"
#include "blaze/eval.h"
#include "blaze/forward.h"
#include "blaze/types.h"
#include "blaze/util.h"

namespace blaze {

std::vector<SpecialFormParts> Eval::s_special_form_parts;

Eval::Eval(ValuePtr ast, EnvironmentPtr env)
	: m_ast(ast)
	, m_env(env)
	, m_outer_env(env)
{
}

// -----------------------------------------

void Eval::registerSpecialForm(SpecialFormParts special_form_parts)
{
	s_special_form_parts.push_back(special_form_parts);
}

void Eval::eval()
{
	m_ast = evalImpl();
}

ValuePtr Eval::evalImpl()
{
	ValuePtr ast = nullptr;
	EnvironmentPtr env = nullptr;

	if (env == nullptr) {
		env = m_outer_env;
	}

	while (true) {
		if (Error::the().hasAnyError()) {
			return nullptr;
		}

		ast = m_ast;
		env = m_env;

		if (is<Symbol>(ast.get())) {
			return evalSymbol(ast, env);
		}
		if (is<Vector>(ast.get())) {
			return evalVector(ast, env);
		}
		if (is<HashMap>(ast.get())) {
			return evalHashMap(ast, env);
		}
		if (!is<List>(ast.get())) {
			return ast;
		}

		auto list = std::static_pointer_cast<List>(ast);

		if (list->empty()) {
			return ast;
		}

		// Special forms
		if (is<Symbol>(list->front().get())) {
			auto symbol = std::static_pointer_cast<Symbol>(list->front())->symbol();
			const auto& nodes = list->rest();
			if (symbol == "def!") {
				return evalDef(nodes, env);
			}
			if (symbol == "defmacro!") {
				return evalDefMacro(nodes, env);
			}
			if (symbol == "describe") {
				return evalDescribe(nodes, env);
			}
			if (symbol == "fn*") {
				return evalFn(nodes, env);
			}
			if (symbol == "quasiquoteexpand") {
				return evalQuasiQuoteExpand(nodes);
			}
			if (symbol == "quote") {
				return evalQuote(nodes);
			}
			if (symbol == "try*") {
				return evalTry(nodes, env);
			}
			// Tail call optimized functions
			if (symbol == "and") {
				evalAnd(nodes, env);
				continue; // TCO
			}
			if (symbol == "do") {
				evalDo(nodes, env);
				continue; // TCO
			}
			if (symbol == "if") {
				evalIf(nodes, env);
				continue; // TCO
			}
			if (symbol == "let*") {
				evalLet(nodes, env);
				continue; // TCO
			}
			if (symbol == "macroexpand-1") {
				evalMacroExpand1(nodes, env);
				continue; // TCO
			}
			if (symbol == "or") {
				evalOr(nodes, env);
				continue; // TCO
			}
			if (symbol == "quasiquote") {
				evalQuasiQuote(nodes, env);
				continue; // TCO
			}
			if (symbol == "while") {
				evalWhile(nodes, env);
				continue; // TCO
			}
		}

		m_ast = list->front();
		m_env = env;
		auto evaluated_front = evalImpl();
		auto unevaluated_nodes = list->rest();

		if (is<Macro>(evaluated_front.get())) { // FIXME
			auto lambda = std::static_pointer_cast<Lambda>(evaluated_front);
			m_ast = lambda->body();
			m_env = Environment::create(lambda, std::move(unevaluated_nodes));
			m_ast = evalImpl();
			m_env = env;
			continue; // TCO
		}

		ValueVector evaluated_nodes(unevaluated_nodes.size());
		for (size_t i = 0; i < unevaluated_nodes.size(); ++i) {
			m_ast = unevaluated_nodes[i];
			m_env = env;
			evaluated_nodes[i] = evalImpl();
		}

		// Regular list
		if (is<Lambda>(evaluated_front.get())) {
			auto lambda = std::static_pointer_cast<Lambda>(evaluated_front);

			m_ast = lambda->body();
			m_env = Environment::create(lambda, std::move(evaluated_nodes));
			continue; // TCO
		}

		// Function call
		return apply(evaluated_front, evaluated_nodes);
	}
}

ValuePtr Eval::evalSymbol(ValuePtr ast, EnvironmentPtr env)
{
	auto result = env->get(std::static_pointer_cast<Symbol>(ast)->symbol());
	if (!result) {
		Error::the().add(::format("'{}' not found", ast));
		return nullptr;
	}

	return result;
}

ValuePtr Eval::evalVector(ValuePtr ast, EnvironmentPtr env)
{
	const auto& nodes = std::static_pointer_cast<Collection>(ast)->nodesRead();
	size_t count = nodes.size();
	auto evaluated_nodes = ValueVector(count);

	for (size_t i = 0; i < count; ++i) {
		m_ast = nodes[i];
		m_env = env;
		ValuePtr eval_node = evalImpl();
		if (eval_node == nullptr) {
			return nullptr;
		}
		evaluated_nodes.at(i) = eval_node;
	}

	return makePtr<Vector>(evaluated_nodes);
}

ValuePtr Eval::evalHashMap(ValuePtr ast, EnvironmentPtr env)
{
	const auto& elements = std::static_pointer_cast<HashMap>(ast)->elements();
	Elements evaluated_elements;
	for (const auto& element : elements) {
		m_ast = element.second;
		m_env = env;
		ValuePtr element_node = evalImpl();
		if (element_node == nullptr) {
			return nullptr;
		}
		evaluated_elements.insert_or_assign(element.first, element_node);
	}

	return makePtr<HashMap>(evaluated_elements);
}

// -----------------------------------------

ValuePtr Eval::apply(ValuePtr function, const ValueVector& nodes)
{
	if (!is<Function>(function.get())) {
		Error::the().add(::format("invalid function: {}", function));
		return nullptr;
	}

	auto func = std::static_pointer_cast<Function>(function)->function();

	return func(nodes.begin(), nodes.end());
}

} // namespace blaze
