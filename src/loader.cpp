#include "loader.h"
#include "scratch/scratchblocks.h"
#include <assert.h>

#define LOADER_LOG(x, ...) printf(x, __VA_ARGS__)

template <class TKey, class TVal, class TAnyKey>
inline const TVal* Loader_Find(const std::map<TKey, TVal>& Map, const TAnyKey& Key)
{
	auto it = Map.find(Key);
	return it == Map.end() ? 0 : &(*it).second;
}

bool Loader_ParseBlock(const char* Json, jsmntok_t* JSNBlock, LoaderBlock& Block, const JsnBlockMap& Map);
bool Loader_ParseInput(const char* Json, jsmntok_t* JSNInput, LoaderInput& Input);
bool Loader_ParseField(const char* Json, jsmntok_t* JSNField, LoaderField& Field);
bool Loader_ParseMutation(const char* Json, jsmntok_t* JSNMut, LoaderMutation& Mut, bool GetArgs);
bool Loader_InputSafety(LoaderBlock& Block, JsnBlockMap& Map, ScratchTarget& Target);
bool Loader_FixArgs(JsnBlockMap::value_type& Pair, LoaderState& Loader);

bool Loader_LoadChain(ScratchChain& Chain, ScratchTarget& Target, const LoaderBlock& BlockInfo, LoaderState& Loader);

bool Loader_LoadBlock(
	ScratchChain& Chain,
	const LoaderBlock& BlockInfo,
	LoaderState& Loader,
	ScratchTarget& Target);

const LoaderBlock* Loader_GetProto(const LoaderBlock& BlockInfo, LoaderState& Loader);

bool Loader_InlineInput(const LoaderInput& Input, ScratchChain& Chain, LoaderState& Loader, ScratchTarget& Target);

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
	jsmntok_t*		j_blocks, *j_pair;
	union // One-time use
	{
		LoaderBlock*	block;
		ScratchChain*	chain;
		bool			isStage;
	};
	ScratchTarget*	target;
	std::string		name;
	JsnBlockMap		map;
	ScratchBlockMap loaded;
	LoaderState		state = { loaded, map };

	if (!Json_ParseObject(Json, JSNTarget,
		"name", &name,
		"isStage", &isStage,
		"blocks", &j_blocks))
		return false;

	LOADER_LOG("Target: \"%s\" %d\n", name.c_str(), (int)isStage);

	Tree.Targets().emplace_back(name.c_str(), isStage);
	target = &Tree.Targets().back();

	// Map all keys to a LoaderBlock struct
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
		map.emplace(Json_ToString(Json, j_pair), LoaderBlock());

	// Re-iterate, fill all block structs and references
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
	{
		block = &map[Json_ToString(Json, j_pair)];
		if (!Loader_ParseBlock(Json, Json_Pair_Value(j_pair), *block, map))
			return false;
	}

	// This scratch VM uses stack index for arguments
	// So some syntax will be edited to use an index instead of name
	for (auto& pair : map) // Check all blocks
	{
		if (!Loader_InputSafety(pair.second, map, *target) ||
			!Loader_FixArgs(pair, state))
			return false;
	}

	// Load the blocks in to the VM
	for (auto& pair : map)
	{
		// Not top level or already loaded
		if (!(pair.second.flags & LoaderBlockFlag_TopLevel) || loaded.find(&pair.second) != loaded.end())
			continue;

		target->Chains().emplace_back();
		chain = &target->Chains().back();

		if (!Loader_LoadChain(*chain, *target, pair.second, state))
		{
			target->Chains().pop_back();
			return false;
		}
	}

	return true;
}

bool Loader_ParseBlock(const char* Json, jsmntok_t* JSNBlock, LoaderBlock& Block, const JsnBlockMap& Map)
{
	std::string op, next, parent;
	jsmntok_t* j_inputs, * j_fields, * j_pair, * j_mut;
	bool topLevel;
	union
	{
		LoaderInput* input;
		LoaderField* field;
	};

	j_mut = Json_Find(Json, JSNBlock, "mutation"); // Optional
	if (!Json_ParseObject(Json, JSNBlock,
		"opcode",	&op,
		"fields",	&j_fields,
		"inputs",	&j_inputs,
		"topLevel",	&topLevel,
		"next",		&next,
		"parent",	&parent))
		return false;

	if (!next.empty() && !(Block.next = Loader_Find(Map, next)))
		return assert(0 && "Undefined reference to next block"), false; 
	if (!parent.empty() && !(Block.parent = Loader_Find(Map, parent)))
		return assert(0 && "Undefined reference to parent block"), false;

	Block.opcode = ScratchOpcode_FromString(op.c_str());
	if (Block.opcode == ScratchOpcode_unknown)
		return assert(0 && "Scratch opcode string unknown"), false;

	LOADER_LOG("\t%s\n", op.c_str());

	if (j_mut)
		Block.flags |= LoaderBlockFlag_Mutation;
	if (topLevel)
		Block.flags |= LoaderBlockFlag_TopLevel;
	if (!Block.next && ScratchOpcode_HasReturn(Block.opcode))
		Block.flags |= LoaderBlockFlag_Inline;

	j_pair = Json_StartObject(j_inputs);
	for (int i = 0; i < j_inputs->size; ++i, j_pair = Json_Next(j_pair))
	{
		input = &Block.inputs[Json_ToString(Json, j_pair)];
		if (!Loader_ParseInput(Json, Json_Pair_Value(j_pair), *input))
			return false;
	}

	j_pair = Json_StartObject(j_fields);
	for (int i = 0; i < j_fields->size; ++i, j_pair = Json_Next(j_pair))
	{
		field = &Block.fields[Json_ToString(Json, j_pair)];
		if (!Loader_ParseField(Json, Json_Pair_Value(j_pair), *field))
			return false;
	}

	if ((Block.flags & LoaderBlockFlag_Mutation) &&
		!Loader_ParseMutation(Json, j_mut, Block.mutation, Block.opcode == procedures_prototype))
		return false;

	return true;
}

bool Loader_ParseInput(const char* Json, jsmntok_t* JSNInputArr, LoaderInput& Input)
{
	jsmntok_t* data, * type, * next;
	long type_val;

	if (!(data = Json_GetIndex(JSNInputArr, 1)))
		return assert(0 && "Expected at least 2 items in Scratch input array"), false;

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
			return assert(0 && "Expected type int and value string for Scratch input's non-shadow value"), false;

		Input.type = type_val;
		for (int i = 1; i < data->size; ++i, next = Json_Next(next))
			Input.vals.push_back(Json_ToString(Json, next));

		return true;
	}
	else if (Json_IsNull(Json, data))
	{
		LOADER_LOG("Warning: Got JSON null for input value. Ignoring input.\n");
		return true;
	}

	return assert(0 && "Unexpected type for Scratch input"), false;
}

bool Loader_ParseField(const char* Json, jsmntok_t* JSNField, LoaderField& Field)
{
	jsmntok_t* j_next = Json_StartArray(JSNField);
	std::string* next;

	for (int i = 0; i < JSNField->size; ++i, j_next = Json_Next(j_next))
	{
		Field.vals.emplace_back();
		next = &Field.vals.back();
		if (!Json_GetValue(Json, j_next, next))
		{
			Field.vals.pop_back();
			return false;
		}
	}
	return true;
}

bool Loader_ParseMutation(const char* Json, jsmntok_t* JSNMut, LoaderMutation& Mut, bool GetArgs)
{
	// Okay, so Scratch does this AWESOME thing, maybe it's a bug, maybe it's not:
	// All "argument..." and "warp" values are JSON strings... in JSON.
	// Why? I have no clue. The wiki doesn't seem to notice this, either.
	// https://en.scratch-wiki.info/wiki/Scratch_File_Format#Mutations

	std::string	warp, jsnstr;
	std::vector<std::string> names;
	jsmntok_t*	j_next;
	JsonParser	p;

	if (!Json_ParseObject(Json, JSNMut,
		"warp", &warp,
		"proccode", &Mut.proccode))
		return false;

	Mut.warp = warp == "true";
	if (!Mut.warp && warp != "false")
		return false; // Bad data, not a JSON bool

	if (GetArgs)
	{
		// Parse argumentnames array
		if (!Json_ParseObject(Json, JSNMut, "argumentnames", &jsnstr) ||
			p.Parse(jsnstr.c_str(), jsnstr.length()) < 1 ||
			!(j_next = Json_StartArray(&p.tokens[0])))
			return false; // Invalid JSON string

		// Get all names
		for (int i = 0; i < p.tokens[0].size; ++i, j_next = Json_Next(j_next))
		{
			if (j_next->type != JSMN_STRING)
				return assert(0 && "Name array contains non-string"), false;
			names.push_back(Json_ToString(jsnstr.c_str(), j_next));
		}

		// Parse argumentids array
		if (!Json_ParseObject(Json, JSNMut, "argumentids", &jsnstr) ||
			p.Parse(jsnstr.c_str(), jsnstr.length()) < 1 ||
			!(j_next = Json_StartArray(&p.tokens[0])))
			return false; // Invalid JSON string

		if (p.tokens[0].size != names.size())
			return assert(0 && "Mismatched Name and ID arrays"), false;

		// Populate map with list of names' corresponding Arg IDs
		for (int i = 0; i < p.tokens[0].size; ++i, j_next = Json_Next(j_next))
			Mut.argmap[names[i]] = Json_ToString(jsnstr.c_str(), j_next);
	}
	return true;
}

bool Loader_InputSafety(LoaderBlock& Block, JsnBlockMap& Map, ScratchTarget& Target)
{
	LoaderBlock* block;
	for (auto& pair : Block.inputs)
	{
		if (pair.second.type == ScratchInputType_Block)
		{
			auto it = Map.find(pair.second.vals[0]);
			if (it == Map.end())
				return assert(0 && "Bad reference to block in input"), false;
			block = &(*it).second;

			if (block->flags & LoaderBlockFlag_Inline)
				pair.second.type = ScratchInputType_Inline;
		}
	}
	return true;
}

bool Loader_FixArgs(JsnBlockMap::value_type& Pair, LoaderState& Loader)
{
	const LoaderBlock* proto;
	LoaderBlock* block = &Pair.second;
	LoaderInput* arg;

	if (block->opcode != procedures_call &&
		block->opcode != argument_reporter_boolean &&
		block->opcode != argument_reporter_string_number)
		return true;

	if (block->opcode != procedures_call && !block->parent)
		return true; // Detached arg reporters exist when you edit a custom block and remove arg(s)

	proto = Loader_GetProto(*block, Loader);
	if (!proto)
		return assert(0 && "Unknown procedure prototype"), false;

	if (block->opcode == procedures_call)
	{
		for (auto& arg_pair : proto->mutation.argmap)
		{
			auto it = block->inputs.find(arg_pair.second);
			if (it == block->inputs.end())
				return assert(0 && "Procedure call is missing some args"), false;

			block->inputs.emplace(arg_pair.first, std::move((*it).second));
			block->inputs.erase(it); // Associative container inserts don't invalidate iterators
		}

		for (auto it = block->inputs.begin(); it != block->inputs.end();)
		{
			if (!Loader_Find(proto->mutation.argmap, (*it).first))
			{
				LOADER_LOG("Warning: Method \"%s\" has unused arg \"%s\"\n", Pair.first.c_str(), (*it).first.c_str());
				it = block->inputs.erase(it);
			}
			else
				++it;
		}
	}
	else
	{
		// Index in alphabetical order.
		int index = -1;
		for (auto& pair : proto->mutation.argmap)
		{
			if (pair.first == block->fields["VALUE"].vals[0])
			{
				index = -index - 1;
				break;
			}
			--index;
		}

		if (index < 0)
			return assert(0 && "Using mismatched args, couldn't make index"), false;

		arg = &block->inputs["ARG"];
		arg->type = ScratchInputType_Arg;
		arg->vals = { std::to_string(index) };
	}
	
	return true;
}

bool Loader_LoadChain(ScratchChain& Chain, ScratchTarget& Target, const LoaderBlock& BlockInfo, LoaderState& Loader)
{
	Loader.loaded[&BlockInfo] = &Chain;
	if (!Loader_LoadBlock(Chain, BlockInfo, Loader, Target))
		return false;

	for (const LoaderBlock* block = BlockInfo.next; block; block = block->next)
	{
		if (!Loader_LoadBlock(Chain, *block, Loader, Target))
			return false;
	}

	return true;
}

bool Loader_LoadBlock(
	ScratchChain& Chain,
	const LoaderBlock& BlockInfo,
	LoaderState& Loader,
	ScratchTarget& Target)
{
	const LoaderBlock* l_callee = 0;
	const LoaderBlock* proto = 0;
	union
	{
		ScratchMethod* const* pCallee = 0;
		const LoaderBlock* block;
		ScratchMethod* callee;
		ScratchMethod* input;
		ScratchChain* chain;
	}; // blame scratch mutations for this finicky code

	if (BlockInfo.opcode == ScratchOpcode_unknown)
		return false;
	else if (BlockInfo.opcode == argument_reporter_boolean ||
		BlockInfo.opcode == argument_reporter_string_number)
		printf("");

	// Inline certain inputs that don't need branching logic
	for (auto it = BlockInfo.inputs.rbegin(); it != BlockInfo.inputs.rend(); ++it)
	{
		if ((*it).second.type != ScratchInputType_Block)
		{
			if (!Loader_InlineInput((*it).second, Chain, Loader, Target))
				return false;
		}
	}

	Chain.AddOpcode((EScratchOpcode)BlockInfo.opcode);

	// Refer to previous comment
	if (BlockInfo.opcode == procedures_call)
	{
		proto = Loader_GetProto(BlockInfo, Loader);
		if (!proto)
			return false;

		// Get the procedure being called
		if (!(l_callee = proto->parent) ||
			l_callee->opcode != procedures_definition)
			return false;

		if (pCallee = Loader_Find(Loader.loaded, l_callee))
			callee = *pCallee;
		else // Not yet loaded. Load it ourselves.
		{
			// TODO: Have Loader_LoadTarget populate map with blank chains to avoid this code
			Target.Chains().emplace_back();
			chain = &Target.Chains().back();
			if (!Loader_LoadChain(*chain, Target, *l_callee, Loader))
			{
				Target.Chains().pop_back();
				return false;
			}

			callee = chain;
		}

		Chain.AddInput(callee);
	}

	// Load ScratchInputType_Block as an input for this opcode
	for (auto& l_input : BlockInfo.inputs)
	{
		if (l_input.second.type == ScratchInputType_Block)
		{
			ScratchChain* chain = new ScratchChain();
			if (!Loader_LoadChain(*chain, Target, *Loader_Find(Loader.map, l_input.second.vals[0]), Loader))
			{
				delete chain;
				return false;
			}
			Chain.AddInput(chain);
		}
	}

	// Give the call opcode a helper to pop args not known at runtime
	if (BlockInfo.opcode == procedures_call)
		Chain.AddInput(new ScratchPop(proto->mutation.argmap.size()));

	return true;
}

const LoaderBlock* Loader_GetProto(const LoaderBlock& BlockInfo, LoaderState& Loader)
{
	union
	{
		const LoaderInput* custom;
		const LoaderBlock* block;
	};

	if (BlockInfo.opcode == procedures_definition)
	{
		if (custom = Loader_Find(BlockInfo.inputs, "custom_block"))
			return Loader_Find(Loader.map, custom->vals[0]);
		return assert(0 && "Definition missing 'custom_block' input"), nullptr;
	}
	else if (BlockInfo.opcode == procedures_call)
	{
		// Search by name (proccode)
		// Proto names aren't unique. Undefined behavior if there are name conflicts.
		for (auto& pair : Loader.map)
		{
			block = &pair.second;
			if (block->opcode == procedures_prototype &&
				block->mutation.proccode == BlockInfo.mutation.proccode)
				return block;
		}
	}
	else if (BlockInfo.opcode == argument_reporter_boolean ||
		BlockInfo.opcode == argument_reporter_string_number)
	{
		for (block = BlockInfo.parent; block && block->parent; block = block->parent);

		if (block && block->opcode == procedures_definition)
			return Loader_GetProto(*block, Loader);
	}

	return 0;
}

bool Loader_InlineInput(const LoaderInput& Input, ScratchChain& Chain, LoaderState& Loader, ScratchTarget& Target)
{
	if (Input.type == ScratchInputType_Inline)
		return Loader_LoadBlock(Chain, *Loader_Find(Loader.map, Input.vals[0]), Loader, Target);
	else if (Input.type != ScratchInputType_Block)
	{
		ScratchMethod* input = 0;

		switch (Input.type)
		{
		case ScratchInputType_Arg:
			input = new ScratchArg(atoi(Input.vals[0].c_str()));
			break;
		case ScratchInputType_Number:
		case ScratchInputType_PositiveNum:
		case ScratchInputType_PositiveInt:
		case ScratchInputType_Int:
		case ScratchInputType_Angle:
		case ScratchInputType_Color:
		case ScratchInputType_String:
			input = new ScratchLiteral(Input.vals[0].c_str()); break;
		default:
			input = new Scratch_NotImplemented();
		}

		assert(input);
		Chain.AddOpcode(ScratchOpcode_push);
		Chain.AddInput(input);
		return true;
	}

	return assert(0 && "Can't inline ScratchInputType_Block"), false;
}
