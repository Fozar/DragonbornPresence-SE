#pragma once
#include "skse64/GameReferences.h"

Actor* GetPlayer();
const char* Cp1251ToUtf8(const char* str);
bool is_valid_utf8(const char * string);
const char* Format(const char* fmt, ...);
