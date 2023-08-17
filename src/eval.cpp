/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // sd::advance, std::next, std::prev
#include <list>
#include <memory> // std::static_pointer_cast
#include <span>   // std::span
#include <string>

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "eval.h"
#include "forward.h"
#include "types.h"
#include "util.h"

namespace blaze {

Eval::Eval(ValuePtr ast, EnvironmentPtr env)
	: m_ast(ast)
	, m_env(env)
{
}

// -----------------------------------------

void Eval::eval()
{
	m_ast_stack = std::stack<ValuePtr>();
	m_env_stack = std::stack<EnvironmentPtr>();
	m_ast_stack.push(m_ast);
	m_env_stack.push(m_env);

	m_ast = evalImpl();
}

ValuePtr Eval::evalImpl()
{
	ValuePtr ast = nullptr;
	EnvironmentPtr env = nullptr;

	while (true) {
		if (Error::the().hasAnyError()) {
			return nullptr;
		}

		if (m_ast_stack.size() == 0) {
			return nullptr;
		}

		if (m_env_stack.size() == 0) {
			m_env_stack.push(m_env);
		}

		ast = m_ast_stack.top();
		env = m_env_stack.top();

		m_ast_stack.pop();
		m_env_stack.pop();

		if (!is<List>(ast.get())) {
			return evalAst(ast, env);
		}

		ast = macroExpand(ast, env);

		if (!is<List>(ast.get())) {
			return evalAst(ast, env);
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
			if (symbol == "fn*") {
				return evalFn(nodes, env);
			}
			if (symbol == "macroexpand") {
				return evalMacroExpand(nodes, env);
			}
			if (symbol == "quasiquoteexpand") {
				return evalQuasiQuoteExpand(nodes);
			}
			if (symbol == "quote") {
				return evalQuote(nodes);
			}
			// Tail call optimized functions
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
			if (symbol == "quasiquote") {
				evalQuasiQuote(nodes, env);
				continue; // TCO
			}
			if (symbol == "try*") {
				evalTry(nodes, env);
				continue; // TCO
			}
		}

		auto evaluated_list = std::static_pointer_cast<List>(evalAst(ast, env));

		if (evaluated_list == nullptr) {
			return nullptr;
		}

		// Regular list
		if (is<Lambda>(evaluated_list->front().get())) {
			auto lambda = std::static_pointer_cast<Lambda>(evaluated_list->front());

			m_ast_stack.push(lambda->body());
			m_env_stack.push(Environment::create(lambda, evaluated_list->rest()));
			continue; // TCO
		}

		// Function call
		return apply(evaluated_list);
	}
}

ValuePtr Eval::evalAst(ValuePtr ast, EnvironmentPtr env)
{
	if (ast == nullptr || env == nullptr) {
		return nullptr;
	}

	Value* ast_raw_ptr = ast.get();
	if (is<Symbol>(ast_raw_ptr)) {
		auto result = env->get(std::static_pointer_cast<Symbol>(ast)->symbol());
		if (!result) {
			Error::the().add(::format("'{}' not found", ast));
			return nullptr;
		}

		return result;
	}
	else if (is<Collection>(ast_raw_ptr)) {
		const auto& nodes = std::static_pointer_cast<Collection>(ast)->nodes();
		size_t count = nodes.size();
		auto evaluated_nodes = ValueVector(count);

		for (size_t i = 0; i < count; ++i) {
			m_ast_stack.push(nodes[i]);
			m_env_stack.push(env);
			ValuePtr eval_node = evalImpl();
			if (eval_node == nullptr) {
				return nullptr;
			}
			evaluated_nodes.at(i) = eval_node;
		}

		if (is<List>(ast_raw_ptr)) {
			return makePtr<List>(evaluated_nodes);
		}

		return makePtr<Vector>(evaluated_nodes);
	}
	else if (is<HashMap>(ast_raw_ptr)) {
		const auto& elements = std::static_pointer_cast<HashMap>(ast)->elements();
		Elements evaluated_elements;
		for (const auto& element : elements) {
			m_ast_stack.push(element.second);
			m_env_stack.push(env);
			ValuePtr element_node = evalImpl();
			if (element_node == nullptr) {
				return nullptr;
			}
			evaluated_elements.insert_or_assign(element.first, element_node);
		}

		return makePtr<HashMap>(evaluated_elements);
	}

	return ast;
}

// -----------------------------------------

// (x y z)
bool Eval::isMacroCall(ValuePtr ast, EnvironmentPtr env)
{
	auto list = dynamic_cast<List*>(ast.get());

	if (list == nullptr || list->empty()) {
		return false;
	}

	auto front = list->front().get();

	if (!is<Symbol>(front)) {
		return false;
	}

	auto symbol = dynamic_cast<Symbol*>(front)->symbol();
	auto value = env->get(symbol).get();

	if (!is<Macro>(value)) {
		return false;
	}

	return true;
}

// (x y z)
ValuePtr Eval::macroExpand(ValuePtr ast, EnvironmentPtr env)
{
	while (isMacroCall(ast, env)) {
		auto list = std::static_pointer_cast<List>(ast);
		auto value = env->get(std::static_pointer_cast<Symbol>(list->front())->symbol());
		auto lambda = std::static_pointer_cast<Lambda>(value);

		m_ast_stack.push(lambda->body());
		m_env_stack.push(Environment::create(lambda, list->rest()));
		ast = evalImpl();
	}

	return ast;
}

//-----------------------------------------

ValuePtr Eval::apply(std::shared_ptr<List> evaluated_list)
{
	if (evaluated_list == nullptr) {
		return nullptr;
	}

	auto nodes = evaluated_list->nodes();

	if (!is<Function>(nodes.front().get())) {
		Error::the().add(::format("invalid function: {}", nodes.front()));
		return nullptr;
	}

	auto function = std::static_pointer_cast<Function>(nodes.front())->function();

	return function(nodes.begin() + 1, nodes.end());
}

} // namespace blaze
