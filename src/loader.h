#pragma once
#include "json.h"
#include "scratch/scratchtree.h"
#include <vector>
#include <string>
#include <map>

class ScratchMethod;
struct LoaderBlock;
typedef std::map<const LoaderBlock*, ScratchMethod*> ScratchBlockMap;
typedef std::map<std::string, LoaderBlock> JsnBlockMap;

enum EScratchInputType
{
	// Reserved for loader
	ScratchInputType_Arg	= -2,
	ScratchInputType_Block	= -1,

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

struct LoaderMutation
{
	std::string proccode;
	std::map<std::string, std::string> argmap; // Maps arg name to coresponding arg setter key
	bool warp;
};

struct LoaderInput
{
	std::vector<std::string> vals; // Has at least 1
	int type;
};

struct LoaderField {
	std::vector<std::string> vals; // Has at least 1
};

struct LoaderBlock
{
	LoaderMutation mutation;
	std::map<std::string, LoaderInput> inputs;
	std::map<std::string, LoaderField> fields;
	const LoaderBlock* _next;
	const LoaderBlock* _parent;
	int opcode;
	bool topLevel;
	bool hasMutation;
};

struct LoaderState
{
	ScratchBlockMap& loaded;
	const JsnBlockMap& map;
};

bool Loader_LoadProject(const char* Json, jsmntok_t* JSNProj, ScratchTree& Tree);

bool Loader_LoadTarget(const char* Json, jsmntok_t* JSNTarget, ScratchTree& Tree);