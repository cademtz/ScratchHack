#pragma once
#include "json.h"
#include "scratch/scratchtree.h"

enum EScratchInputType
{
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

bool Loader_LoadProject(const char* Json, jsmntok_t* JSNProj, ScratchTree& Tree);

bool Loader_LoadTarget(const char* Json, jsmntok_t* JSNTarget, ScratchTree& Tree);