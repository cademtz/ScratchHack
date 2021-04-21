#include <stdio.h>
#include <urlmon.h>
#include <atlbase.h>
#include <string>
#include "json.h"

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

	/*for (size_t i = 0; i < count; ++i)
	{
		const char* type = "undef";
		switch (Tokens[i].type)
		{
		case JSMN_OBJECT:		type = "obj"; break;
		case JSMN_ARRAY:		type = "arr"; break;
		case JSMN_STRING:		type = "str"; break;
		case JSMN_PRIMITIVE:	type = "prm"; break;
		}

		printf("%s %d\t%.*s\n", type, Tokens[i].size, Tokens[i].end - Tokens[i].start, json.c_str() + Tokens[i].start);
	}*/

	jsmntok_t* targets = Json_Find(json.c_str(), Tokens, "targets");
	if (targets)
	{
		puts("Found targets!");

		jsmntok_t* item = targets + 1;
		for (int i = 0; i < targets->size; ++i, item = Json_Next(item))
		{
			jsmntok_t* name = Json_Find(json.c_str(), item, "name");
			if (name)
				printf("Target: %.*s\n", Json_Strlen(name), json.c_str() + name->start);
		}
		//printf("%.*s\n", Json_Strlen(meta), json.c_str() + meta->start);
	}

	printf("%d tokens\n", count);

	CoUninitialize();

	return 0;
}
