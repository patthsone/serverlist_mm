# serverlist_mm

Metamod:Source plugin (CS2) by PattHs: a server list with online, and one-click connect.

- `!servers` / `servers` in chat or `mm_servers` in the console opens a menu with the configured servers and their
  online (`12/64`); servers that do not answer are greyed out ("offline").
- Choosing a server (E) shows its **name** and **map**, then asks **Yes / No**. *Yes* sends `connect <ip:port>` to
  the player, *No* returns to the list.
- Every `annonce_interval` seconds one online server is advertised in chat (round robin).

Server data comes from a direct **A2S_INFO** query (UDP) made by a background thread every `update_interval`
seconds; the menu shows the last result, so opening it never waits for the network. No Steam API key is needed.

## Requirements (other Metamod plugins on the server)

| Plugin | Why |
| --- | --- |
| Utils (`IUtilsApi`), Players (`IPlayersApi`) | chat commands, timers, chat output, `connect` |
| Menus (`IMenusApi`) | the menus |

## Install

1. Copy the contents of the release zip (`addons/…`) to `game/csgo/addons/`.
2. Edit `addons/configs/serverlist.cfg` (servers, intervals).
3. `meta load addons/metamod/ServerList` (or restart).

## Config (`addons/configs/serverlist.cfg`)

```
"cfg"
{
    "annonce_interval"  "90.0"     // seconds between chat adverts, 0 = off
    "update_interval"   "20.0"     // seconds between server queries (min 5)
    "apikey"            ""         // unused in this build, accepted so an old cfg still loads
    "servers"
    {
        "Server #FF2"  { "ip" "45.136.204.250:27015" }   // block name = what players see, ip = host:port
    }
}
```

`ip` may contain only letters, digits, `.` and `-` plus `:port`; anything else is rejected at load time because the
address is put into a console `connect` command. Texts are in `addons/translations/serverlist.phrases.txt` (ru/en).


