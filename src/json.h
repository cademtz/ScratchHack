#pragma once
#ifndef _JSON_CPP
	#define JSMN_HEADER
#endif
#include <jsmn.h>

typedef union _json_val
{
	bool b;
	long i;
	double d;
} JsonVal;

// - String length of token
inline int Json_Strlen(const jsmntok_t* Tk) { return Tk->end - Tk->start; }

// - Gets next item in JSON tree
inline jsmntok_t* Json_Next(jsmntok_t* Pos)
{
	for (int count = 1; count > 0; --count, ++Pos)
		count += Pos->size;
	return Pos;
}

inline bool Json_IsPair(jsmntok_t* Pair) { return Pair->type == JSMN_STRING && Pair->size == 1; }
inline jsmntok_t* Json_Pair_Value(jsmntok_t* Pos) {
	return Json_IsPair(Pos) ? Pos + 1 : 0;
}


// - Finds first value in object by name
jsmntok_t* Json_Find(const char* Json, jsmntok_t* Obj, const char* Name);

// - Gets the first item in an array
inline jsmntok_t* Json_StartArray(jsmntok_t* Array) {
	return Array->type == JSMN_ARRAY ? Array + 1 : 0;
}

// - Gets the first key-value pair in an object
inline jsmntok_t* Json_StartObject(jsmntok* Obj) {
	return Obj->type == JSMN_OBJECT ? Obj + 1 : 0;
}

bool Json_GetBool(const char* Json, jsmntok_t* Val, bool* out_Bool);
bool Json_GetInt(const char* Json, jsmntok_t* Val, long* out_Int);
bool Json_GetDouble(const char* Json, jsmntok_t* Val, double* out_Double);