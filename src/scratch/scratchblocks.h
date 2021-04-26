#pragma once
#include <vector>
#include "scratchopcodes.h"
#include "scratchtree.h"

class ScratchStack : private std::vector<ScratchValue>
{
public:
	inline void Push(const ScratchValue& Value) { push_back(Value); }
	ScratchValue Pop();
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
	ScratchLiteral(const ScratchValue& Value) :m_value(Value) {}
	int Exec(ScratchState& State) override { State.ret = m_value; return 0; }

private:
	const ScratchValue m_value;
};

// - Chain of scratch blocks to be executed
class ScratchChain : public ScratchMethod
{
public:
	ScratchChain() { }
	~ScratchChain();

	inline void AddOpcode(EScratchOpcode Op) {
		m_ops.push_back(Op);
	}
	inline void AddInput(ScratchMethod* Input) {
		m_inputs.push_back(Input);
	}

	inline std::vector<EScratchOpcode>& Code() { return m_ops; }

	int Exec(ScratchState& State);

private:
	ScratchMethod* NextInput(size_t& InputCount);
	inline void BinOpHack(ScratchState& State, size_t& InputCount, ScratchValue& First)
	{
		NextInput(InputCount)->Exec(State);
		First = State.ret;
		NextInput(InputCount)->Exec(State);
	}

	std::vector<EScratchOpcode> m_ops;
	std::vector<ScratchMethod*> m_inputs;
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
