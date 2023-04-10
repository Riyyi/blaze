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

		// Special forms
		auto nodes = list->nodes();
		if (is<Symbol>(nodes.front().get())) {
			auto symbol = std::static_pointer_cast<Symbol>(nodes.front())->symbol();
			nodes.pop_front();
			if (symbol == "def!") {
				return evalDef(nodes, env);
			}
			if (symbol == "let*") {
				evalLet(nodes, env);
				continue; // TCO
			}
			if (symbol == "quote") {
				return evalQuote(nodes);
			}
			if (symbol == "quasiquote") {
				evalQuasiQuote(nodes, env);
				continue; // TCO
			}
			if (symbol == "quasiquoteexpand") {
				return evalQuasiQuoteExpand(nodes, env);
			}
			if (symbol == "do") {
				evalDo(nodes, env);
				continue; // TCO
			}
			if (symbol == "if") {
				evalIf(nodes, env);
				continue; // TCO
			}
			if (symbol == "fn*") {
				return evalFn(nodes, env);
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

ValuePtr Eval::evalDef(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("def!", nodes.size(), 2);

	// First argument needs to be a Symbol
	VALUE_CAST(symbol, Symbol, nodes.front());

	// Eval second argument
	m_ast_stack.push(*std::next(nodes.begin()));
	m_env_stack.push(env);
	ValuePtr value = evalImpl();

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol->symbol(), value);
}

ValuePtr Eval::evalQuote(const std::list<ValuePtr>& nodes)
{
	CHECK_ARG_COUNT_IS("quote", nodes.size(), 1);

	return nodes.front();
}

static bool isSymbol(ValuePtr value, const std::string& symbol)
{
	if (!is<Symbol>(value.get())) {
		return false;
	}

	auto valueSymbol = std::static_pointer_cast<Symbol>(value)->symbol();

	if (valueSymbol != symbol) {
		return false;
	}

	return true;
}

static ValuePtr startsWith(ValuePtr ast, const std::string& symbol)
{
	if (!is<List>(ast.get())) {
		return nullptr;
	}

	auto nodes = std::static_pointer_cast<List>(ast)->nodes();

	if (nodes.empty() || !isSymbol(nodes.front(), symbol)) {
		return nullptr;
	}

	// Dont count the Symbol as part of the arguments
	CHECK_ARG_COUNT_IS(symbol, nodes.size() - 1, 1);

	return *std::next(nodes.begin());
}

static ValuePtr evalQuasiQuoteImpl(ValuePtr ast)
{
	if (is<HashMap>(ast.get()) || is<Symbol>(ast.get())) {
		return makePtr<List>(makePtr<Symbol>("quote"), ast);
	}

	if (!is<Collection>(ast.get())) {
		return ast;
	}

	// `~2 or `(unquote 2)
	const auto unquote = startsWith(ast, "unquote"); // x
	if (unquote) {
		return unquote;
	}

	// `~@(list 2 2 2) or `(splice-unquote (list 2 2 2))
	const auto splice_unquote = startsWith(ast, "splice-unquote"); // (list 2 2 2)
	if (splice_unquote) {
		return splice_unquote;
	}

	ValuePtr result = makePtr<List>();

	auto nodes = std::static_pointer_cast<Collection>(ast)->nodes();

	// `() or `(1 ~2 3) or `(1 ~@(list 2 2 2) 3)
	for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
		const auto elt = *it;

		const auto splice_unquote = startsWith(elt, "splice-unquote"); // (list 2 2 2)
		if (splice_unquote) {
			// (cons 1 (concat (list 2 2 2) (cons 3 ())))
			result = makePtr<List>(makePtr<Symbol>("concat"), splice_unquote, result);
			continue;
		}

		// (cons 1 (cons 2 (cons 3 ())))
		result = makePtr<List>(makePtr<Symbol>("cons"), evalQuasiQuoteImpl(elt), result);
	}

	if (is<List>(ast.get())) {
		return result;
	}

	// Wrap result in (vec) for Vector types
	return makePtr<List>(makePtr<Symbol>("vec"), result);
}

void Eval::evalQuasiQuote(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("quasiquote", nodes.size(), 1, void());

	auto result = evalQuasiQuoteImpl(nodes.front());

	m_ast_stack.push(result);
	m_env_stack.push(env);
	return; // TCO
}

ValuePtr Eval::evalQuasiQuoteExpand(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("quasiquoteexpand", nodes.size(), 1);

	return evalQuasiQuoteImpl(nodes.front());
}

// (let* (x 1) x)
void Eval::evalLet(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("let*", nodes.size(), 2, void());

	// First argument needs to be a List or Vector
	VALUE_CAST(bindings, Collection, nodes.front(), void());
	auto binding_nodes = bindings->nodes();

	// List or Vector needs to have an even number of elements
	CHECK_ARG_COUNT_EVEN("bindings", binding_nodes.size(), void());

	// Create new environment
	auto let_env = Environment::create(env);

	for (auto it = binding_nodes.begin(); it != binding_nodes.end(); std::advance(it, 2)) {
		// First element needs to be a Symbol
		VALUE_CAST(elt, Symbol, (*it), void());

		std::string key = elt->symbol();
		m_ast_stack.push(*std::next(it));
		m_env_stack.push(let_env);
		ValuePtr value = evalImpl();
		let_env->set(key, value);
	}

	// TODO: Remove limitation of 3 arguments
	// Eval all arguments in this new env, return last sexp of the result
	m_ast_stack.push(*std::next(nodes.begin()));
	m_env_stack.push(let_env);
	return; // TCO
}

void Eval::evalDo(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_AT_LEAST("do", nodes.size(), 1, void());

	// Evaluate all nodes except the last
	for (auto it = nodes.begin(); it != std::prev(nodes.end(), 1); ++it) {
		m_ast_stack.push(*it);
		m_env_stack.push(env);
		evalImpl();
	}

	// Eval last node
	m_ast_stack.push(nodes.back());
	m_env_stack.push(env);
	return; // TCO
}

void Eval::evalIf(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_BETWEEN("if", nodes.size(), 2, 3, void());

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());
	auto third_argument = (nodes.size() == 3) ? *std::next(std::next(nodes.begin())) : makePtr<Constant>(Constant::Nil);

	m_ast_stack.push(first_argument);
	m_env_stack.push(env);
	auto first_evaluated = evalImpl();
	if (!is<Constant>(first_evaluated.get())
	    || std::static_pointer_cast<Constant>(first_evaluated)->state() == Constant::True) {
		m_ast_stack.push(second_argument);
		m_env_stack.push(env);
		return; // TCO
	}

	m_ast_stack.push(third_argument);
	m_env_stack.push(env);
	return; // TCO
}

// (fn* (x) x)
ValuePtr Eval::evalFn(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("fn*", nodes.size(), 2);

	// First element needs to be a List or Vector
	VALUE_CAST(collection, Collection, nodes.front());

	std::vector<std::string> bindings;
	for (auto node : collection->nodes()) {
		// All nodes need to be a Symbol
		VALUE_CAST(symbol, Symbol, node);
		bindings.push_back(symbol->symbol());
	}

	// TODO: Remove limitation of 3 arguments
	// Wrap all other nodes in list and add that as lambda body
	return makePtr<Lambda>(bindings, *std::next(nodes.begin()), env);
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
