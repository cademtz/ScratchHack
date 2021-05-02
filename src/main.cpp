#include <stdio.h>
#include <urlmon.h>
#include <atlbase.h>
#include <string>
#include "json.h"
#include "loader.h"
#include "scratch/scratchopcodes.h"
#include "scratch/scratchblocks.h"

#pragma comment(lib, "Urlmon.lib")

int main()
{
	char buf[128];
	ULONG len;
	HRESULT err;
	int count;
	jsmntok_t* tokens;

	if (!SUCCEEDED(err = CoInitialize(0)))
		return printf("Error %d\n", (int)err), -1;

	CComPtr<IStream> stream;
	if (!SUCCEEDED(err = URLOpenBlockingStream(0, TEXT("https://projects.scratch.mit.edu/518709868"), &stream, 0, 0)))
		return printf("Error %d\n", (int)err), -1;

	std::string json;

	do
	{
		err = stream.p->Read(buf, sizeof(buf), &len);
		if (len > 0)
			json.append(buf, len);
	} while (SUCCEEDED(err) && err != S_FALSE);

	stream.Release();

	jsmn_parser p;
	jsmn_init(&p);

	count = jsmn_parse(&p, json.c_str(), json.size(), 0, 0);
	printf("%d tokens\n", count);

	if (count < 1)
		return -1;

	tokens = new jsmntok_t[count];
	jsmn_init(&p);
	if (jsmn_parse(&p, json.c_str(), json.size(), tokens, count) < 1)
		return printf("Failed to parse JSON, likely invalid.\n"), -1;

	ScratchTree tree;
	CScratchLoader loader(json.c_str(), json.length());
	printf("Parser status: %d\n", (int)loader.ParseProject());
	printf("Loader status: %d\n", (int)loader.LoadProject(&tree));

	ScratchState state;
	for (auto& target : tree.Targets())
	{
		printf("Target \"%s\":\n", target.Name().c_str());
		if (target.Name() != "main")
			continue;

		for (ScratchChain* chain : target.Chains())
		{
			if (chain->Code().front() != event_whenflagclicked)
				continue;

			printf("\tExecuting chain %s:\n", ScratchOpcode_ToString((int)chain->Code().front()));
			chain->Exec(state);
			putchar('\n');
		}
		putchar('\n');
	}

	CoUninitialize();

	return 0;
}
