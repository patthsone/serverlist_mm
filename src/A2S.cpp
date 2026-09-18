#include "A2S.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace a2s
{

namespace
{

struct Reader
{
    const uint8_t* p;
    size_t         n;
    size_t         i   = 0;
    bool           bad = false;

    Reader(const uint8_t* data, size_t len) : p(data), n(len) {}

    uint8_t U8()
    {
        if (i + 1 > n) { bad = true; return 0; }
        return p[i++];
    }
    uint16_t U16()
    {
        if (i + 2 > n) { bad = true; return 0; }
        uint16_t v = (uint16_t)(p[i] | (p[i + 1] << 8));
        i += 2;
        return v;
    }
    uint32_t U32()
    {
        if (i + 4 > n) { bad = true; return 0; }
        uint32_t v = (uint32_t)p[i] | ((uint32_t)p[i + 1] << 8) | ((uint32_t)p[i + 2] << 16) | ((uint32_t)p[i + 3] << 24);
        i += 4;
        return v;
    }
    std::string CStr()
    {
        size_t start = i;
        while (i < n && p[i] != 0) ++i;
        if (i >= n) { bad = true; return std::string(); }
        std::string s((const char*)p + start, i - start);
        ++i;
        return s;
    }
};

const char kQueryText[] = "Source Engine Query";
}

bool ParseAddress(const std::string& text, std::string& host, int& port)
{
    std::string h = text;
    int p = 27015;

    size_t colon = text.rfind(':');
    if (colon != std::string::npos)
    {
        h = text.substr(0, colon);
        std::string ps = text.substr(colon + 1);
        if (ps.empty() || ps.size() > 5 || ps.find_first_not_of("0123456789") != std::string::npos) return false;
        p = atoi(ps.c_str());
    }
    if (p < 1 || p > 65535 || h.empty() || h.size() > 253) return false;

    for (unsigned char c : h)
    {
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-';
        if (!ok) return false;
    }
    host = h;
    port = p;
    return true;
}

std::vector<uint8_t> BuildRequest(bool withChallenge, uint32_t challenge)
{
    std::vector<uint8_t> req = { 0xFF, 0xFF, 0xFF, 0xFF, 0x54 };
    req.insert(req.end(), kQueryText, kQueryText + sizeof(kQueryText));
    if (withChallenge)
    {
        for (int b = 0; b < 4; ++b) req.push_back((uint8_t)((challenge >> (8 * b)) & 0xFF));
    }
    return req;
}

std::string CleanText(const std::string& s)
{
    std::string out;
    out.reserve(s.size());

    size_t i = 0;
    while (i < s.size())
    {
        unsigned char c = (unsigned char)s[i];
        size_t len = 0;
        if (c < 0x80) len = 1;
        else if (c >= 0xC2 && c <= 0xDF) len = 2;
        else if (c >= 0xE0 && c <= 0xEF) len = 3;
        else if (c >= 0xF0 && c <= 0xF4) len = 4;

        bool valid = len > 0 && i + len <= s.size();
        for (size_t k = 1; valid && k < len; ++k) valid = ((unsigned char)s[i + k] & 0xC0) == 0x80;

        if (!valid) { ++i; continue; }
        if (len == 1 && (c < 0x20 || c == 0x7F)) { ++i; continue; }

        out.append(s, i, len);
        i += len;
    }
    return out;
}

Reply ParseReply(const uint8_t* data, size_t len, Info& out, uint32_t& challenge)
{
    Reader r(data, len);
    if (r.U32() != 0xFFFFFFFFu || r.bad) return Reply::Bad;

    uint8_t type = r.U8();
    if (r.bad) return Reply::Bad;

    if (type == 0x41)
    {
        challenge = r.U32();
        return r.bad ? Reply::Bad : Reply::Challenge;
    }
    if (type != 0x49) return Reply::Bad;

    Info info;
    r.U8();
    info.name   = CleanText(r.CStr());
    info.map    = CleanText(r.CStr());
    info.folder = CleanText(r.CStr());
    info.game   = CleanText(r.CStr());
    r.U16();
    info.players    = r.U8();
    info.maxPlayers = r.U8();
    info.bots       = r.U8();
    if (r.bad) return Reply::Bad;

    info.ok = true;
    out = std::move(info);
    return Reply::Info;
}

std::vector<Info> QueryAll(const std::vector<Target>& targets, int timeoutMs)
{
    struct Slot
    {
        int  fd      = -1;
        bool done    = false;
        bool retried = false;
    };

    std::vector<Info> results(targets.size());
    std::vector<Slot> slots(targets.size());

    std::vector<uint8_t> first = BuildRequest(false, 0);

    for (size_t i = 0; i < targets.size(); ++i)
    {
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        addrinfo* res = nullptr;
        std::string port = std::to_string(targets[i].port);
        if (getaddrinfo(targets[i].host.c_str(), port.c_str(), &hints, &res) != 0 || !res)
        {
            slots[i].done = true;
            continue;
        }

        int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (fd < 0)
        {
            freeaddrinfo(res);
            slots[i].done = true;
            continue;
        }

        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (connect(fd, res->ai_addr, res->ai_addrlen) != 0 || send(fd, first.data(), first.size(), 0) < 0)
        {
            close(fd);
            slots[i].done = true;
        }
        else
        {
            slots[i].fd = fd;
        }
        freeaddrinfo(res);
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    for (;;)
    {
        std::vector<pollfd> fds;
        std::vector<size_t> owner;
        for (size_t i = 0; i < slots.size(); ++i)
        {
            if (slots[i].done || slots[i].fd < 0) continue;
            pollfd p;
            p.fd      = slots[i].fd;
            p.events  = POLLIN;
            p.revents = 0;
            fds.push_back(p);
            owner.push_back(i);
        }
        if (fds.empty()) break;

        long long left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0) break;

        int ready = poll(fds.data(), (nfds_t)fds.size(), (int)left);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            break;
        }
        if (ready == 0) break;

        for (size_t k = 0; k < fds.size(); ++k)
        {
            if (!fds[k].revents) continue;
            Slot& slot = slots[owner[k]];

            uint8_t buf[1500];
            ssize_t got = recv(slot.fd, buf, sizeof(buf), 0);
            if (got < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                slot.done = true;
                continue;
            }

            uint32_t challenge = 0;
            Reply kind = ParseReply(buf, (size_t)got, results[owner[k]], challenge);
            if (kind == Reply::Info)
            {
                slot.done = true;
            }
            else if (kind == Reply::Challenge && !slot.retried)
            {
                slot.retried = true;
                std::vector<uint8_t> again = BuildRequest(true, challenge);
                if (send(slot.fd, again.data(), again.size(), 0) < 0) slot.done = true;
            }
        }
    }

    for (Slot& s : slots)
        if (s.fd >= 0) close(s.fd);

    return results;
}
}
