#include "Reconnection.h"

using namespace WPEFramework;

Core::NodeId Connector()
{
    const TCHAR* comPath = ::getenv(_T("COMMUNICATOR_PATH"));

    if (comPath == nullptr) {
#ifdef __WINDOWS__
        comPath = _T("127.0.0.1:62000");
#else
        comPath = _T("/tmp/communicator");
#endif
    }

    return Core::NodeId(comPath);
}

int main()
{
    std::vector<string> instances;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create());
    ASSERT(engine != nullptr);
    Core::ProxyType<RPC::CommunicatorClient> comChannel(
        Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(),
            Core::ProxyType<Core::IIPCServer>(engine)));
    ASSERT(comChannel != nullptr);
    engine->Announcements(comChannel->Announcement());

    PluginHost::IShell* systemInterface = comChannel->Open<PluginHost::IShell>(string());
    if (systemInterface != nullptr) {
        Core::Sink<Catalog> mySink;
        mySink.Load(systemInterface, instances);
        fprintf(stderr, "Before release\n");
        systemInterface->Release();
    }

    for (const auto& i : instances) {
        fprintf(stderr, "%s\n", i);
    }

    return 0;
}