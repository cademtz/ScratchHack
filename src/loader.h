#pragma once
#include "json.h"
#include "scratch/scratchtree.h"
#include <vector>
#include <string>
#include <map>

class ScratchChain;
struct ParsedBlock;
typedef std::map<ParsedBlock*, ScratchChain*> ScratchBlockMap;
typedef std::map<std::string, ParsedBlock> JsnBlockMap;

enum EScratchInputType
{
	// Reserved for loader
	ScratchInputType_Arg	= -3,
	ScratchInputType_Block	= -2,
	ScratchInputType_Inline = -1,

	ScratchInputType_Number = 4,
	ScratchInputType_PositiveNum,
	ScratchInputType_PositiveInt,
	ScratchInputType_Int,
	ScratchInputType_Angle,
	ScratchInputType_Color,
	ScratchInputType_String,
	ScratchInputType_Broadcast,
	ScratchInputType_Variable,
	ScratchInputType_List,
};

enum EScratchShadow
{
	ScratchShadow_Used = 1,
	ScratchShadow_None,
	ScratchShadow_Unused,
};

enum ELoaderBlockFlags
{
	LoaderBlockFlag_TopLevel	= (1 << 0),
	LoaderBlockFlag_Mutation	= (1 << 1),
	LoaderBlockFlag_Inline		= (1 << 2), // - Should be inlined
};

struct ParsedMutation
{
	std::string proccode;
	std::map<std::string, std::string> argmap; // Maps arg name to coresponding arg setter key
	bool warp;
};

struct ParsedInput
{
	std::vector<std::string> vals; // Has at least 1
	int type;
};

struct ParsedField {
	std::vector<std::string> vals; // Has at least 1
};

struct ParsedBlock
{
	ParsedMutation mutation;
	std::map<std::string, ParsedInput> inputs;
	std::map<std::string, ParsedField> fields;
	const ParsedBlock* next;
	const ParsedBlock* parent;
	int opcode;
	int flags; // ORed values from ELoaderBlockFlags
};

struct ParsedTarget
{
	JsnBlockMap map;
	ScratchBlockMap loaded;
	std::string name;
	bool isStage;
};

class CScratchLoader
{
public:
	CScratchLoader(const char* Json, size_t JsonLen = (size_t)-1);
	~CScratchLoader() { ResetParser(); }

	bool ParseProject();
	bool LoadProject(ScratchTree* Tree);

private:
	void ResetParser();
	bool ParseTarget(jsmntok_t* JsnTarget, ParsedTarget& Target);
	bool ParseBlock(jsmntok_t* JsnBlock, ParsedBlock& Block);
	bool ParseInput(jsmntok_t* JsnInputArr, ParsedInput& Input);
	bool ParseField(jsmntok_t* JsnField, ParsedField& Field);
	bool ParseMutation(jsmntok_t* JsnMut, ParsedMutation& Mut, bool GetArgs);

	// Helpers
	bool InputSafety(ParsedBlock& Block);
	bool FixArgs(JsnBlockMap::value_type& Pair);
	const ParsedBlock* GetProto(const ParsedBlock& BlockInfo);

	bool LoadChain(ScratchChain& Chain, const ParsedBlock& BlockInfo);
	bool LoadBlock(ScratchChain& Chain, const ParsedBlock& BlockInfo);
	bool InlineInput(const ParsedInput& Input, ScratchChain& Chain);

	const char* m_json;
	size_t m_jsonLen;
	jsmntok_t* m_tokens;

	std::list<ParsedTarget> m_targets;
	ParsedTarget* m_target;
	ScratchTree* m_tree = 0;
	bool m_parsed = false;
};
