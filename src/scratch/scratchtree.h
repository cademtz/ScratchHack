#pragma once
#include <list>
#include <string>
#include <vector>
#include <stdint.h>

#define _SCRATCH_INTBYTE(val, idx) (uint8_t)(((int32_t)val << (idx * 8)) & 0xFF)

class ScratchTarget;
class ScratchBlock;
class ScratchList;
class ScratchVar;

struct ScratchColor { uint8_t rgba[4]; };

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

	inline const std::vector<ScratchTarget>& Targets() const { return m_targets; }
	inline std::vector<ScratchTarget>& Targets() { return m_targets; }

private:
	std::vector<ScratchTarget> m_targets;
};

class ScratchTarget
{
public:
	ScratchTarget() {}
	ScratchTarget(const char* Name, bool IsStage) : m_name(Name), m_isStage(IsStage) { }
	~ScratchTarget() { for (auto block : m_blocks) delete block; }

	inline bool IsStage() const { return m_isStage; }
	inline const std::string Name() const { return m_name; }
	inline std::vector<ScratchVar>& Vars() { return m_vars; }
	inline std::vector<ScratchList>& Lists() { return m_lists; }

private:
	bool m_isStage = false;
	std::string m_name;
	std::vector<ScratchVar> m_vars;
	std::vector<ScratchList> m_lists;
	std::vector<ScratchBlock*> m_blocks;
};

class ScratchValue
{
public:
	ScratchValue() : m_value("0"), m_number(0), m_types(ScratchType_Number) { }

	inline bool IsType(EScratchType Type) const { return m_types & Type; }
	inline bool GetBool() const { IsType(ScratchType_Bool) && m_number == 1; }
	inline double GetNumber() const { return IsType(ScratchType_Number) ? m_number : 0; }
	inline const std::string& GetString() const { return m_value; }

	void Set(const char* Value);
	inline void Set(double Value);
	inline void Set(const std::string& Value) { Set(Value.c_str()); }

private:
	std::string m_value;
	double m_number;
	int m_types;
};

class ScratchVar
{
public:
	ScratchVar(const char* Name, ScratchValue Value) : m_name(Name), m_value(Value) { }

	inline const std::string& Name() const { return m_name; }
	inline ScratchValue& Value() { return m_value; }

private:
	std::string m_name;
	ScratchValue m_value;
};

class ScratchList
{
public:
	ScratchList(const char* Name) : m_name(Name) { }

	inline const std::string& Name() const { return m_name; }
	inline std::list<ScratchValue>& Values() { return m_values; }

private:
	std::string m_name;
	std::list<ScratchValue> m_values;
};
