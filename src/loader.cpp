#include "loader.h"
#include <string>

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
	jsmntok_t* name, * isStage, * blockmap, * pair;
	ScratchTarget*	target;
	ScratchBlock*	block_val;
	std::string		name_val;
	bool			isStage_val;

	if (!(name = Json_Find(Json, JSNTarget, "name")) ||
		!(isStage = Json_Find(Json, JSNTarget, "isStage")) ||
		!(blockmap = Json_Find(Json, JSNTarget, "blocks")))
		return false;

	if (!(pair = Json_StartObject(blockmap)) ||
		!Json_GetBool(Json, isStage, &isStage_val))
		return false;

	name_val = std::string(Json + name->start, Json_Strlen(name));
	printf("Target: \"%s\" %d\n", name_val.c_str(), (int)isStage_val);

	Tree.Targets().emplace_back(name_val.c_str(), isStage_val);
	target = &Tree.Targets().back();

	for (int i = 0; i < blockmap->size; ++i, pair = Json_Pair_Next(pair))
	{
		block_val = Loader_LoadBlock(Json, pair, *target);
		if (!block_val)
		{
			Tree.Targets().pop_back(); // Tree will delete all blocks on destruction
			return false;
		}
	}

	return true;
}

ScratchBlock* Loader_LoadBlock(const char* Json, jsmntok_t* JSNBlockPair, ScratchTarget& Target)
{
	jsmntok_t* key = JSNBlockPair, * block, * opcode;
	ScratchBlock* block_val;
	
	if (!(block = Json_Pair_Value(JSNBlockPair)) ||
		!(opcode = Json_Find(Json, block, "opcode")))
		return 0;

	printf("\tBlock \"%.*s\" opcode: %.*s\n",
		Json_Strlen(key), Json + key->start,
		Json_Strlen(opcode), Json + opcode->start);

	return (ScratchBlock*)-1; // Dummy value. Blocks aren't usable yet.
}
