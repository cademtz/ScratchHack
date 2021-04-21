#pragma once
#ifndef _JSON_CPP
	#define JSMN_HEADER
#endif
#include <jsmn.h>

// - String length of token
inline int Json_Strlen(const jsmntok_t* Tk) { return Tk->end - Tk->start; }

// - Gets next item in tree
inline jsmntok_t* Json_Next(jsmntok_t* Pos)
{
	for (int count = 1; count > 0; --count, ++Pos)
		count += Pos->size;
	return Pos;
}

// - Finds first value in object by name
jsmntok_t* Json_Find(const char* Json, jsmntok_t* Obj, const char* Name);
