#pragma once

#include <string>
#include <vector>

namespace slq
{

struct ServerState
{
    bool        ok = false;
    std::string name;
    std::string map;
    int         players    = 0;
    int         maxPlayers = 0;
};

void Start();
void Stop();

std::vector<ServerState> Snapshot();
}
