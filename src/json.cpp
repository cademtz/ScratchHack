#define _JSON_CPP
#include "json.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

jsmntok_t* Json_Find(const char* Json, jsmntok_t* Obj, const char* Name)
{
	assert(Obj->type == JSMN_OBJECT && "Expected JSON object");

	jsmntok_t* next = Obj + 1;
	for (int i = 0; i < Obj->size; ++i, next = Json_Next(next))
	{
		if (!strncmp(Json + next->start, Name, Json_Strlen(next)))
			return next + 1; // Return value from key:value pair
	}
	return nullptr;
}

bool Json_GetBool(const char* Json, jsmntok_t* Val, bool* out_Bool)
{
	char c = Json[Val->start];
	if (Val->type == JSMN_PRIMITIVE && (c == 't' || c == 'f'))
	{
		*out_Bool = c == 't';
		return true;
	}
    return false;
}

bool Json_GetInt(const char* Json, jsmntok_t* Val, long* out_Int)
{
	char* endptr;
	if (Val->type == JSMN_PRIMITIVE)
	{
		*out_Int = strtol(Json + Val->start, &endptr, 10);
		return !errno && endptr == Json + Val->end;
	}
	return false;
}

bool Json_GetDouble(const char* Json, jsmntok_t* Val, double* out_Double)
{
	char* endptr;
	if (Val->type == JSMN_PRIMITIVE)
	{
		*out_Double = strtol(Json + Val->start, &endptr, 10);
		return !errno && endptr == Json + Val->end;
	}
	return false;
}
