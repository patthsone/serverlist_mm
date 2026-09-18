#include "Menu.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "Common.h"
#include "Config.h"
#include "Query.h"

using namespace slc;

namespace slm
{

static const size_t kRowNameMaxBytes = 28;

static std::string Truncate(const std::string& s, size_t maxBytes)
{
    if (s.size() <= maxBytes) return s;
    size_t cut = maxBytes;
    while (cut > 0 && ((unsigned char)s[cut] & 0xC0) == 0x80) --cut;
    return s.substr(0, cut) + "...";
}

static bool CanUse(int slot)
{
    if (!g_pPlayers || !g_pMenus || slot < 0) return false;
    if (!g_pPlayers->IsConnected(slot) || g_pPlayers->IsFakeClient(slot)) return false;
    if (g_Settings.servers.empty())
    {
        SayKey(slot, "NoServers", "{PREFIX}No servers are configured");
        return false;
    }
    return true;
}

void ShowList(int slot)
{
    if (!CanUse(slot)) return;

    std::vector<slq::ServerState> state = slq::Snapshot();

    Menu menu;
    menu.szTitle = Tr("ListTitle", "Servers");

    for (size_t i = 0; i < g_Settings.servers.size(); ++i)
    {
        const slq::ServerState* st = i < state.size() ? &state[i] : nullptr;
        bool up = st && st->ok;

        std::string label = Truncate(g_Settings.servers[i].name, kRowNameMaxBytes) + "  ";
        label += up ? Fmt("%d/%d", st->players, st->maxPlayers) : std::string(Tr("Offline", "offline"));

        g_pMenus->AddItemMenu(menu, std::to_string(i).c_str(), label.c_str(), up ? ITEM_DEFAULT : ITEM_DISABLED);
    }

    g_pMenus->SetExitMenu(menu, true);
    g_pMenus->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!szBack || !*szBack) return;
        ShowDetail(iSlot, atoi(szBack));
    });

    g_pMenus->DisplayPlayerMenu(menu, slot, true, true);
}

void ShowDetail(int slot, int idx)
{
    if (!CanUse(slot)) return;
    if (idx < 0 || (size_t)idx >= g_Settings.servers.size()) return;

    std::vector<slq::ServerState> state = slq::Snapshot();
    if ((size_t)idx >= state.size() || !state[idx].ok)
    {
        SayKey(slot, "ServerOffline", "{PREFIX}This server is not responding right now");
        ShowList(slot);
        return;
    }
    const slq::ServerState& st = state[idx];
    const ServerEntry& se = g_Settings.servers[idx];

    std::string name = st.name.empty() ? se.name : st.name;

    Menu menu;
    menu.szTitle = Tr("ConfirmTitle", "Join this server?");

    g_pMenus->AddItemMenu(menu, "", Fmt(Tr("DetailName", "Name: %s"), Truncate(name, kRowNameMaxBytes).c_str()).c_str(), ITEM_DISABLED);
    g_pMenus->AddItemMenu(menu, "", Fmt(Tr("DetailMap", "Map: %s - %d/%d"), Truncate(st.map, kRowNameMaxBytes).c_str(), st.players, st.maxPlayers).c_str(),
                          ITEM_DISABLED);
    g_pMenus->AddItemMenu(menu, ("y:" + std::to_string(idx)).c_str(), Tr("Yes", "Yes"));
    g_pMenus->AddItemMenu(menu, "n", Tr("No", "No"));

    g_pMenus->SetExitMenu(menu, true);
    g_pMenus->SetBackMenu(menu, true);
    g_pMenus->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!szBack || !*szBack) return;

        if (!strncmp(szBack, "y:", 2))
        {
            int i = atoi(szBack + 2);
            if (i < 0 || (size_t)i >= g_Settings.servers.size() || !g_pPlayers) return;

            std::vector<slq::ServerState> now = slq::Snapshot();
            if ((size_t)i >= now.size() || !now[i].ok)
            {
                SayKey(iSlot, "ServerOffline", "{PREFIX}This server is not responding right now");
                ShowList(iSlot);
                return;
            }

            g_pMenus->ClosePlayerMenu(iSlot);

            std::string cmd = "connect " + g_Settings.servers[i].addr;
            g_pPlayers->UseClientCommand(iSlot, cmd.c_str());
            return;
        }

        ShowList(iSlot);
    });

    g_pMenus->DisplayPlayerMenu(menu, slot, true, true);
}

void Announce()
{
    static size_t next = 0;

    std::vector<slq::ServerState> state = slq::Snapshot();
    size_t n = g_Settings.servers.size();

    for (size_t k = 0; k < n; ++k)
    {
        size_t i = (next + k) % n;
        if (i >= state.size() || !state[i].ok) continue;

        next = i + 1;
        const slq::ServerState& st = state[i];
        SayAll(Fmt(Tr("Announce", "{PREFIX}{GOLD}%s{DEFAULT} - %s, {GREEN}%d/%d{DEFAULT} - type {GREEN}!servers"),
                   g_Settings.servers[i].name.c_str(), st.map.c_str(), st.players, st.maxPlayers));
        return;
    }
}
}
