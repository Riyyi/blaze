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

		auto list = std::static_pointer_cast<List>(ast);

		if (list->empty()) {
			return ast;
		}

		ast = macroExpand(ast, env);

		if (!is<List>(ast.get())) {
			return evalAst(ast, env);
		}

		// Macro-expand modifies `ast', so get the new list
		list = std::static_pointer_cast<List>(ast);

		// Special forms
		auto nodes = list->nodes();
		if (is<Symbol>(nodes.front().get())) {
			auto symbol = std::static_pointer_cast<Symbol>(nodes.front())->symbol();
			nodes.pop_front();
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
		}

		auto evaluated_list = std::static_pointer_cast<List>(evalAst(ast, env));

		if (evaluated_list == nullptr) {
			return nullptr;
		}

		// Regular list
		if (is<Lambda>(evaluated_list->nodes().front().get())) {
			auto evaluated_nodes = evaluated_list->nodes();

			// car
			auto lambda = std::static_pointer_cast<Lambda>(evaluated_nodes.front());
			// cdr
			evaluated_nodes.pop_front();

			m_ast_stack.push(lambda->body());
			m_env_stack.push(Environment::create(lambda, evaluated_nodes));
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
			Error::the().add(format("'{}' not found", ast));
			return nullptr;
		}
		return result;
	}
	else if (is<Collection>(ast_raw_ptr)) {
		std::shared_ptr<Collection> result = nullptr;
		(is<List>(ast_raw_ptr)) ? result = makePtr<List>() : result = makePtr<Vector>();
		auto nodes = std::static_pointer_cast<Collection>(ast)->nodes();
		for (auto node : nodes) {
			m_ast_stack.push(node);
			m_env_stack.push(env);
			ValuePtr eval_node = evalImpl();
			if (eval_node == nullptr) {
				return nullptr;
			}
			result->add(eval_node);
		}
		return result;
	}
	else if (is<HashMap>(ast_raw_ptr)) {
		auto result = makePtr<HashMap>();
		auto elements = std::static_pointer_cast<HashMap>(ast)->elements();
		for (auto& element : elements) {
			m_ast_stack.push(element.second);
			m_env_stack.push(env);
			ValuePtr element_node = evalImpl();
			if (element_node == nullptr) {
				return nullptr;
			}
			result->add(element.first, element_node);
		}
		return result;
	}

	return ast;
}

// -----------------------------------------

// (x y z)
bool Eval::isMacroCall(ValuePtr ast, EnvironmentPtr env)
{
	if (!is<List>(ast.get())) {
		return false;
	}

	auto nodes = std::static_pointer_cast<List>(ast)->nodes();

	if (nodes.size() == 0 || !is<Symbol>(nodes.front().get())) {
		return false;
	}

	auto value = env->get(std::static_pointer_cast<Symbol>(nodes.front())->symbol());

	if (!is<Lambda>(value.get()) || !std::static_pointer_cast<Lambda>(value)->isMacro()) {
		return false;
	}

	return true;
}

// (x y z)
ValuePtr Eval::macroExpand(ValuePtr ast, EnvironmentPtr env)
{
	while (isMacroCall(ast, env)) {
		auto nodes = std::static_pointer_cast<List>(ast)->nodes();
		auto value = env->get(std::static_pointer_cast<Symbol>(nodes.front())->symbol());
		auto lambda = std::static_pointer_cast<Lambda>(value);
		nodes.pop_front();

		m_ast_stack.push(lambda->body());
		m_env_stack.push(Environment::create(lambda, nodes));
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
		Error::the().add(format("invalid function: {}", nodes.front()));
		return nullptr;
	}

	// car
	auto function = std::static_pointer_cast<Function>(nodes.front())->function();
	// cdr
	nodes.pop_front();

	return function(nodes);
}

} // namespace blaze
