#include "ServerList.h"

#include <string>

#include "src/Config.h"
#include "src/Menu.h"
#include "src/Query.h"

ServerList g_ServerList;
PLUGIN_EXPOSE(ServerList, g_ServerList);

CEntitySystem*     g_pEntitySystem     = nullptr;
CGlobalVars*       gpGlobals           = nullptr;
IVEngineServer2*   engine              = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;

IUtilsApi*   g_pUtils   = nullptr;
IPlayersApi* g_pPlayers = nullptr;
IMenusApi*   g_pMenus   = nullptr;

CGameEntitySystem* GameEntitySystem()
{
    return g_pUtils ? g_pUtils->GetCGameEntitySystem() : nullptr;
}

using namespace slc;

static bool   g_Initialized = false;
static CTimer* g_AnnounceTimer = nullptr;

static bool OnServersCommand(int slot, const char* content)
{
    if (slot < 0) return true;
    slm::ShowList(slot);
    return true;
}

static void InitializeOnce()
{
    if (g_Initialized) return;
    g_Initialized = true;

    LoadPhrases();
    LoadSettings();

    g_pUtils->RegCommand(g_PLID, g_Settings.consoleCommands, g_Settings.chatCommands, OnServersCommand);

    slq::Start();

    if (g_Settings.annonceInterval > 0.0f && !g_Settings.servers.empty())
    {
        g_AnnounceTimer = g_pUtils->CreateTimer(g_Settings.annonceInterval, []() -> float {
            slm::Announce();
            return g_Settings.annonceInterval;
        });
    }
}

static void OnStartupServer()
{
    g_pGameEntitySystem = GameEntitySystem();
    gpGlobals           = g_pUtils->GetCGlobalVars();
    g_pEntitySystem     = g_pUtils->GetCEntitySystem();

    InitializeOnce();
}

bool ServerList::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

    g_SMAPI->AddListener(this, this);
    return true;
}

bool ServerList::Unload(char* error, size_t maxlen)
{
    slq::Stop();

    if (g_pUtils)
    {
        if (g_AnnounceTimer) g_pUtils->RemoveTimer(g_AnnounceTimer);
        g_pUtils->ClearAllHooks(g_PLID);
    }
    g_AnnounceTimer = nullptr;
    g_Initialized   = false;

    ConVar_Unregister();
    return true;
}

void ServerList::AllPluginsLoaded()
{
    int ret = 0;

    g_pUtils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) { SetFailState("Missing Utils plugin"); return; }

    g_pPlayers = (IPlayersApi*)g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) { SetFailState("Missing Players plugin"); return; }

    g_pMenus = (IMenusApi*)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) { SetFailState("Missing Menus plugin"); return; }

    g_pUtils->StartupServer(g_PLID, OnStartupServer);

    InitializeOnce();
}

void ServerList::SetFailState(const char* error)
{
    if (g_pUtils) g_pUtils->ErrorLog("%s %s\n", GetLogTag(), error);
    else ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
    std::string cmd = "meta unload " + std::to_string(g_PLID);
    engine->ServerCommand(cmd.c_str());
}

const char* ServerList::GetLicense()     { return "Public"; }
const char* ServerList::GetVersion()     { return "1.0.0"; }
const char* ServerList::GetDate()        { return __DATE__; }
const char* ServerList::GetLogTag()      { return "[ServerList]"; }
const char* ServerList::GetAuthor()      { return "PattHs"; }
const char* ServerList::GetDescription() { return "Server list with online and one-click connect"; }
const char* ServerList::GetName()        { return "ServerList"; }
const char* ServerList::GetURL()         { return ""; }
