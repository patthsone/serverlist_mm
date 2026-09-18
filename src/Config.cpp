#include "Config.h"

#include <algorithm>

#include "A2S.h"
#include "Common.h"

namespace slc
{

Settings g_Settings;

static const char* kConfigPath = "addons/configs/serverlist.cfg";

bool LoadSettings()
{
    g_Settings = Settings();

    KeyValues* kv = new KeyValues("cfg");
    if (!kv->LoadFromFile(g_pFullFileSystem, kConfigPath))
    {
        LogError("Failed to load %s", kConfigPath);
        delete kv;
        return false;
    }

    g_Settings.annonceInterval = kv->GetFloat("annonce_interval", 90.0f);
    g_Settings.updateInterval  = std::max(5.0f, kv->GetFloat("update_interval", 20.0f));
    if (g_Settings.annonceInterval > 0.0f && g_Settings.annonceInterval < 10.0f) g_Settings.annonceInterval = 10.0f;

    if (KeyValues* servers = kv->FindKey("servers", false))
    {
        for (KeyValues* sub = servers->GetFirstTrueSubKey(); sub; sub = sub->GetNextTrueSubKey())
        {
            ServerEntry e;
            e.name = sub->GetName();
            const char* ip = sub->GetString("ip", "");

            if (!a2s::ParseAddress(ip, e.host, e.port))
            {
                LogWarn("Server \"%s\": bad ip \"%s\" (expected host:port), skipped", e.name.c_str(), ip);
                continue;
            }
            e.addr = e.host + ":" + std::to_string(e.port);
            g_Settings.servers.push_back(std::move(e));
        }
    }

    delete kv;
    LogInfo("Config loaded: %d server(s), update every %.0fs, advert every %.0fs", (int)g_Settings.servers.size(),
            g_Settings.updateInterval, g_Settings.annonceInterval);
    return true;
}
}
