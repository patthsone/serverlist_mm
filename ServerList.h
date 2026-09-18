#ifndef _INCLUDE_SERVERLIST_H_
#define _INCLUDE_SERVERLIST_H_

#include "src/Common.h"

class ServerList final : public ISmmPlugin, public IMetamodListener
{
public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;
    void AllPluginsLoaded() override;
    void SetFailState(const char* error);

public:
    const char* GetAuthor() override;
    const char* GetName() override;
    const char* GetDescription() override;
    const char* GetURL() override;
    const char* GetLicense() override;
    const char* GetVersion() override;
    const char* GetDate() override;
    const char* GetLogTag() override;
};

extern ServerList g_ServerList;

#endif
