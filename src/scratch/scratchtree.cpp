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

ScratchTarget::~ScratchTarget()
{
	for (ScratchChain* chain : m_chains)
		delete chain;
	for (ScratchVar* var : m_vars)
		delete var;
	for (ScratchList* list : m_lists)
		delete list;
}

void ScratchValue::Set(const char* Value)
{
	m_value = Value;
	m_number = 0;
	m_types = ScratchType_String;

	if (!_Scratch_stricmp(Value, "true") || !_Scratch_stricmp(Value, "false"))
	{
		m_types |= ScratchType_Bool;
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
			m_types |= ScratchType_Number;
	}
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

bool ScratchValue::Equals(const ScratchValue& Other)
{
	int overlap = Other.GetTypes() & GetTypes();
	if (overlap & ScratchType_Bool)
		return Other.GetBool() == GetBool();
	else if (overlap & ScratchType_Number)
		return Other.GetNumber() == GetNumber();
	return Other.GetString() == GetString();
}

bool ScratchList::ValueAt(size_t Index, ScratchValue& Out)
{
	if (Index >= m_values.size())
		return false;

	for (auto it = m_values.begin(); it != m_values.end(); ++it, --Index)
	{
		if (Index == 0)
		{
			Out = *it;
			break;
		}
	}
	return true;
}

bool ScratchList::Insert(size_t Index, const ScratchValue& Value)
{
	if (Index > m_values.size())
		return false;

	m_values.insert(m_values.begin() + Index, Value);
	return true;
}

bool ScratchList::Replace(size_t Index, const ScratchValue& Value)
{
	if (Index >= m_values.size())
		return false;

	m_values[Index] = Value;
	return true;
}

bool ScratchList::Delete(size_t Index)
{
	if (Index >= m_values.size())
		return false;
	m_values.erase(m_values.begin() + Index);
	return true;
}

size_t ScratchList::ItemNum(const ScratchValue& Value)
{
	size_t num = 0;
	for (auto it = m_values.begin(); it != m_values.end(); ++it, ++num)
	{
		if ((*it).Equals(Value))
			return num + 1;
	}

	return 0;
}
