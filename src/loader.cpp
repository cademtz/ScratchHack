#include "loader.h"
#include "scratch/scratchblocks.h"

#define LOADER_LOG(x, ...) printf(x, __VA_ARGS__)

typedef std::map<std::string, LoaderBlock> JsnBlockMap;

bool Loader_ParseBlock(const char* Json, jsmntok_t* JSNBlock, LoaderBlock& Block);
bool Loader_ParseInput(const char* Json, jsmntok_t* JSNInput, LoaderInput& Input);
bool Loader_ParseMutation(const char* Json, jsmntok_t* JSNMut, LoaderMutation& Mut);

bool Loader_LoadChain(ScratchChain& Chain, ScratchTarget& Target, const LoaderBlock& BlockInfo, const JsnBlockMap& Map);

bool Loader_LoadBlock(
	ScratchChain& Chain,
	const LoaderBlock& BlockInfo,
	const JsnBlockMap& Map,
	ScratchTarget& Target);

ScratchMethod* Loader_LoadInput(const LoaderInput& Input, const JsnBlockMap& Map, ScratchTarget& Target);

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
	jsmntok_t* j_pair;
	ScratchTarget*	target;
	ScratchChain*	chain;
	LoaderBlock*	block;
	std::string		name;
	JsnBlockMap		map;
	bool			isStage;

	if (!Json_ParseObject(Json, JSNTarget,
		"name", &name,
		"isStage", &isStage,
		"blocks", &j_blocks))
		return false;

	LOADER_LOG("Target: \"%s\" %d\n", name.c_str(), (int)isStage);

	Tree.Targets().emplace_back(name.c_str(), isStage);
	target = &Tree.Targets().back();

	// Parse and map all blocks by key
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
	{
		block = &map[Json_ToString(Json, j_pair)];
		if (!Loader_ParseBlock(Json, Json_Pair_Value(j_pair), *block))
		{
			Tree.Targets().pop_back();
			return false;
		}
	}

	for (auto& pair : map)
	{
		if (!pair.second.topLevel)
			continue;

		target->Chains().emplace_back();
		chain = &target->Chains().back();

		if (!Loader_LoadChain(*chain, *target, pair.second, map))
		{
			Tree.Targets().pop_back();
			return false;
		}
	}

	return true;
}

bool Loader_ParseBlock(const char* Json, jsmntok_t* JSNBlock, LoaderBlock& Block)
{
	std::string op;
	jsmntok_t* j_inputs, * j_fields, * j_pair, * j_mut;
	LoaderInput* input;

	j_mut = Json_Find(Json, JSNBlock, "mutation"); // Optional
	if (!Json_ParseObject(Json, JSNBlock,
		"opcode",	&op,
		"fields",	&j_fields,
		"inputs",	&j_inputs,
		"topLevel",	&Block.topLevel,
		"next",		&Block.next))
		return false;

	Block.opcode = ScratchOpcode_FromString(op.c_str());
	if (Block.opcode == ScratchOpcode_unknown)
		return false;

	LOADER_LOG("\t%s\n", op.c_str());

	j_pair = Json_StartObject(j_inputs);
	for (int i = 0; i < j_inputs->size; ++i, j_pair = Json_Next(j_pair))
	{
		input = &Block.inputs[Json_ToString(Json, j_pair)];
		if (!Loader_ParseInput(Json, Json_Pair_Value(j_pair), *input))
			return false;
	}

	Block.hasMutation = j_mut != 0;
	if (Block.hasMutation && !Loader_ParseMutation(Json, j_mut, Block.mutation))
		return false;

	return true;
}

bool Loader_ParseInput(const char* Json, jsmntok_t* JSNInputArr, LoaderInput& Input)
{
	jsmntok_t* data, * type, * next;
	long type_val;

	if (!(data = Json_GetIndex(JSNInputArr, 1)))
		return false;

	if (data->type == JSMN_STRING)
	{
		Input.type = ScratchInputType_Block;
		Input.vals.push_back(Json_ToString(Json, data));
		return true;
	}
	else if (data->type == JSMN_ARRAY)
	{
		if (!(type = Json_GetIndex(data, 0)) ||
			!(next = Json_GetIndex(data, 1)) ||
			!Json_GetInt(Json, type, &type_val))
			return false;

		Input.type = type_val;
		for (int i = 1; i < data->size; ++i, next = Json_Next(next))
			Input.vals.push_back(Json_ToString(Json, next));

		return true;
	}

	return false;
}

bool Loader_ParseMutation(const char* Json, jsmntok_t* JSNMut, LoaderMutation& Mut)
{
	// Okay, so Scratch does this AWESOME thing, maybe it's a bug, maybe it's not:
	// All "argument..." and "warp" values are JSON strings... in JSON.
	// Why? I have no clue. The wiki doesn't seem to notice this, either.
	// https://en.scratch-wiki.info/wiki/Scratch_File_Format#Mutations

	std::string	warp, argIds;
	jsmntok_t*	j_argIds, * j_next;
	jsmn_parser	p;
	jsmntok_t*	tokens;
	int			count;
	bool		okay;

	if (!Json_ParseObject(Json, JSNMut,
		"argumentids",	&j_argIds,
		"warp",			&warp))
		return false;

	Mut.warp = !_Scratch_stricmp(warp.c_str(), "\"true\"");
	if (!Mut.warp && !_Scratch_stricmp(warp.c_str(), "\"false\""))
		return false; // Bad data, not a JSON bool

	if (!Json_StrictToString(Json, j_argIds, &argIds)) // Parse JSON escapes
		return false; // Invalid JSON string

	jsmn_init(&p);
	count = jsmn_parse(&p, argIds.c_str(), argIds.length(), 0, 0);
	if (count < 0)
		return false;
	else if (count == 0)
		return true; // No arguments to parse. Finished.

	okay = false;
	tokens = new jsmntok_t[count];

	jsmn_init(&p);
	if (jsmn_parse(&p, argIds.c_str(), argIds.length(), tokens, count) > 0 &&
		(j_next = Json_StartArray(tokens)))
	{
		okay = true;
		for (int i = 0; i < tokens->size; ++i, j_next = Json_Next(j_next))
		{
			if (j_next->type != JSMN_STRING)
			{
				okay = false;
				break;
			}

			Mut.argIds.insert(Json_ToString(Json, j_next));
		}
	}

	delete[] tokens;
	return okay;
}

bool Loader_LoadChain(ScratchChain& Chain, ScratchTarget& Target, const LoaderBlock& BlockInfo, const JsnBlockMap& Map)
{
	const LoaderBlock* block;

	if (!Loader_LoadBlock(Chain, BlockInfo, Map, Target))
		return false;

	auto it = Map.find(BlockInfo.next);
	if (it != Map.end())
	{
		do
		{
			block = &(*it).second;
			if (!Loader_LoadBlock(Chain, *block, Map, Target))
			{
				Target.Chains().pop_back();
				return false;
			}
			it = Map.find(block->next);
		} while (it != Map.end());
	}

	return true;
}

bool Loader_LoadBlock(
	ScratchChain& Chain,
	const LoaderBlock& BlockInfo,
	const JsnBlockMap& Map,
	ScratchTarget& Target)
{
	ScratchMethod* input;

	if (BlockInfo.opcode == ScratchOpcode_unknown)
		return false;

	Chain.AddOpcode((EScratchOpcode)BlockInfo.opcode);
	for (auto& l_input : BlockInfo.inputs)
	{
		if (!(input = Loader_LoadInput(l_input.second, Map, Target)))
			return false;
		Chain.AddInput(input);
	}

	return true;
}

ScratchMethod* Loader_LoadInput(const LoaderInput& Input, const JsnBlockMap& Map, ScratchTarget& Target)
{
	if (Input.type == ScratchInputType_Block)
	{
		auto it = Map.find(Input.vals[0]);
		if (it == Map.cend())
			return 0;

		ScratchChain* chain = new ScratchChain();
		if (Loader_LoadChain(*chain, Target, (*it).second, Map))
			return chain;
		delete chain;
		return 0; // Chain failed to load
	}
	else
	{
		switch (Input.type)
		{
		case ScratchInputType_Number:
		case ScratchInputType_PositiveNum:
		case ScratchInputType_PositiveInt:
		case ScratchInputType_Int:
		case ScratchInputType_Angle:
		case ScratchInputType_Color:
		case ScratchInputType_String:
			return new ScratchLiteral(Input.vals[0].c_str());
		default:
			return new Scratch_NotImplemented();
		}
	}

	return 0;
}
