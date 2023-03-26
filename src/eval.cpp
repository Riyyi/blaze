/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast
#include <span>   // std::span
#include <string>
#include <vector>

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "eval.h"
#include "ruc/meta/assert.h"
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
	if (is<Symbol>(nodes[0].get())) {
		auto symbol = std::static_pointer_cast<Symbol>(nodes[0])->symbol();
		if (symbol == "def!") {
			return evalDef(nodes, env);
		}
		if (symbol == "let*") {
			return evalLet(nodes, env);
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

ASTNodePtr Eval::evalDef(const std::vector<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() != 3) {
		Error::the().addError(format("wrong number of arguments: def!, {}", nodes.size() - 1));
		return nullptr;
	}

	// First element needs to be a Symbol
	if (!is<Symbol>(nodes[1].get())) {
		Error::the().addError(format("wrong type argument: symbol, {}", nodes[1]));
		return nullptr;
	}

	std::string symbol = std::static_pointer_cast<Symbol>(nodes[1])->symbol();
	ASTNodePtr value = evalImpl(nodes[2], env);

	// Dont overwrite symbols after an error
	if (Error::the().hasAnyError()) {
		return nullptr;
	}

	// Modify existing environment
	return env->set(symbol, value);
}

ASTNodePtr Eval::evalLet(const std::vector<ASTNodePtr>& nodes, EnvironmentPtr env)
{
	if (nodes.size() != 3) {
		Error::the().addError(format("wrong number of arguments: let*, {}", nodes.size() - 1));
		return nullptr;
	}

	// First argument needs to be a List or Vector
	if (!is<List>(nodes[1].get()) && !is<Vector>(nodes[1].get())) {
		Error::the().addError(format("wrong argument type: list, '{}'", nodes[1]));
		return nullptr;
	}

	// Get the nodes out of the List or Vector
	std::vector<ASTNodePtr> binding_nodes;
	if (is<List>(nodes[1].get())) {
		auto bindings = std::static_pointer_cast<List>(nodes[1]);
		binding_nodes = bindings->nodes();
	}
	else {
		auto bindings = std::static_pointer_cast<Vector>(nodes[1]);
		binding_nodes = bindings->nodes();
	}

	// List or Vector needs to have an even number of elements
	size_t count = binding_nodes.size();
	if (count % 2 != 0) {
		Error::the().addError(format("wrong number of arguments: {}, {}", "let* bindings", count));
		return nullptr;
	}

	// Create new environment
	auto let_env = makePtr<Environment>(env);

	for (size_t i = 0; i < count; i += 2) {
		// First element needs to be a Symbol
		if (!is<Symbol>(binding_nodes[i].get())) {
			Error::the().addError(format("wrong argument type: symbol, '{}'", binding_nodes[i]));
			return nullptr;
		}

		std::string key = std::static_pointer_cast<Symbol>(binding_nodes[i])->symbol();
		ASTNodePtr value = evalImpl(binding_nodes[i + 1], let_env);
		let_env->set(key, value);
	}

	// TODO: Remove limitation of 3 arguments
	//       Eval all values in this new env, return last sexp of the result
	return evalImpl(nodes[2], let_env);
}

ASTNodePtr Eval::apply(std::shared_ptr<List> evaluated_list)
{
	if (evaluated_list == nullptr) {
		return nullptr;
	}

	auto nodes = evaluated_list->nodes();

	if (!is<Function>(nodes[0].get())) {
		Error::the().addError(format("invalid function: {}", nodes[0]));
		return nullptr;
	}

	// car
	auto lambda = std::static_pointer_cast<Function>(nodes[0])->lambda();
	// cdr
	std::span<ASTNodePtr> span { nodes.data() + 1, nodes.size() - 1 };

	return lambda(span);
}

} // namespace blaze
