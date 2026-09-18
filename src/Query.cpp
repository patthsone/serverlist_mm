#include "Query.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "A2S.h"
#include "Config.h"

namespace slq
{

static std::thread              g_Thread;
static std::mutex               g_Mutex;
static std::condition_variable  g_Wake;
static bool                     g_Stop = false;
static std::vector<ServerState> g_State;

static void Worker(std::vector<a2s::Target> targets, int intervalMs)
{
    for (;;)
    {
        std::vector<a2s::Info> infos = a2s::QueryAll(targets, 1500);

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_State.assign(targets.size(), ServerState());
            for (size_t i = 0; i < infos.size() && i < g_State.size(); ++i)
            {
                g_State[i].ok         = infos[i].ok;
                g_State[i].name       = infos[i].name;
                g_State[i].map        = infos[i].map;
                g_State[i].players    = infos[i].players;
                g_State[i].maxPlayers = infos[i].maxPlayers;
            }
        }

        std::unique_lock<std::mutex> lock(g_Mutex);
        if (g_Wake.wait_for(lock, std::chrono::milliseconds(intervalMs), [] { return g_Stop; })) return;
    }
}

void Start()
{
    Stop();

    std::vector<a2s::Target> targets;
    for (const slc::ServerEntry& s : slc::g_Settings.servers)
    {
        a2s::Target t;
        t.host = s.host;
        t.port = s.port;
        targets.push_back(t);
    }

    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Stop = false;
        g_State.assign(targets.size(), ServerState());
    }
    if (targets.empty()) return;

    g_Thread = std::thread(Worker, std::move(targets), (int)(slc::g_Settings.updateInterval * 1000.0f));
}

void Stop()
{
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Stop = true;
    }
    g_Wake.notify_all();
    if (g_Thread.joinable()) g_Thread.join();
}

std::vector<ServerState> Snapshot()
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    return g_State;
}
}
