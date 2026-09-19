#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace a2s
{

struct Info
{
    bool        ok = false;
    std::string name;
    std::string map;
    std::string folder;
    std::string game;
    std::string error;
    int         players    = 0;
    int         maxPlayers = 0;
    int         bots       = 0;
};

struct Target
{
    std::string host;
    int         port = 27015;
};

enum class Reply { Info, Challenge, Bad };

bool ParseAddress(const std::string& text, std::string& host, int& port);

std::vector<uint8_t> BuildRequest(bool withChallenge, uint32_t challenge);

Reply ParseReply(const uint8_t* data, size_t len, Info& out, uint32_t& challenge);

std::vector<Info> QueryAll(const std::vector<Target>& targets, int timeoutMs);

std::string CleanText(const std::string& s);
}
