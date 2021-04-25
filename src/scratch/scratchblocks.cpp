#include "scratchblocks.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <Windows.h>

#define SCRATCH_PRINT(fmt, ...) printf("\t\t" fmt, __VA_ARGS__)

ScratchValue ScratchStack::Pop()
{
	assert(!empty());
	ScratchValue val = back();
	pop_back();
	return val;
}

int ScratchChain::Exec(ScratchState& State)
{
	// TODO: Goal is fewest local vars possible
	size_t input = 0;
	ScratchValue temp;
	ScratchMethod* mth;

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
		case control_if_else:
		case control_if:  // TODO: MUST ensure all inputs used. It's affecting optimization (both writing and compiling)
			NextInput(input)->Exec(State);
			temp = State.ret; // Cond
			mth = NextInput(input);
			if (temp.GetBool())
				mth->Exec(State);
			if (op == control_if_else)
			{
				mth = NextInput(input);
				if (!temp.GetBool())
					mth->Exec(State);
			}
			break;
		//case control_stop:
		case control_wait:
			NextInput(input)->Exec(State);
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
			NextInput(input)->Exec(State);
			SCRATCH_PRINT("looks_say: %s\n", State.ret.GetString().c_str());
			break;

		case looks_thinkforsecs:
		case looks_sayforsecs:
			NextInput(input)->Exec(State);
			SCRATCH_PRINT("looks_sayforsecs: %s\n", State.ret.GetString().c_str());
			NextInput(input)->Exec(State);
			Sleep((DWORD)(State.ret.GetNumber() * 1000));
			break;

		case operator_add:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() + State.ret.GetNumber());
			break;

		case operator_subtract:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() - State.ret.GetNumber());
			break;

		case operator_multiply:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() * State.ret.GetNumber());
			break;

		case operator_divide:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() / State.ret.GetNumber());
			break;

		//case operator_random:
		case operator_lt:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() < State.ret.GetNumber());
			break;

		case operator_equals:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() == State.ret.GetNumber());
			break;

		case operator_gt:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetNumber() > State.ret.GetNumber());
			break;

		case operator_and:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetBool() && State.ret.GetBool());
			break;

		case operator_or: // TODO: Should be optimized, but operation MUST eat all inputs first
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetBool() || State.ret.GetBool());
			break;

		case operator_join:
			BinOpHack(State, input, temp);
			State.ret.Set(temp.GetString() + State.ret.GetString());
			break;

		case operator_letter_of:
			BinOpHack(State, input, temp);
			
			// TODO: Looks ridiculous. Perhaps have a union to store primitive types
			if ((int)temp.GetNumber() >= 1 && (int)temp.GetNumber() <= State.ret.GetString().length())
				State.ret.Set(std::string() + State.ret.GetString()[(size_t)temp.GetNumber() - 1]);
			else
				State.ret.Set("");
			break;

		case operator_length:
			NextInput(input)->Exec(State);
			State.ret.Set((double)State.ret.GetString().length());
			break;

		//case operator_contains:
		case operator_mod:
			BinOpHack(State, input, temp);
			State.ret.Set((int)temp.GetNumber() % (int)State.ret.GetNumber());
			break;

		case operator_round:
			NextInput(input)->Exec(State);
			State.ret.Set(round(State.ret.GetNumber()));
			break;

		//case operator_mathop:

		default:
			SCRATCH_PRINT("<unknown>\n");
		}
	}
	return 0;
}

ScratchMethod* ScratchChain::NextInput(size_t& InputCount)
{
	assert(InputCount < m_inputs.size());
	return m_inputs[InputCount++];
}
