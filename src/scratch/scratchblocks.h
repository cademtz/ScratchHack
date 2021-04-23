#pragma once
#include <vector>
#include "scratchopcodes.h"
#include "scratchtree.h"

class ScratchBlock
{
public:
	static ScratchBlock* FromOpcode(EScratchOpcode Opcode, ScratchInputs& Inputs);

	virtual void Execute(ScratchValue& Result) = 0;
	inline EScratchOpcode Opcode() const { return m_opcode; }
	inline std::vector<ScratchBlock*>& Children() { return m_children; }

	virtual ~ScratchBlock() { for (ScratchBlock* block : m_children) delete block; }

protected:
	ScratchBlock(EScratchOpcode Opcode) : m_opcode(Opcode) { }

private:
	std::vector<ScratchBlock*> m_children;
	const EScratchOpcode m_opcode;
};

class ScratchBlock_NotImplemented : public ScratchBlock
{
public:
	ScratchBlock_NotImplemented(EScratchOpcode Opcode) : ScratchBlock(Opcode) { }
	void Execute(ScratchValue& Result) override;
};

class ScratchBlock_Nop : public ScratchBlock
{
public:
	ScratchBlock_Nop(EScratchOpcode Opcode) : ScratchBlock(Opcode) { }
	void Execute(ScratchValue& Result) override {
		Result.Set("<ScratchBlock_Nop>");
	}
};

class ScratchBlock_Literal : public ScratchBlock
{
public:
	ScratchBlock_Literal(const ScratchValue& Value)
		: ScratchBlock(ScratchOpcode_unknown), m_value(Value) { }
	void Execute(ScratchValue& Result) override { Result = m_value; }

private:
	ScratchValue m_value;
};

// Private interfaces
class ScratchBinary : public ScratchBlock
{
protected:
	ScratchBinary(ScratchInputs& Inputs, EScratchOpcode Op, const char* One, const char* Two);
	~ScratchBinary() { delete m_one; delete m_two; }

	inline ScratchBlock* One() { return m_one; }
	inline ScratchBlock* Two() { return m_two; }

private:
	ScratchBlock* m_one, * m_two;
};
class ScratchUnary : public ScratchBlock
{
protected:
	ScratchUnary(ScratchInputs& Inputs, EScratchOpcode Op, const char* In);
	~ScratchUnary() { delete m_one; }

	inline ScratchBlock* One() { return m_one; }

private:
	ScratchBlock* m_one;
};

class ScratchOperator_Join : public ScratchBinary
{
public:
	ScratchOperator_Join(ScratchInputs& Inputs) :
		ScratchBinary(Inputs, operator_join, "STRING1", "STRING2") { }
	void Execute(ScratchValue& Result) override;
};

class ScratchOperator_Equals : public ScratchBinary
{
public:
	ScratchOperator_Equals(ScratchInputs& Inputs) :
		ScratchBinary(Inputs, operator_equals, "OPERAND1", "OPERAND2") { }
	void Execute(ScratchValue& Result) override;
};

class ScratchLooks_Say : public ScratchUnary
{
public:
	ScratchLooks_Say(ScratchInputs& Inputs)
		: ScratchUnary(Inputs, looks_say, "MESSAGE") { }
	void Execute(ScratchValue& Result) override;
};
class ScratchLooks_SayForSecs : public ScratchBinary
{
public:
	ScratchLooks_SayForSecs(ScratchInputs& Inputs)
		: ScratchBinary(Inputs, looks_sayforsecs, "MESSAGE", "SECS") { }
	void Execute(ScratchValue& Result) override;
};

class ScratchLooks_Think : public ScratchLooks_Say
{
public:
	ScratchLooks_Think(ScratchInputs& Inputs) : ScratchLooks_Say(Inputs) { }
};
class ScratchLooks_ThinkForSecs : public ScratchLooks_SayForSecs
{
public:
	ScratchLooks_ThinkForSecs(ScratchInputs& Inputs)
		: ScratchLooks_SayForSecs(Inputs) { }
};