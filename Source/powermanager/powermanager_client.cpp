/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unordered_map>

#include <core/Portability.h>
#include <core/Services.h>

#include <interfaces/IPowerManager.h>
#include <plugins/Types.h>

#include "powermanager_client.h"

using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using WakeupSrcType = WPEFramework::Exchange::IPowerManager::WakeupSrcType;
using WakeupReason = WPEFramework::Exchange::IPowerManager::WakeupReason;
using SystemMode = WPEFramework::Exchange::IPowerManager::SystemMode;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

namespace /*unnamed*/ {

const std::unordered_map<PowerState, PowerManager_PowerState_t>& powerStateMap()
{
    static const std::unordered_map<PowerState, PowerManager_PowerState_t> map = {
        { PowerState::POWER_STATE_UNKNOWN, POWER_STATE_UNKNOWN },
        { PowerState::POWER_STATE_OFF, POWER_STATE_OFF },
        { PowerState::POWER_STATE_STANDBY, POWER_STATE_STANDBY },
        { PowerState::POWER_STATE_ON, POWER_STATE_ON },
        { PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, POWER_STATE_STANDBY_LIGHT_SLEEP },
        { PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, POWER_STATE_STANDBY_DEEP_SLEEP },
    };

    return map;
}

PowerManager_PowerState_t convert(const PowerState from)
{
    auto& map = powerStateMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : POWER_STATE_UNKNOWN;
}

PowerState convert(const PowerManager_PowerState_t from)
{
    auto& map = powerStateMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return PowerState::POWER_STATE_UNKNOWN;
}

// TODO: re-check if required
const std::unordered_map<WakeupSrcType, PowerManager_WakeupSrcType_t>& wakeupSrcTypeMap()
{
    static const std::unordered_map<WakeupSrcType, PowerManager_WakeupSrcType_t> map = {
        { WakeupSrcType::WAKEUP_SRC_UNKNOWN, WAKEUP_SRC_UNKNOWN },
        { WakeupSrcType::WAKEUP_SRC_VOICE, WAKEUP_SRC_VOICE },
        { WakeupSrcType::WAKEUP_SRC_PRESENCEDETECTED, WAKEUP_SRC_PRESENCEDETECTED },
        { WakeupSrcType::WAKEUP_SRC_BLUETOOTH, WAKEUP_SRC_BLUETOOTH },
        { WakeupSrcType::WAKEUP_SRC_RF4CE, WAKEUP_SRC_RF4CE },
        { WakeupSrcType::WAKEUP_SRC_WIFI, WAKEUP_SRC_WIFI },
        { WakeupSrcType::WAKEUP_SRC_IR, WAKEUP_SRC_IR },
        { WakeupSrcType::WAKEUP_SRC_POWERKEY, WAKEUP_SRC_POWERKEY },
        { WakeupSrcType::WAKEUP_SRC_TIMER, WAKEUP_SRC_TIMER },
        { WakeupSrcType::WAKEUP_SRC_CEC, WAKEUP_SRC_CEC },
        { WakeupSrcType::WAKEUP_SRC_LAN, WAKEUP_SRC_LAN },
    };
    return map;
}

PowerManager_WakeupSrcType_t convert(const WakeupSrcType from)
{
    auto& map = wakeupSrcTypeMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : WAKEUP_SRC_UNKNOWN;
}

WakeupSrcType convert(const PowerManager_WakeupSrcType_t from)
{
    auto& map = wakeupSrcTypeMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return WakeupSrcType::WAKEUP_SRC_UNKNOWN;
}

const std::unordered_map<WakeupReason, PowerManager_WakeupReason_t>& wakeupReasonMap()
{
    static const std::unordered_map<WakeupReason, PowerManager_WakeupReason_t> map = {
        { WakeupReason::WAKEUP_REASON_UNKNOWN, WAKEUP_REASON_UNKNOWN },
        { WakeupReason::WAKEUP_REASON_IR, WAKEUP_REASON_IR },
        { WakeupReason::WAKEUP_REASON_BLUETOOTH, WAKEUP_REASON_BLUETOOTH },
        { WakeupReason::WAKEUP_REASON_RF4CE, WAKEUP_REASON_RF4CE },
        { WakeupReason::WAKEUP_REASON_GPIO, WAKEUP_REASON_GPIO },
        { WakeupReason::WAKEUP_REASON_LAN, WAKEUP_REASON_LAN },
        { WakeupReason::WAKEUP_REASON_WIFI, WAKEUP_REASON_WIFI },
        { WakeupReason::WAKEUP_REASON_TIMER, WAKEUP_REASON_TIMER },
        { WakeupReason::WAKEUP_REASON_FRONTPANEL, WAKEUP_REASON_FRONTPANEL },
        { WakeupReason::WAKEUP_REASON_WATCHDOG, WAKEUP_REASON_WATCHDOG },
        { WakeupReason::WAKEUP_REASON_SOFTWARERESET, WAKEUP_REASON_SOFTWARERESET },
        { WakeupReason::WAKEUP_REASON_THERMALRESET, WAKEUP_REASON_THERMALRESET },
        { WakeupReason::WAKEUP_REASON_WARMRESET, WAKEUP_REASON_WARMRESET },
        { WakeupReason::WAKEUP_REASON_COLDBOOT, WAKEUP_REASON_COLDBOOT },
        { WakeupReason::WAKEUP_REASON_STRAUTHFAIL, WAKEUP_REASON_STRAUTHFAIL },
        { WakeupReason::WAKEUP_REASON_CEC, WAKEUP_REASON_CEC },
        { WakeupReason::WAKEUP_REASON_PRESENCE, WAKEUP_REASON_PRESENCE },
        { WakeupReason::WAKEUP_REASON_VOICE, WAKEUP_REASON_VOICE },
    };
    return map;
}

PowerManager_WakeupReason_t convert(const WakeupReason from)
{
    auto& map = wakeupReasonMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : WAKEUP_REASON_UNKNOWN;
}

// TODO: re-check required ?
WakeupReason convert(const PowerManager_WakeupReason_t from)
{
    auto& map = wakeupReasonMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return WakeupReason::WAKEUP_REASON_UNKNOWN;
}

const std::unordered_map<PowerManager_SystemMode_t, SystemMode>& systemModeMap()
{
    static const std::unordered_map<PowerManager_SystemMode_t, SystemMode> map = {
        { SYSTEM_MODE_UNKNOWN, SystemMode::SYSTEM_MODE_UNKNOWN },
        { SYSTEM_MODE_NORMAL, SystemMode::SYSTEM_MODE_NORMAL },
        { SYSTEM_MODE_EAS, SystemMode::SYSTEM_MODE_EAS },
        { SYSTEM_MODE_WAREHOUSE, SystemMode::SYSTEM_MODE_WAREHOUSE },
    };
    return map;
}

SystemMode convert(const PowerManager_SystemMode_t from)
{
    auto& map = systemModeMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : SystemMode::SYSTEM_MODE_UNKNOWN;
}

const std::unordered_map<ThermalTemperature, PowerManager_ThermalTemperature_t>& thermalTemperatureMap()
{
    static const std::unordered_map<ThermalTemperature, PowerManager_ThermalTemperature_t> map = {
        { ThermalTemperature::THERMAL_TEMPERATURE_UNKNOWN, THERMAL_TEMPERATURE_UNKNOWN },
        { ThermalTemperature::THERMAL_TEMPERATURE_NORMAL, THERMAL_TEMPERATURE_NORMAL },
        { ThermalTemperature::THERMAL_TEMPERATURE_HIGH, THERMAL_TEMPERATURE_HIGH },
        { ThermalTemperature::THERMAL_TEMPERATURE_CRITICAL, THERMAL_TEMPERATURE_CRITICAL },
    };
    return map;
}

PowerManager_ThermalTemperature_t convert(const ThermalTemperature from)
{
    auto& map = thermalTemperatureMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : THERMAL_TEMPERATURE_UNKNOWN;
}

static string Callsign()
{
    static constexpr const TCHAR Default[] = _T("org.rdk.PowerManager");
    return (Default);
}

class PowerManagerClient : public RPC::SmartInterfaceType<Exchange::IPowerManager> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPowerManager>;
    using PowerModeChangedCallbacks = std::map<PowerManager_PowerModeChangedCb, void*>;
    using PowerModePreChangeCallbacks = std::map<PowerManager_PowerModePreChangeCb, void*>;
    using DeepSleepTimeoutCallbacks = std::map<PowerManager_DeepSleepTimeoutCb, void*>;
    using NetworkStandbyModeChangedCallbacks = std::map<PowerManager_NetworkStandbyModeChangedCb, void*>;
    using ThermalModeChangedCallbacks = std::map<PowerManager_ThermalModeChangedCb, void*>;
    using RebootBeginCallbacks = std::map<PowerManager_RebootBeginCb, void*>;

    class Notification : public Exchange::IPowerManager::INotification {
    private:
        PowerManagerClient& _parent;

    public:
        Notification(PowerManagerClient& parent)
            : _parent(parent)
        {
        }

        virtual void OnPowerModeChanged(const PowerState& currentState, const PowerState& newState) override
        {
            _parent.NotifyPowerModeChanged(currentState, newState);
        }

        virtual void OnPowerModePreChange(const PowerState& currentState, const PowerState& newState) override
        {
            _parent.NotifyPowerModePreChange(currentState, newState);
        }

        virtual void OnDeepSleepTimeout(const int& wakeupTimeout) override
        {
            _parent.NotifyDeepSleepTimeout(wakeupTimeout);
        }

        virtual void OnNetworkStandbyModeChanged(const bool& enabled) override
        {
            _parent.NotifyNetworkStandbyModeChanged(enabled);
        }

        virtual void OnThermalModeChanged(const ThermalTemperature& currentThermalLevel, const ThermalTemperature& newThermalLevel, const float& currentTemperature) override
        {
            _parent.NotifyThermalModeChanged(currentThermalLevel, newThermalLevel, currentTemperature);
        }

        virtual void OnRebootBegin(const string& rebootReasonCustom, const string& rebootReasonOther, const string& rebootRequestor) override
        {
            _parent.NotifyRebootBegin(rebootReasonCustom, rebootReasonOther, rebootRequestor);
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IPowerManager::INotification)
        END_INTERFACE_MAP
    };

    PowerManagerClient()
        : BaseClass()
        , _lock()
        , _powerManagerInterface(nullptr)
        , _powerManagerNotification(*this)
    {
        ASSERT(_singleton == nullptr);
        _singleton = this;

        uint32_t result = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign());

        PluginHost::IShell* controllerInterface = BaseClass::ControllerInterface();

        // avoid compiler warnings
        (void)controllerInterface;
    }

    ~PowerManagerClient()
    {
        BaseClass::Close(Core::infinite);
        ASSERT(_singleton != nullptr);
        _singleton = nullptr;
    }

    virtual void Operational(const bool upAndRunning) override
    {
        _lock.Lock();

        if (upAndRunning) {
            // PowerManager is Activated
            if (_powerManagerInterface == nullptr) {
                _powerManagerInterface = BaseClass::Interface();
                if (_powerManagerInterface != nullptr) {
                    _powerManagerInterface->Register(&_powerManagerNotification);
                }
            }
        } else {
            // PowerManager is Deactivated
            if (_powerManagerInterface != nullptr) {
                _powerManagerInterface->Unregister(&_powerManagerNotification);
                _powerManagerInterface->Release();
                _powerManagerInterface = nullptr;
            }
        }

        _lock.Unlock();
    }

public:
    static PowerManagerClient& Instance()
    {
        static PowerManagerClient* instance = new PowerManagerClient;
        ASSERT(instance != nullptr);
        return *instance;
    }

    // TODO: foresee CRASH if instance is used after Dispose
    static void Dispose()
    {
        ASSERT(_singleton != nullptr);
        if (_singleton != nullptr) {
            delete _singleton;
        }
    }

    uint32_t GetPowerState(PowerManager_PowerState_t* currentState, PowerManager_PowerState_t* previousState)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        PowerState _currentState = PowerState::POWER_STATE_UNKNOWN;
        PowerState _previousState = PowerState::POWER_STATE_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerState(_currentState, _previousState);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *currentState = convert(_currentState);
            *previousState = convert(_previousState);
        }

        return result;
    }

    uint32_t SetPowerState(const int keyCode, const PowerManager_PowerState_t powerState, const char* reason)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        PowerState _powerState = convert(powerState);

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetPowerState(keyCode, _powerState, reason);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetThermalState(float* currentTemperature)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetThermalState(*currentTemperature);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetTemperatureThresholds(float high, float critical)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetTemperatureThresholds(high, critical);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetTemperatureThresholds(float* high, float* critical)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetTemperatureThresholds(*high, *critical);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetOvertempGraceInterval(const int graceInterval)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetOvertempGraceInterval(graceInterval);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetOvertempGraceInterval(int* graceInterval)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetOvertempGraceInterval(*graceInterval);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetDeepSleepTimer(const int timeOut)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetDeepSleepTimer(timeOut);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetLastWakeupReason(PowerManager_WakeupReason_t* wakeupReason)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        WakeupReason _wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetLastWakeupReason(_wakeupReason);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *wakeupReason = convert(_wakeupReason);
        }

        return result;
    }

    uint32_t GetLastWakeupKeyCode(int* keycode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            _powerManagerInterface->GetLastWakeupKeyCode(*keycode);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetNetworkStandbyMode(const bool standbyMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetNetworkStandbyMode(standbyMode);
        }

        _lock.Unlock();

        return result;
    }
    uint32_t GetNetworkStandbyMode(bool* standbyMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetNetworkStandbyMode(*standbyMode);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetWakeupSrcConfig(const int powerMode, const int wakeSrcType, int config)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetWakeupSrcConfig(powerMode, wakeSrcType, config);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetWakeupSrcConfig(int& powerMode, int& srcType, int& config)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetWakeupSrcConfig(powerMode, srcType, config);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t SetSystemMode(const PowerManager_SystemMode_t currentMode, const PowerManager_SystemMode_t newMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        SystemMode _currentMode = convert(currentMode);
        SystemMode _newMode = convert(newMode);

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetSystemMode(_currentMode, _newMode);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetPowerStateBeforeReboot(PowerManager_PowerState_t* powerStateBeforeReboot)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        PowerState _powerStateBeforeReboot = PowerState::POWER_STATE_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerStateBeforeReboot(_powerStateBeforeReboot);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *powerStateBeforeReboot = convert(_powerStateBeforeReboot);
        }

        return result;
    }

    void NotifyPowerModeChanged(const PowerState& currentState, const PowerState& newState)
    {
        PowerManager_PowerState_t _currentState = convert(currentState);
        PowerManager_PowerState_t _newState = convert(newState);
        _lock.Lock();

        for (auto& index : _powerModeChanedCallbacks) {
            index.first(_currentState, _newState, index.second);
        }

        _lock.Unlock();
    }

    void NotifyPowerModePreChange(const PowerState& currentState, const PowerState& newState)
    {
        PowerManager_PowerState_t _currentState = convert(currentState);
        PowerManager_PowerState_t _newState = convert(newState);
        _lock.Lock();

        for (auto& index : _powerModePreChangeCallbacks) {
            index.first(_currentState, _newState, index.second);
        }

        _lock.Unlock();
    }

    void NotifyDeepSleepTimeout(const int& wakeupTimeout)
    {
        _lock.Lock();

        for (auto& index : _deepSleepTimeoutCallbacks) {
            index.first(wakeupTimeout, index.second);
        }

        _lock.Unlock();
    }

    void NotifyNetworkStandbyModeChanged(const bool& enabled)
    {
        _lock.Lock();

        for (auto& index : _networkStandbyModeChangedCallbacks) {
            index.first(enabled, index.second);
        }

        _lock.Unlock();
    }

    void NotifyThermalModeChanged(const ThermalTemperature& currentThermalLevel, const ThermalTemperature& newThermalLevel, const float& currentTemperature)
    {
        PowerManager_ThermalTemperature_t _currentThermalLevel = convert(currentThermalLevel);
        PowerManager_ThermalTemperature_t _newThermalLevel = convert(newThermalLevel);

        _lock.Lock();

        for (auto& index : _thermalModeChangedCallbacks) {
            index.first(_currentThermalLevel, _newThermalLevel, currentTemperature, index.second);
        }

        _lock.Unlock();
    }

    void NotifyRebootBegin(const string& rebootReasonCustom, const string& rebootReasonOther, const string& rebootRequestor)
    {
        _lock.Lock();

        for (auto& index : _rebootBeginCallbacks) {
            index.first(rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str(), index.second);
        }

        _lock.Unlock();
    }

    // Generic template function for callback register
    template <typename CallbackType>
    uint32_t RegisterCallback(std::map<CallbackType, void*>& callbackMap, CallbackType callback, void* userdata)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        ASSERT(callback != nullptr);

        _lock.Lock();
        if (callbackMap.find(callback) == callbackMap.end()) {
            callbackMap.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));

            result = Core::ERROR_NONE;
        }
        _lock.Unlock();

        return result;
    }

    // Generic template function for callback unregister
    template <typename CallbackType>
    uint32_t UnRegisterCallback(std::map<CallbackType, void*>& callbackMap, CallbackType callback)
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        ASSERT(callback != nullptr);

        _lock.Lock();

        auto it = callbackMap.find(callback);

        if (it != callbackMap.end()) {
            callbackMap.erase(it);
            result = Core::ERROR_NONE;
        }

        _lock.Unlock();

        return (result);
    }

    uint32_t RegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_powerModeChanedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback)
    {
        return UnRegisterCallback(_powerModeChanedCallbacks, callback);
    }

    uint32_t RegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback, void* userdata)
    {
        return RegisterCallback(_powerModePreChangeCallbacks, callback, userdata);
    }

    uint32_t UnRegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback)
    {
        return UnRegisterCallback(_powerModePreChangeCallbacks, callback);
    }

    uint32_t RegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback, void* userdata)
    {
        return RegisterCallback(_deepSleepTimeoutCallbacks, callback, userdata);
    }

    uint32_t UnRegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback)
    {
        return UnRegisterCallback(_deepSleepTimeoutCallbacks, callback);
    }

    uint32_t RegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_networkStandbyModeChangedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback)
    {
        return UnRegisterCallback(_networkStandbyModeChangedCallbacks, callback);
    }

    uint32_t RegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_thermalModeChangedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback)
    {
        return UnRegisterCallback(_thermalModeChangedCallbacks, callback);
    }

    uint32_t RegisterRebootBeginCallback(PowerManager_RebootBeginCb callback, void* userdata)
    {
        return RegisterCallback(_rebootBeginCallbacks, callback, userdata);
    }

    uint32_t UnRegisterRebootBeginCallback(PowerManager_RebootBeginCb callback)
    {
        return UnRegisterCallback(_rebootBeginCallbacks, callback);
    }

private:
    static PowerManagerClient* _singleton;
    Core::CriticalSection _lock;
    Exchange::IPowerManager* _powerManagerInterface; // remote PowerManager plugin interface
    Core::Sink<Notification> _powerManagerNotification;

    // containers for notification registertion
    PowerModeChangedCallbacks _powerModeChanedCallbacks;
    PowerModePreChangeCallbacks _powerModePreChangeCallbacks;
    DeepSleepTimeoutCallbacks _deepSleepTimeoutCallbacks;
    NetworkStandbyModeChangedCallbacks _networkStandbyModeChangedCallbacks;
    ThermalModeChangedCallbacks _thermalModeChangedCallbacks;
    RebootBeginCallbacks _rebootBeginCallbacks;
};
} // nameless namespace

PowerManagerClient* PowerManagerClient::_singleton = nullptr;

extern "C" {

uint32_t PowerManager_GetPowerState(PowerManager_PowerState_t* currentState, PowerManager_PowerState_t* previousState)
{
    ASSERT(currentState != nullptr);
    ASSERT(previousState != nullptr);
    return PowerManagerClient::Instance().GetPowerState(currentState, previousState);
}

uint32_t PowerManager_SetPowerState(const int keyCode, const PowerManager_PowerState_t powerstate, const char* reason)
{
    return PowerManagerClient::Instance().SetPowerState(keyCode, powerstate, reason);
}

uint32_t PowerManager_GetThermalState(float* currentTemperature)
{
    ASSERT(currentTemperature != nullptr);
    return PowerManagerClient::Instance().GetThermalState(currentTemperature);
}

uint32_t PowerManager_SetTemperatureThresholds(float high, float critical)
{
    return PowerManagerClient::Instance().SetTemperatureThresholds(high, critical);
}

uint32_t PowerManager_GetTemperatureThresholds(float* high, float* critical)
{
    ASSERT(high != nullptr);
    ASSERT(critical != nullptr);
    return PowerManagerClient::Instance().GetTemperatureThresholds(high, critical);
}

uint32_t PowerManager_SetOvertempGraceInterval(const int graceInterval)
{
    return PowerManagerClient::Instance().SetOvertempGraceInterval(graceInterval);
}

uint32_t PowerManager_GetOvertempGraceInterval(int* graceInterval /* @out */)
{
    ASSERT(graceInterval != nullptr);
    return PowerManagerClient::Instance().GetOvertempGraceInterval(graceInterval);
}

uint32_t PowerManager_SetDeepSleepTimer(const int timeOut)
{
    return PowerManagerClient::Instance().SetDeepSleepTimer(timeOut);
}

uint32_t PowerManager_GetLastWakeupReason(PowerManager_WakeupReason_t* wakeupReason)
{
    ASSERT(wakeupReason != nullptr);
    return PowerManagerClient::Instance().GetLastWakeupReason(wakeupReason);
}

uint32_t PowerManager_GetLastWakeupKeyCode(int* keycode)
{
    ASSERT(keycode != nullptr)
    return PowerManagerClient::Instance().GetLastWakeupKeyCode(keycode);
}

uint32_t PowerManager_Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
{
    ASSERT(rebootRequestor != nullptr)
    ASSERT(rebootReasonCustom != nullptr)
    ASSERT(rebootReasonOther != nullptr)
    return PowerManagerClient::Instance().Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
}

uint32_t PowerManager_SetNetworkStandbyMode(const bool standbyMode)
{
    return PowerManagerClient::Instance().SetNetworkStandbyMode(standbyMode);
}

uint32_t PowerManager_GetNetworkStandbyMode(bool* standbyMode)
{
    ASSERT(standbyMode != nullptr);
    return PowerManagerClient::Instance().GetNetworkStandbyMode(standbyMode);
}

uint32_t PowerManager_SetWakeupSrcConfig(const int powerMode, const int wakeSrcType, int config)
{
    return PowerManagerClient::Instance().SetWakeupSrcConfig(powerMode, wakeSrcType, config);
}

uint32_t PowerManager_GetWakeupSrcConfig(int* powerMode, int* srcType, int* config)
{
    ASSERT(powerMode != nullptr);
    ASSERT(srcType != nullptr);
    ASSERT(config != nullptr);
    return PowerManagerClient::Instance().GetWakeupSrcConfig(*powerMode, *srcType, *config);
}

uint32_t PowerManager_SetSystemMode(const PowerManager_SystemMode_t currentMode, const PowerManager_SystemMode_t newMode)
{
    return PowerManagerClient::Instance().SetSystemMode(currentMode, newMode);
}

uint32_t PowerManager_GetPowerStateBeforeReboot(PowerManager_PowerState_t* powerStateBeforeReboot)
{
    ASSERT(powerStateBeforeReboot != nullptr);
    return PowerManagerClient::Instance().GetPowerStateBeforeReboot(powerStateBeforeReboot);
}

uint32_t RegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterPowerModeChangedCallback(callback, userdata);
}

uint32_t UnRegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback)
{
    return PowerManagerClient::Instance().UnRegisterPowerModeChangedCallback(callback);
}

uint32_t RegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterPowerModePreChangeCallback(callback, userdata);
}

uint32_t UnRegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback)
{
    return PowerManagerClient::Instance().UnRegisterPowerModePreChangeCallback(callback);
}

uint32_t RegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterDeepSleepTimeoutCallback(callback, userdata);
}

uint32_t UnRegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback)
{
    return PowerManagerClient::Instance().UnRegisterDeepSleepTimeoutCallback(callback);
}

uint32_t RegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterNetworkStandbyModeChangedCallback(callback, userdata);
}

uint32_t UnRegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback)
{
    return PowerManagerClient::Instance().UnRegisterNetworkStandbyModeChangedCallback(callback);
}

uint32_t RegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterThermalModeChangedCallback(callback, userdata);
}

uint32_t UnRegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback)
{
    return PowerManagerClient::Instance().UnRegisterThermalModeChangedCallback(callback);
}

uint32_t RegisterRebootBeginCallback(PowerManager_RebootBeginCb callback, void* userdata)
{
    return PowerManagerClient::Instance().RegisterRebootBeginCallback(callback, userdata);
}

uint32_t UnRegisterRebootBeginCallback(PowerManager_RebootBeginCb callback)
{
    return PowerManagerClient::Instance().UnRegisterRebootBeginCallback(callback);
}
void PowerManager_Dispose()
{
    PowerManagerClient::Dispose();
}
} // extern "C"
