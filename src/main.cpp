#include <stdio.h>
#include <urlmon.h>
#include <atlbase.h>
#include <string>
#include "json.h"
#include "loader.h";

#pragma comment(lib, "Urlmon.lib")

jsmntok_t Tokens[200'000];

int main()
{
	char buf[128];
	ULONG len;
	HRESULT err;
	int count;

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

	//printf("Got:\n%s\n", json.c_str());

	stream.Release();

	jsmn_parser p;
	jsmn_init(&p);

	count = jsmn_parse(&p, json.c_str(), json.size(), Tokens, sizeof(Tokens) / sizeof(Tokens[0]));

	printf("%d tokens\n", count);

	ScratchTree tree;
	printf("Loader status: %d\n", (int)Loader_LoadProject(json.c_str(), Tokens, tree));

	CoUninitialize();

	return 0;
}
