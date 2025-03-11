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
#include <algorithm>
#include <iostream>
#include <list>
#include <type_traits>

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

// Templated Clallback list avoid code duplication, for individual callback types
// This class expects mechanism to register / unregister for individual & unique notifications with PowerManager
// via RegisterNotificationLocked and UnregisterNotificationLocked methods. To be implemented in PowerController (i,e PARENT)
template <typename CallbackType, typename PARENT>
class CallbackList : public std::list<CallbackType> {
    PARENT& _parent;
    bool _registered;

public:
    CallbackList(PARENT& parent)
        : _parent(parent)
        , _registered(false)
    {
    }

    // Locked method expected to be called from locked context
    uint32_t RegisterCallbackLocked(typename CallbackType::Type callback, void* userdata)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        auto it = std::find_if(this->begin(), this->end(), [&callback](const CallbackType& cb) {
            return cb.callback == callback;
        });

        if (it == this->end()) {
            this->emplace_back(callback, userdata);
            result = Core::ERROR_NONE;
            RegisterNotificationLocked();
        }

        return result;
    }

    // Locked method expected to be called from locked context
    uint32_t UnRegisterCallbackLocked(typename CallbackType::Type callback)
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        auto it = std::find_if(this->begin(), this->end(), [&callback](const CallbackType& cb) {
            return cb.callback == callback;
        });

        if (it != this->end()) {
            this->erase(it);
            result = Core::ERROR_NONE;
            UnregisterNotificationLocked(false);
        }

        return result;
    }

    // Locked method expected to be called from locked context
    inline void RegisterNotificationLocked()
    {
        if (!_registered && !this->empty() && _parent.IsActivatedLocked()) {
            _registered = _parent.template RegisterNotificationLocked<CallbackType>();
        }
    }

    // Locked method expected to be called from locked context
    // @param forced A boolean indicating whether to forcefully unregister from notification
    //               regardless of the callback list's state / unregister status.
    //               This is required to handle PowerManager restart scenarios.
    inline void UnregisterNotificationLocked(const bool forced)
    {
        if (_registered && _parent.IsActivatedLocked() && (forced || this->empty())) {
            bool unregistered = _parent.template UnregisterNotificationLocked<CallbackType>();

            // ---------------------------------------
            // | forced | unregistered | _registered |
            // |--------|--------------|-------------|
            // |   0    |       0      |      1      |
            // |   0    |       1      |      0      |
            // |   1    |       0      |      0      |
            // |   1    |       1      |      0      |
            // ---------------------------------------
            _registered = !forced && !unregistered;
        }
    }
};

struct OperationalStateChangeCb {
    using Type = PowerController_OperationalStateChangeCb;
    Type callback;
    void* userdata;

    OperationalStateChangeCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct NetworkStandbyModeChangedCb {
    using Type = PowerController_NetworkStandbyModeChangedCb;
    Type callback;
    void* userdata;

    NetworkStandbyModeChangedCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct PowerModePreChangedCb {
    using Type = PowerController_PowerModePreChangeCb;
    Type callback;
    void* userdata;

    PowerModePreChangedCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct PowerModeChangedCb {
    using Type = PowerController_PowerModeChangedCb;
    Type callback;
    void* userdata;

    PowerModeChangedCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct DeepSleepTimeoutCb {
    using Type = PowerController_DeepSleepTimeoutCb;
    Type callback;
    void* userdata;

    DeepSleepTimeoutCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct ThermalModeChangedCb {
    using Type = PowerController_ThermalModeChangedCb;
    Type callback;
    void* userdata;

    ThermalModeChangedCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

struct RebootBeginCb {
    using Type = PowerController_RebootBeginCb;
    Type callback;
    void* userdata;

    RebootBeginCb(Type cb, void* ud)
        : callback(cb)
        , userdata(ud)
    {
    }
};

class PowerController : public RPC::SmartInterfaceType<Exchange::IPowerManager> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPowerManager>;
    using OperationalStateChangeCallbacks = CallbackList<OperationalStateChangeCb, PowerController>;
    using PowerModePreChangeCallbacks = CallbackList<PowerModePreChangedCb, PowerController>;
    using PowerModeChangedCallbacks = CallbackList<PowerModeChangedCb, PowerController>;
    using DeepSleepTimeoutCallbacks = CallbackList<DeepSleepTimeoutCb, PowerController>;
    using NetworkStandbyModeChangedCallbacks = CallbackList<NetworkStandbyModeChangedCb, PowerController>;
    using ThermalModeChangedCallbacks = CallbackList<ThermalModeChangedCb, PowerController>;
    using RebootBeginCallbacks = CallbackList<RebootBeginCb, PowerController>;

    class Notification : public Exchange::IPowerManager::IRebootNotification,
                         public Exchange::IPowerManager::IModeChangedNotification,
                         public Exchange::IPowerManager::IModePreChangeNotification,
                         public Exchange::IPowerManager::IDeepSleepTimeoutNotification,
                         public Exchange::IPowerManager::INetworkStandbyModeChangedNotification,
                         public Exchange::IPowerManager::IThermalModeChangedNotification {
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

        virtual void OnPowerModePreChange(const PowerState& currentState, const PowerState& newState, const int transactionId, const int stateChangeAfter) override
        {
            _parent.NotifyPowerModePreChange(currentState, newState, transactionId, stateChangeAfter);
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
        INTERFACE_ENTRY(Exchange::IPowerManager::IRebootNotification)
        INTERFACE_ENTRY(Exchange::IPowerManager::IModePreChangeNotification)
        INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
        INTERFACE_ENTRY(Exchange::IPowerManager::IDeepSleepTimeoutNotification)
        INTERFACE_ENTRY(Exchange::IPowerManager::INetworkStandbyModeChangedNotification)
        INTERFACE_ENTRY(Exchange::IPowerManager::IThermalModeChangedNotification)
        END_INTERFACE_MAP

        template <typename T>
        inline T* baseInterface()
        {
            static_assert(std::is_base_of<T, Notification>(), "base type mismatch");
            return static_cast<T*>(this);
        }
    };

    PowerController()
        : BaseClass()
        , _powerManagerInterface(nullptr)
        , _powerManagerNotification(*this)
        , _operationalStateChangeCallbacks(*this)
        , _powerModePreChangeCallbacks(*this)
        , _powerModeChangedCallbacks(*this)
        , _deepSleepTimeoutCallbacks(*this)
        , _networkStandbyModeChangedCallbacks(*this)
        , _thermalModeChangedCallbacks(*this)
        , _rebootBeginCallbacks(*this)
        , _shutdown(false)
    {
        Connect();
    }

    ~PowerController()
    {
        _shutdown = true;
        /* Close destroys _powerManagerInterface too */
        BaseClass::Close(Core::infinite);
    }

    virtual void Operational(const bool upAndRunning) override
    {
        _apiLock.Lock();

        // avoid misleading log during shutdown
        if (!upAndRunning && _shutdown) {
            std::cout << "PowerController::Operational (" << callSign << ") " << upAndRunning << std::endl;
        }

        if (upAndRunning) {
            // Communicatior opened && PowerManager is Activated
            if (nullptr == _powerManagerInterface) {
                _powerManagerInterface = BaseClass::Interface();
                if (_powerManagerInterface != nullptr) {
                    RegisterNotificationsLocked();
                    std::cout << "PowerController successfully established COM-RPC connection with PowerManager plugin\n";
                } else {
                    // Internal error powerManager is running, but QueryInterface failed for it ?
                    std::cerr << "PowerController failed to establish COM-RPC connection with PowerManager plugin\n";
                }
            }
        } else {
            // PowerManager is Deactivated || Communicator closed
            if (nullptr != _powerManagerInterface) {
                UnregisterNotificationsLocked();
                _powerManagerInterface->Release();
                _powerManagerInterface = nullptr;
            } else {
                std::cerr << "Unexpected, powerManager just deactivated, but interface already null ?\n";
            }
        }
        _apiLock.Unlock();

        _callbackLock.Lock();
        // avoid notifying operational state changed if shuting down because of Term
        if (!_shutdown) {
            for (auto& cb : _operationalStateChangeCallbacks) {
                cb.callback(upAndRunning, cb.userdata);
            }
        }
        _callbackLock.Unlock();
    }

    // Locked method expected to be called from locked context
    void RegisterNotificationsLocked()
    {
        _powerModeChangedCallbacks.RegisterNotificationLocked();
        _powerModePreChangeCallbacks.RegisterNotificationLocked();
        _deepSleepTimeoutCallbacks.RegisterNotificationLocked();
        _networkStandbyModeChangedCallbacks.RegisterNotificationLocked();
        _thermalModeChangedCallbacks.RegisterNotificationLocked();
        _rebootBeginCallbacks.RegisterNotificationLocked();
    }

    // Locked method expected to be called from locked context
    void UnregisterNotificationsLocked()
    {
        _powerModeChangedCallbacks.UnregisterNotificationLocked(true);
        _powerModePreChangeCallbacks.UnregisterNotificationLocked(true);
        _deepSleepTimeoutCallbacks.UnregisterNotificationLocked(true);
        _networkStandbyModeChangedCallbacks.UnregisterNotificationLocked(true);
        _thermalModeChangedCallbacks.UnregisterNotificationLocked(true);
        _rebootBeginCallbacks.UnregisterNotificationLocked(true);
    }

    inline bool IsConnected() const
    {
        return (~0 != ConnectionId());
    }

public:
    // Locked method expected to be called from locked context
    inline bool IsActivatedLocked() const
    {
        return (nullptr != _powerManagerInterface);
    }

    uint32_t Connect()
    {
        uint32_t status = Core::ERROR_NONE;
        std::string errMsg = "";

        _apiLock.Lock();
        do {
            if (!IsConnected()) {
                uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callSign);
                if (Core::ERROR_NONE != res) {
                    std::cerr << "/tmp/communicator com channel open failed. Is Thunder running?\n";
                    errMsg = "COM-RPC channel open failed";
                    status = Core::ERROR_UNAVAILABLE;
                    break;
                }
            }

            if (nullptr == _powerManagerInterface) {
                errMsg = "PowerManager plugin is not activated yet";
                status = Core::ERROR_NOT_EXIST;
            }
        } while (false);

        _apiLock.Unlock();

        std::cout << "PowerController::Connect (" << callSign << ") status: " << status << ", errMsg: \"" << errMsg << "\"" << std::endl;

        return status;
    }

    // Locked method expected to be called from locked context (take care in specializations too)
    template <typename CallbackType>
    bool RegisterNotificationLocked()
    {
        // static_assert(std::false_type::value, "Specialization required for CallbackType");
        return false;
    }

    // Locked method expected to be called from locked context (take care in specializations too)
    template <typename CallbackType>
    bool UnregisterNotificationLocked()
    {
        // static_assert(std::false_type::value, "Specialization required for CallbackType");
        return false;
    }

    static void Init()
    {
        _apiLock.Lock();
        if (nullptr == _instance) {
            ASSERT(0 == _nClients);
            _instance = new PowerController();
        }
        _nClients++;
        _apiLock.Unlock();
    }

    static void Term()
    {
        _apiLock.Lock();
        if (_nClients > 0) {
            _nClients--;
        }
        if (0 == _nClients && nullptr != _instance) {
            delete _instance;
            _instance = nullptr;
        }
        _apiLock.Unlock();
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

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerState(currentState_, previousState_);
        }

        _apiLock.Unlock();

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

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetPowerState(keyCode, powerState_, reason);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetThermalState(float* currentTemperature)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetThermalState(*currentTemperature);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetTemperatureThresholds(float high, float critical)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetTemperatureThresholds(high, critical);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetTemperatureThresholds(float* high, float* critical)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetTemperatureThresholds(*high, *critical);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetOvertempGraceInterval(const int graceInterval)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetOvertempGraceInterval(graceInterval);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetOvertempGraceInterval(int* graceInterval)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetOvertempGraceInterval(*graceInterval);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetDeepSleepTimer(const int timeOut)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetDeepSleepTimer(timeOut);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        WakeupReason wakeupReason_ = WakeupReason::WAKEUP_REASON_UNKNOWN;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetLastWakeupReason(wakeupReason_);
        }

        _apiLock.Unlock();

        if (Core::ERROR_NONE == result) {
            *wakeupReason = convert(wakeupReason_);
        }

        return result;
    }

    uint32_t GetLastWakeupKeyCode(int* keycode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            _powerManagerInterface->GetLastWakeupKeyCode(*keycode);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetNetworkStandbyMode(const bool standbyMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetNetworkStandbyMode(standbyMode);
        }

        _apiLock.Unlock();

        return result;
    }
    uint32_t GetNetworkStandbyMode(bool* standbyMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetNetworkStandbyMode(*standbyMode);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetWakeupSrcConfig(const int powerMode, const int wakeSrcType, int config)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetWakeupSrcConfig(powerMode, wakeSrcType, config);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetWakeupSrcConfig(int& powerMode, int& srcType, int& config)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetWakeupSrcConfig(powerMode, srcType, config);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t SetSystemMode(const PowerController_SystemMode_t currentMode, const PowerController_SystemMode_t newMode)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        SystemMode currentMode_ = convert(currentMode);
        SystemMode newMode_ = convert(newMode);

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetSystemMode(currentMode_, newMode_);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        PowerState powerStateBeforeReboot_ = PowerState::POWER_STATE_UNKNOWN;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerStateBeforeReboot(powerStateBeforeReboot_);
        }

        _apiLock.Unlock();

        if (Core::ERROR_NONE == result) {
            *powerStateBeforeReboot = convert(powerStateBeforeReboot_);
        }

        return result;
    }

    uint32_t AddPowerModePreChangeClient(const std::string& clientName, uint32_t& clientId)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->AddPowerModePreChangeClient(clientName, clientId);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t RemovePowerModePreChangeClient(const uint32_t clientId)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->RemovePowerModePreChangeClient(clientId);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delay)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->DelayPowerModeChangeBy(clientId, transactionId, delay);
        }

        _apiLock.Unlock();

        return result;
    }

    uint32_t PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->PowerModePreChangeComplete(clientId, transactionId);
        }

        _apiLock.Unlock();

        return result;
    }

    void NotifyPowerModeChanged(const PowerState& currentState, const PowerState& newState)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _callbackLock.Lock();

        for (auto& cb : _powerModeChangedCallbacks) {
            cb.callback(currentState_, newState_, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyPowerModePreChange(const PowerState& currentState, const PowerState& newState, const int transactionId, const int stateChangeAfter)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _callbackLock.Lock();

        for (auto& cb : _powerModePreChangeCallbacks) {
            cb.callback(currentState_, newState_, transactionId, stateChangeAfter, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyDeepSleepTimeout(const int& wakeupTimeout)
    {
        _callbackLock.Lock();

        for (auto& cb : _deepSleepTimeoutCallbacks) {
            cb.callback(wakeupTimeout, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyNetworkStandbyModeChanged(const bool& enabled)
    {
        _callbackLock.Lock();

        for (auto& cb : _networkStandbyModeChangedCallbacks) {
            cb.callback(enabled, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyThermalModeChanged(const ThermalTemperature& currentThermalLevel, const ThermalTemperature& newThermalLevel, const float& currentTemperature)
    {
        PowerController_ThermalTemperature_t currentThermalLevel_ = convert(currentThermalLevel);
        PowerController_ThermalTemperature_t newThermalLevel_ = convert(newThermalLevel);

        _callbackLock.Lock();

        for (auto& cb : _thermalModeChangedCallbacks) {
            cb.callback(currentThermalLevel_, newThermalLevel_, currentTemperature, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyRebootBegin(const string& rebootReasonCustom, const string& rebootReasonOther, const string& rebootRequestor)
    {
        _callbackLock.Lock();

        for (auto& cb : _rebootBeginCallbacks) {
            cb.callback(rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str(), cb.userdata);
        }

        _callbackLock.Unlock();
    }

    // Generic template function for callback register
    template <typename CallbackType, typename PARENT>
    uint32_t RegisterCallback(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback, void* userdata)
    {
        uint32_t result = Core::ERROR_INVALID_PARAMETER;

        ASSERT(nullptr != callback);

        if (nullptr != callback) {
            _callbackLock.Lock();

            result = callbacklist.RegisterCallbackLocked(callback, userdata);

            _callbackLock.Unlock();
        }

        return result;
    }

    // Generic template function for callback unregister
    template <typename CallbackType, typename PARENT>
    uint32_t UnRegisterCallback(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback)
    {
        uint32_t result = Core::ERROR_INVALID_PARAMETER;

        ASSERT(nullptr != callback);

        if (nullptr != callback) {
            _callbackLock.Lock();

            result = callbacklist.UnRegisterCallbackLocked(callback);

            _callbackLock.Unlock();
        }

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
    static Core::CriticalSection _apiLock;
    static Core::CriticalSection _callbackLock;

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
template <>
bool PowerController::RegisterNotificationLocked<OperationalStateChangeCb>()
{
    // Operational state change notification is managed by SmartInterfaceType
    return true;
}

template <>
bool PowerController::RegisterNotificationLocked<PowerModePreChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModePreChangeNotification>());
}

template <>
bool PowerController::RegisterNotificationLocked<PowerModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
}

template <>
bool PowerController::RegisterNotificationLocked<DeepSleepTimeoutCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IDeepSleepTimeoutNotification>());
}

template <>
bool PowerController::RegisterNotificationLocked<NetworkStandbyModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
}

template <>
bool PowerController::RegisterNotificationLocked<ThermalModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IThermalModeChangedNotification>());
}

template <>
bool PowerController::RegisterNotificationLocked<RebootBeginCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IRebootNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<OperationalStateChangeCb>()
{
    // Operational state change notification is managed by SmartInterfaceType
    return true;
}

template <>
bool PowerController::UnregisterNotificationLocked<PowerModePreChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModePreChangeNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<PowerModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<DeepSleepTimeoutCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IDeepSleepTimeoutNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<NetworkStandbyModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<ThermalModeChangedCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IThermalModeChangedNotification>());
}

template <>
bool PowerController::UnregisterNotificationLocked<RebootBeginCb>()
{
    return Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IRebootNotification>());
}

} // nameless namespace

/* static */ PowerController* PowerController::_instance = nullptr;
/* static */ int PowerController::_nClients = 0;
/* static */ Core::CriticalSection PowerController::_apiLock;
/* static */ Core::CriticalSection PowerController::_callbackLock;

extern "C" {

void PowerController_Init()
{
    PowerController::Init();
}

void PowerController_Term()
{
    PowerController::Term();
}

uint32_t PowerController_Connect()
{
    return PowerController::Instance().Connect();
}

bool PowerController_IsOperational()
{
    return PowerController::Instance().IsOperational();
}

uint32_t PowerController_GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
{
    ASSERT(nullptr != currentState);
    ASSERT(nullptr != previousState);
    return PowerController::Instance().GetPowerState(currentState, previousState);
}

uint32_t PowerController_SetPowerState(const int keyCode, const PowerController_PowerState_t powerstate, const char* reason)
{
    return PowerController::Instance().SetPowerState(keyCode, powerstate, reason);
}

uint32_t PowerController_GetThermalState(float* currentTemperature)
{
    ASSERT(nullptr != currentTemperature);
    return PowerController::Instance().GetThermalState(currentTemperature);
}

uint32_t PowerController_SetTemperatureThresholds(float high, float critical)
{
    return PowerController::Instance().SetTemperatureThresholds(high, critical);
}

uint32_t PowerController_GetTemperatureThresholds(float* high, float* critical)
{
    ASSERT(nullptr != high);
    ASSERT(nullptr != critical);
    return PowerController::Instance().GetTemperatureThresholds(high, critical);
}

uint32_t PowerController_SetOvertempGraceInterval(const int graceInterval)
{
    return PowerController::Instance().SetOvertempGraceInterval(graceInterval);
}

uint32_t PowerController_GetOvertempGraceInterval(int* graceInterval /* @out */)
{
    ASSERT(nullptr != graceInterval);
    return PowerController::Instance().GetOvertempGraceInterval(graceInterval);
}

uint32_t PowerController_SetDeepSleepTimer(const int timeOut)
{
    return PowerController::Instance().SetDeepSleepTimer(timeOut);
}

uint32_t PowerController_GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
{
    ASSERT(nullptr != wakeupReason);
    return PowerController::Instance().GetLastWakeupReason(wakeupReason);
}

uint32_t PowerController_GetLastWakeupKeyCode(int* keycode)
{
    ASSERT(nullptr != keycode);
    return PowerController::Instance().GetLastWakeupKeyCode(keycode);
}

uint32_t PowerController_Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
{
    ASSERT(nullptr != rebootRequestor);
    ASSERT(nullptr != rebootReasonCustom);
    ASSERT(nullptr != rebootReasonOther);
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
    ASSERT(nullptr != powerMode);
    ASSERT(nullptr != srcType);
    ASSERT(nullptr != config);
    return PowerController::Instance().GetWakeupSrcConfig(*powerMode, *srcType, *config);
}

uint32_t PowerController_SetSystemMode(const PowerController_SystemMode_t currentMode, const PowerController_SystemMode_t newMode)
{
    return PowerController::Instance().SetSystemMode(currentMode, newMode);
}

uint32_t PowerController_GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
{
    ASSERT(nullptr != powerStateBeforeReboot);
    return PowerController::Instance().GetPowerStateBeforeReboot(powerStateBeforeReboot);
}

uint32_t PowerController_AddPowerModePreChangeClient(const char* clientName, uint32_t* clientId)
{
    ASSERT(nullptr != clientName);
    ASSERT(nullptr != clientId);
    return PowerController::Instance().AddPowerModePreChangeClient(clientName, *clientId);
}

uint32_t PowerController_RemovePowerModePreChangeClient(const uint32_t clientId)
{
    return PowerController::Instance().RemovePowerModePreChangeClient(clientId);
}

uint32_t PowerController_DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delayPeriod)
{
    return PowerController::Instance().DelayPowerModeChangeBy(clientId, transactionId, delayPeriod);
}

uint32_t PowerController_PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
{
    return PowerController::Instance().PowerModePreChangeComplete(clientId, transactionId);
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
