/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>

#include "forward.h" // EnvironmentPtr

namespace blaze {

// All of these combined become a Function in the Environment
struct SpecialFormParts {
	std::string_view name;
	std::string_view signature;
	std::string_view documentation;
};

class Eval {
public:
	Eval(ValuePtr ast, EnvironmentPtr env);
	virtual ~Eval() = default;

	static void registerSpecialForm(SpecialFormParts special_form_parts);

	void eval();

	ValuePtr ast() const { return m_ast; }

private:
	ValuePtr evalImpl();
	ValuePtr evalSymbol(ValuePtr ast, EnvironmentPtr env);
	ValuePtr evalVector(ValuePtr ast, EnvironmentPtr env);
	ValuePtr evalHashMap(ValuePtr ast, EnvironmentPtr env);

	ValuePtr evalDef(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalDefMacro(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalDescribe(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalFn(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalQuasiQuoteExpand(const ValueVector& nodes);
	ValuePtr evalQuote(const ValueVector& nodes);
	ValuePtr evalTry(const ValueVector& nodes, EnvironmentPtr env);

	void evalAnd(const ValueVector& nodes, EnvironmentPtr env);
	void evalDo(const ValueVector& nodes, EnvironmentPtr env);
	void evalIf(const ValueVector& nodes, EnvironmentPtr env);
	void evalLet(const ValueVector& nodes, EnvironmentPtr env);
	void evalMacroExpand1(const ValueVector& nodes, EnvironmentPtr env);
	void evalOr(const ValueVector& nodes, EnvironmentPtr env);
	void evalQuasiQuote(const ValueVector& nodes, EnvironmentPtr env);
	void evalWhile(const ValueVector& nodes, EnvironmentPtr env);

	ValuePtr apply(ValuePtr function, const ValueVector& nodes);

	ValuePtr m_ast;
	EnvironmentPtr m_env;
	EnvironmentPtr m_outer_env;

	static std::vector<SpecialFormParts> s_special_form_parts;
};

} // namespace blaze
