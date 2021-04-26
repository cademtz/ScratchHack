#pragma once
#include "json.h"
#include "scratch/scratchtree.h"
#include <vector>

enum EScratchInputType
{
	ScratchInputType_Block = 0,
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
	std::string argIds;
	std::string argNames;
	bool warp;
};

struct LoaderInput
{
	std::vector<std::string> vals; // Has at least 1
	int type;
};

struct LoaderBlock
{
	std::string next;
	std::vector<LoaderInput> inputs;
	int opcode;
	bool topLevel;
};

bool Loader_LoadProject(const char* Json, jsmntok_t* JSNProj, ScratchTree& Tree);

bool Loader_LoadTarget(const char* Json, jsmntok_t* JSNTarget, ScratchTree& Tree);