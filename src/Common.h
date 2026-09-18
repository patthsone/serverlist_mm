#pragma once

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "igameevents.h"
#include "utlvector.h"
#include "ehandle.h"
#include "vector.h"
#include <KeyValues.h>
#include <filesystem.h>

#include "include/menus.h"

#include <cstdarg>
#include <string>

extern IUtilsApi*   g_pUtils;
extern IPlayersApi* g_pPlayers;
extern IMenusApi*   g_pMenus;
extern IVEngineServer2* engine;

PLUGIN_GLOBALVARS();

namespace slc
{

void LogInfo(const char* fmt, ...);
void LogWarn(const char* fmt, ...);
void LogError(const char* fmt, ...);

void        LoadPhrases();
const char* Tr(const char* key, const char* fallback = "");

std::string Colorize(const std::string& text);
void        Say(int slot, const std::string& text);
void        SayAll(const std::string& text);
void        SayKey(int slot, const char* key, const char* fallback = "");

std::string Fmt(const char* fmt, ...);
}
