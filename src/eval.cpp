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

// -----------------------------------------

#define EVAL_LET(ast, nodes, env)                                                                  \
	{                                                                                              \
		if (nodes.size() != 2) {                                                                   \
			Error::the().add(format("wrong number of arguments: let*, {}", nodes.size()));         \
			return nullptr;                                                                        \
		}                                                                                          \
                                                                                                   \
		auto first_argument = *nodes.begin();                                                      \
		auto second_argument = *std::next(nodes.begin());                                          \
                                                                                                   \
		/* First argument needs to be a List or Vector */                                          \
		if (!is<Collection>(first_argument.get())) {                                               \
			Error::the().add(format("wrong argument type: collection, '{}'", first_argument));     \
			return nullptr;                                                                        \
		}                                                                                          \
                                                                                                   \
		/* Get the nodes out of the List or Vector */                                              \
		std::list<ASTNodePtr> binding_nodes;                                                       \
		auto bindings = std::static_pointer_cast<Collection>(first_argument);                      \
		binding_nodes = bindings->nodes();                                                         \
                                                                                                   \
		/* List or Vector needs to have an even number of elements */                              \
		size_t count = binding_nodes.size();                                                       \
		if (count % 2 != 0) {                                                                      \
			Error::the().add(format("wrong number of arguments: {}, {}", "let* bindings", count)); \
			return nullptr;                                                                        \
		}                                                                                          \
                                                                                                   \
		/* Create new environment */                                                               \
		auto let_env = Environment::create(env);                                                   \
                                                                                                   \
		for (auto it = binding_nodes.begin(); it != binding_nodes.end(); std::advance(it, 2)) {    \
			/* First element needs to be a Symbol */                                               \
			if (!is<Symbol>(*it->get())) {                                                         \
				Error::the().add(format("wrong argument type: symbol, '{}'", *it));                \
				return nullptr;                                                                    \
			}                                                                                      \
                                                                                                   \
			std::string key = std::static_pointer_cast<Symbol>(*it)->symbol();                     \
			m_ast_stack.push(*std::next(it));                                                      \
			m_env_stack.push(let_env);                                                             \
			ASTNodePtr value = evalImpl();                                                         \
			let_env->set(key, value);                                                              \
		}                                                                                          \
                                                                                                   \
		/* TODO: Remove limitation of 3 arguments */                                               \
		/*       Eval all values in this new env, return last sexp of the result */                \
		m_ast_stack.push(second_argument);                                                         \
		m_env_stack.push(let_env);                                                                 \
		continue; /* TCO */                                                                        \
	}

#define EVAL_DO(ast, nodes, env)                                                         \
	{                                                                                    \
		if (nodes.size() == 0) {                                                         \
			Error::the().add(format("wrong number of arguments: do, {}", nodes.size())); \
			return nullptr;                                                              \
		}                                                                                \
                                                                                         \
		/* Evaluate all nodes except the last */                                         \
		for (auto it = nodes.begin(); it != std::prev(nodes.end(), 1); ++it) {           \
			m_ast_stack.push(*it);                                                       \
			m_env_stack.push(env);                                                       \
			evalImpl();                                                                  \
		}                                                                                \
                                                                                         \
		/* Eval last node */                                                             \
		m_ast_stack.push(nodes.back());                                                  \
		m_env_stack.push(env);                                                           \
		continue; /* TCO */                                                              \
	}

#define EVAL_IF(ast, nodes, env)                                                                                       \
	{                                                                                                                  \
		if (nodes.size() != 2 && nodes.size() != 3) {                                                                  \
			Error::the().add(format("wrong number of arguments: if, {}", nodes.size()));                               \
			return nullptr;                                                                                            \
		}                                                                                                              \
                                                                                                                       \
		auto first_argument = *nodes.begin();                                                                          \
		auto second_argument = *std::next(nodes.begin());                                                              \
		auto third_argument = (nodes.size() == 3) ? *std::next(std::next(nodes.begin())) : makePtr<Value>(Value::Nil); \
                                                                                                                       \
		m_ast_stack.push(first_argument);                                                                              \
		m_env_stack.push(env);                                                                                         \
		auto first_evaluated = evalImpl();                                                                             \
		if (!is<Value>(first_evaluated.get())                                                                          \
		    || std::static_pointer_cast<Value>(first_evaluated)->state() == Value::True) {                             \
			m_ast_stack.push(second_argument);                                                                         \
			m_env_stack.push(env);                                                                                     \
			continue; /* TCO */                                                                                        \
		}                                                                                                              \
		else {                                                                                                         \
			m_ast_stack.push(third_argument);                                                                          \
			m_env_stack.push(env);                                                                                     \
			continue; /* TCO */                                                                                        \
		}                                                                                                              \
	}

void Eval::eval()
{
	m_ast_stack = std::stack<ASTNodePtr>();
	m_env_stack = std::stack<EnvironmentPtr>();
	m_ast_stack.push(m_ast);
	m_env_stack.push(m_env);

	m_ast = evalImpl();
}

ASTNodePtr Eval::evalImpl()
{
	ASTNodePtr ast = nullptr;
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
				EVAL_LET(ast, nodes, env);
			}
			if (symbol == "do") {
				EVAL_DO(ast, nodes, env);
			}
			if (symbol == "if") {
				EVAL_IF(ast, nodes, env);
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

ASTNodePtr Eval::evalAst(ASTNodePtr ast, EnvironmentPtr env)
{
	if (ast == nullptr || env == nullptr) {
		return nullptr;
	}

	ASTNode* ast_raw_ptr = ast.get();
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
			ASTNodePtr eval_node = evalImpl();
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
			ASTNodePtr element_node = evalImpl();
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
		Error::the().add(format("wrong number of arguments: def!, {}", nodes.size()));
		return nullptr;
	}

	auto first_argument = *nodes.begin();
	auto second_argument = *std::next(nodes.begin());

	// First element needs to be a Symbol
	if (!is<Symbol>(first_argument.get())) {
		Error::the().add(format("wrong argument type: symbol, {}", first_argument));
		return nullptr;
	}

	std::string symbol = std::static_pointer_cast<Symbol>(first_argument)->symbol();
	m_ast_stack.push(second_argument);
	m_env_stack.push(env);
	ASTNodePtr value = evalImpl();

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol, value);
}

#define ARG_COUNT_CHECK(name, comparison, size)                                    \
	if (comparison) {                                                              \
		Error::the().add(format("wrong number of arguments: {}, {}", name, size)); \
		return nullptr;                                                            \
	}

#define AST_CHECK(type, value)                                                 \
	if (!is<type>(value.get())) {                                              \
		Error::the().add(format("wrong argument type: {}, {}", #type, value)); \
		return nullptr;                                                        \
	}

#define AST_CAST(type, value, variable) \
	AST_CHECK(type, value)              \
	auto variable = std::static_pointer_cast<type>(value);

ASTNodePtr Eval::evalFn(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	ARG_COUNT_CHECK("fn*", nodes.size() != 2, nodes.size());

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
