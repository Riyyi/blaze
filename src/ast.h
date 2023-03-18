/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint> // int64_t
#include <string>
#include <string_view>
#include <typeinfo> // typeid
#include <vector>

namespace blaze {

class ASTNode {
public:
	virtual ~ASTNode() = default;

	std::string className() const { return typeid(*this).name(); }

	template<typename T>
	bool fastIs() const = delete;

	virtual bool isVector() const { return false; }
	virtual bool isHashMap() const { return false; }
	virtual bool isList() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isNumber() const { return false; }
	virtual bool isSpecialSymbol() const { return false; }
	virtual bool isSymbol() const { return false; }
};

// -----------------------------------------

// []
class Vector final : public ASTNode {
public:
	Vector();
	virtual ~Vector();

	virtual bool isVector() const override { return true; }

private:
	std::vector<ASTNode*> m_nodes;
};

// -----------------------------------------

// {}
class HashMap final : public ASTNode {
public:
	HashMap();
	virtual ~HashMap();

	virtual bool isHashMap() const override { return true; }

private:
	std::vector<ASTNode*> m_nodes;
};

// -----------------------------------------

// ()
class List final : public ASTNode {
public:
	List() = default;
	virtual ~List() override;

	virtual bool isList() const override { return true; }

	void addNode(ASTNode* node);

	const std::vector<ASTNode*>& nodes() const { return m_nodes; }

private:
	std::vector<ASTNode*> m_nodes;
};

// -----------------------------------------

// "string"
class String final : public ASTNode {
public:
	String(const std::string& data);
	virtual ~String() = default;

	virtual bool isString() const override { return true; }

	const std::string& data() const { return m_data; }

private:
	std::string m_data;
};

// -----------------------------------------

// 123
class Number final : public ASTNode {
public:
	Number(int64_t number);
	virtual ~Number() = default;

	virtual bool isNumber() const override { return true; }

	int64_t number() const { return m_number; }

private:
	int64_t m_number { 0 };
};

// -----------------------------------------

// true, false, nil
class SpecialSymbol final : public ASTNode {
public:
	SpecialSymbol();
	virtual ~SpecialSymbol();

	virtual bool isSpecialSymbol() const override { return true; }

private:
	std::string m_symbol;
};

// -----------------------------------------

// Other symbols
class Symbol final : public ASTNode {
public:
	Symbol(const std::string& symbol);
	virtual ~Symbol() = default;

	virtual bool isSymbol() const override { return true; }

	std::string symbol() const { return m_symbol; }

private:
	std::string m_symbol;
};

// -----------------------------------------

// clang-format off
template<>
inline bool ASTNode::fastIs<Vector>() const { return isVector(); }

template<>
inline bool ASTNode::fastIs<HashMap>() const { return isHashMap(); }

template<>
inline bool ASTNode::fastIs<List>() const { return isList(); }

template<>
inline bool ASTNode::fastIs<String>() const { return isString(); }

template<>
inline bool ASTNode::fastIs<Number>() const { return isNumber(); }

template<>
inline bool ASTNode::fastIs<SpecialSymbol>() const { return isSpecialSymbol(); }

template<>
inline bool ASTNode::fastIs<Symbol>() const { return isSymbol(); }
// clang-format on

} // namespace blaze
