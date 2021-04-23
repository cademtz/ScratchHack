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

jsmntok_t* Json_GetIndex(jsmntok_t* ObjOrArr, int Index);

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

#ifdef __cplusplus
#include <string>
inline std::string Json_ToString(const char* Json, jsmntok_t* Tk) {
	return std::string(Json + Tk->start, Json_Strlen(Tk));
}

inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, jsmntok_t** out_Tk) {
	return (*out_Tk = Tk), true;
}
inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, bool* out_Bool) {
	return Json_GetBool(Json, Tk, out_Bool);
}
inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, long* out_Int) {
	return Json_GetInt(Json, Tk, out_Int);
}
inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, int* out_Int) {
	long i;
	return Json_GetInt(Json, Tk, &i) ? (*out_Int = (int)i), true : false;
}
inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, double* out_Double) {
	return Json_GetDouble(Json, Tk, out_Double);
}
inline bool Json_GetValue(const char* Json, jsmntok_t* Tk, std::string* out_Str) 
{
	*out_Str = Json_ToString(Json, Tk);
	return true;
}

inline bool Json_ParseObject(const char* Json, jsmntok_t* Obj) { return true; }

template<class T, class ...TArgs>
inline bool Json_ParseObject(const char* Json, jsmntok_t* Obj, const char* Key, T Value, TArgs... KeyValues)
{
	if (jsmntok_t* j_value = Json_Find(Json, Obj, Key))
		return Json_GetValue(Json, j_value, Value) && Json_ParseObject(Json, Obj, KeyValues...);
	return false;
}

#endif