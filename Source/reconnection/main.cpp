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
    StateChangeNotifier notifier;
    Observer obs1(1);
    Observer obs2(2);

    notifier.Register("PlayerInfo", &obs1);
    notifier.Register("DisplayInfo", &obs1);
    notifier.Register("DisplayInfo", &obs2);

    while (true) {
    }

    Core::Singleton::Dispose();
    return 0;
}