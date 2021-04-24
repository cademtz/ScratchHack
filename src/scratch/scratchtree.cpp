#include "scratchtree.h"
#include "scratchblocks.h"
#include <assert.h>

// muh cross-platform  *<:o)  honk honk
int _Scratch_stricmp(const char* One, const char* Two)
{
	char a = 0, b = 0;
	for (; *One && *Two && (a = tolower(*One)) == (b = tolower(*Two)); ++One, ++Two);
	return a - b;
}

/*ScratchChain::~ScratchChain()
{
	for (ScratchBlock* block : *this)
		delete block;
}*/

void ScratchValue::Set(const char* Value)
{
	m_value = Value;
	m_number = 0;

	if (!_Scratch_stricmp(Value, "true") || !_Scratch_stricmp(Value, "false"))
	{
		m_types = ScratchType_Bool;
		m_number = tolower(Value[0]) == 't';
	}
	/*else if (Value[0] == '#') // TODO: Hex color support? (Unofficial)
	{

	}*/
	else
	{
		char* end;

		m_number = strtod(Value, &end);
		for (; isspace(*end); ++end); // Ensure no real content exists after number
		if (!errno && end != Value && !*end) // Successfully parsed double to end of string
			m_types = ScratchType_Number;
	}

	m_types |= ScratchType_String; // Every type can be represented by a string
}

 void ScratchValue::Set(bool Value)
{
	m_value = Value ? "true" : "false";
	m_number = Value;
	m_types = ScratchType_String | ScratchType_Bool;
}

void ScratchValue::Set(double Value)
{
	m_value = std::to_string(Value);
	m_number = Value;
	m_types = ScratchType_String | ScratchType_Number;
}

void ScratchValue::Set(int Value)
{
	m_value = std::to_string(Value);
	m_number = Value;
	m_types = ScratchType_String | ScratchType_Number;
}
