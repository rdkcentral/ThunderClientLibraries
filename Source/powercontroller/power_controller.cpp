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

// std includes
#include <iostream>
#include <unordered_map>

// Thunder includes
#include <interfaces/IPowerManager.h>
#include <plugins/Types.h>

#include "power_controller.h"

using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using WakeupSrcType = WPEFramework::Exchange::IPowerManager::WakeupSrcType;
using WakeupReason = WPEFramework::Exchange::IPowerManager::WakeupReason;
using SystemMode = WPEFramework::Exchange::IPowerManager::SystemMode;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

namespace /*unnamed*/ {

const std::unordered_map<PowerState, PowerController_PowerState_t>& powerStateMap()
{
    static const std::unordered_map<PowerState, PowerController_PowerState_t> map = {
        { PowerState::POWER_STATE_UNKNOWN, POWER_STATE_UNKNOWN },
        { PowerState::POWER_STATE_OFF, POWER_STATE_OFF },
        { PowerState::POWER_STATE_STANDBY, POWER_STATE_STANDBY },
        { PowerState::POWER_STATE_ON, POWER_STATE_ON },
        { PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, POWER_STATE_STANDBY_LIGHT_SLEEP },
        { PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, POWER_STATE_STANDBY_DEEP_SLEEP },
    };

    return map;
}

PowerController_PowerState_t convert(const PowerState from)
{
    auto& map = powerStateMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : POWER_STATE_UNKNOWN;
}

PowerState convert(const PowerController_PowerState_t from)
{
    auto& map = powerStateMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return PowerState::POWER_STATE_UNKNOWN;
}

const std::unordered_map<WakeupSrcType, PowerController_WakeupSrcType_t>& wakeupSrcTypeMap()
{
    static const std::unordered_map<WakeupSrcType, PowerController_WakeupSrcType_t> map = {
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

PowerController_WakeupSrcType_t convert(const WakeupSrcType from)
{
    auto& map = wakeupSrcTypeMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : WAKEUP_SRC_UNKNOWN;
}

WakeupSrcType convert(const PowerController_WakeupSrcType_t from)
{
    auto& map = wakeupSrcTypeMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return WakeupSrcType::WAKEUP_SRC_UNKNOWN;
}

const std::unordered_map<WakeupReason, PowerController_WakeupReason_t>& wakeupReasonMap()
{
    static const std::unordered_map<WakeupReason, PowerController_WakeupReason_t> map = {
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

PowerController_WakeupReason_t convert(const WakeupReason from)
{
    auto& map = wakeupReasonMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : WAKEUP_REASON_UNKNOWN;
}

WakeupReason convert(const PowerController_WakeupReason_t from)
{
    auto& map = wakeupReasonMap();

    for (const auto& kv : map) {
        if (kv.second == from) {
            return kv.first;
        }
    }
    return WakeupReason::WAKEUP_REASON_UNKNOWN;
}

const std::unordered_map<PowerController_SystemMode_t, SystemMode>& systemModeMap()
{
    static const std::unordered_map<PowerController_SystemMode_t, SystemMode> map = {
        { SYSTEM_MODE_UNKNOWN, SystemMode::SYSTEM_MODE_UNKNOWN },
        { SYSTEM_MODE_NORMAL, SystemMode::SYSTEM_MODE_NORMAL },
        { SYSTEM_MODE_EAS, SystemMode::SYSTEM_MODE_EAS },
        { SYSTEM_MODE_WAREHOUSE, SystemMode::SYSTEM_MODE_WAREHOUSE },
    };
    return map;
}

SystemMode convert(const PowerController_SystemMode_t from)
{
    auto& map = systemModeMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : SystemMode::SYSTEM_MODE_UNKNOWN;
}

const std::unordered_map<ThermalTemperature, PowerController_ThermalTemperature_t>& thermalTemperatureMap()
{
    static const std::unordered_map<ThermalTemperature, PowerController_ThermalTemperature_t> map = {
        { ThermalTemperature::THERMAL_TEMPERATURE_UNKNOWN, THERMAL_TEMPERATURE_UNKNOWN },
        { ThermalTemperature::THERMAL_TEMPERATURE_NORMAL, THERMAL_TEMPERATURE_NORMAL },
        { ThermalTemperature::THERMAL_TEMPERATURE_HIGH, THERMAL_TEMPERATURE_HIGH },
        { ThermalTemperature::THERMAL_TEMPERATURE_CRITICAL, THERMAL_TEMPERATURE_CRITICAL },
    };
    return map;
}

PowerController_ThermalTemperature_t convert(const ThermalTemperature from)
{
    auto& map = thermalTemperatureMap();
    auto it = map.find(from);
    return (it != map.end()) ? it->second : THERMAL_TEMPERATURE_UNKNOWN;
}

static constexpr const TCHAR callSign[] = _T("org.rdk.PowerManager");

class PowerController : public RPC::SmartInterfaceType<Exchange::IPowerManager> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPowerManager>;
    using OperationalStateChangeCallbacks = std::map<PowerController_OperationalStateChangeCb, void*>;
    using PowerModeChangedCallbacks = std::map<PowerController_PowerModeChangedCb, void*>;
    using PowerModePreChangeCallbacks = std::map<PowerController_PowerModePreChangeCb, void*>;
    using DeepSleepTimeoutCallbacks = std::map<PowerController_DeepSleepTimeoutCb, void*>;
    using NetworkStandbyModeChangedCallbacks = std::map<PowerController_NetworkStandbyModeChangedCb, void*>;
    using ThermalModeChangedCallbacks = std::map<PowerController_ThermalModeChangedCb, void*>;
    using RebootBeginCallbacks = std::map<PowerController_RebootBeginCb, void*>;

    class Notification : public Exchange::IPowerManager::INotification {
    private:
        PowerController& _parent;

    public:
        Notification(PowerController& parent)
            : _parent(parent)
        {
        }

        Notification(const Notification&) = delete; // Delete copy constructor
        Notification& operator=(const Notification&) = delete; // Delete copy assignment operator

        Notification(Notification&&) = delete; // Delete move constructor
        Notification& operator=(Notification&&) = delete; // Delete move assignment operator

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

    PowerController()
        : BaseClass()
        , _powerManagerInterface(nullptr)
        , _powerManagerNotification(*this)
        , _shutdown(false)
    {
        uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callSign);
        ASSERT(Core::ERROR_NONE == res);
        if (Core::ERROR_NONE != res) {
            std::cerr << "Unexpected, /tmp/communicator comm channel is always expected\n";
        }
    }

    ~PowerController()
    {
        _shutdown = true;
        /* Close destroys _powerManagerInterface too */
        BaseClass::Close(Core::infinite);
    }

    virtual void Operational(const bool upAndRunning) override
    {
        _lock.Lock();

        if (upAndRunning) {
            // Communicatior opened && PowerManager is Activated
            if (_powerManagerInterface == nullptr) {
                _powerManagerInterface = BaseClass::Interface();
                if (_powerManagerInterface != nullptr) {
                    _powerManagerInterface->Register(&_powerManagerNotification);
                } else {
                    // Internal error powerManager is running, but QueryInterface failed for it ?
                    std::cerr << "Unexpected, powerManager is activated, but interface null ?\n";
                }
            }
        } else {
            // PowerManager is Deactivated || Communicator closed
            if (_powerManagerInterface != nullptr) {
                _powerManagerInterface->Unregister(&_powerManagerNotification);
                _powerManagerInterface->Release();
                _powerManagerInterface = nullptr;
            } else {
                std::cerr << "Unexpected, powerManager just deactivated, but interface already null ?\n";
            }
        }

        // avoid notifying operational state changed if shuting down because of Term
        if (!_shutdown) {
            for (auto& index : _operationalStateChangeCallbacks) {
                index.first(upAndRunning, index.second);
            }
        }

        _lock.Unlock();
    }

public:
    static void Init()
    {
        _lock.Lock();
        if (nullptr == _instance) {
            ASSERT(0 == _nClients);
            _instance = new PowerController();
        }
        _nClients++;
        _lock.Unlock();
    }

    static void Term()
    {
        _lock.Lock();
        if (_nClients > 0) {
            _nClients--;
        }
        if (0 == _nClients && nullptr != _instance) {
            delete _instance;
            _instance = nullptr;
        }
        _lock.Unlock();
    }

    static PowerController& Instance()
    {
        ASSERT(nullptr != _instance);
        return *_instance;
    }

    uint32_t GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        PowerState currentState_ = PowerState::POWER_STATE_UNKNOWN;
        PowerState previousState_ = PowerState::POWER_STATE_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerState(currentState_, previousState_);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *currentState = convert(currentState_);
            *previousState = convert(previousState_);
        }

        return result;
    }

    uint32_t SetPowerState(const int keyCode, const PowerController_PowerState_t powerState, const char* reason)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        PowerState powerState_ = convert(powerState);

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetPowerState(keyCode, powerState_, reason);
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

    uint32_t GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        WakeupReason wakeupReason_ = WakeupReason::WAKEUP_REASON_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetLastWakeupReason(wakeupReason_);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *wakeupReason = convert(wakeupReason_);
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

    uint32_t SetSystemMode(const PowerController_SystemMode_t currentMode, const PowerController_SystemMode_t newMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        SystemMode currentMode_ = convert(currentMode);
        SystemMode newMode_ = convert(newMode);

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetSystemMode(currentMode_, newMode_);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        PowerState powerStateBeforeReboot_ = PowerState::POWER_STATE_UNKNOWN;

        _lock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerStateBeforeReboot(powerStateBeforeReboot_);
        }

        _lock.Unlock();

        if (Core::ERROR_NONE == result) {
            *powerStateBeforeReboot = convert(powerStateBeforeReboot_);
        }

        return result;
    }

    void NotifyPowerModeChanged(const PowerState& currentState, const PowerState& newState)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _lock.Lock();

        for (auto& index : _powerModeChangedCallbacks) {
            index.first(currentState_, newState_, index.second);
        }

        _lock.Unlock();
    }

    void NotifyPowerModePreChange(const PowerState& currentState, const PowerState& newState)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _lock.Lock();

        for (auto& index : _powerModePreChangeCallbacks) {
            index.first(currentState_, newState_, index.second);
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
        PowerController_ThermalTemperature_t currentThermalLevel_ = convert(currentThermalLevel);
        PowerController_ThermalTemperature_t newThermalLevel_ = convert(newThermalLevel);

        _lock.Lock();

        for (auto& index : _thermalModeChangedCallbacks) {
            index.first(currentThermalLevel_, newThermalLevel_, currentTemperature, index.second);
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

    uint32_t RegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
    {
        return RegisterCallback(_operationalStateChangeCallbacks, callback, userdata);
    }

    uint32_t UnRegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback)
    {
        return UnRegisterCallback(_operationalStateChangeCallbacks, callback);
    }

    uint32_t RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_powerModeChangedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
    {
        return UnRegisterCallback(_powerModeChangedCallbacks, callback);
    }

    uint32_t RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
    {
        return RegisterCallback(_powerModePreChangeCallbacks, callback, userdata);
    }

    uint32_t UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
    {
        return UnRegisterCallback(_powerModePreChangeCallbacks, callback);
    }

    uint32_t RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
    {
        return RegisterCallback(_deepSleepTimeoutCallbacks, callback, userdata);
    }

    uint32_t UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
    {
        return UnRegisterCallback(_deepSleepTimeoutCallbacks, callback);
    }

    uint32_t RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_networkStandbyModeChangedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
    {
        return UnRegisterCallback(_networkStandbyModeChangedCallbacks, callback);
    }

    uint32_t RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_thermalModeChangedCallbacks, callback, userdata);
    }

    uint32_t UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
    {
        return UnRegisterCallback(_thermalModeChangedCallbacks, callback);
    }

    uint32_t RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
    {
        return RegisterCallback(_rebootBeginCallbacks, callback, userdata);
    }

    uint32_t UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
    {
        return UnRegisterCallback(_rebootBeginCallbacks, callback);
    }

private:
    static int _nClients; // Init() count
    static PowerController* _instance;
    static Core::CriticalSection _lock;

    Exchange::IPowerManager* _powerManagerInterface; // remote PowerManager plugin interface
    Core::Sink<Notification> _powerManagerNotification;

    // containers for notification registertion
    OperationalStateChangeCallbacks _operationalStateChangeCallbacks;
    PowerModeChangedCallbacks _powerModeChangedCallbacks;
    PowerModePreChangeCallbacks _powerModePreChangeCallbacks;
    DeepSleepTimeoutCallbacks _deepSleepTimeoutCallbacks;
    NetworkStandbyModeChangedCallbacks _networkStandbyModeChangedCallbacks;
    ThermalModeChangedCallbacks _thermalModeChangedCallbacks;
    RebootBeginCallbacks _rebootBeginCallbacks;

    bool _shutdown;
};
} // nameless namespace

PowerController* PowerController::_instance = nullptr;
int PowerController::_nClients = 0;
Core::CriticalSection PowerController::_lock;

extern "C" {

void PowerController_Init()
{
    PowerController::Init();
}

void PowerController_Term()
{
    PowerController::Term();
}

bool PowerController_IsOperational()
{
    return PowerController::Instance().IsOperational();
}

uint32_t PowerController_GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
{
    ASSERT(currentState != nullptr);
    ASSERT(previousState != nullptr);
    return PowerController::Instance().GetPowerState(currentState, previousState);
}

uint32_t PowerController_SetPowerState(const int keyCode, const PowerController_PowerState_t powerstate, const char* reason)
{
    return PowerController::Instance().SetPowerState(keyCode, powerstate, reason);
}

uint32_t PowerController_GetThermalState(float* currentTemperature)
{
    ASSERT(currentTemperature != nullptr);
    return PowerController::Instance().GetThermalState(currentTemperature);
}

uint32_t PowerController_SetTemperatureThresholds(float high, float critical)
{
    return PowerController::Instance().SetTemperatureThresholds(high, critical);
}

uint32_t PowerController_GetTemperatureThresholds(float* high, float* critical)
{
    ASSERT(high != nullptr);
    ASSERT(critical != nullptr);
    return PowerController::Instance().GetTemperatureThresholds(high, critical);
}

uint32_t PowerController_SetOvertempGraceInterval(const int graceInterval)
{
    return PowerController::Instance().SetOvertempGraceInterval(graceInterval);
}

uint32_t PowerController_GetOvertempGraceInterval(int* graceInterval /* @out */)
{
    ASSERT(graceInterval != nullptr);
    return PowerController::Instance().GetOvertempGraceInterval(graceInterval);
}

uint32_t PowerController_SetDeepSleepTimer(const int timeOut)
{
    return PowerController::Instance().SetDeepSleepTimer(timeOut);
}

uint32_t PowerController_GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
{
    ASSERT(wakeupReason != nullptr);
    return PowerController::Instance().GetLastWakeupReason(wakeupReason);
}

uint32_t PowerController_GetLastWakeupKeyCode(int* keycode)
{
    ASSERT(keycode != nullptr);
    return PowerController::Instance().GetLastWakeupKeyCode(keycode);
}

uint32_t PowerController_Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
{
    ASSERT(rebootRequestor != nullptr);
    ASSERT(rebootReasonCustom != nullptr);
    ASSERT(rebootReasonOther != nullptr);
    return PowerController::Instance().Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
}

uint32_t PowerController_SetNetworkStandbyMode(const bool standbyMode)
{
    return PowerController::Instance().SetNetworkStandbyMode(standbyMode);
}

uint32_t PowerController_GetNetworkStandbyMode(bool* standbyMode)
{
    ASSERT(standbyMode != nullptr);
    return PowerController::Instance().GetNetworkStandbyMode(standbyMode);
}

uint32_t PowerController_SetWakeupSrcConfig(const int powerMode, const int wakeSrcType, int config)
{
    return PowerController::Instance().SetWakeupSrcConfig(powerMode, wakeSrcType, config);
}

uint32_t PowerController_GetWakeupSrcConfig(int* powerMode, int* srcType, int* config)
{
    ASSERT(powerMode != nullptr);
    ASSERT(srcType != nullptr);
    ASSERT(config != nullptr);
    return PowerController::Instance().GetWakeupSrcConfig(*powerMode, *srcType, *config);
}

uint32_t PowerController_SetSystemMode(const PowerController_SystemMode_t currentMode, const PowerController_SystemMode_t newMode)
{
    return PowerController::Instance().SetSystemMode(currentMode, newMode);
}

uint32_t PowerController_GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
{
    ASSERT(powerStateBeforeReboot != nullptr);
    return PowerController::Instance().GetPowerStateBeforeReboot(powerStateBeforeReboot);
}

uint32_t PowerController_RegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
{
    return PowerController::Instance().RegisterOperationalStateChangedCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback)
{
    return PowerController::Instance().UnRegisterOperationalStateChangedCallback(callback);
}

uint32_t PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
{
    return PowerController::Instance().RegisterPowerModeChangedCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
{
    return PowerController::Instance().UnRegisterPowerModeChangedCallback(callback);
}

uint32_t PowerController_RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
{
    return PowerController::Instance().RegisterPowerModePreChangeCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
{
    return PowerController::Instance().UnRegisterPowerModePreChangeCallback(callback);
}

uint32_t PowerController_RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
{
    return PowerController::Instance().RegisterDeepSleepTimeoutCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
{
    return PowerController::Instance().UnRegisterDeepSleepTimeoutCallback(callback);
}

uint32_t PowerController_RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
{
    return PowerController::Instance().RegisterNetworkStandbyModeChangedCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
{
    return PowerController::Instance().UnRegisterNetworkStandbyModeChangedCallback(callback);
}

uint32_t PowerController_RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
{
    return PowerController::Instance().RegisterThermalModeChangedCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
{
    return PowerController::Instance().UnRegisterThermalModeChangedCallback(callback);
}

uint32_t PowerController_RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
{
    return PowerController::Instance().RegisterRebootBeginCallback(callback, userdata);
}

uint32_t PowerController_UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
{
    return PowerController::Instance().UnRegisterRebootBeginCallback(callback);
}

} // extern "C"
