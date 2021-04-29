#include "scratchblocks.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <Windows.h>

#define SCRATCH_PRINT(fmt, ...) printf("\t\t" fmt, __VA_ARGS__)

void ScratchStack::Pop(size_t Count)
{
	assert(Count <= size());
	resize(size() - Count);
}

const ScratchValue& ScratchStack::Peek(size_t Pos)
{
	assert(Pos < size());
	return at(size() - Pos - 1);
}

int ScratchArg::Exec(ScratchState& State)
{
	//assert(m_index < State.stack.size() && "Out of bounds access to Scratch stack");
	State.ret = State.stack.Peek(m_index);
	return 0;
}

ScratchChain::~ScratchChain()
{
	//for (ScratchMethod* input : m_inputs)
	//	delete input;
}

int ScratchChain::Exec(ScratchState& State)
{
	size_t block = 0;
	ScratchValue temp;

	for (EScratchOpcode op : m_ops)
	{
		switch (op)
		{
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
			GetInput(block, 0)->Exec(State);
			if (State.ret.GetBool())
				GetInput(block, 1)->Exec(State);
			else if (op == control_if_else)
				GetInput(block, 2)->Exec(State);
			break;

		//case control_stop:
		case control_wait:
			GetInput(block, 0)->Exec(State);
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
			GetInput(block, 0)->Exec(State);
			SCRATCH_PRINT("looks_say: %s\n", State.ret.GetString().c_str());
			break;

		case looks_thinkforsecs:
		case looks_sayforsecs:
			GetInput(block, 0)->Exec(State);
			SCRATCH_PRINT("looks_sayforsecs: %s\n", State.ret.GetString().c_str());
			GetInput(block, 1)->Exec(State);
			Sleep((DWORD)(State.ret.GetNumber() * 1000));
			break;

		case operator_add:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() + State.ret.GetNumber());
			break;

		case operator_subtract:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() - State.ret.GetNumber());
			break;

		case operator_multiply:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() * State.ret.GetNumber());
			break;

		case operator_divide:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() / State.ret.GetNumber());
			break;

		//case operator_random:
		case operator_lt:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() < State.ret.GetNumber());
			break;

		case operator_equals:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() == State.ret.GetNumber());
			break;

		case operator_gt:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetNumber() > State.ret.GetNumber());
			break;

		case operator_and:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetBool() && State.ret.GetBool());
			break;

		case operator_or: // TODO: Should be optimized, but operation MUST eat all inputs first
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetBool() || State.ret.GetBool());
			break;

		case operator_join:
			BinOpHack(State, block, temp);
			State.ret.Set(temp.GetString() + State.ret.GetString());
			break;

		case operator_letter_of:
			BinOpHack(State, block, temp);
			
			// TODO: Looks ridiculous. Perhaps have a union to store primitive types
			if ((int)temp.GetNumber() >= 1 && (int)temp.GetNumber() <= State.ret.GetString().length())
				State.ret.Set(std::string() + State.ret.GetString()[(size_t)temp.GetNumber() - 1]);
			else
				State.ret.Set("");
			break;

		case operator_length:
			GetInput(block, 0)->Exec(State);//NextInput(input)->Exec(State);
			State.ret.Set((double)State.ret.GetString().length());
			break;

		//case operator_contains:
		case operator_mod:
			BinOpHack(State, block, temp);
			State.ret.Set((int)temp.GetNumber() % (int)State.ret.GetNumber());
			break;

		case operator_round:
			GetInput(block, 0)->Exec(State);//NextInput(input)->Exec(State);
			State.ret.Set(round(State.ret.GetNumber()));
			break;

		//case operator_mathop:

		case procedures_definition: break; // Current custom proc description, does nothing.
		case procedures_call:
		{
			size_t nargs = GetInputCount(block) - 1;
			for (size_t arg = 0; arg < nargs; ++arg)
			{
				GetInput(block, nargs - arg)->Exec(State); // Push args in reverse order
				State.stack.Push(State.ret);
			}

			GetInput(block, 0)->Exec(State);
			State.stack.Pop(nargs);

			break;
		}
		case procedures_prototype: assert(0 && "Prototype should not be executed"); break;
		case procedures_declaration: assert(0 && "Declaration should not be executed"); break;
		case argument_reporter_string_number:
		case argument_reporter_boolean:
			GetInput(block, 0)->Exec(State);
			break;

		default:
			SCRATCH_PRINT("<unknown>\n");
		}

		++block;
	}
	return 0;
}

ScratchMethod* ScratchChain::GetInput(size_t Block, size_t Index)
{
	assert(Block < m_ops.size() && "Block was past end of list");
	assert(Index < GetInputCount(Block) && "Index exceeds available inputs for this block");
	return m_inputs[m_inputmap[Block] + Index];
}

size_t ScratchChain::GetInputCount(size_t Block)
{
	assert(Block < m_ops.size() && "Block was past end of list");
	if (Block == m_ops.size() - 1)
		return m_inputs.size() - m_inputmap.back();
	return m_inputmap[Block + 1] - m_inputmap[Block];
}
