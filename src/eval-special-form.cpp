/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::distance, std::next, std::prev
#include <list>
#include <memory>
#include <span>
#include <string>

#include "ast.h"
#include "env/environment.h"
#include "error.h"
#include "eval.h"
#include "forward.h"
#include "types.h"
#include "util.h"

namespace blaze {

static ValuePtr evalQuasiQuoteImpl(ValuePtr ast);

// (def! x 2)
ValuePtr Eval::evalDef(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("def!", nodes.size(), 2);

	// First argument needs to be a Symbol
	VALUE_CAST(symbol, Symbol, nodes.front());

	// Eval second argument
	m_ast = *std::next(nodes.begin());
	m_env = env;
	ValuePtr value = evalImpl();

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol->symbol(), value);
}

// (defmacro! x (fn* (x) x))
ValuePtr Eval::evalDefMacro(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("defmacro!", nodes.size(), 2);

	// First argument needs to be a Symbol
	VALUE_CAST(symbol, Symbol, nodes.front());

	// Eval second argument
	m_ast = *std::next(nodes.begin());
	m_env = env;
	ValuePtr value = evalImpl();
	VALUE_CAST(lambda, Lambda, value);

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol->symbol(), makePtr<Macro>(*lambda));
}

// (fn* (x) x)
ValuePtr Eval::evalFn(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_AT_LEAST("fn*", nodes.size(), 2);

	// First element needs to be a List or Vector
	VALUE_CAST(collection, Collection, nodes.front());
	const auto& collection_nodes = collection->nodesRead();

	std::vector<std::string> bindings;
	bindings.reserve(collection_nodes.size());
	for (const auto& node : collection_nodes) {
		// All nodes need to be a Symbol
		VALUE_CAST(symbol, Symbol, node);
		bindings.push_back(symbol->symbol());
	}

	// If more than one s-exp in lambda body, wrap in list
	if (nodes.size() > 2) {
		auto first = std::next(nodes.begin());
		auto last = std::prev(nodes.end());
		auto body_nodes = ValueVector(std::distance(first, last) + 2);
		body_nodes.at(0) = makePtr<Symbol>("do");
		std::copy(first, nodes.end(), body_nodes.begin() + 1);
		auto body = makePtr<List>(body_nodes);

		return makePtr<Lambda>(bindings, body, env);
	}

	return makePtr<Lambda>(bindings, *std::next(nodes.begin()), env);
}

// -----------------------------------------

// (quasiquoteexpand x)
ValuePtr Eval::evalQuasiQuoteExpand(const ValueVector& nodes)
{
	CHECK_ARG_COUNT_IS("quasiquoteexpand", nodes.size(), 1);

	return evalQuasiQuoteImpl(nodes.front());
}

// (quote x)
ValuePtr Eval::evalQuote(const ValueVector& nodes)
{
	CHECK_ARG_COUNT_IS("quote", nodes.size(), 1);

	return nodes.front();
}

// (try* x ... (catch* y z))
ValuePtr Eval::evalTry(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_AT_LEAST("try*", nodes.size(), 1);

	// Is last node a catch* block
	bool is_last_node_catch = false;
	std::span<const ValuePtr> catch_nodes;
	if (nodes.size() >= 2 && is<List>(nodes.back().get())) {
		VALUE_CAST(list, List, nodes.back());
		catch_nodes = list->nodesRead();
		if (!list->empty() && is<Symbol>(catch_nodes.front().get())) {
			VALUE_CAST(catch_symbol, Symbol, catch_nodes.front());
			if (catch_symbol->symbol() == "catch*") {
				CHECK_ARG_COUNT_IS("catch*", catch_nodes.size() - 1, 2);
				is_last_node_catch = true;
			}
		}
	}

	// Dont have to eval on malformed (catch*)
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Try

	ValuePtr result;
	auto end = (!is_last_node_catch) ? nodes.end() : std::prev(nodes.end(), 1);
	for (auto it = nodes.begin(); it != end; ++it) {
		m_ast = *it;
		m_env = env;
		result = evalImpl();
	}
	if (!Error::the().hasAnyError()) {
		return result;
	}

	// Catch

	if (!is_last_node_catch) {
		return nullptr;
	}

	// Get the error message
	auto error = (Error::the().hasOtherError())
	                 ? makePtr<String>(Error::the().otherError())
	                 : Error::the().exception();
	Error::the().clearErrors();

	VALUE_CAST(catch_binding, Symbol, (*std::next(catch_nodes.begin())));

	// Create new Environment that binds 'y' to the value of the exception
	auto catch_env = Environment::create(env);
	catch_env->set(catch_binding->symbol(), error);

	// Evaluate 'z' using the new Environment
	m_ast = catch_nodes.back();
	m_env = catch_env;
	return evalImpl();
}

// -----------------------------------------

// (and 1 2 3)
void Eval::evalAnd(const ValueVector& nodes, EnvironmentPtr env)
{
	ValuePtr result = makePtr<Constant>(Constant::True);
	for (auto node : nodes) {
		m_ast = node;
		m_env = env;
		result = evalImpl();

		if (is<Constant>(result.get())) {
			VALUE_CAST(constant, Constant, result, void());
			if (constant->state() == Constant::Nil || constant->state() == Constant::False) {
				m_ast = makePtr<Constant>(Constant::Nil);
				m_env = env;
				return; // TCO
			}
		}
	}

	m_ast = result;
	m_env = env;
	return; // TCO
}

// (do 1 2 3)
void Eval::evalDo(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_AT_LEAST("do", nodes.size(), 1, void());

	// Evaluate all nodes except the last
	for (auto it = nodes.begin(); it != std::prev(nodes.end(), 1); ++it) {
		m_ast = *it;
		m_env = env;
		evalImpl();
	}

	// Eval last node
	m_ast = nodes.back();
	m_env = env;
	return; // TCO
}

// (if x true false)
void Eval::evalIf(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_BETWEEN("if", nodes.size(), 2, 3, void());

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());
	auto third_argument = (nodes.size() == 3) ? *std::next(std::next(nodes.begin())) : makePtr<Constant>(Constant::Nil);

	m_ast = first_argument;
	m_env = env;
	auto first_evaluated = evalImpl();
	if (!is<Constant>(first_evaluated.get())
	    || std::static_pointer_cast<Constant>(first_evaluated)->state() == Constant::True) {
		m_ast = second_argument;
		m_env = env;
		return; // TCO
	}

	m_ast = third_argument;
	m_env = env;
	return; // TCO
}

// (let* (x 1) x)
void Eval::evalLet(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("let*", nodes.size(), 2, void());

	// First argument needs to be a List or Vector
	VALUE_CAST(bindings, Collection, nodes.front(), void());
	const auto& binding_nodes = bindings->nodesRead();

	// List or Vector needs to have an even number of elements
	CHECK_ARG_COUNT_EVEN("bindings", binding_nodes.size(), void());

	// Create new environment
	auto let_env = Environment::create(env);

	for (auto it = binding_nodes.begin(); it != binding_nodes.end(); std::advance(it, 2)) {
		// First element needs to be a Symbol
		VALUE_CAST(elt, Symbol, (*it), void());

		std::string key = elt->symbol();
		m_ast = *std::next(it);
		m_env = let_env;
		ValuePtr value = evalImpl();
		let_env->set(key, value);
	}

	// TODO: Remove limitation of 3 arguments
	// Eval all arguments in this new env, return last sexp of the result
	m_ast = *std::next(nodes.begin());
	m_env = let_env;
	return; // TCO
}

// -----------------------------------------

// (x y z)
static bool isMacroCall(ValuePtr ast, EnvironmentPtr env)
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

void Eval::evalMacroExpand1(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("macroexpand-1", nodes.size(), 1, void());

	if (!isMacroCall(nodes.front(), env)) {
		m_ast = nodes.front();
		m_env = env;
		return;
	}

	auto list = std::static_pointer_cast<List>(nodes.front());

	auto value = env->get(std::static_pointer_cast<Symbol>(list->front())->symbol());
	auto lambda = std::static_pointer_cast<Lambda>(value);

	m_ast = lambda->body();
	m_env = Environment::create(lambda, list->rest());
	return; // TCO
}

// -----------------------------------------

// (or 1 2 3)
void Eval::evalOr(const ValueVector& nodes, EnvironmentPtr env)
{
	ValuePtr result;
	for (auto node : nodes) {
		m_ast = node;
		m_env = env;
		result = evalImpl();

		if (!is<Constant>(result.get())) {
			m_ast = result;
			m_env = env;
			return; // TCO
		}

		VALUE_CAST(constant, Constant, result, void());
		if (constant->state() == Constant::True) {
			m_ast = result;
			m_env = env;
			return; // TCO
		}
	}

	m_ast = makePtr<Constant>(Constant::Nil);
	m_env = env;
	return; // TCO
}

// -----------------------------------------

static bool isSymbol(ValuePtr value, const std::string& symbol)
{
	if (!is<Symbol>(value.get())) {
		return false;
	}

	auto value_symbol = std::static_pointer_cast<Symbol>(value)->symbol();

	if (value_symbol != symbol) {
		return false;
	}

	return true;
}

static ValuePtr startsWith(ValuePtr ast, const std::string& symbol)
{
	if (!is<List>(ast.get())) {
		return nullptr;
	}

	const auto& nodes = std::static_pointer_cast<List>(ast)->nodesRead();

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

	auto collection = std::static_pointer_cast<Collection>(ast);

	// `() or `(1 ~2 3) or `(1 ~@(list 2 2 2) 3)
	for (auto it = collection->beginReverse(); it != collection->endReverse(); ++it) {
		const auto& elt = *it;

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
void Eval::evalQuasiQuote(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_IS("quasiquote", nodes.size(), 1, void());

	auto result = evalQuasiQuoteImpl(nodes.front());

	m_ast = result;
	m_env = env;
	return; // TCO
}

// -----------------------------------------

// (while true body...)
void Eval::evalWhile(const ValueVector& nodes, EnvironmentPtr env)
{
	CHECK_ARG_COUNT_AT_LEAST("while", nodes.size(), 2, void());

	// Condition
	ValuePtr predicate = *nodes.begin();

	m_ast = predicate;
	m_env = env;
	ValuePtr condition = evalImpl();
	while (!is<Constant>(condition.get())
	       || std::static_pointer_cast<Constant>(condition)->state() == Constant::True) {
		for (auto it = nodes.begin() + 1; it != nodes.end(); ++it) {
			m_ast = *it;
			m_env = env;
			evalImpl();
		}

		m_ast = predicate;
		m_env = env;
		condition = evalImpl();
	}

	m_ast = makePtr<Constant>();
	m_env = env;
	return; // TCO
}

// -----------------------------------------

} // namespace blaze
