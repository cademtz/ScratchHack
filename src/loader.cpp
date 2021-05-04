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
template <class TKey, class TVal, class TAnyKey>
inline TVal* Loader_Find(std::map<TKey, TVal>& Map, const TAnyKey& Key)
{
	auto it = Map.find(Key);
	return it == Map.end() ? 0 : &(*it).second;
}

CScratchLoader::CScratchLoader(const char* Json, size_t JsonLen)
	: m_json(Json), m_jsonLen(JsonLen)
{
	if (JsonLen == (size_t)-1)
		m_jsonLen = strlen(Json);
}

bool CScratchLoader::ParseProject()
{
	JsonParser p;
	jsmntok_t* targets, * item;
	std::string name;
	bool isStage;

	ResetParser();

	if (p.Parse(m_json, m_jsonLen) < 1)
		return assert(0 && "Failed to parse JSON"), false;

	m_tokens = &p.tokens[0];
	targets = Json_Find(m_json, m_tokens, "targets");

	if (!targets || !(item = Json_StartArray(targets)))
		return assert(0 && "Couldn't find targets in Scratch JSON project"), false;

	// Populate the list and sort the stage FIRST (contains global vars, must parse & load first)
	for (int i = 0; i < targets->size; ++i, item = Json_Next(item))
	{
		if (!Json_ParseObject(m_json, item,
			"name",		&name,
			"isStage",	&isStage))
			return assert(0 && "Couldn't parse Scratch JSON target"), false;

		if (isStage)
		{
			m_targets.emplace_front();
			m_target = &m_targets.front();
		}
		else
		{
			m_targets.emplace_back();
			m_target = &m_targets.back();
		}

		m_target->name = name;
		m_target->isStage = isStage;
	}

	// Re-iterate and load all other targets
	item = Json_StartArray(targets);
	for (auto it = m_targets.begin(); it != m_targets.end(); ++it, item = Json_Next(item))
	{
		m_target = &(*it);
		if (!ParseTarget(item, *m_target))
			return false;
	}

	m_parsed = true;
	return true;
}

bool CScratchLoader::LoadProject(ScratchTree* Tree)
{
	ScratchTarget* target;
	union
	{
		ScratchVar* var;
		ScratchList* var_list;
	};

	if (!m_parsed)
		return assert(0 && "Project was not parsed or failed to parse"), false;
	
	m_tree = Tree;
	for (ParsedTarget& parsed : m_targets)
	{
		m_target = &parsed;

		// Initialize variables
		for (auto& pair : parsed.varmap)
		{
			if (pair.second.isList)
			{
				var_list = new ScratchList(pair.second.name.c_str());
				for (auto& val : pair.second.vals)
					var_list->ValueList().push_back(val);
				var = var_list;
			}
			else
				var = new ScratchVar(pair.second.name.c_str(), pair.second.vals.front());
			pair.second.loaded = var;
		}

		// Populate loaded map
		for (auto& pair : parsed.map)
		{
			if (pair.second.flags & LoaderBlockFlag_TopLevel)
				parsed.loaded[&pair.second] = new ScratchChain();
		}

		// Load the blocks in to the VM
		for (auto& pair : parsed.loaded)
		{
			if (!LoadChain(*pair.second, *pair.first))
			{
				m_tree = 0;
				return false;
			}
		}
	}

	// By now, everything is safe to place in the tree
	for (ParsedTarget& parsed : m_targets)
	{
		Tree->Targets().emplace_back(parsed.name.c_str(), parsed.isStage);
		target = &Tree->Targets().back();

		for (auto& pair : parsed.varmap)
		{
			if (pair.second.isList)
				target->Lists().push_back((ScratchList*)pair.second.loaded);
			else
				target->Vars().push_back(pair.second.loaded);
		}

		for (auto& pair : parsed.loaded)
			target->Chains().push_back(pair.second);
	}

	m_targets.clear();
	return true;
}

void CScratchLoader::ResetParser()
{
	m_tree = 0;
	m_parsed = false;

	for (ParsedTarget& target : m_targets)
	{
		// Was left behind if loader failed
		for (auto& pair : target.loaded)
			delete pair.second;
		for (auto& pair : target.varmap)
			delete pair.second.loaded;
	}
	m_targets.clear();
}

bool CScratchLoader::ParseTarget(jsmntok_t* JsnTarget, ParsedTarget& Target)
{
	jsmntok_t* j_blocks, * j_vars, * j_lists;
	jsmntok_t* j_pair;
	union // One-time use
	{
		ParsedVar* var;
		ParsedBlock* block;
		ScratchChain* chain;
	};

	m_target = &Target;

	if (!Json_ParseObject(m_json, JsnTarget,
		"blocks",		&j_blocks,
		"variables",	&j_vars,
		"lists",		&j_lists))
		return assert(0 && "Scratch JSON target missing required members"), false;

	// Populate var map
	if (!ParseVars(j_vars, false, Target) ||
		!ParseVars(j_lists, true, Target))
		return false;

	// Map all keys to a ParsedBlock struct
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
		m_target->map.emplace(Json_ToString(m_json, j_pair), ParsedBlock());

	// Re-iterate, fill all block structs and references
	j_pair = Json_StartObject(j_blocks);
	for (int i = 0; i < j_blocks->size; ++i, j_pair = Json_Next(j_pair))
	{
		block = &m_target->map[Json_ToString(m_json, j_pair)];
		if (!ParseBlock(Json_Pair_Value(j_pair), *block))
			return false;
	}

	// Prepare the syntax to better fit the VM, such as stack indexes for args
	for (auto& pair : m_target->map) // Check all blocks
	{
		if (!InputSafety(pair.second) || !FixArgs(pair))
			return false;
	}

	return true;
}

bool CScratchLoader::ParseVars(jsmntok_t* JsnVarArr, bool IsListsArr, ParsedTarget& Target)
{
	jsmntok_t* j_pair, * j_arr, * j_item;
	ParsedVar*	var;
	std::string	str;
	JsonParser	p;

	j_pair = Json_StartObject(JsnVarArr);
	for (int i = 0; i < JsnVarArr->size; ++i, j_pair = Json_Next(j_pair))
	{
		var = &Target.varmap[Json_ToString(m_json, j_pair)];
		var->isList = IsListsArr;
		var->vals.emplace_back();

		j_arr = Json_Pair_Value(j_pair);
		if (!Json_ParseArray(m_json, j_arr,
			0, &var->name,
			1, &var->vals[0]))
			return assert(0 && "Bad Scratch JSON variable array"), false;

		if (IsListsArr)
		{
			str = std::move(var->vals.front());
			var->vals.clear();
			if (p.Parse(str.c_str(), str.length()) < 1) // Oh boy. I love JSON inside JSON.
				return assert(0 && "Failed to parse JSON in Scratch JSON list"), false;

			j_item = Json_StartArray(&p.tokens[0]);
			for (int j = 0; j < p.tokens[0].size; ++j, j_item = Json_Next(j_item))
				var->vals.push_back(Json_ToString(str.c_str(), j_item));
		}
	}

	return true;
}

bool CScratchLoader::ParseBlock(jsmntok_t* JsnBlock, ParsedBlock& Block)
{
	std::string op, next, parent;
	jsmntok_t* j_inputs, * j_fields, * j_pair, * j_mut;
	bool topLevel;
	union
	{
		ParsedInput* input;
		ParsedField* field;
	};

	if (JsnBlock->type == JSMN_ARRAY)
		return true; // This happens when you leave a variable getter block detached... idk why...

	j_mut = Json_Find(m_json, JsnBlock, "mutation"); // Optional
	if (!Json_ParseObject(m_json, JsnBlock,
		"opcode",	&op,
		"fields",	&j_fields,
		"inputs",	&j_inputs,
		"topLevel",	&topLevel,
		"next",		&next,
		"parent",	&parent))
		return false;

	if (!next.empty() && !(Block.next = Loader_Find(m_target->map, next)))
		return assert(0 && "Undefined reference to next block"), false; 
	if (!parent.empty() && !(Block.parent = Loader_Find(m_target->map, parent)))
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
		input = &Block.inputs[Json_ToString(m_json, j_pair)];
		if (!ParseInput(Json_Pair_Value(j_pair), *input))
			return false;
	}

	j_pair = Json_StartObject(j_fields);
	for (int i = 0; i < j_fields->size; ++i, j_pair = Json_Next(j_pair))
	{
		field = &Block.fields[Json_ToString(m_json, j_pair)];
		if (!ParseField(Json_Pair_Value(j_pair), *field))
			return false;
	}

	if ((Block.flags & LoaderBlockFlag_Mutation) &&
		!ParseMutation(j_mut, Block.mutation, Block.opcode == procedures_prototype))
		return false;

	return true;
}

bool CScratchLoader::ParseInput(jsmntok_t* JsnInputArr, ParsedInput& Input)
{
	jsmntok_t* data, * type, * next;
	long type_val;

	if (!(data = Json_GetIndex(JsnInputArr, 1)))
		return assert(0 && "Expected at least 2 items in Scratch input array"), false;

	if (data->type == JSMN_STRING)
	{
		Input.type = ScratchInputType_Block;
		Input.vals = { Json_ToString(m_json, data) };
		return true;
	}
	else if (data->type == JSMN_ARRAY)
	{
		if (!(type = Json_GetIndex(data, 0)) ||
			!(next = Json_GetIndex(data, 1)) ||
			!Json_GetInt(m_json, type, &type_val))
			return assert(0 && "Expected type int and value string for Scratch input's non-shadow value"), false;

		Input.type = (EScratchInputType)type_val;
		for (int i = 1; i < data->size; ++i, next = Json_Next(next))
			Input.vals.push_back(Json_ToString(m_json, next));

		return true;
	}
	else if (Json_IsNull(m_json, data))
	{
		Input.type = ScratchInputType_Null;
		Input.vals = { "0" };
		LOADER_LOG("Warning: Got JSON null for input value\n");
		return true;
	}

	return assert(0 && "Unexpected type for Scratch input"), false;
}

bool CScratchLoader::ParseField(jsmntok_t* JsnField, ParsedField& Field)
{
	jsmntok_t* j_next = Json_StartArray(JsnField);
	std::string* next;

	for (int i = 0; i < JsnField->size; ++i, j_next = Json_Next(j_next))
	{
		Field.vals.emplace_back();
		next = &Field.vals.back();
		if (!Json_GetValue(m_json, j_next, next))
		{
			Field.vals.pop_back();
			return false;
		}
	}
	return true;
}

bool CScratchLoader::ParseMutation(jsmntok_t* JsnMut, ParsedMutation& Mut, bool GetArgs)
{
	// Okay, so Scratch does this AWESOME thing, maybe it's a bug, maybe it's not:
	// All "argument..." and "warp" values are JSON strings... in JSON.
	// Why? I have no clue. The wiki doesn't seem to notice this, either.
	// https://en.scratch-wiki.info/wiki/Scratch_File_Format#Mutations

	std::string	warp, jsnstr;
	std::vector<ParsedArg> args;
	jsmntok_t*	j_next;
	JsonParser	p;

	if (!Json_ParseObject(m_json, JsnMut,
		"warp", &warp,
		"proccode", &Mut.proccode))
		return false;

	Mut.warp = warp == "true";
	if (!Mut.warp && warp != "false")
		return false; // Bad data, not a JSON bool

	const char* dump = m_json + JsnMut->start;

	if (GetArgs)
	{
		// Parse argumentids array
		if (!Json_ParseObject(m_json, JsnMut, "argumentids", &jsnstr) ||
			p.Parse(jsnstr.c_str(), jsnstr.length()) < 1 ||
			!(j_next = Json_StartArray(&p.tokens[0])))
			return false; // Invalid JSON string

		// Get all IDs
		for (int i = 0; i < p.tokens[0].size; ++i, j_next = Json_Next(j_next))
		{
			if (j_next->type != JSMN_STRING)
				return assert(0 && "Name array contains non-string"), false;
			args.push_back({ Json_ToString(jsnstr.c_str(), j_next) });
		}

		// Parse argumentdefaults array
		if (!Json_ParseObject(m_json, JsnMut, "argumentdefaults", &jsnstr) ||
			p.Parse(jsnstr.c_str(), jsnstr.length()) < 1 ||
			!(j_next = Json_StartArray(&p.tokens[0])))
			return false; // Invalid JSON string

		if (p.tokens[0].size < args.size()) // Extra defaults are left over if mutation args get removed in Scratch editor
			return assert(0 && "Mismatched mutation arg arrays"), false;

		// Get all defaults
		for (int i = 0; i < args.size(); ++i, j_next = Json_Next(j_next))
		{
			if (j_next->type != JSMN_STRING)
				return assert(0 && "Name array contains non-string"), false;
			args[i].deefault = Json_ToString(jsnstr.c_str(), j_next);
		}

		// Parse argumentnames array
		if (!Json_ParseObject(m_json, JsnMut, "argumentnames", &jsnstr) ||
			p.Parse(jsnstr.c_str(), jsnstr.length()) < 1 ||
			!(j_next = Json_StartArray(&p.tokens[0])))
			return false; // Invalid JSON string

		if (p.tokens[0].size != args.size())
			return assert(0 && "Mismatched mutation arg arrays"), false;

		// Populate map with list of names' corresponding Arg IDs
		for (int i = 0; i < p.tokens[0].size; ++i, j_next = Json_Next(j_next))
			Mut.argmap[Json_ToString(jsnstr.c_str(), j_next)] = std::move(args[i]);
	}
	return true;
}

bool CScratchLoader::InputSafety(ParsedBlock& Block)
{
	ParsedBlock* block;
	for (auto& pair : Block.inputs)
	{
		if (pair.second.type != ScratchInputType_Block &&
			pair.second.type != ScratchInputType_Inline)
			continue;

		auto it = m_target->map.find(pair.second.vals[0]);
		if (it == m_target->map.end())
			return assert(0 && "Bad reference to block in input"), false;

		if (pair.second.type == ScratchInputType_Block)
		{
			block = &(*it).second;

			if (block->flags & LoaderBlockFlag_Inline)
				pair.second.type = ScratchInputType_Inline;
		}
	}
	return true;
}

bool CScratchLoader::FixArgs(JsnBlockMap::value_type& Pair)
{
	const ParsedBlock* proto;
	const ParsedArg* arg;
	ParsedBlock* block = &Pair.second;
	ParsedInput* input;

	if (block->opcode != procedures_call &&
		block->opcode != argument_reporter_boolean &&
		block->opcode != argument_reporter_string_number)
		return true;

	if (block->opcode != procedures_call && !block->parent)
		return true; // Detached arg reporters exist when you edit a custom block and remove arg(s)

	proto = GetProto(*block);
	if (!proto)
		return assert(0 && "Unknown procedure prototype"), false;

	if (block->opcode == procedures_call)
	{
		// Rename inputs from arg IDs to arg names
		for (auto& arg_pair : proto->mutation.argmap)
		{
			auto it = block->inputs.find(arg_pair.second.id);
			if (it == block->inputs.end()) // Procedure call is missing some args
			{
				input = &block->inputs[arg_pair.first];
				input->type = ScratchInputType_Null; // Insert arg with temp value (will be resolved to default)
			}
			else
			{
				block->inputs.emplace(arg_pair.first, std::move((*it).second));
				block->inputs.erase(it); // Associative container inserts don't invalidate iterators
			}
		}

		for (auto it = block->inputs.begin(); it != block->inputs.end();)
		{
			arg = Loader_Find(proto->mutation.argmap, (*it).first);
			if (!arg)
			{
				LOADER_LOG("Warning: Method \"%s\" has unused arg \"%s\"\n", Pair.first.c_str(), (*it).first.c_str());
				it = block->inputs.erase(it);
				continue;
			}

			if ((*it).second.type == ScratchInputType_Null)
			{
				// Use default arg, specified by prototype
				input = &(*it).second;
				input->type = ScratchInputType_String;
				input->vals = { arg->deefault };
			}
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

		input = &block->inputs["ARG"];
		input->type = ScratchInputType_Arg;
		input->vals = { std::to_string(index) };
	}
	
	return true;
}

bool CScratchLoader::LoadChain(ScratchChain& Chain, const ParsedBlock& BlockInfo)
{
	if (!LoadBlock(Chain, BlockInfo))
		return false;

	for (const ParsedBlock* block = BlockInfo.next; block; block = block->next)
	{
		if (!LoadBlock(Chain, *block))
			return false;
	}

	return true;
}

bool CScratchLoader::LoadBlock(ScratchChain& Chain, const ParsedBlock& BlockInfo)
{
	const ParsedBlock* proto = 0;
	union
	{
		const ParsedBlock* l_callee = 0;
		const ParsedField* field;
		ScratchChain* callee;
		ScratchVar* var;
	}; // blame scratch mutations for this finicky code

	if (BlockInfo.opcode == ScratchOpcode_unknown)
		return false;

	// Inline certain inputs that don't need branching logic
	for (auto it = BlockInfo.inputs.rbegin(); it != BlockInfo.inputs.rend(); ++it)
	{
		if ((*it).second.type != ScratchInputType_Block)
		{
			if (!InlineInput((*it).second, Chain))
				return false;
		}
	}

	Chain.AddOpcode((EScratchOpcode)BlockInfo.opcode);

	// Refer to previous comment
	if (BlockInfo.opcode == procedures_call)
	{
		proto = GetProto(BlockInfo);
		if (!proto)
			return false;

		// Get the procedure being called
		if (!(l_callee = proto->parent) ||
			l_callee->opcode != procedures_definition)
			return false;

		callee = *Loader_Find(m_target->loaded, (ParsedBlock*)l_callee);
		Chain.AddInput(callee);
	}

	// Block wants a variable accessor
	if (field = Loader_Find(BlockInfo.fields, "VARIABLE"))
	{
		if (field->vals.size() < 2)
			return assert(0 && "VARIABLE field should have two elements in array: Name and ID"), false;

		var = GetVar(field->vals[1]);
		if (!var || var->IsList())
			return assert(0 && "Loader expected Scratch var but was given a Scratch list"), false;

		Chain.AddInput(new ScratchSetVar(var));
	}
	else if (field = Loader_Find(BlockInfo.fields, "LIST"))
	{
		if (field->vals.size() < 2)
			return assert(0 && "LIST field should have two elements in array: Name and ID"), false;

		var = GetVar(field->vals[1]);
		if (!var || !var->IsList())
			return assert(0 && "Loader expected Scratch list but was given a Scratch var"), false;

		Chain.AddInput(new ScratchSetList((ScratchList*)var));
	}

	// Load ScratchInputType_Block as an input for this opcode
	for (auto& l_input : BlockInfo.inputs)
	{
		if (l_input.second.type == ScratchInputType_Block)
		{
			ScratchChain* chain = new ScratchChain();
			if (!LoadChain(*chain, *Loader_Find(m_target->map, l_input.second.vals[0])))
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

const ParsedBlock* CScratchLoader::GetProto(const ParsedBlock& BlockInfo)
{
	union
	{
		const ParsedInput* custom;
		const ParsedBlock* block;
	};

	if (BlockInfo.opcode == procedures_definition)
	{
		if (custom = Loader_Find(BlockInfo.inputs, "custom_block"))
			return Loader_Find(m_target->map, custom->vals[0]);
		return assert(0 && "Definition missing 'custom_block' input"), nullptr;
	}
	else if (BlockInfo.opcode == procedures_call)
	{
		// Search by name (proccode)
		// Proto names aren't unique. Undefined behavior if there are name conflicts.
		for (auto& pair : m_target->map)
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
		// Get the top block in the chain (Should always be a definition block)
		// TODO: Do I check if this gets used outside a proc? Probably should...
		for (block = BlockInfo.parent; block && block->parent; block = block->parent);

		if (block && block->opcode == procedures_definition)
			return GetProto(*block);
	}

	return 0;
}

ScratchVar* CScratchLoader::GetVar(const std::string& Name)
{
	ParsedVar* var = 0;
	for (auto& target : m_targets)
	{
		if (var = Loader_Find(target.varmap, Name))
			return var->loaded;
	}

	assert(var && "Undefined reference to a Scratch variable/list in project");
	return nullptr;
}

bool CScratchLoader::InlineInput(const ParsedInput& Input, ScratchChain& Chain)
{
	if (Input.type == ScratchInputType_Inline)
		return LoadBlock(Chain, *Loader_Find(m_target->map, Input.vals[0]));
	else if (Input.type != ScratchInputType_Block)
	{
		ScratchMethod* input = 0;

		switch (Input.type)
		{
		case ScratchInputType_Arg:
			input = new ScratchArg(atoi(Input.vals[0].c_str()));
			break;
		case ScratchInputType_Null:
			input = new ScratchLiteral(0); break;
		case ScratchInputType_Number:
		case ScratchInputType_PositiveNum:
		case ScratchInputType_PositiveInt:
		case ScratchInputType_Int:
		case ScratchInputType_Angle:
		case ScratchInputType_Color:
		case ScratchInputType_String:
			input = new ScratchLiteral(Input.vals[0].c_str()); break;
		case ScratchInputType_Variable:
		{
			ScratchVar* var = GetVar(Input.vals[1]);
			if (!var || var->IsList())
				return assert(0 && "Loader expected Scratch var but was given a Scratch list"), false;

			input = new ScratchPushVar(var);
			break;
		}
		case ScratchInputType_List:
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
