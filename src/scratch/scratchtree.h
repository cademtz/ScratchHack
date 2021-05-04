#pragma once
#include <map>
#include <list>
#include <deque>
#include <string>
#include <stdint.h>
#include "scratchopcodes.h"

#define _SCRATCH_INTBYTE(val, idx) (uint8_t)(((int32_t)val << (idx * 8)) & 0xFF)

class ScratchMethod;
class ScratchTarget;
class ScratchChain;
class ScratchList;
class ScratchVar;

struct ScratchColor { uint8_t rgba[4]; };

int _Scratch_stricmp(const char* One, const char* Two); // Funny util

enum EScratchType
{
	ScratchType_Bool	= 1 << 1,
	ScratchType_Number	= 1 << 2,
	ScratchType_Color	= 1 << 3,
	ScratchType_String	= 1 << 4,
};

class ScratchTree
{
public:
	ScratchTree() { }

	inline const std::list<ScratchTarget>& Targets() const { return m_targets; }
	inline std::list<ScratchTarget>& Targets() { return m_targets; }

private:
	std::list<ScratchTarget> m_targets;
};

class ScratchTarget
{
public:
	ScratchTarget() {}
	ScratchTarget(const char* Name, bool IsStage) : m_name(Name), m_isStage(IsStage) { }
	~ScratchTarget();

	inline bool IsStage() const { return m_isStage; }
	inline const std::string Name() const { return m_name; }
	inline std::list<ScratchVar*>& Vars() { return m_vars; }
	inline std::list<ScratchList*>& Lists() { return m_lists; }
	inline std::list<ScratchChain*>& Chains() { return m_chains; }

private:
	bool m_isStage = false;
	std::string m_name;
	std::list<ScratchVar*> m_vars;
	std::list<ScratchList*> m_lists;
	std::list<ScratchChain*> m_chains;
};

class ScratchValue
{
public:
	ScratchValue() : m_number(0), m_types(ScratchType_String) { }

	template <class T>
	ScratchValue(const T& Value) { Set(Value); }

	inline bool IsType(int Types) const { return m_types & Types; }
	inline int GetTypes() const { return m_types; }
	inline bool GetBool() const { return IsType(ScratchType_Bool) && m_number == 1; }
	inline double GetNumber() const { return IsType(ScratchType_Number) ? m_number : 0; }
	inline const std::string& GetString() const { return m_value; }

	void Set(const char* Value);
	void Set(bool Value);
	void Set(double Value);
	void Set(int Value);
	inline void Set(const std::string& Value) { Set(Value.c_str()); }

	bool Equals(const ScratchValue& Other);

private:
	std::string m_value;
	double m_number;
	int m_types;
};

class ScratchVar
{
public:
	ScratchVar(const char* Name) : m_name(Name) { }
	ScratchVar(const char* Name, const ScratchValue& Val) : m_name(Name), m_values({ Val }) { }

	inline bool IsList() const { return m_isList; }
	inline const std::string& Name() const { return m_name; }
	inline const ScratchValue& Value() const { return m_values.front(); }
	inline ScratchValue& Value() { return m_values.front(); }

protected:
	ScratchVar(const char* Name, bool IsList) : m_name(Name), m_isList(IsList) { }

	std::deque<ScratchValue> m_values;

private:
	std::string m_name;
	const bool m_isList = false;
};

class ScratchList : public ScratchVar
{
public:
	ScratchList(const char* Name) : ScratchVar(Name, true) { }
	std::deque<ScratchValue>& ValueList() { return m_values; }

	bool ValueAt(size_t Index, ScratchValue& Out);
	bool Insert(size_t Index, const ScratchValue& Value);
	bool Replace(size_t Index, const ScratchValue& Value);
	bool Delete(size_t Index);
	inline void Add(const ScratchValue& Value) { m_values.push_back(Value); }
	inline void Clear() { m_values.clear(); }

	// - Returns a SCRATCH index (starts at 1), and 0 on failure
	size_t ItemNum(const ScratchValue& Value);
};
