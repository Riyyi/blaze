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

Eval::Eval(ASTNode* ast, Environment* env)
	: m_ast(ast)
	, m_env(env)
{
}

void Eval::eval()
{
	m_ast = evalImpl(m_ast, m_env);
}

ASTNode* Eval::evalImpl(ASTNode* ast, Environment* env)
{
	if (!is<List>(ast)) {
		return evalAst(ast, env);
	}
	if (static_cast<List*>(ast)->empty()) {
		return ast;
	}

	return apply(static_cast<List*>(evalAst(ast, env)));
}

ASTNode* Eval::evalAst(ASTNode* ast, Environment* env)
{
	if (is<Symbol>(ast)) {
		auto result = env->lookup(static_cast<Symbol*>(ast)->symbol());
		if (!result) {
			Error::the().addError(format("'{}' not found", ast));
		}
		return result;
	}
	else if (is<List>(ast)) {
		auto result = new List();
		auto nodes = static_cast<List*>(ast)->nodes();
		for (auto node : nodes) {
			result->addNode(evalImpl(node, env));
		}
		return result;
	}
	else if (is<Vector>(ast)) {
		auto result = new Vector();
		auto nodes = static_cast<Vector*>(ast)->nodes();
		for (auto node : nodes) {
			result->addNode(evalImpl(node, env));
		}
		return result;
	}
	else if (is<HashMap>(ast)) {
		auto result = new HashMap();
		auto elements = static_cast<HashMap*>(ast)->elements();
		for (auto& element : elements) {
			result->addElement(element.first, evalImpl(element.second, env));
		}
		return result;
	}

	return ast;
}

ASTNode* Eval::apply(List* evaluated_list)
{
	auto nodes = evaluated_list->nodes();

	if (!is<Function>(nodes[0])) {
		return nullptr;
	}

	// car
	auto lambda = static_cast<Function*>(nodes[0])->lambda();
	// cdr
	std::span<ASTNode*> span { nodes.data() + 1, nodes.size() - 1 };

	return lambda(span);
}

} // namespace blaze
