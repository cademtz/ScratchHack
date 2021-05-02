#include "scratchblocks.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <Windows.h>

#define SCRATCH_PRINT(fmt, ...) printf("\t\t" fmt, __VA_ARGS__)

void ScratchStack::Pop(ScratchValue& Out)
{
	assert(!empty());
	Out = std::move(back());
	pop_back();
}

void ScratchStack::Pop(size_t Count)
{
	assert(Count <= size());
	resize(size() - Count);
}

const ScratchValue& ScratchStack::Peek(size_t Pos) const
{
	assert(Pos < size());
	return at(size() - Pos - 1);
}

int ScratchArg::Exec(ScratchState& State)
{
	//assert(m_index < State.stack.size() && "Out of bounds access to Scratch stack");
	//State.ret = State.stack.Peek(m_index);
	size_t off = State.stack.Size() - State.base;
	State.stack.Push(State.stack.Peek(m_index + off)); // Very efficient xd
	return 0;
}

ScratchChain::~ScratchChain()
{
	//for (ScratchMethod* input : m_inputs)
	//	delete input;
}

void ScratchChain::InsertOpcode(EScratchOpcode Op, size_t Index)
{
	if (Index == m_ops.size() - 1)
		return AddOpcode(Op);
	m_ops.insert(m_ops.begin() + Index, Op);
	m_inputmap.insert(m_inputmap.begin() + Index, m_inputmap[Index + 1]);
}

void ScratchChain::InsertInput(size_t OpIndex, size_t Index, ScratchMethod* Input)
{
	m_inputs.insert(m_inputs.begin() + m_inputmap[OpIndex] + Index, Input);
	if (OpIndex < m_ops.size() - 1) // Fix inputmap for shifted inputs ahead
	{
		for (size_t i = OpIndex + 1; i < m_ops.size(); ++i)
			++m_inputmap[i];
	}
}

int ScratchChain::Exec(ScratchState& State)
{
	size_t block = 0;
	size_t base = State.base;
	ScratchValue temp;

#ifdef _DEBUG
	size_t _stacksize = State.stack.Size();
#endif

	for (EScratchOpcode op : m_ops)
	{
		size_t inputsize = GetInputCount(block);

		switch (op)
		{
		case ScratchOpcode_push:
			GetInput(block, 0)->Exec(State);
			break;

			// Current event description, does nothing
		case event_whentouchingobject:
		case event_touchingobjectmenu:
		case event_whenflagclicked:
		case event_whenthisspriteclicked:
		case event_whenstageclicked:
		case event_whenbroadcastreceived:
		case event_whenbackdropswitchesto:
		case event_whengreaterthan:
			break;

		//case control_forever:
		//case control_repeat:
		case control_if:
		case control_if_else:
			State.stack.Pop(State.ret);
			if (State.ret.GetBool())
				GetInput(block, 0)->Exec(State);
			else if (op == control_if_else)
				GetInput(block, 1)->Exec(State);
			break;

		//case control_stop:
		case control_wait:
			State.stack.Pop(State.ret);
			Sleep((DWORD)(State.ret.GetNumber() * 1000));
			break;
		//case control_wait_until:
		//case control_repeat_until:
		//case control_while: // Obsolete
		//case control_for_each: // Obsolete 
		//case control_start_as_clone:
		//case control_create_clone_of_menu:
		//case control_create_clone_of:
		//case control_delete_this_clone:
		//case control_get_counter: // Obsolete 
		//case control_incr_counter: // Obsolete
		//case control_clear_counter:
		//case control_all_at_once: // Obsolete

		case looks_think:
		case looks_say:
			State.stack.Pop(State.ret);
			SCRATCH_PRINT("looks_say: %s\n", State.ret.GetString().c_str());
			break;

		case looks_thinkforsecs:
		case looks_sayforsecs:
			State.stack.Pop(State.ret);
			SCRATCH_PRINT("looks_sayforsecs: %s\n", State.ret.GetString().c_str());
			State.stack.Pop(State.ret);
			Sleep((DWORD)(State.ret.GetNumber() * 1000));
			break;

		case operator_add:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() + State.ret.GetNumber());
			break;

		case operator_subtract:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() - State.ret.GetNumber());
			break;

		case operator_multiply:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() * State.ret.GetNumber());
			break;

		case operator_divide:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() / State.ret.GetNumber());
			break;

		//case operator_random:
		case operator_lt:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() < State.ret.GetNumber());
			break;

		case operator_equals:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() == State.ret.GetNumber());
			break;

		case operator_gt:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetNumber() > State.ret.GetNumber());
			break;

		case operator_and:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetBool() && State.ret.GetBool());
			break;

		case operator_or:
			GetInput(block, 0)->Exec(State);
			if (!State.ret.GetBool())
				GetInput(block, 1)->Exec(State);
			break;

		case operator_join:
			BinOpHack(State, block, temp);
			State.stack.Push(temp.GetString() + State.ret.GetString());
			break;

		case operator_letter_of:
			BinOpHack(State, block, temp);
			
			// TODO: Looks ridiculous. Perhaps have a union to store primitive types
			if ((int)temp.GetNumber() >= 1 && (int)temp.GetNumber() <= State.ret.GetString().length())
				State.stack.Push(State.ret.GetString().substr((size_t)temp.GetNumber() - 1));
			else
				State.stack.Push("");
			break;

		case operator_length:
			State.stack.Pop(State.ret);
			State.stack.Push((double)State.ret.GetString().length());
			break;

		//case operator_contains:
		case operator_mod:
			BinOpHack(State, block, temp);
			State.stack.Push((int)temp.GetNumber() % (int)State.ret.GetNumber());
			break;

		case operator_round:
			State.stack.Pop(State.ret);
			State.stack.Push(round(State.ret.GetNumber()));
			break;

		//case operator_mathop:

		case procedures_definition: break; // Current custom proc description, does nothing.
		case procedures_call:
			State.base = State.stack.Size();
			GetInput(block, 0)->Exec(State); // Call
			State.base = base;
			GetInput(block, 1)->Exec(State); // Pop
			break;

		case procedures_prototype: assert(0 && "Prototype should not be executed"); break;
		case procedures_declaration: assert(0 && "Declaration should not be executed"); break;
		case argument_reporter_string_number:
		case argument_reporter_boolean:
			break; // ScratchOpcode_push appears beforehand and does the work

		default:
			SCRATCH_PRINT("<unknown>\n");
		}

		++block;
	}

#ifdef _DEBUG
	assert(_stacksize == State.stack.Size() && "Uh oh! Looks like your stack... is out of wack!");
#endif

	return 0;
}

ScratchMethod* ScratchChain::GetInput(size_t Block, size_t Index)
{
	assert(Block < m_ops.size() && "Block was past end of list");
	assert(Index < GetInputCount(Block) && "Index exceeds available inputs for this block");
	return m_inputs[m_inputmap[Block] + Index];
}

size_t ScratchChain::GetInputCount(size_t Block) const
{
	assert(Block < m_ops.size() && "Block was past end of list");
	if (Block == m_ops.size() - 1)
		return m_inputs.size() - m_inputmap.back();
	return m_inputmap[Block + 1] - m_inputmap[Block];
}
