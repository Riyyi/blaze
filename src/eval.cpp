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

namespace blaze {

Eval::Eval(ASTNodePtr ast, EnvironmentPtr env)
	: m_ast(ast)
	, m_env(env)
{
}

void Eval::eval()
{
	m_ast = evalImpl(m_ast, m_env);
}

ASTNodePtr Eval::evalImpl(ASTNodePtr ast, EnvironmentPtr env)
{
	if (ast == nullptr || env == nullptr) {
		return nullptr;
	}

	if (!is<List>(ast.get())) {
		return evalAst(ast, env);
	}

	auto list = std::static_pointer_cast<List>(ast);

	if (list->empty()) {
		return ast;
	}

	// Environment
	auto nodes = list->nodes();
	if (is<Symbol>(nodes.front().get())) {
		auto symbol = std::static_pointer_cast<Symbol>(nodes.front())->symbol();
		nodes.pop_front();
		if (symbol == "def!") {
			return evalDef(nodes, env);
		}
		if (symbol == "let*") {
			return evalLet(nodes, env);
		}
		if (symbol == "do") {
			return evalDo(nodes, env);
		}
		if (symbol == "if") {
			return evalIf(nodes, env);
		}
		if (symbol == "fn*") {
			return evalFn(nodes, env);
		}
	}

	// Function call
	return apply(std::static_pointer_cast<List>(evalAst(ast, env)));
}

ASTNodePtr Eval::evalAst(ASTNodePtr ast, EnvironmentPtr env)
{
	if (ast == nullptr || env == nullptr) {
		return nullptr;
	}

	ASTNode* ast_raw_ptr = ast.get();
	if (is<Symbol>(ast_raw_ptr)) {
		auto result = env->get(std::static_pointer_cast<Symbol>(ast)->symbol());
		if (!result) {
			Error::the().addError(format("'{}' not found", ast));
			return nullptr;
		}
		return result;
	}
	else if (is<List>(ast_raw_ptr)) {
		auto result = makePtr<List>();
		auto nodes = std::static_pointer_cast<List>(ast)->nodes();
		for (auto node : nodes) {
			ASTNodePtr eval_node = evalImpl(node, env);
			if (eval_node == nullptr) {
				return nullptr;
			}
			result->addNode(eval_node);
		}
		return result;
	}
	else if (is<Vector>(ast_raw_ptr)) {
		auto result = makePtr<Vector>();
		auto nodes = std::static_pointer_cast<Vector>(ast)->nodes();
		for (auto node : nodes) {
			ASTNodePtr eval_node = evalImpl(node, env);
			if (eval_node == nullptr) {
				return nullptr;
			}
			result->addNode(eval_node);
		}
		return result;
	}
	else if (is<HashMap>(ast_raw_ptr)) {
		auto result = makePtr<HashMap>();
		auto elements = std::static_pointer_cast<HashMap>(ast)->elements();
		for (auto& element : elements) {
			ASTNodePtr element_node = evalImpl(element.second, env);
			if (element_node == nullptr) {
				return nullptr;
			}
			result->addElement(element.first, element_node);
		}
		return result;
	}

	return ast;
}

ASTNodePtr Eval::evalDef(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() != 2) {
		Error::the().addError(format("wrong number of arguments: def!, {}", nodes.size()));
		return nullptr;
	}

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());

	// First element needs to be a Symbol
	if (!is<Symbol>(first_argument.get())) {
		Error::the().addError(format("wrong argument type: symbol, {}", first_argument));
		return nullptr;
	}

	std::string symbol = std::static_pointer_cast<Symbol>(first_argument)->symbol();
	ASTNodePtr value = evalImpl(second_argument, env);

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol, value);
}

ASTNodePtr Eval::evalLet(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() != 2) {
		Error::the().addError(format("wrong number of arguments: let*, {}", nodes.size()));
		return nullptr;
	}

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());

	// First argument needs to be a List or Vector
	if (!is<Collection>(first_argument.get())) {
		Error::the().addError(format("wrong argument type: collection, '{}'", first_argument));
		return nullptr;
	}

	// Get the nodes out of the List or Vector
	std::list<ASTNodePtr> binding_nodes;
	auto bindings = std::static_pointer_cast<Collection>(first_argument);
	binding_nodes = bindings->nodes();

	// List or Vector needs to have an even number of elements
	size_t count = binding_nodes.size();
	if (count % 2 != 0) {
		Error::the().addError(format("wrong number of arguments: {}, {}", "let* bindings", count));
		return nullptr;
	}

	// Create new environment
	auto let_env = Environment::create(env);

	for (auto it = binding_nodes.begin(); it != binding_nodes.end(); std::advance(it, 2)) {
		// First element needs to be a Symbol
		if (!is<Symbol>(*it->get())) {
			Error::the().addError(format("wrong argument type: symbol, '{}'", *it));
			return nullptr;
		}

		std::string key = std::static_pointer_cast<Symbol>(*it)->symbol();
		ASTNodePtr value = evalImpl(*std::next(it), let_env);
		let_env->set(key, value);
	}

	// TODO: Remove limitation of 3 arguments
	//       Eval all values in this new env, return last sexp of the result
	return evalImpl(second_argument, let_env);
}

ASTNodePtr Eval::evalDo(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() == 0) {
		Error::the().addError(format("wrong number of arguments: do, {}", nodes.size()));
		return nullptr;
	}

	// Evaluate all nodes except the last
	for (auto it = nodes.begin(); it != std::prev(nodes.end(), 1); ++it) {
		evalImpl(*it, env);
	}

	// Eval and return last node
	return evalImpl(nodes.back(), env);
}

ASTNodePtr Eval::evalIf(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() != 2 && nodes.size() != 3) {
		Error::the().addError(format("wrong number of arguments: if, {}", nodes.size()));
		return nullptr;
	}

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());
	auto third_argument = (nodes.size() == 3) ? *std::next(std::next(nodes.begin())) : makePtr<Value>(Value::Nil);

	auto first_evaluated = evalImpl(first_argument, env);
	if (!is<Value>(first_evaluated.get())
	    || std::static_pointer_cast<Value>(first_evaluated)->state() == Value::True) {
		return evalImpl(second_argument, env);
	}
	else {
		return evalImpl(third_argument, env);
	}
}

#define ARG_COUNT_CHECK(name, size, comparison)                                         \
	if (size comparison) {                                                              \
		Error::the().addError(format("wrong number of arguments: {}, {}", name, size)); \
		return nullptr;                                                                 \
	}

#define AST_CHECK(type, value)                                                      \
	if (!is<type>(value.get())) {                                                   \
		Error::the().addError(format("wrong argument type: {}, {}", #type, value)); \
		return nullptr;                                                             \
	}

#define AST_CAST(type, value, variable) \
	AST_CHECK(type, value)              \
	auto variable = std::static_pointer_cast<type>(value);

ASTNodePtr Eval::evalFn(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	ARG_COUNT_CHECK("fn*", nodes.size(), != 2);

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());

	// First element needs to be a List or Vector
	AST_CAST(Collection, first_argument, collection);

	std::vector<std::string> bindings;
	for (auto node : collection->nodes()) {
		// All nodes need to be a Symbol
		AST_CAST(Symbol, node, symbol);
		bindings.push_back(symbol->symbol());
	}

	return makePtr<Lambda>(bindings, second_argument, env);
}

ASTNodePtr Eval::apply(std::shared_ptr<List> evaluated_list)
{
	if (evaluated_list == nullptr) {
		return nullptr;
	}

	auto nodes = evaluated_list->nodes();

	if (!is<Function>(nodes.front().get()) && !is<Lambda>(nodes.front().get())) {
		Error::the().addError(format("invalid function: {}", nodes.front()));
		return nullptr;
	}

	// Function

	if (is<Function>(nodes.front().get())) {
		// car
		auto function = std::static_pointer_cast<Function>(nodes.front())->function();
		// cdr
		nodes.pop_front();

		return function(nodes);
	}

	// Lambda

	// car
	auto lambda = std::static_pointer_cast<Lambda>(nodes.front());
	// cdr
	nodes.pop_front();

	auto lambda_env = Environment::create(lambda, nodes);

	return evalImpl(lambda->body(), lambda_env);
}

} // namespace blaze
