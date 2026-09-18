#pragma once

#include <string>
#include <vector>

namespace slc
{

struct ServerEntry
{
    std::string name;
    std::string host;
    int         port = 27015;
    std::string addr;
};

struct Settings
{
    float annonceInterval = 90.0f;
    float updateInterval  = 20.0f;
    std::vector<ServerEntry> servers;
    std::vector<std::string> chatCommands    = { "!servers", "servers" };
    std::vector<std::string> consoleCommands = { "mm_servers" };
};

extern Settings g_Settings;

bool LoadSettings();
}
