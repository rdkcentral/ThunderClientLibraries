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
#include <functional>
#include <iostream>
#include <list>
#include <thread>
#include <type_traits>

#include <sys/inotify.h>
#include <sys/syscall.h>

// Thunder includes
#include <interfaces/IPowerManager.h>
#include <plugins/Types.h>

#include "power_controller.h"

#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
#define LOGINFO(fmt, ...)                                                                                                                                                 \
    do {                                                                                                                                                                  \
        fprintf(stdout, "[PowerController][%d] INFO  [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), Thunder::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stdout);                                                                                                                                                   \
    } while (0)
#define LOGWARN(fmt, ...)                                                                                                                                                 \
    do {                                                                                                                                                                  \
        fprintf(stdout, "[PowerController][%d] WARN  [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), Thunder::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stdout);                                                                                                                                                   \
    } while (0)
#define LOGERR(fmt, ...)                                                                                                                                                  \
    do {                                                                                                                                                                  \
        fprintf(stderr, "[PowerController][%d] ERROR [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), Thunder::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                                                                                   \
    } while (0)
#else
#define LOGINFO(fmt, ...)
#define LOGWARN(fmt, ...)
#define LOGERR(fmt, ...)
#endif

using namespace Thunder;
using PowerState = Thunder::Exchange::IPowerManager::PowerState;
using WakeupSrcType = Thunder::Exchange::IPowerManager::WakeupSrcType;
using WakeupReason = Thunder::Exchange::IPowerManager::WakeupReason;
using SystemMode = Thunder::Exchange::IPowerManager::SystemMode;
using ThermalTemperature = Thunder::Exchange::IPowerManager::ThermalTemperature;

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

// Templated Callback list avoid code duplication, for individual callback types
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

#define INVALID_PID (-1)
#define INVALID_FD (-1)
#define POLL_TIMEOUT (2000)

using PIDFileChangedCb = std::function<void(const int pid)>;

class PIDFileMonitor {
    const std::string _pidFile; // pid file path `/tmp/wpeframework.pid`
    const std::string _processName; // process name extracted from pid file `wpeframework`
    PIDFileChangedCb _callback; // callback to notify PID file changes

    int _fd; // inotify fd
    int _wd; // inotify watch descriptor
    int _pid; // current PID

    bool _running; // current running state
    volatile std::atomic<bool> _shutdown; // thread shutdown flag

    std::unique_ptr<std::thread> _thread; // thread to monitor PID file changes

    std::string parseProcessName(const std::string& pidFile)
    {
        std::string processName;

        // Find the last '/' and '.pid'
        size_t slash = pidFile.find_last_of('/');
        size_t dot = pidFile.find_last_of('.');

        if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
            processName = pidFile.substr(slash + 1, dot - slash - 1);
        }

        return processName;
    }

    char* trim(char* str)
    {
        char* end;

        // Trim leading space
        while (isspace((unsigned char)*str))
            str++;

        // If the string is empty, return it
        if (*str == '\0')
            return str;

        // Trim trailing space
        end = str + strlen(str) - 1;
        while (end > str && isspace((unsigned char)*end))
            end--;

        // Null-terminate the string
        *(end + 1) = '\0';

        return str;
    }

    int parsePID(char* buf, size_t maxSz)
    {
        long pid = INVALID_PID;
        char* end = nullptr;

        do {
            // if buf is empty, avoid parsing
            if (strnlen(buf, maxSz) == 0) {
                LOGERR("empty buffer");
                break;
            }

            // trim leading/trailing spaces
            buf = trim(buf);

            // reset errno, strtol sets errno on failure
            errno = 0;

            // read as base 10
            pid = strtol(buf, &end, 10);

            if (errno) {
                LOGERR("strtol failed, err: %s", strerror(errno));
                pid = INVALID_PID;
                break;
            }

            if (!pid) {
                // strtol returns 0 if no valid conversion could be performed
                LOGERR("strtol failed, no valid conversion");
                pid = INVALID_PID;
            }
        } while (false);

        return (int)pid;
    }

    int readPID()
    {
        constexpr size_t maxSz = 64;
        int pid = INVALID_PID;

        FILE* file = fopen(_pidFile.c_str(), "r");

        do {
            // Open the file in read mode
            if (nullptr == file) {
                LOGERR("%s open failed, err: %s", _pidFile.c_str(), strerror(errno));
                break;
            }

            // Read file content (only first line)
            char buffer[maxSz] = { 0 };

            if (nullptr == fgets(buffer, maxSz, file) && !feof(file)) {
                LOGERR("%s read failed, err: %s", _pidFile.c_str(), strerror(errno));
                break;
            }

            // Convert the file content to an integer
            pid = parsePID(buffer, maxSz);

        } while (false);

        if (nullptr != file) {
            fclose(file);
        }

        LOGINFO("%s PID: %d", _pidFile.c_str(), pid);

        return pid;
    }

    int handle_inotify_event(int fd)
    {
        // buffer to read 10 inotify events at a time
        constexpr size_t maxSz = 10 * (sizeof(struct inotify_event) + 16);

        /* Some systems cannot read integer variables if they are not
           properly aligned. On other systems, incorrect alignment may
           decrease performance. Hence, the buffer used for reading from
           the inotify file descriptor should have the same alignment as
           struct inotify_event. */

        char buffer[maxSz]
            __attribute__((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event* event;
        ssize_t bytesRead;

        /* Loop while events can be read from inotify file descriptor. */
        for (;;) {

            /* Read some events. */
            bytesRead = read(fd, buffer, sizeof(buffer));
            if (bytesRead == -1 && errno != EAGAIN) {
                LOGERR("read failed, err: %s", strerror(errno));
                return bytesRead; // error
            }

            if (bytesRead <= 0)
                break;

            /* Loop over all events in the buffer. */
            for (char* ptr = buffer; ptr < buffer + bytesRead;
                ptr += sizeof(struct inotify_event) + event->len) {

                event = (const struct inotify_event*)ptr;

                if (event->mask & IN_CLOSE_WRITE) {
                    _pid = readPID();

                    LOGINFO("inotify event IN_CLOSE_WRITE, PID: %d", _pid);

                    ASSERT(INVALID_PID != _pid);

                    if (INVALID_PID != _pid) {
                        _callback(_pid);
                    }
                }
            }
        }
        return bytesRead;
    }

    void monitorThread()
    {
        ASSERT(INVALID_PID != _pid);

        LOGINFO("started, pid file: %s", _pidFile.c_str());

        do {

            _fd = inotify_init1(IN_NONBLOCK);

            if (_fd < 0) {
                LOGERR("inotify_init failed, err: %s", strerror(errno));
                _fd = INVALID_FD;
                break;
            }

            _wd = inotify_add_watch(_fd, _pidFile.c_str(), IN_CLOSE_WRITE);

            if (_wd < 0) {
                LOGERR("inotify_add_watch failed, err: %s", strerror(errno));
                _wd = INVALID_FD;
                break;
            }

            struct pollfd pfd = { .fd = _fd, .events = POLLIN };

            while (!_shutdown) {

                int poll_res = poll(&pfd, 1, POLL_TIMEOUT);

                if (poll_res == -1) {
                    if (errno == EINTR)
                        continue;

                    LOGERR("FATAL poll failed, Err[%s], fd[%d], wd[%d]", strerror(errno), _fd, _wd);
                    break;
                } else if (poll_res == 0) {
                    // timedout
                    continue;
                } else {
                    if (pfd.revents & POLLIN) {
                        /* Inotify events are available. */
                        handle_inotify_event(_fd);
                    }
                }
            }
        } while (false);

        if (INVALID_FD != _wd) {
            close(_wd);
            _wd = INVALID_FD;
        }
        if (INVALID_FD != _fd) {
            close(_fd);
            _fd = INVALID_FD;
        }
        LOGINFO("exiting\n");
    }

public:
    PIDFileMonitor(const std::string& pidFile, PIDFileChangedCb callback)
        : _pidFile(pidFile)
        , _processName(parseProcessName(pidFile))
        , _callback(std::move(callback))
        , _fd(INVALID_FD)
        , _wd(INVALID_FD)
        , _pid(INVALID_PID)
        , _shutdown(false)
        , _thread(nullptr)
    {
    }

    inline bool Running() const
    {
        return _thread && _thread->joinable();
    }

    void Run()
    {
        // ensure valid pid file & pid before starting monitor thread
        if (INVALID_PID == _pid) {
            _pid = readPID();
        }

        if (!_shutdown && !Running()) {
            _thread = std::unique_ptr<std::thread>(new std::thread(std::bind(&PIDFileMonitor::monitorThread, this)));
        }
    }

    int PID()
    {
        if (INVALID_PID == _pid) {
            _pid = readPID();
        }
        return _pid;
    }

    ~PIDFileMonitor()
    {
        LOGINFO("destructor");

        _shutdown = true;

        if (_thread) {
            if (_thread->joinable()) {
                _thread->join();
            }
            _thread.reset();
        }
    }

    PIDFileMonitor(const PIDFileMonitor&) = delete;
    PIDFileMonitor& operator=(const PIDFileMonitor&) = delete;
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

        virtual void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
        {
            _parent.NotifyPowerModeChanged(currentState, newState);
        }

        virtual void OnPowerModePreChange(const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) override
        {
            _parent.NotifyPowerModePreChange(currentState, newState, transactionId, stateChangeAfter);
        }

        virtual void OnDeepSleepTimeout(const int wakeupTimeout) override
        {
            _parent.NotifyDeepSleepTimeout(wakeupTimeout);
        }

        virtual void OnNetworkStandbyModeChanged(const bool enabled) override
        {
            _parent.NotifyNetworkStandbyModeChanged(enabled);
        }

        virtual void OnThermalModeChanged(const ThermalTemperature currentThermalLevel, const ThermalTemperature newThermalLevel, const float currentTemperature) override
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
        , _pidMonitor("/tmp/wpeframework.pid", std::bind(&PowerController::pidChangedCb, this, std::placeholders::_1))
        , _pid(INVALID_PID)
        , _connected(false)
        , _shutdown(false)
    {
        (void)Connect();
    }

    ~PowerController()
    {
        _shutdown = true;
        (void)Disconnect(); /* Internally it will Close destroys _powerManagerInterface too */
    }

    void pidChangedCb(const int pid)
    {
        LOGINFO("Thunder OLD PID: %d, NEW PID: %d", _pid, pid);

        if (_pid != pid) {
            Disconnect();
            Connect();
            _pid = pid;
        }
    }

    virtual void Operational(const bool upAndRunning) override
    {
        LOGINFO("callsign: (%s), running: %d", callSign, upAndRunning);

        _apiLock.Lock();

        // avoid misleading log during shutdown
        if ((upAndRunning && _shutdown) || !_shutdown) {
            LOGINFO("callsign: (%s), running: %d", callSign, upAndRunning);
        }

        if (upAndRunning) {
            // Communicatior opened && PowerManager is Activated
            if (nullptr == _powerManagerInterface) {
                _powerManagerInterface = BaseClass::Interface();
                if (_powerManagerInterface != nullptr) {
                    RegisterNotificationsLocked();
                    LOGINFO("Established COM-RPC connection with PowerManager plugin");
                } else {
                    // Internal error powerManager is running, but QueryInterface failed for it ?
                    LOGERR("Failed to Establish COM-RPC connection with PowerManager plugin");
                }
            }
        } else {
            // PowerManager is Deactivated || Communicator closed
            if (nullptr != _powerManagerInterface) {
                UnregisterNotificationsLocked();
                _powerManagerInterface->Release();
                _powerManagerInterface = nullptr;
            } else {
                LOGERR("Unexpected, powerManager just deactivated, but interface already null ?");
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

    inline bool isConnected() const
    {
        return _connected;
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

        LOGINFO("Entering ...");
        _apiLock.Lock();
        try {
            do {
                if (!isConnected()) {
                    LOGINFO("Opening COM-RPC channel ...");
                    uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callSign);
                    if (Core::ERROR_NONE == res) {
                        LOGINFO("COM-RPC channel opened successfully");
                        _connected = true;
                        if (!_pidMonitor.Running()) {
                            // read pid file once to get initial PID
                            _pid = _pidMonitor.PID();
                            _pidMonitor.Run();
                        }
                    }
                    else {
                        LOGWARN("COM-RPC channel open failed with status[%u]. Is Thunder running ?", res);
                        status = Core::ERROR_UNAVAILABLE;
                        break;
                    }
                }
                else {
                    LOGINFO("COM-RPC channel already open");
                }

                if (nullptr == _powerManagerInterface) {
                    LOGWARN("PowerManager plugin is not activated yet");
                    status = Core::ERROR_NOT_EXIST;
                }
            } while (false);
        }
        catch (const std::exception& ex) {
            LOGERR("Exception in PowerController::Connect [%s]", ex.what());
            status = Core::ERROR_GENERAL;
        }
        catch (...) {
            LOGERR("Unknown exception in PowerController::Connect");
            status = Core::ERROR_GENERAL;
        }
        _apiLock.Unlock();

        LOGINFO("Exiting... status[%u]", status);

        return status;
    }

    uint32_t Disconnect()
    {
        uint32_t status = Core::ERROR_GENERAL;

        LOGINFO("Entering ...");

        _apiLock.Lock();

        if (isConnected()) {
            LOGINFO("Closing COM-RPC channel ...");
            status = BaseClass::Close(Core::infinite);
            if (Core::ERROR_NONE != status) {
                LOGERR("COM-RPC channel close failed, status: %u", status);
            }
            else {
                LOGINFO("COM-RPC channel closed");
            }
            _connected = false;
        }

        _apiLock.Unlock();

        LOGINFO("Exiting ... status[%u]", status);

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
        LOGINFO("Entering ...");
        _apiLock.Lock();
        if (nullptr == _instance) {
            _instance = new PowerController();
            if (nullptr == _instance) {
                LOGERR("PowerController::Init failed, out of memory ?");
            }
        }
        _apiLock.Unlock();
        LOGINFO("Exiting ...");
    }

    static void Term()
    {
        LOGINFO("Entering ...");
        _apiLock.Lock();
        if (nullptr != _instance) {
            delete _instance;
            _instance = nullptr;
        }
        _apiLock.Unlock();
        LOGINFO("Exiting ...");
    }

    static PowerController* Instance()
    {
        return _instance;
    }

    Core::hresult GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        PowerState currentState_ = PowerState::POWER_STATE_UNKNOWN;
        PowerState previousState_ = PowerState::POWER_STATE_UNKNOWN;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerState(currentState_, previousState_);
        }
        LOGINFO("result[%d], Prev[%d], Current[%d]", result, previousState_, currentState_);

        _apiLock.Unlock();

        if (Core::ERROR_NONE == result) {
            *currentState = convert(currentState_);
            *previousState = convert(previousState_);
        }

        return result;
    }

    Core::hresult SetPowerState(const int keyCode, const PowerController_PowerState_t powerState, const char* reason)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        PowerState powerState_ = convert(powerState);

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetPowerState(keyCode, powerState_, reason);
        }
        LOGINFO("result[%d], keyCode[%d], powerState[%d], reason[%s]", result, keyCode, powerState_, reason);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult GetThermalState(float* currentTemperature)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == currentTemperature) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetThermalState(*currentTemperature);
        }
        LOGINFO("result[%d], currentTemperature[%f]", result, *currentTemperature);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult SetTemperatureThresholds(float high, float critical)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetTemperatureThresholds(high, critical);
        }
        LOGINFO("result[%d], high[%f], critical[%f]", result, high, critical);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult GetTemperatureThresholds(float* high, float* critical)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == high || nullptr == critical) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetTemperatureThresholds(*high, *critical);
        }
        LOGINFO("result[%d], high[%f], critical[%f]", result, *high, *critical);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult SetOvertempGraceInterval(const int graceInterval)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetOvertempGraceInterval(graceInterval);
        }
        LOGINFO("result[%d], graceInterval[%d]", result, graceInterval);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult GetOvertempGraceInterval(int* graceInterval)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == graceInterval) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetOvertempGraceInterval(*graceInterval);
        }
        LOGINFO("result[%d], graceInterval[%d]", result, *graceInterval);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult SetDeepSleepTimer(const int timeOut)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetDeepSleepTimer(timeOut);
        }
        LOGINFO("result[%d], timeOut[%d]", result, timeOut);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        WakeupReason wakeupReason_ = WakeupReason::WAKEUP_REASON_UNKNOWN;

        if (nullptr == wakeupReason) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetLastWakeupReason(wakeupReason_);
        }
        LOGINFO("result[%d], wakeupReason[%d]", result, wakeupReason_);

        _apiLock.Unlock();

        if (Core::ERROR_NONE == result) {
            *wakeupReason = convert(wakeupReason_);
        }

        return result;
    }

    Core::hresult GetLastWakeupKeyCode(int* keycode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == keycode) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            _powerManagerInterface->GetLastWakeupKeyCode(*keycode);
        }
        LOGINFO("result[%d], keycode[%d]", result, *keycode);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == rebootRequestor || nullptr == rebootReasonCustom || nullptr == rebootReasonOther) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
        }
        LOGINFO("result[%d], rebootRequestor[%s], rebootReasonCustom[%s], rebootReasonOther[%s]", result, rebootRequestor, rebootReasonCustom, rebootReasonOther);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult SetNetworkStandbyMode(const bool standbyMode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetNetworkStandbyMode(standbyMode);
        }
        LOGINFO("result[%d], standbyMode[%d]", result, standbyMode);

        _apiLock.Unlock();

        return result;
    }
    Core::hresult GetNetworkStandbyMode(bool* standbyMode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == standbyMode) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetNetworkStandbyMode(*standbyMode);
        }
        LOGINFO("result[%d], standbyMode[%d]", result, *standbyMode);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        PowerState powerStateBeforeReboot_ = PowerState::POWER_STATE_UNKNOWN;

        if (nullptr == powerStateBeforeReboot) {
            return Core::ERROR_BAD_REQUEST;
        }

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerStateBeforeReboot(powerStateBeforeReboot_);
        }
        LOGINFO("result[%d], powerStateBeforeReboot[%d]", result, powerStateBeforeReboot_);

        _apiLock.Unlock();

        if (Core::ERROR_NONE == result) {
            *powerStateBeforeReboot = convert(powerStateBeforeReboot_);
        }

        return result;
    }

    Core::hresult AddPowerModePreChangeClient(const std::string& clientName, uint32_t& clientId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->AddPowerModePreChangeClient(clientName, clientId);
        }
        LOGINFO("result[%d], clientName[%s], clientId[%d]", result, clientName.c_str(), clientId);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult RemovePowerModePreChangeClient(const uint32_t clientId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->RemovePowerModePreChangeClient(clientId);
        }
        LOGINFO("result[%d], clientId[%d]", result, clientId);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delay)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->DelayPowerModeChangeBy(clientId, transactionId, delay);
        }
        LOGINFO("result[%d], clientId[%d], transactionId[%d], delay[%d]", result, clientId, transactionId, delay);

        _apiLock.Unlock();

        return result;
    }

    Core::hresult PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _apiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->PowerModePreChangeComplete(clientId, transactionId);
        }
        LOGINFO("result[%d], clientId[%d], transactionId[%d]", result, clientId, transactionId);

        _apiLock.Unlock();

        return result;
    }

    void NotifyPowerModeChanged(const PowerState currentState, const PowerState newState)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _callbackLock.Lock();

        for (auto& cb : _powerModeChangedCallbacks) {
            cb.callback(currentState_, newState_, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyPowerModePreChange(const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        _callbackLock.Lock();

        for (auto& cb : _powerModePreChangeCallbacks) {
            cb.callback(currentState_, newState_, transactionId, stateChangeAfter, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyDeepSleepTimeout(const int wakeupTimeout)
    {
        _callbackLock.Lock();

        for (auto& cb : _deepSleepTimeoutCallbacks) {
            cb.callback(wakeupTimeout, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyNetworkStandbyModeChanged(const bool enabled)
    {
        _callbackLock.Lock();

        for (auto& cb : _networkStandbyModeChangedCallbacks) {
            cb.callback(enabled, cb.userdata);
        }

        _callbackLock.Unlock();
    }

    void NotifyThermalModeChanged(const ThermalTemperature currentThermalLevel, const ThermalTemperature newThermalLevel, const float currentTemperature)
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
    Core::hresult RegisterCallback(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback, void* userdata)
    {
        Core::hresult result = Core::ERROR_INVALID_PARAMETER;

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
    Core::hresult UnRegisterCallback(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback)
    {
        Core::hresult result = Core::ERROR_INVALID_PARAMETER;

        ASSERT(nullptr != callback);

        if (nullptr != callback) {
            _callbackLock.Lock();

            result = callbacklist.UnRegisterCallbackLocked(callback);

            _callbackLock.Unlock();
        }

        return (result);
    }

    Core::hresult RegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
    {
        return RegisterCallback(_operationalStateChangeCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback)
    {
        return UnRegisterCallback(_operationalStateChangeCallbacks, callback);
    }

    Core::hresult RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_powerModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
    {
        return UnRegisterCallback(_powerModeChangedCallbacks, callback);
    }

    Core::hresult RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
    {
        return RegisterCallback(_powerModePreChangeCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
    {
        return UnRegisterCallback(_powerModePreChangeCallbacks, callback);
    }

    Core::hresult RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
    {
        return RegisterCallback(_deepSleepTimeoutCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
    {
        return UnRegisterCallback(_deepSleepTimeoutCallbacks, callback);
    }

    Core::hresult RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_networkStandbyModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
    {
        return UnRegisterCallback(_networkStandbyModeChangedCallbacks, callback);
    }

    Core::hresult RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
    {
        return RegisterCallback(_thermalModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
    {
        return UnRegisterCallback(_thermalModeChangedCallbacks, callback);
    }

    Core::hresult RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
    {
        return RegisterCallback(_rebootBeginCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
    {
        return UnRegisterCallback(_rebootBeginCallbacks, callback);
    }

private:
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

    PIDFileMonitor _pidMonitor;
    int _pid;
    bool _connected;
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
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->Connect();
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_Disconnect()
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->Disconnect();
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

bool PowerController_IsOperational()
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->IsOperational();
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
{
    ASSERT(nullptr != currentState);
    ASSERT(nullptr != previousState);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetPowerState(currentState, previousState);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_SetPowerState(const int keyCode, const PowerController_PowerState_t powerstate, const char* reason)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->SetPowerState(keyCode, powerstate, reason);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetThermalState(float* currentTemperature)
{
    ASSERT(nullptr != currentTemperature);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetThermalState(currentTemperature);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_SetTemperatureThresholds(float high, float critical)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->SetTemperatureThresholds(high, critical);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetTemperatureThresholds(float* high, float* critical)
{
    ASSERT(nullptr != high);
    ASSERT(nullptr != critical);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetTemperatureThresholds(high, critical);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_SetOvertempGraceInterval(const int graceInterval)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->SetOvertempGraceInterval(graceInterval);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetOvertempGraceInterval(int* graceInterval /* @out */)
{
    ASSERT(nullptr != graceInterval);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetOvertempGraceInterval(graceInterval);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_SetDeepSleepTimer(const int timeOut)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->SetDeepSleepTimer(timeOut);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
{
    ASSERT(nullptr != wakeupReason);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetLastWakeupReason(wakeupReason);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetLastWakeupKeyCode(int* keycode)
{
    ASSERT(nullptr != keycode);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetLastWakeupKeyCode(keycode);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
{
    ASSERT(nullptr != rebootRequestor);
    ASSERT(nullptr != rebootReasonCustom);
    ASSERT(nullptr != rebootReasonOther);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_SetNetworkStandbyMode(const bool standbyMode)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->SetNetworkStandbyMode(standbyMode);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetNetworkStandbyMode(bool* standbyMode)
{
    ASSERT(standbyMode != nullptr);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetNetworkStandbyMode(standbyMode);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
{
    ASSERT(nullptr != powerStateBeforeReboot);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->GetPowerStateBeforeReboot(powerStateBeforeReboot);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_AddPowerModePreChangeClient(const char* clientName, uint32_t* clientId)
{
    ASSERT(nullptr != clientName);
    ASSERT(nullptr != clientId);
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->AddPowerModePreChangeClient(clientName, *clientId);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RemovePowerModePreChangeClient(const uint32_t clientId)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RemovePowerModePreChangeClient(clientId);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delayPeriod)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->DelayPowerModeChangeBy(clientId, transactionId, delayPeriod);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->PowerModePreChangeComplete(clientId, transactionId);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterOperationalStateChangedCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterOperationalStateChangedCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterPowerModeChangedCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterPowerModeChangedCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterPowerModePreChangeCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterPowerModePreChangeCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterDeepSleepTimeoutCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterDeepSleepTimeoutCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterNetworkStandbyModeChangedCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterNetworkStandbyModeChangedCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterThermalModeChangedCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterThermalModeChangedCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->RegisterRebootBeginCallback(callback, userdata);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

uint32_t PowerController_UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
{
    PowerController *instance = PowerController::Instance();
    if (instance) {
        return instance->UnRegisterRebootBeginCallback(callback);
    }
    return POWER_CONTROLLER_ERROR_UNAVAILABLE;
}

} // extern "C"
