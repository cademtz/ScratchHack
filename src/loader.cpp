#include "loader.h"
#include <string>
#include <map>
#include "scratch/scratchblocks.h"

#define LOADER_LOG(x, ...) printf(x, __VA_ARGS__)

typedef std::map<std::string, jsmntok_t*> JsnBlockMap;

ScratchBlock* Loader_LoadBlock(
	const char* Json,
	jsmntok_t* JSNBlock,
	const JsnBlockMap& Map,
	ScratchTarget& Target);

bool Loader_LoadChain(
	const char* Json,
	jsmntok_t* JSNBlock,
	const JsnBlockMap& Map,
	ScratchTarget& Target);

ScratchBlock* Loader_LoadInput(const char* Json, jsmntok_t* JSNInputArr, const JsnBlockMap& Map, ScratchTarget& Target);

bool Loader_LoadProject(const char* Json, jsmntok_t* JSNProj, ScratchTree& Tree)
{
	jsmntok_t* targets = Json_Find(Json, JSNProj, "targets");
	jsmntok_t* item;

	if (!targets || !(item = Json_StartArray(targets)))
		return false;

	for (int i = 0; i < targets->size; ++i, item = Json_Next(item))
	{
		if (!Loader_LoadTarget(Json, item, Tree))
			return false;
	}

	return true;
}

bool Loader_LoadTarget(const char* Json, jsmntok_t* JSNTarget, ScratchTree& Tree)
{
	jsmntok_t* j_blocks;
	jsmntok_t* j_pair, * j_block;
	ScratchTarget*	target;
	std::string		name, nextkey;
	JsnBlockMap		map;
	bool			isStage;
	bool			topLevel;

	if (!Json_ParseObject(Json, JSNTarget,
		"name", &name,
		"isStage", &isStage,
		"blocks", &j_blocks))
		return false;

	LOADER_LOG("Target: \"%s\" %d\n", name.c_str(), (int)isStage);

	Tree.Targets().emplace_back(name.c_str(), isStage);
	target = &Tree.Targets().back();

	// Map locations of blocks by key
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
		map[Json_ToString(Json, j_pair)] = Json_Pair_Value(j_pair);

	// Re-iterate, loads all blocks
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
	{
		j_block = Json_Pair_Value(j_pair);
		if (!Json_ParseObject(Json, j_block, "topLevel", &topLevel))
			break;

		if (topLevel)
		{
			if (!Loader_LoadChain(Json, j_block, map, *target))
			{
				Tree.Targets().pop_back();
				return false;
			}
		}
	}

	return true;
}

bool Loader_LoadChain(
	const char* Json,
	jsmntok_t* JSNBlock,
	const JsnBlockMap& Map,
	ScratchTarget& Target)
{
	ScratchBlock*	block;
	ScratchChain*	chain;
	std::string		nextkey;
	jsmntok_t*		j_block = JSNBlock;

	if (!(block = Loader_LoadBlock(Json, j_block, Map, Target)))
		return false;

	Target.Chains().emplace_back();
	chain = &Target.Chains().back();
	chain->push_back(block);

	// Loop all blocks in chain
	if (Json_ParseObject(Json, j_block, "next", &nextkey))
	{
		auto it = Map.find(nextkey);
		if (it != Map.end())
		{
			do
			{
				j_block = (*it).second;
				if (!(block = Loader_LoadBlock(Json, j_block, Map, Target)) ||
					!Json_ParseObject(Json, j_block, "next", &nextkey))
					break;

				chain->push_back(block);
				it = Map.find(nextkey);
			} while (it != Map.end());
		}
	}
	
	return true;
}

ScratchBlock* Loader_LoadBlock(
	const char* Json,
	jsmntok_t* JSNBlock,
	const JsnBlockMap& Map,
	ScratchTarget& Target)
{
	std::string op;
	jsmntok_t* j_inputs, * j_fields, * j_pair;
	ScratchBlock* input;
	ScratchInputs slots;
	
	if (!Json_ParseObject(Json, JSNBlock,
		"opcode", &op,
		"fields", &j_fields,
		"inputs", &j_inputs))
		return 0;

	LOADER_LOG("\t%s\n", op.c_str());

	j_pair = Json_StartObject(j_inputs);
	for (int i = 0; i < j_inputs->size; ++i, j_pair = Json_Next(j_pair))
	{
		if (!(input = Loader_LoadInput(Json, Json_Pair_Value(j_pair), Map, Target)))
			return 0;

		slots.SetSlot(Json_ToString(Json, j_pair).c_str(), input);
	}

	return ScratchBlock::FromOpcode(
		(EScratchOpcode)ScratchOpcode_FromString(op.c_str()), slots);
}

ScratchBlock* Loader_LoadInput(const char* Json, jsmntok_t* JSNInputArr, const JsnBlockMap& Map, ScratchTarget& Target)
{
	jsmntok_t* data, * type, * first;
	long type_val;

	if (!(data = Json_GetIndex(JSNInputArr, 1)))
		return 0;

	if (data->type == JSMN_STRING)
	{
		auto it = Map.find(Json_ToString(Json, data));
		if (it == Map.cend())
			return 0;

		if (ScratchBlock* block = Loader_LoadBlock(Json, (*it).second, Map, Target))
		{
			if (Loader_LoadChain(Json, (*it).second, Map, Target))
				return block;
		}
		return 0; // parent block or its chain failed to load
	}
	else
	{
		if (!(type = Json_GetIndex(data, 0)) ||
			!(first = Json_GetIndex(data, 1)) ||
			!Json_GetInt(Json, type, &type_val))
			return 0;

		switch (type_val)
		{
		case ScratchInputType_Number:
		case ScratchInputType_PositiveNum:
		case ScratchInputType_PositiveInt:
		case ScratchInputType_Int:
		case ScratchInputType_Angle:
		case ScratchInputType_Color:
		case ScratchInputType_String:
			return new ScratchBlock_Literal(ScratchValue(Json_ToString(Json, first).c_str()));
		default:
			return new ScratchBlock_NotImplemented(ScratchOpcode_unknown);
		}
	}

	return 0;
}
