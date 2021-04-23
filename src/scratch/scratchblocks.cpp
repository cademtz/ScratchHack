#include "scratchblocks.h"
#include <Windows.h>

ScratchBlock* ScratchBlock::FromOpcode(EScratchOpcode Opcode, ScratchInputs& Inputs)
{
	switch (Opcode)
	{
	case event_whenflagclicked: return new ScratchBlock_Nop(event_whenflagclicked);
	case operator_join: return new ScratchOperator_Join(Inputs);
	case operator_equals: return new ScratchOperator_Equals(Inputs);
	case looks_say: return new ScratchLooks_Say(Inputs);
	case looks_sayforsecs: return new ScratchLooks_SayForSecs(Inputs);
	case looks_think: return new ScratchLooks_Think(Inputs);
	case looks_thinkforsecs: return new ScratchLooks_ThinkForSecs(Inputs);
	}

    return new ScratchBlock_NotImplemented(Opcode);
}

void ScratchBlock_NotImplemented::Execute(ScratchValue& Result)
{
	Result.Set("<ScratchBlock_NotImplemented>");
	printf("unknown block");
}

ScratchBinary::ScratchBinary(ScratchInputs& Inputs, EScratchOpcode Op, const char* One, const char* Two)
	: ScratchBlock(Op)
{
	m_one = Inputs.GetSlot(One);
	m_two = Inputs.GetSlot(Two);
}

ScratchUnary::ScratchUnary(ScratchInputs& Inputs, EScratchOpcode Op, const char* In)
	: ScratchBlock(Op) {
	m_one = Inputs.GetSlot(In);
}

void ScratchOperator_Join::Execute(ScratchValue& Result)
{
	std::string join;

	Two()->Execute(Result);
	join = Result.GetString();
	One()->Execute(Result);
	Result.Set(Result.GetString() + join);
}

void ScratchOperator_Equals::Execute(ScratchValue& Result)
{
	One()->Execute(Result);
	ScratchValue first = Result;
	Two()->Execute(Result);

	int overlap = first.GetTypes() & Result.GetTypes();
	if (overlap)
	{
		if ((overlap & ScratchType_Bool) || (overlap & ScratchType_Number))
			Result.Set(first.GetNumber() == Result.GetNumber());
		//else if (overlap & ScratchType_Color)
		else
			Result.Set(first.GetString() == Result.GetString());
	}
	else
		Result.Set(false);
}

void ScratchLooks_Say::Execute(ScratchValue& Result)
{
	One()->Execute(Result);
	printf("Say: %s", Result.GetString().c_str());
}

void ScratchLooks_SayForSecs::Execute(ScratchValue& Result)
{
	One()->Execute(Result);
	printf("%s: %s", ScratchOpcode_ToString((int)Opcode()), Result.GetString().c_str());

	Two()->Execute(Result);
	Sleep((DWORD)(Result.GetNumber() * 1000));
}
