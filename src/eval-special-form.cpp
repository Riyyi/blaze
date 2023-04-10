/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <list>
#include <memory>

#include "ast.h"
#include "error.h"
#include "eval.h"
#include "forward.h"
#include "types.h"
#include "util.h"

namespace blaze {

static ValuePtr evalQuasiQuoteImpl(ValuePtr ast);

// (def! x 2)
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

// (defmacro! x (fn* (x) x))
ValuePtr Eval::evalDefMacro(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("defmacro!", nodes.size(), 2);

	// First argument needs to be a Symbol
	VALUE_CAST(symbol, Symbol, nodes.front());

	// Eval second argument
	m_ast_stack.push(*std::next(nodes.begin()));
	m_env_stack.push(env);
	ValuePtr value = evalImpl();
	VALUE_CAST(lambda, Lambda, value);

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol->symbol(), makePtr<Lambda>(lambda, true));
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

ValuePtr Eval::evalMacroExpand(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("macroexpand", nodes.size(), 1);

	return macroExpand(nodes.front(), env);
}

// (quasiquoteexpand x)
ValuePtr Eval::evalQuasiQuoteExpand(const std::list<ValuePtr>& nodes)
{
	CHECK_ARG_COUNT_IS("quasiquoteexpand", nodes.size(), 1);

	return evalQuasiQuoteImpl(nodes.front());
}

// (quote x)
ValuePtr Eval::evalQuote(const std::list<ValuePtr>& nodes)
{
	CHECK_ARG_COUNT_IS("quote", nodes.size(), 1);

	return nodes.front();
}

// (do 1 2 3)
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

// (if x true false)
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

// -----------------------------------------

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

// (quasiquote x)
void Eval::evalQuasiQuote(const std::list<ValuePtr>& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("quasiquote", nodes.size(), 1, void());

	auto result = evalQuasiQuoteImpl(nodes.front());

	m_ast_stack.push(result);
	m_env_stack.push(env);
	return; // TCO
}

// -----------------------------------------

} // namespace blaze
