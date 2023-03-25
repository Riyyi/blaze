/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <span> // std::span

#include "ast.h"
#include "environment.h"
#include "eval.h"
#include "ruc/meta/assert.h"
#include "types.h"

namespace blaze {

Eval::Eval(ASTNodePtr ast, Environment* env)
	: m_ast(ast)
	, m_env(env)
{
}

void Eval::eval()
{
	m_ast = evalImpl(m_ast, m_env);
}

ASTNodePtr Eval::evalImpl(ASTNodePtr ast, Environment* env)
{
	if (!is<List>(ast.get())) {
		return evalAst(ast, env);
	}
	if (static_cast<List*>(ast.get())->empty()) {
		return ast;
	}

	return apply(static_pointer_cast<List>(evalAst(ast, env)));
}

ASTNodePtr Eval::evalAst(ASTNodePtr ast, Environment* env)
{
	ASTNode* ast_raw_ptr = ast.get();
	if (is<Symbol>(ast_raw_ptr)) {
		auto result = env->lookup(static_pointer_cast<Symbol>(ast)->symbol());
		if (!result) {
			// TODO: Maybe add backlink to parent nodes?
			if (is<List>(m_ast)) {
				Error::the().addError(format("symbol's function definition is void: {}", ast_raw_ptr));
			}
			else {
				Error::the().addError(format("symbol's value as variable is void: {}", ast_raw_ptr));
			}
		}
		return result;
	}
	else if (is<List>(ast_raw_ptr)) {
		auto result = makePtr<List>();
		auto nodes = static_pointer_cast<List>(ast)->nodes();
		for (auto node : nodes) {
			result->addNode(evalImpl(node, env));
		}
		return result;
	}
	else if (is<Vector>(ast_raw_ptr)) {
		auto result = makePtr<Vector>();
		auto nodes = static_pointer_cast<Vector>(ast)->nodes();
		for (auto node : nodes) {
			result->addNode(evalImpl(node, env));
		}
		return result;
	}
	else if (is<HashMap>(ast_raw_ptr)) {
		auto result = makePtr<HashMap>();
		auto elements = static_pointer_cast<HashMap>(ast)->elements();
		for (auto& element : elements) {
			result->addElement(element.first, evalImpl(element.second, env));
		}
		return result;
	}

	return ast;
}

ASTNodePtr Eval::apply(std::shared_ptr<List> evaluated_list)
{
	auto nodes = evaluated_list->nodes();

	if (!is<Function>(nodes[0].get())) {
		Error::the().addError(format("invalid function: {}", nodes[0]));
		return nullptr;
	}

	// car
	auto lambda = static_pointer_cast<Function>(nodes[0])->lambda();
	// cdr
	std::span<ASTNodePtr> span { nodes.data() + 1, nodes.size() - 1 };

	return lambda(span);
}

} // namespace blaze
