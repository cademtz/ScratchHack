#define _JSON_CPP
#include "json.h"
#include <string.h>
#include <assert.h>

jsmntok_t* Json_Find(const char* Json, jsmntok_t* Obj, const char* Name)
{
	assert(Obj->type == JSMN_OBJECT && "Expected JSON object");

	jsmntok_t* next = Obj + 1;
	for (int count = Obj->size; count > 0; --count, next = Json_Next(next))
	{
		if (!strncmp(Json + next->start, Name, Json_Strlen(next)))
			return next + 1; // Return value from key:value pair
	}
	return nullptr;
}
