#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "A2S.h"

static int g_failed = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++g_failed; } \
    } while (0)

static void PutStr(std::vector<uint8_t>& v, const std::string& s)
{
    v.insert(v.end(), s.begin(), s.end());
    v.push_back(0);
}

static std::vector<uint8_t> InfoPacket(const std::string& name, const std::string& map, int players, int max, int bots)
{
    std::vector<uint8_t> v = { 0xFF, 0xFF, 0xFF, 0xFF, 0x49, 17 };
    PutStr(v, name);
    PutStr(v, map);
    PutStr(v, "csgo");
    PutStr(v, "Counter-Strike 2");
    v.push_back(0x50); v.push_back(0x0E);
    v.push_back((uint8_t)players);
    v.push_back((uint8_t)max);
    v.push_back((uint8_t)bots);
    v.insert(v.end(), { 'd', 'l', 0, 1 });
    return v;
}

static int BindUdp(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family      = AF_INET;
    a.sin_port        = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (sockaddr*)&a, sizeof(a)) != 0) { close(fd); return -1; }
    timeval tv = { 3, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return fd;
}

static void ServeFromOtherPort(int rx, int tx, std::vector<uint8_t> reply)
{
    uint8_t buf[512];
    sockaddr_in from;
    socklen_t len = sizeof(from);
    if (recvfrom(rx, buf, sizeof(buf), 0, (sockaddr*)&from, &len) <= 0) return;
    std::vector<uint8_t> challenge = { 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0xBE, 0xBA, 0xFE, 0xCA };
    sendto(tx, challenge.data(), challenge.size(), 0, (sockaddr*)&from, len);
    len = sizeof(from);
    if (recvfrom(rx, buf, sizeof(buf), 0, (sockaddr*)&from, &len) <= 0) return;
    sendto(tx, reply.data(), reply.size(), 0, (sockaddr*)&from, len);
}

int main()
{
    std::string host; int port = 0;
    CHECK(a2s::ParseAddress("45.136.204.250:27015", host, port) && host == "45.136.204.250" && port == 27015);
    CHECK(a2s::ParseAddress("play.example.com", host, port) && host == "play.example.com" && port == 27015);
    CHECK(!a2s::ParseAddress("1.2.3.4:0", host, port));
    CHECK(!a2s::ParseAddress("1.2.3.4:70000", host, port));
    CHECK(!a2s::ParseAddress("1.2.3.4:abc", host, port));
    CHECK(!a2s::ParseAddress("1.2.3.4;quit:27015", host, port));
    CHECK(!a2s::ParseAddress("1.2.3.4 \"x:27015", host, port));
    CHECK(!a2s::ParseAddress(":27015", host, port));
    CHECK(!a2s::ParseAddress("", host, port));

    std::vector<uint8_t> req = a2s::BuildRequest(false, 0);
    CHECK(req.size() == 5 + 20);
    CHECK(req[4] == 0x54 && req.back() == 0);
    std::vector<uint8_t> reqc = a2s::BuildRequest(true, 0x11223344u);
    CHECK(reqc.size() == req.size() + 4);
    CHECK(reqc[reqc.size() - 4] == 0x44 && reqc.back() == 0x11);

    a2s::Info info; uint32_t challenge = 0;
    std::vector<uint8_t> pkt = InfoPacket("LUXE #FF2", "de_dust2", 12, 64, 3);
    CHECK(a2s::ParseReply(pkt.data(), pkt.size(), info, challenge) == a2s::Reply::Info);
    CHECK(info.ok && info.name == "LUXE #FF2" && info.map == "de_dust2" && info.game == "Counter-Strike 2");
    CHECK(info.players == 12 && info.maxPlayers == 64 && info.bots == 3);

    pkt = InfoPacket("Сервер \x01LUXE\xFF", "de_mirage", 1, 10, 0);
    a2s::Info ru;
    CHECK(a2s::ParseReply(pkt.data(), pkt.size(), ru, challenge) == a2s::Reply::Info);
    CHECK(ru.name == "Сервер LUXE");

    std::vector<uint8_t> ch = { 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x78, 0x56, 0x34, 0x12 };
    a2s::Info none;
    CHECK(a2s::ParseReply(ch.data(), ch.size(), none, challenge) == a2s::Reply::Challenge);
    CHECK(challenge == 0x12345678u && !none.ok);

    a2s::Info bad;
    std::vector<uint8_t> shortPkt = { 0xFF, 0xFF, 0xFF };
    CHECK(a2s::ParseReply(shortPkt.data(), shortPkt.size(), bad, challenge) == a2s::Reply::Bad);
    std::vector<uint8_t> split = { 0xFE, 0xFF, 0xFF, 0xFF, 0x49, 1 };
    CHECK(a2s::ParseReply(split.data(), split.size(), bad, challenge) == a2s::Reply::Bad);
    std::vector<uint8_t> truncated(pkt.begin(), pkt.begin() + 12);
    CHECK(a2s::ParseReply(truncated.data(), truncated.size(), bad, challenge) == a2s::Reply::Bad);
    std::vector<uint8_t> unterminated = { 0xFF, 0xFF, 0xFF, 0xFF, 0x49, 17, 'a', 'b' };
    CHECK(a2s::ParseReply(unterminated.data(), unterminated.size(), bad, challenge) == a2s::Reply::Bad);
    CHECK(!bad.ok);

    CHECK(a2s::QueryAll({}, 50).empty());
    std::vector<a2s::Target> unreachable = { { "127.0.0.1", 1 } };
    std::vector<a2s::Info> res = a2s::QueryAll(unreachable, 300);
    CHECK(res.size() == 1 && !res[0].ok);

    int rx = BindUdp(28111), tx = BindUdp(28112);
    CHECK(rx >= 0 && tx >= 0);
    if (rx >= 0 && tx >= 0)
    {
        std::thread server(ServeFromOtherPort, rx, tx, InfoPacket("FPS", "de_dust2", 7, 64, 0));
        std::vector<a2s::Target> asked = { { "127.0.0.1", 28111 } };
        std::vector<a2s::Info> other = a2s::QueryAll(asked, 2000);
        server.join();
        CHECK(other.size() == 1 && other[0].ok && other[0].name == "FPS" && other[0].players == 7);
    }
    if (rx >= 0) close(rx);
    if (tx >= 0) close(tx);

    if (g_failed == 0) std::printf("all a2s tests passed\n");
    return g_failed ? 1 : 0;
}
