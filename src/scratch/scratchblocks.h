#pragma once
#include <vector>
#include "scratchopcodes.h"
#include "scratchtree.h"

class ScratchStack : private std::vector<ScratchValue>
{
public:
	inline void Push(const ScratchValue& Value) { push_back(Value); }
	inline void PushMove(ScratchValue& Value) { emplace_back(std::move(Value)); }
	void Pop(ScratchValue& Out);
	void Pop(size_t Count);

	const ScratchValue& Peek(size_t Pos) const;
	inline size_t Size() const { return size(); }
};

struct ScratchState
{
	size_t base = 0;
	ScratchValue ret;
	ScratchStack stack;
};

class ScratchMethod
{
public:
	virtual int Exec(ScratchState& State) = 0;
};

class ScratchLiteral : public ScratchMethod
{
public:
	ScratchLiteral(const ScratchValue& Value) : m_value(Value) {}
	int Exec(ScratchState& State) override { State.stack.Push(m_value); return 0; }

private:
	const ScratchValue m_value;
};

class ScratchArg : public ScratchMethod
{
public:
	ScratchArg(int StackIndex) : m_index(StackIndex) {}
	int Exec(ScratchState& State) override;

private:
	const int m_index;
};

class ScratchPushVar : public ScratchMethod
{
public:
	ScratchPushVar(const ScratchVar* Var) : m_var(Var) {}
	int Exec(ScratchState& State) override { State.stack.Push(m_var->Value()); return 0; }

private:
	const ScratchVar* const m_var;
};

class ScratchSetVar : public ScratchMethod
{
public:
	ScratchSetVar(ScratchVar* Var) : m_var(Var) {}
	int Exec(ScratchState& State) override;

	inline ScratchVar* GetVar() const { return m_var; }

private:
	ScratchVar* const m_var;
};

class ScratchSetList : public ScratchMethod
{
public:
	ScratchSetList(ScratchList* List) : m_list(List) {}
	int Exec(ScratchState& State) override;

	inline ScratchList* GetList() const { return m_list; }

private:
	ScratchList* const m_list;
};

class ScratchPop : public ScratchMethod
{
public:
	ScratchPop(size_t Count) : m_count(Count) {}
	int Exec(ScratchState& State) override { State.stack.Pop(m_count); return 0; }

private:
	const size_t m_count;
};

// - Chain of scratch blocks to be executed
class ScratchChain : public ScratchMethod
{
public:
	ScratchChain() { }
	~ScratchChain();

	inline void AddOpcode(EScratchOpcode Op)
	{
		m_ops.push_back(Op);
		m_inputmap.push_back(m_inputs.size());
	}

	// - Adds an input to the current opcode
	inline void AddInput(ScratchMethod* Input) {
		m_inputs.push_back(Input);
	}

	void InsertOpcode(EScratchOpcode Op, size_t Index);
	void InsertInput(size_t OpIndex, size_t Index, ScratchMethod* Input);
	size_t GetInputCount(size_t Block) const;

	inline const std::vector<EScratchOpcode>& Code() const { return m_ops; }

	int Exec(ScratchState& State);

private:
	ScratchMethod* GetInput(size_t Block, size_t Index);
	inline void BinOpHack(ScratchState& State, size_t Block, ScratchValue& First)
	{
		State.stack.Pop(First);
		State.stack.Pop(State.ret);
	}

	std::vector<EScratchOpcode> m_ops;
	std::vector<ScratchMethod*> m_inputs;
	std::vector<size_t> m_inputmap; // Maps block pos to its available inputs
};

class Scratch_NotImplemented : public ScratchMethod
{
public:
	Scratch_NotImplemented() { }

	int Exec(ScratchState& State) override
	{
		printf("Scratch_NotImplemented\n");
		State.ret.Set("<Not implemented>");
		return 0;
	}
};
