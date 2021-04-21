#pragma once
#include "json.h"
#include "scratch/scratchtree.h"

bool Loader_LoadProject(const char* Json, jsmntok_t* JSNProj, ScratchTree& Tree);

bool Loader_LoadTarget(const char* Json, jsmntok_t* JSNTarget, ScratchTree& Tree);

ScratchBlock* Loader_LoadBlock(const char* Json, jsmntok_t* JSNBlockPair, ScratchTarget& Target);