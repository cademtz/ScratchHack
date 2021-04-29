#pragma once
#include <vector>
#include "scratchopcodes.h"
#include "scratchtree.h"

class ScratchStack : private std::vector<ScratchValue>
{
public:
	inline void Push(const ScratchValue& Value) { push_back(Value); }
	void Pop(size_t Count);
	const ScratchValue& Peek(size_t Pos);
};

struct ScratchState
{
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
	int Exec(ScratchState& State) override { State.ret = m_value; return 0; }

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

	inline std::vector<EScratchOpcode>& Code() { return m_ops; }

	int Exec(ScratchState& State);

private:
	ScratchMethod* GetInput(size_t Block, size_t Index);
	size_t GetInputCount(size_t Block);
	inline void BinOpHack(ScratchState& State, size_t Block, ScratchValue& First)
	{
		GetInput(Block, 0)->Exec(State);
		First = State.ret;
		GetInput(Block, 1)->Exec(State);
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
