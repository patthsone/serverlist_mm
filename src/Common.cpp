#include "Common.h"

#include <cstdio>
#include <map>
#include <vector>

namespace slc
{

static std::map<std::string, std::string> g_phrases;

static void VLog(const Color& color, const char* fmt, va_list ap)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    ConColorMsg(color, "[ServerList] %s\n", buf);
}

void LogInfo(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); VLog(Color(0, 200, 0, 255), fmt, ap);   va_end(ap); }
void LogWarn(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); VLog(Color(255, 150, 0, 255), fmt, ap); va_end(ap); }
void LogError(const char* fmt, ...) { va_list ap; va_start(ap, fmt); VLog(Color(255, 0, 0, 255), fmt, ap);   va_end(ap); }

std::string Fmt(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return buf;
}

void LoadPhrases()
{
    g_phrases.clear();
    KeyValues* kv = new KeyValues("Phrases");
    if (!kv->LoadFromFile(g_pFullFileSystem, "addons/translations/serverlist.phrases.txt"))
    {
        LogWarn("addons/translations/serverlist.phrases.txt not found, using built-in English texts");
        delete kv;
        return;
    }
    const char* lang = g_pUtils ? g_pUtils->GetLanguage() : "en";
    for (KeyValues* p = kv->GetFirstTrueSubKey(); p; p = p->GetNextTrueSubKey())
    {
        const char* v = p->GetString(lang, nullptr);
        if (!v || !*v) v = p->GetString("en", nullptr);
        if (v) g_phrases[p->GetName()] = v;
    }
    delete kv;
}

const char* Tr(const char* key, const char* fallback)
{
    auto it = g_phrases.find(key);
    return (it != g_phrases.end() && !it->second.empty()) ? it->second.c_str() : fallback;
}

static std::string ReplaceToken(std::string s, const std::string& token, const std::string& value)
{
    size_t pos = 0;
    while ((pos = s.find(token, pos)) != std::string::npos)
    {
        s.replace(pos, token.size(), value);
        pos += value.size();
    }
    return s;
}

std::string Colorize(const std::string& text)
{
    static const std::pair<const char*, const char*> kColors[] = {
        { "{DEFAULT}", "\x01" },    { "{DARKRED}", "\x02" },  { "{LIGHTPURPLE}", "\x03" }, { "{GREEN}", "\x04" },
        { "{OLIVE}", "\x05" },      { "{LIME}", "\x06" },     { "{RED}", "\x07" },         { "{GRAY}", "\x08" },
        { "{YELLOW}", "\x09" },     { "{SILVER}", "\x0A" },   { "{LIGHTBLUE}", "\x0B" },   { "{BLUE}", "\x0C" },
        { "{PURPLE}", "\x0D" },     { "{PINK}", "\x0E" },     { "{LIGHTRED}", "\x0F" },    { "{GOLD}", "\x10" },
    };

    std::string out = ReplaceToken(text, "{PREFIX}", Tr("Prefix", "{GREEN}[ServerList] {DEFAULT}"));
    for (const auto& c : kColors) out = ReplaceToken(out, c.first, c.second);
    if (out.empty() || out[0] != ' ') out.insert(out.begin(), ' ');
    return out;
}

void Say(int slot, const std::string& text)
{
    if (!g_pUtils || slot < 0) return;
    g_pUtils->PrintToChat(slot, "%s", Colorize(text).c_str());
}

void SayAll(const std::string& text)
{
    if (!g_pUtils) return;
    g_pUtils->PrintToChatAll("%s", Colorize(text).c_str());
}

void SayKey(int slot, const char* key, const char* fallback)
{
    Say(slot, Tr(key, fallback));
}
}
