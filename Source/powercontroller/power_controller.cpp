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

#include <sys/eventfd.h>
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
        fprintf(stdout, "[PowerController][%d] INFO  [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stdout);                                                                                                                                                   \
    } while (0)
#define LOGWARN(fmt, ...)                                                                                                                                                 \
    do {                                                                                                                                                                  \
        fprintf(stdout, "[PowerController][%d] WARN  [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stdout);                                                                                                                                                   \
    } while (0)
#define LOGERR(fmt, ...)                                                                                                                                                  \
    do {                                                                                                                                                                  \
        fprintf(stderr, "[PowerController][%d] ERROR [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                                                                                   \
    } while (0)
#else
#define LOGINFO(fmt, ...)
#define LOGWARN(fmt, ...)
#define LOGERR(fmt, ...)
#endif

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
// via RegisterThunderNotificationLocked and UnregisterNotificationLocked methods. To be implemented in PowerController (i,e PARENT)
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

    inline size_t Count() const
    {
        return this->size();
    }

    // Locked method expected to be called from locked context
    uint32_t AddCallbackLocked(typename CallbackType::Type callback, void* userdata)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        LOGINFO(">>>");

        auto it = std::find_if(this->begin(), this->end(), [&callback](const CallbackType& cb) {
            return cb.callback == callback;
        });

        if (it == this->end()) {
            LOGINFO("Callback identifier[%p]", (void*)callback);
            this->emplace_back(callback, userdata);
            result = Core::ERROR_NONE;
            RegisterNotificationInternalLocked();
        }

        LOGINFO("<<< result: %d", result);

        return result;
    }

    // Locked method expected to be called from locked context
    uint32_t RemoveCallbackLocked(typename CallbackType::Type callback)
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        LOGINFO(">>>");

        auto it = std::find_if(this->begin(), this->end(), [&callback](const CallbackType& cb) {
            return cb.callback == callback;
        });

        if (it != this->end()) {
            LOGINFO("Callback identifier[%p]", (void*)callback);
            this->erase(it);
            result = Core::ERROR_NONE;
            UnregisterNotificationInternalLocked(false);
        }

        LOGINFO("<<< result: %d", result);

        return result;
    }

    // Locked method expected to be called from locked context
    inline void RegisterNotificationInternalLocked()
    {
        if (!_registered && !this->empty() && _parent.IsActivatedLocked()) {
            _registered = _parent.template RegisterThunderNotificationLocked<CallbackType>();
        }
    }

    // Locked method expected to be called from locked context
    // @param forced A boolean indicating whether to forcefully unregister from notification
    //               regardless of the callback list's state / unregister status.
    //               This is required to handle PowerManager restart scenarios.
    inline void UnregisterNotificationInternalLocked(const bool forced)
    {
        if (_registered && _parent.IsActivatedLocked() && (forced || this->empty())) {
            bool unregistered = _parent.template UnregisterThunderNotificationLocked<CallbackType>();

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
    int _wakeupFd; // eventfd used to wake poll() on shutdown
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
        LOGINFO(">>>");
        ASSERT(INVALID_PID != _pid);

        LOGINFO("started, pid file: %s", _pidFile.c_str());

        do {

            _fd = inotify_init1(IN_NONBLOCK);

            if (_fd < 0) {
                LOGERR("inotify_init failed, err: %s", strerror(errno));
                _fd = INVALID_FD;
                break;
            }

            LOGINFO("inotify initialized _fd: %d, Adding inotify watch",_fd);
            _wd = inotify_add_watch(_fd, _pidFile.c_str(), IN_CLOSE_WRITE);

            if (_wd < 0) {
                LOGERR("inotify_add_watch failed, err: %s", strerror(errno));
                _wd = INVALID_FD;
                break;
            }

            struct pollfd pfds[2] = {
                { .fd = _fd,       .events = POLLIN },
                { .fd = _wakeupFd, .events = POLLIN },
            };

            LOGINFO("inotify watch added, _wd: %d, start polling for events", _wd);

            while (!_shutdown) {

                int poll_res = poll(pfds, 2, POLL_TIMEOUT);

                if (poll_res == -1) {
                    if (errno == EINTR)
                        continue;

                    LOGERR("FATAL poll failed, Err[%s], fd[%d], wd[%d]", strerror(errno), _fd, _wd);
                    break;
                } else if (poll_res == 0) {
                    // timedout
                    continue;
                } else {
                    if (pfds[1].revents & POLLIN) {
                        /* Wakeup signal received, exit loop */
                        LOGINFO("Wakeup signal received, exiting monitor loop");
                        break;
                    }
                    if (pfds[0].revents & POLLIN) {
                        /* Inotify events are available. */
                        handle_inotify_event(_fd);
                    }
                }
            }
        } while (false);

        if (INVALID_FD != _wd) {
            LOGINFO("Removing inotify watch, wd: %d", _wd);
            if (INVALID_FD != _fd) {
                int result = inotify_rm_watch(_fd, _wd);
                if (result < 0) {
                    LOGERR("inotify_rm_watch failed, err: %s", strerror(errno));
                }
            }
            _wd = INVALID_FD;
        }
        if (INVALID_FD != _fd) {
            LOGINFO("Closing inotify fd: %d", _fd);
            close(_fd);
            _fd = INVALID_FD;
        }
        LOGINFO("<<<");
    }

public:
    PIDFileMonitor(const std::string& pidFile, PIDFileChangedCb callback)
        : _pidFile(pidFile)
        , _processName(parseProcessName(pidFile))
        , _callback(std::move(callback))
        , _fd(INVALID_FD)
        , _wd(INVALID_FD)
        , _wakeupFd(INVALID_FD)
        , _pid(INVALID_PID)
        , _shutdown(false)
        , _thread(nullptr)
    {
        LOGINFO(">>> pidFile: %s, processName: %s", _pidFile.c_str(), _processName.c_str());
        _wakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (_wakeupFd < 0) {
            LOGERR("eventfd creation failed, err: %s", strerror(errno));
            _wakeupFd = INVALID_FD;
        }
        LOGINFO("<<<");
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
            if (_thread->joinable()) {
                LOGINFO("PID file monitor thread started successfully");
                _running = true;
            } else {
                LOGERR("Failed to start PID file monitor thread");
                _thread.reset();
            }
        }
    }

    void Stop()
    {
        LOGINFO(">>>");
        _shutdown = true;

        if (_running) {
            // Signal the wakeup eventfd to unblock poll() immediately
            if (INVALID_FD != _wakeupFd) {
                uint64_t val = 1;
                LOGINFO("Signaling monitor thread to shutdown via eventfd");
                if (write(_wakeupFd, &val, sizeof(val)) < 0) {
                    LOGERR("eventfd write failed, err: %s", strerror(errno));
                }
            }

            LOGINFO("Waiting for monitor thread to exit");

            if (_thread) {
                if (_thread->joinable()) {
                    _thread->join();
                }
                _thread.reset();
                _running = false;
            }
        }

        if (INVALID_FD != _wakeupFd) {
            LOGINFO("Closing wakeupFd ...");
            close(_wakeupFd);
            _wakeupFd = INVALID_FD;
        }

        LOGINFO("<<<");
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
        LOGINFO(">>>");
        Stop();
        LOGINFO("<<<");
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
        (void)Disconnect(true); /* Internally it will Close destroys _powerManagerInterface too */
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
        LOGINFO(">>> Callsign: (%s), Operational: %d", callSign, upAndRunning);
        _internalApiLock.Lock();

        // avoid misleading log during shutdown
        if ((upAndRunning && _shutdown) || !_shutdown) {
            LOGINFO("Plugin is %s", upAndRunning ? "Activated" : "Deactivated");
        }

        if (upAndRunning) {
            // Communicatior opened && PowerManager is Activated
            if (nullptr == _powerManagerInterface) {
                _powerManagerInterface = BaseClass::Interface();
                if (_powerManagerInterface != nullptr) {
                    LOGINFO("Established COM-RPC connection with PowerManager plugin");
                    SubscribeAllNotificationsLocked();
                    LOGINFO("Subscribed for PowerManager plugin notifications");
                } else {
                    // Internal error powerManager is running, but QueryInterface failed for it ?
                    LOGERR("Failed to Establish COM-RPC connection with PowerManager plugin");
                }
            }
        } else {
            // PowerManager is Deactivated || Communicator closed
            if (nullptr != _powerManagerInterface) {
                LOGINFO("Unsubscribing from PowerManager plugin notifications");
                UnsubscribeAllNotificationsLocked();
                LOGINFO("Releasing COM-RPC connection with PowerManager plugin");
                _powerManagerInterface->Release();
                LOGINFO("COM-RPC connection with PowerManager plugin released");
                _powerManagerInterface = nullptr;
            } else {
                LOGERR("Unexpected, powerManager just deactivated, but interface already null ?");
            }
        }
        _internalApiLock.Unlock();
        LOGINFO("<<<");

        // avoid notifying operational state changed if shuting down because of Term
        if (!_shutdown) {
            _callbackLock.Lock();
            LOGINFO(">>> Callback Lock acquired");
            for (auto& cb : _operationalStateChangeCallbacks) {
                auto start = std::chrono::steady_clock::now();
                cb.callback(upAndRunning, cb.userdata);
                auto elapsed = std::chrono::steady_clock::now() - start;
                LOGINFO("Callback %p took %lld ms to process OperationalStateChange notification", (void*)cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
            }
            _callbackLock.Unlock();
            LOGINFO("<<< Callback Lock released");
        }
        LOGINFO("<<<");
    }

    // Locked method expected to be called from locked context
    void SubscribeAllNotificationsLocked()
    {
        LOGINFO(">>>");
        _powerModeChangedCallbacks.RegisterNotificationInternalLocked();
        _powerModePreChangeCallbacks.RegisterNotificationInternalLocked();
        _deepSleepTimeoutCallbacks.RegisterNotificationInternalLocked();
        _networkStandbyModeChangedCallbacks.RegisterNotificationInternalLocked();
        _thermalModeChangedCallbacks.RegisterNotificationInternalLocked();
        _rebootBeginCallbacks.RegisterNotificationInternalLocked();
        LOGINFO("<<<");
    }

    // Locked method expected to be called from locked context
    void UnsubscribeAllNotificationsLocked()
    {
        LOGINFO(">>>");
        _powerModeChangedCallbacks.UnregisterNotificationInternalLocked(true);
        _powerModePreChangeCallbacks.UnregisterNotificationInternalLocked(true);
        _deepSleepTimeoutCallbacks.UnregisterNotificationInternalLocked(true);
        _networkStandbyModeChangedCallbacks.UnregisterNotificationInternalLocked(true);
        _thermalModeChangedCallbacks.UnregisterNotificationInternalLocked(true);
        _rebootBeginCallbacks.UnregisterNotificationInternalLocked(true);
        LOGINFO("<<<");
    }

    inline bool isConnected() const
    {
        LOGINFO("Checking COM-RPC channel, _connected: %d", _connected);
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

        LOGINFO(">>>");
        _internalApiLock.Lock();
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
        _internalApiLock.Unlock();
        LOGINFO("<<< status[%u]", status);
        return status;
    }

    uint32_t Disconnect(bool shutdown = false)
    {
        uint32_t status = Core::ERROR_GENERAL;

        LOGINFO(">>>");

        _internalApiLock.Lock();

        if (shutdown && _pidMonitor.Running()) {
            LOGINFO("Stopping PID monitor ...");
            _pidMonitor.Stop();
        }

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

        _internalApiLock.Unlock();
        LOGINFO("<<< status[%u]", status);

        return status;
    }

    // Locked method expected to be called from locked context (take care in specializations too)
    template <typename CallbackType>
    bool RegisterThunderNotificationLocked()
    {
        // static_assert(std::false_type::value, "Specialization required for CallbackType");
        return false;
    }

    // Locked method expected to be called from locked context (take care in specializations too)
    template <typename CallbackType>
    bool UnregisterThunderNotificationLocked()
    {
        // static_assert(std::false_type::value, "Specialization required for CallbackType");
        return false;
    }

    static void Init()
    {
        LOGINFO(">>>");
        _internalApiLock.Lock();
        if (nullptr == _instance) {
            _instance = new PowerController();
            if (nullptr == _instance) {
                LOGERR("PowerController::Init failed, out of memory ?");
            }
            else {
                LOGINFO("PowerController::Init successful");
            }
        }
        _internalApiLock.Unlock();
        LOGINFO("<<<");
    }

    static void Term()
    {
        LOGINFO(">>>");
        _internalApiLock.Lock();
        if (nullptr != _instance) {
            delete _instance;
            _instance = nullptr;
        }
        _internalApiLock.Unlock();
        LOGINFO("<<<");
    }

    static PowerController* Instance()
    {
        // Auto-init if instance was freed (e.g. after Term())
        if (nullptr == _instance) {
            LOGINFO("Instance is null, auto-initializing");
            Init();
        }
        return _instance;
    }

    Core::hresult GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        PowerState currentState_ = PowerState::POWER_STATE_UNKNOWN;
        PowerState previousState_ = PowerState::POWER_STATE_UNKNOWN;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerState(currentState_, previousState_);
        }
        LOGINFO("result[%d], Prev[%d], Current[%d]", result, previousState_, currentState_);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

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

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetPowerState(keyCode, powerState_, reason);
        }
        LOGINFO("result[%d], keyCode[%d], powerState[%d], reason[%s]", result, keyCode, powerState_, reason ? reason : "Invalid");

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult GetThermalState(float* currentTemperature)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == currentTemperature) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetThermalState(*currentTemperature);
        }
        LOGINFO("result[%d], currentTemperature[%f]", result, *currentTemperature);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult SetTemperatureThresholds(float high, float critical)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetTemperatureThresholds(high, critical);
        }
        LOGINFO("result[%d], high[%f], critical[%f]", result, high, critical);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult GetTemperatureThresholds(float* high, float* critical)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == high || nullptr == critical) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetTemperatureThresholds(*high, *critical);
        }
        LOGINFO("result[%d], high[%f], critical[%f]", result, *high, *critical);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult SetOvertempGraceInterval(const int graceInterval)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetOvertempGraceInterval(graceInterval);
        }
        LOGINFO("result[%d], graceInterval[%d]", result, graceInterval);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult GetOvertempGraceInterval(int* graceInterval)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == graceInterval) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetOvertempGraceInterval(*graceInterval);
        }
        LOGINFO("result[%d], graceInterval[%d]", result, *graceInterval);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult SetDeepSleepTimer(const int timeOut)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetDeepSleepTimer(timeOut);
        }
        LOGINFO("result[%d], timeOut[%d]", result, timeOut);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        WakeupReason wakeupReason_ = WakeupReason::WAKEUP_REASON_UNKNOWN;

        if (nullptr == wakeupReason) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetLastWakeupReason(wakeupReason_);
        }
        LOGINFO("result[%d], wakeupReason[%d]", result, wakeupReason_);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

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

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            _powerManagerInterface->GetLastWakeupKeyCode(*keycode);
        }
        LOGINFO("result[%d], keycode[%d]", result, *keycode);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == rebootRequestor || nullptr == rebootReasonCustom || nullptr == rebootReasonOther) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
        }
        LOGINFO("result[%d], rebootRequestor[%s], rebootReasonCustom[%s], rebootReasonOther[%s]", result, rebootRequestor, rebootReasonCustom, rebootReasonOther);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult SetNetworkStandbyMode(const bool standbyMode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->SetNetworkStandbyMode(standbyMode);
        }
        LOGINFO("result[%d], standbyMode[%d]", result, standbyMode);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }
    Core::hresult GetNetworkStandbyMode(bool* standbyMode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        if (nullptr == standbyMode) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetNetworkStandbyMode(*standbyMode);
        }
        LOGINFO("result[%d], standbyMode[%d]", result, *standbyMode);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        PowerState powerStateBeforeReboot_ = PowerState::POWER_STATE_UNKNOWN;

        if (nullptr == powerStateBeforeReboot) {
            return Core::ERROR_BAD_REQUEST;
        }

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->GetPowerStateBeforeReboot(powerStateBeforeReboot_);
        }
        LOGINFO("result[%d], powerStateBeforeReboot[%d]", result, powerStateBeforeReboot_);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        if (Core::ERROR_NONE == result) {
            *powerStateBeforeReboot = convert(powerStateBeforeReboot_);
        }

        return result;
    }

    Core::hresult AddPowerModePreChangeClient(const std::string& clientName, uint32_t& clientId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->AddPowerModePreChangeClient(clientName, clientId);
        }
        LOGINFO("result[%d], clientName[%s], clientId[%d]", result, clientName.c_str(), clientId);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult RemovePowerModePreChangeClient(const uint32_t clientId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->RemovePowerModePreChangeClient(clientId);
        }
        LOGINFO("result[%d], clientId[%d]", result, clientId);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delay)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->DelayPowerModeChangeBy(clientId, transactionId, delay);
        }
        LOGINFO("result[%d], clientId[%d], transactionId[%d], delay[%d]", result, clientId, transactionId, delay);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    Core::hresult PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        LOGINFO(">>>");
        _internalApiLock.Lock();

        if (_powerManagerInterface) {
            result = _powerManagerInterface->PowerModePreChangeComplete(clientId, transactionId);
        }
        LOGINFO("result[%d], clientId[%d], transactionId[%d]", result, clientId, transactionId);

        _internalApiLock.Unlock();
        LOGINFO("<<<");

        return result;
    }

    void NotifyPowerModeChanged(const PowerState currentState, const PowerState newState)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);

        LOGINFO(">>> currentState[%d], newState[%d]", currentState_, newState_);
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _powerModeChangedCallbacks.Count());

        for (auto& cb : _powerModeChangedCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(currentState_, newState_, cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process PowerModeChanged notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    void NotifyPowerModePreChange(const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter)
    {
        PowerController_PowerState_t currentState_ = convert(currentState);
        PowerController_PowerState_t newState_ = convert(newState);
        LOGINFO(">>> currentState[%d], newState[%d], transactionId[%d], stateChangeAfter[%d]", currentState_, newState_, transactionId, stateChangeAfter);
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _powerModePreChangeCallbacks.Count());

        for (auto& cb : _powerModePreChangeCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(currentState_, newState_, transactionId, stateChangeAfter, cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process PowerModePreChange notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    void NotifyDeepSleepTimeout(const int wakeupTimeout)
    {
        LOGINFO(">>> wakeupTimeout[%d]", wakeupTimeout);
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _deepSleepTimeoutCallbacks.Count());

        for (auto& cb : _deepSleepTimeoutCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(wakeupTimeout, cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process DeepSleepTimeout notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    void NotifyNetworkStandbyModeChanged(const bool enabled)
    {
        LOGINFO(">>> enabled[%d]", enabled);
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _networkStandbyModeChangedCallbacks.Count());

        for (auto& cb : _networkStandbyModeChangedCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(enabled, cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process NetworkStandbyModeChanged notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    void NotifyThermalModeChanged(const ThermalTemperature currentThermalLevel, const ThermalTemperature newThermalLevel, const float currentTemperature)
    {
        PowerController_ThermalTemperature_t currentThermalLevel_ = convert(currentThermalLevel);
        PowerController_ThermalTemperature_t newThermalLevel_ = convert(newThermalLevel);

        LOGINFO(">>> currentThermalLevel[%d], newThermalLevel[%d], currentTemperature[%f]", currentThermalLevel_, newThermalLevel_, currentTemperature);
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _thermalModeChangedCallbacks.Count());

        for (auto& cb : _thermalModeChangedCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(currentThermalLevel_, newThermalLevel_, currentTemperature, cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process ThermalModeChanged notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    void NotifyRebootBegin(const string& rebootReasonCustom, const string& rebootReasonOther, const string& rebootRequestor)
    {
        LOGINFO(">>> rebootReasonCustom[%s], rebootReasonOther[%s], rebootRequestor[%s]", rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str());
        _callbackLock.Lock();
        LOGINFO("Callback count: %zu", _rebootBeginCallbacks.Count());

        for (auto& cb : _rebootBeginCallbacks) {
            auto start = std::chrono::steady_clock::now();
            cb.callback(rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str(), cb.userdata);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("Callback %p took %lld ms to process RebootBegin notification", cb.callback, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }

        _callbackLock.Unlock();
        LOGINFO("<<<");
    }

    // Generic template function for callback register
    template <typename CallbackType, typename PARENT>
    Core::hresult RegisterCallbackInternal(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback, void* userdata)
    {
        Core::hresult result = Core::ERROR_INVALID_PARAMETER;

        LOGINFO(">>> callback: %p, userdata: %p", callback, userdata);

        ASSERT(nullptr != callback);

        if (nullptr != callback) {
            _callbackLock.Lock();
            LOGINFO(">>> Callback Lock acquired");

            result = callbacklist.AddCallbackLocked(callback, userdata);

            _callbackLock.Unlock();
            LOGINFO("<<< Callback Lock released");
        }

        LOGINFO("<<< result: %d", result);

        return result;
    }

    // Generic template function for callback unregister
    template <typename CallbackType, typename PARENT>
    Core::hresult UnRegisterCallback(CallbackList<CallbackType, PARENT>& callbacklist, typename CallbackType::Type callback)
    {
        Core::hresult result = Core::ERROR_INVALID_PARAMETER;

        LOGINFO(">>> callback: %p", callback);

        ASSERT(nullptr != callback);

        if (nullptr != callback) {
            _callbackLock.Lock();
            LOGINFO(">>> Callback Lock acquired");

            result = callbacklist.RemoveCallbackLocked(callback);
            _callbackLock.Unlock();
            LOGINFO("<<< Callback Lock released");
        }

        LOGINFO("<<< result: %d", result);

        return (result);
    }

    Core::hresult RegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_operationalStateChangeCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterOperationalStateChangedCallback(PowerController_OperationalStateChangeCb callback)
    {
        return UnRegisterCallback(_operationalStateChangeCallbacks, callback);
    }

    Core::hresult RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_powerModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
    {
        return UnRegisterCallback(_powerModeChangedCallbacks, callback);
    }

    Core::hresult RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_powerModePreChangeCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
    {
        return UnRegisterCallback(_powerModePreChangeCallbacks, callback);
    }

    Core::hresult RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_deepSleepTimeoutCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
    {
        return UnRegisterCallback(_deepSleepTimeoutCallbacks, callback);
    }

    Core::hresult RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_networkStandbyModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
    {
        return UnRegisterCallback(_networkStandbyModeChangedCallbacks, callback);
    }

    Core::hresult RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_thermalModeChangedCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
    {
        return UnRegisterCallback(_thermalModeChangedCallbacks, callback);
    }

    Core::hresult RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
    {
        return RegisterCallbackInternal(_rebootBeginCallbacks, callback, userdata);
    }

    Core::hresult UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
    {
        return UnRegisterCallback(_rebootBeginCallbacks, callback);
    }

public:
    static Core::CriticalSection _cApiLock; // C-API level lock: serializes all extern "C" PowerController_* calls
    static bool initialized; // indicates whether PowerController is initialized or not, expected to be accessed under _cApiLock

private:
    static PowerController* _instance;
    static Core::CriticalSection _internalApiLock;
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
bool PowerController::RegisterThunderNotificationLocked<OperationalStateChangeCb>()
{
    // Operational state change notification is managed by SmartInterfaceType
    return true;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<PowerModePreChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModePreChangeNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<PowerModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<DeepSleepTimeoutCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IDeepSleepTimeoutNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<NetworkStandbyModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<ThermalModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IThermalModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::RegisterThunderNotificationLocked<RebootBeginCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Register(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IRebootNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<OperationalStateChangeCb>()
{
    // Operational state change notification is managed by SmartInterfaceType
    return true;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<PowerModePreChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModePreChangeNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<PowerModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<DeepSleepTimeoutCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IDeepSleepTimeoutNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<NetworkStandbyModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<ThermalModeChangedCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IThermalModeChangedNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

template <>
bool PowerController::UnregisterThunderNotificationLocked<RebootBeginCb>()
{
    bool result = false;
    LOGINFO(">>>");
    result = Core::ERROR_NONE == _powerManagerInterface->Unregister(_powerManagerNotification.baseInterface<Exchange::IPowerManager::IRebootNotification>());
    LOGINFO("<<< result: %d", result);
    return result;
}

} // nameless namespace

/* static */ PowerController* PowerController::_instance = nullptr;
/* static */ Core::CriticalSection PowerController::_cApiLock;
/* static */ Core::CriticalSection PowerController::_internalApiLock;
/* static */ Core::CriticalSection PowerController::_callbackLock;
/* static */ bool PowerController::initialized = false;

extern "C" {

void PowerController_Init()
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController::Init();
    PowerController::initialized = true;
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<<");
}

void PowerController_Term()
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController::Term();
    PowerController::initialized = false;
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<<");
}

uint32_t PowerController_Connect()
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->Connect();
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_Disconnect()
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot disconnect");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->Disconnect();
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

bool PowerController_IsOperational()
{
    bool result = false;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot be operational");
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->IsOperational();
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetPowerState(PowerController_PowerState_t* currentState, PowerController_PowerState_t* previousState)
{
    LOGINFO(">>>");
    ASSERT(nullptr != currentState);
    ASSERT(nullptr != previousState);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetPowerState(currentState, previousState);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_SetPowerState(const int keyCode, const PowerController_PowerState_t powerstate, const char* reason)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->SetPowerState(keyCode, powerstate, reason);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetThermalState(float* currentTemperature)
{
    LOGINFO(">>>");
    ASSERT(nullptr != currentTemperature);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetThermalState(currentTemperature);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_SetTemperatureThresholds(float high, float critical)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->SetTemperatureThresholds(high, critical);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetTemperatureThresholds(float* high, float* critical)
{
    LOGINFO(">>>");
    ASSERT(nullptr != high);
    ASSERT(nullptr != critical);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetTemperatureThresholds(high, critical);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_SetOvertempGraceInterval(const int graceInterval)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->SetOvertempGraceInterval(graceInterval);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetOvertempGraceInterval(int* graceInterval /* @out */)
{
    LOGINFO(">>>");
    ASSERT(nullptr != graceInterval);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetOvertempGraceInterval(graceInterval);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_SetDeepSleepTimer(const int timeOut)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->SetDeepSleepTimer(timeOut);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason)
{
    LOGINFO(">>>");
    ASSERT(nullptr != wakeupReason);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetLastWakeupReason(wakeupReason);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetLastWakeupKeyCode(int* keycode)
{
    LOGINFO(">>>");
    ASSERT(nullptr != keycode);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetLastWakeupKeyCode(keycode);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_Reboot(const char* rebootRequestor, const char* rebootReasonCustom, const char* rebootReasonOther)
{
    LOGINFO(">>>");
    ASSERT(nullptr != rebootRequestor);
    ASSERT(nullptr != rebootReasonCustom);
    ASSERT(nullptr != rebootReasonOther);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_SetNetworkStandbyMode(const bool standbyMode)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->SetNetworkStandbyMode(standbyMode);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetNetworkStandbyMode(bool* standbyMode)
{
    LOGINFO(">>>");
    ASSERT(standbyMode != nullptr);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetNetworkStandbyMode(standbyMode);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot)
{
    LOGINFO(">>>");
    ASSERT(nullptr != powerStateBeforeReboot);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->GetPowerStateBeforeReboot(powerStateBeforeReboot);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_AddPowerModePreChangeClient(const char* clientName, uint32_t* clientId)
{
    LOGINFO(">>>");
    ASSERT(nullptr != clientName);
    ASSERT(nullptr != clientId);
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->AddPowerModePreChangeClient(clientName, *clientId);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RemovePowerModePreChangeClient(const uint32_t clientId)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RemovePowerModePreChangeClient(clientId);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delayPeriod)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->DelayPowerModeChangeBy(clientId, transactionId, delayPeriod);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->PowerModePreChangeComplete(clientId, transactionId);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterOperationalStateChangedCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister operational state change callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterOperationalStateChangedCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterPowerModeChangedCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister power mode change callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterPowerModeChangedCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterPowerModePreChangeCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister power mode pre-change callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
        if (instance) {
            result = instance->UnRegisterPowerModePreChangeCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterDeepSleepTimeoutCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister deep sleep timeout callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterDeepSleepTimeoutCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterNetworkStandbyModeChangedCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister network standby mode changed callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterNetworkStandbyModeChangedCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterThermalModeChangedCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister thermal mode changed callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterThermalModeChangedCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata)
{
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    PowerController *instance = PowerController::Instance();
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    if (instance) {
        result = instance->RegisterRebootBeginCallback(callback, userdata);
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

uint32_t PowerController_UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback)
{
    uint32_t result = POWER_CONTROLLER_ERROR_UNAVAILABLE;
    LOGINFO(">>>");
    PowerController::_cApiLock.Lock();
    if (!PowerController::initialized) {
        LOGWARN("Instance is not created, cannot unregister reboot begin callback");
        result = POWER_CONTROLLER_ERROR_NONE;
    }
    else {
        PowerController *instance = PowerController::Instance();
        if (instance) {
            result = instance->UnRegisterRebootBeginCallback(callback);
        }
    }
    PowerController::_cApiLock.Unlock();
    LOGINFO("<<< result: %d", result);
    return result;
}

} // extern "C"
