#pragma once

#include <com/com.h>
#include <core/core.h>
#include <plugins/IShell.h>



namespace WPEFramework {
class Catalog : protected PluginHost::IPlugin::INotification {
public:
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;

    Catalog() = default;
    ~Catalog() override = default;

    void Register(PluginHost::IShell* systemInterface)
    {
        systemInterface->Register(this);

        _isRegistered = true;
    }

    void Unregister(PluginHost::IShell* systemInterface)
    {
        systemInterface->Unregister(this);
    }

private:
    void StateChange(PluginHost::IShell* plugin) override
    {
        ASSERT(plugin != nullptr);

        if (_isRegistered) {
            if (plugin->Callsign() == _callsign) {
                if (plugin->State() == PluginHost::IShell::ACTIVATION) {
                    fprintf(stderr, "Activating\n");
                }
                if (plugin->State() == PluginHost::IShell::DEACTIVATION) {
                    fprintf(stderr, "Deactivating\n");
                }
            }
        }
    }

    BEGIN_INTERFACE_MAP(Catalog)
    INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
    END_INTERFACE_MAP

private:
    bool _isRegistered = false;
    std::string _callsign = "PlayerInfo";
};
}