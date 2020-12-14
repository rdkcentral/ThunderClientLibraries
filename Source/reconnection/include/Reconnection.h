#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <com/com.h>
#include <core/core.h>

#include <interfaces/IDisplayInfo.h>

namespace WPEFramework {
class Catalog : protected PluginHost::IPlugin::INotification {
public:
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;

    Catalog() = default;
    ~Catalog() override = default;

    void Load(PluginHost::IShell* systemInterface, std::vector<string>& modules)
    {
        ASSERT(_instances.size() == 0);

        fprintf(stderr, "Before register\n");

        systemInterface->Register(this);
        // systemInterface->Unregister(this);

        fprintf(stderr, "Before while loop, size: %d\n", _instances.size());
        while (_instances.size() > 0) {
            fprintf(stderr, "In while loop\n");
            PluginHost::IShell* current = _instances.back();
            //Exchange::IConnectionProperties* props = current->QueryInterface<Exchange::IConnectionProperties>();

            //if (props != nullptr) {
            modules.push_back(current->Callsign());
            //      props->Release();
            //}
            current->Release();
            _instances.pop_back();
        }
    }

private:
    void StateChange(PluginHost::IShell* plugin) override
    {
        fprintf(stderr, "state change\n");
        plugin->AddRef();
        _instances.push_back(plugin);
    }

    BEGIN_INTERFACE_MAP(Catalog)
    INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
    END_INTERFACE_MAP

private:
    std::vector<PluginHost::IShell*> _instances;
};
}