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
#ifndef POWERMANAGER_CLIENT_H
#define POWERMANAGER_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PowerManager_PowerState {
    POWER_STATE_UNKNOWN = 0 /* UNKNOWN */,
    POWER_STATE_OFF = 1 /* OFF */,
    POWER_STATE_STANDBY = 2 /* STANDBY */,
    POWER_STATE_ON = 3 /* ON */,
    POWER_STATE_STANDBY_LIGHT_SLEEP = 4 /* LIGHT_SLEEP */,
    POWER_STATE_STANDBY_DEEP_SLEEP = 5 /* DEEP_SLEEP */
} PowerManager_PowerState_t;

typedef enum PowerManager_ThermalTemperature {
    THERMAL_TEMPERATURE_UNKNOWN = 0 /* UNKNOWN Thermal Temperature */,
    THERMAL_TEMPERATURE_NORMAL = 1 /* Normal Thermal Temperature */,
    THERMAL_TEMPERATURE_HIGH = 2 /* High Thermal Temperature */,
    THERMAL_TEMPERATURE_CRITICAL = 4 /* Critial Thermal Temperature */
} PowerManager_ThermalTemperature_t;

typedef enum PowerManager_WakeupSrcType {
    WAKEUP_SRC_UNKNOWN = 0 /* UNKNOWN */,
    WAKEUP_SRC_VOICE = 1 /* VOICE */,
    WAKEUP_SRC_PRESENCEDETECTED = 2 /* PRESENCEDETECTED */,
    WAKEUP_SRC_BLUETOOTH = 3 /* BLUETOOTH */,
    WAKEUP_SRC_WIFI = 4 /* WIFI */,
    WAKEUP_SRC_IR = 5 /* IR */,
    WAKEUP_SRC_POWERKEY = 6 /* POWERKEY */,
    WAKEUP_SRC_TIMER = 7 /* TIMER */,
    WAKEUP_SRC_CEC = 8 /* CEC */,
    WAKEUP_SRC_LAN = 9 /* LAN */,
    WAKEUP_SRC_RF4CE = 10 /* RF4CE */
} PowerManager_WakeupSrcType_t;

typedef enum PowerManager_WakeupReason {
    WAKEUP_REASON_UNKNOWN = 0 /* UNKNOWN */,
    WAKEUP_REASON_IR = 1 /* IR */,
    WAKEUP_REASON_BLUETOOTH = 2 /* BLUETOOTH */,
    WAKEUP_REASON_RF4CE = 3 /* RF4CE */,
    WAKEUP_REASON_GPIO = 4 /* GPIO */,
    WAKEUP_REASON_LAN = 5 /* LAN */,
    WAKEUP_REASON_WIFI = 6 /* WIFI */,
    WAKEUP_REASON_TIMER = 7 /* TIMER */,
    WAKEUP_REASON_FRONTPANEL = 8 /* FRONTPANEL */,
    WAKEUP_REASON_WATCHDOG = 9 /* WATCHDOG */,
    WAKEUP_REASON_SOFTWARERESET = 10 /* SOFTWARERESET */,
    WAKEUP_REASON_THERMALRESET = 11 /* THERMALRESET */,
    WAKEUP_REASON_WARMRESET = 12 /* WARMRESET */,
    WAKEUP_REASON_COLDBOOT = 13 /* COLDBOOT */,
    WAKEUP_REASON_STRAUTHFAIL = 14 /* STR_AUTH_FAIL */,
    WAKEUP_REASON_CEC = 15 /* CEC */,
    WAKEUP_REASON_PRESENCE = 16 /* PRESENCE */,
    WAKEUP_REASON_VOICE = 17 /* VOICE */
} PowerManager_WakeupReason_t;

typedef enum PowerManager_SystemMode {
    SYSTEM_MODE_UNKNOWN = 0 /* UNKNOWN */,
    SYSTEM_MODE_NORMAL = 1 /* NORMAL */,
    SYSTEM_MODE_EAS = 2 /* EAS */,
    SYSTEM_MODE_WAREHOUSE = 3 /* WAREHOUSE */
} PowerManager_SystemMode_t;

/** Gets the Power State.*/
// @text getPowerState
// @brief Get Power State
// @param powerState: Get current power state
uint32_t PowerManager_GetPowerState(PowerManager_PowerState_t* currentState, PowerManager_PowerState_t* previousState);

/** Sets Power State . */
// @text setPowerState
// @brief Set Power State
// @param powerState: Set power to this state
// @param reason: Reason for moving to the power state
uint32_t PowerManager_SetPowerState(const int keyCode, const PowerManager_PowerState_t powerstate, const char* reason);

/** Gets the current Thermal state.*/
// @text getThermalState
// @brief Get Current Thermal State (temperature)
// @param currentTemperature: current temperature
uint32_t PowerManager_GetThermalState(float* currentTemperature /* @out */);

/** Sets the Temperature Thresholds.*/
// @text setTemperatureThresholds
// @brief Set Temperature Thresholds
// @param high: high threshold
// @param critical : critical threshold
uint32_t PowerManager_SetTemperatureThresholds(float high /* @in */, float critical /* @in */);

/** Gets the current Temperature Thresholds.*/
// @text getTemperatureThresholds
// @brief Get Temperature Thresholds
// @param high: high threshold
// @param critical : critical threshold
uint32_t PowerManager_GetTemperatureThresholds(float* high /* @out */, float* critical /* @out */);

/** Sets the current Temperature Grace interval.*/
// @property
// @text PowerManager_SetOvertempGraceInterval
// @brief Set Temperature Thresholds
// @param graceInterval: interval in secs?
uint32_t PowerManager_SetOvertempGraceInterval(const int graceInterval /* @in */);

/** Gets the current Temperature Thresholds.*/
// @property
// @text PowerManager_GetOvertempGraceInterval
// @brief Get Temperature Grace interval
// @param graceInterval: interval in secs?
uint32_t PowerManager_GetOvertempGraceInterval(int* graceInterval /* @out */);

/** Set Deep Sleep Timer for later wakeup */
// @property
// @text setDeepSleepTimer
// @brief Set Deep sleep timer for timeOut period
// @param timeOut: deep sleep timeout
uint32_t PowerManager_SetDeepSleepTimer(const int timeOut /* @in */);

/** Get Last Wakeup reason */
// @property
// @text getLastWakeupReason
// @brief Get Last Wake up reason
// @param wakeupReason: wake up reason
uint32_t PowerManager_GetLastWakeupReason(PowerManager_WakeupReason_t* wakeupReason /* @out */);

/** Get Last Wakeup key code */
// @property
// @text getLastWakeupKeyCode
// @brief Get the key code that can be used for wakeup
// @param keycode: Key code for wakeup
uint32_t PowerManager_GetLastWakeupKeyCode(int* keycode /* @out */);

/** Perform Reboot */
// @text reboot
// @brief Reboot device
uint32_t PowerManager_Reboot(const char* rebootRequestor /* @in */, const char* rebootReasonCustom /* @in */, const char* rebootReasonOther /* @in */);

/** Set Network Standby Mode */
// @property
// @text setNetworkStandbyMode
// @brief Set the standby mode for Network
// @param standbyMode: Network standby mode
uint32_t PowerManager_SetNetworkStandbyMode(const bool standbyMode /* @in */);

/** Get Network Standby Mode */
// @text getNetworkStandbyMode
// @brief Get the standby mode for Network
// @param standbyMode: Network standby mode
uint32_t PowerManager_GetNetworkStandbyMode(bool* standbyMode /* @out */);

/** Set Wakeup source configuration */
// @text setWakeupSrcConfig
// @brief Set the source configuration for device wakeup
// @param powerMode: power mode
// @param wakeSrcType: source type
// @param config: config
uint32_t PowerManager_SetWakeupSrcConfig(const int powerMode /* @in */, const int wakeSrcType /* @in */, int config /* @in */);

/** Get Wakeup source configuration */
// @text getWakeupSrcConfig
// @brief Get the source configuration for device wakeup
// @param powerMode: power mode
// @param srcType: source type
// @param config: config
uint32_t PowerManager_GetWakeupSrcConfig(int* powerMode /* @out */, int* srcType /* @out */, int* config /* @out */);

/** Initiate System mode change */
// @text PowerManager_SetSystemMode
// @brief System mode change
// @param oldMode: old mode
// @param newMode: new mode
uint32_t PowerManager_SetSystemMode(const PowerManager_SystemMode_t currentMode /* @in */, const PowerManager_SystemMode_t newMode /* @in */);

/** Get Power State before reboot */
// @text PowerManager_GetPowerStateBeforeReboot
// @brief Get Power state before reboot
// @param powerStateBeforeReboot: power state
uint32_t PowerManager_GetPowerStateBeforeReboot(PowerManager_PowerState_t* powerStateBeforeReboot /* @out */);

/** Delete / Release Power Manager Plugin Client instance including RPC instance */
// @text PowerManager_Dispose
void PowerManager_Dispose();

/* Callback data types for event notifications from power manager plugin */
typedef void (*PowerManager_PowerModeChangedCb)(const PowerManager_PowerState_t currentState, const PowerManager_PowerState_t newState, void* userdata);
typedef void (*PowerManager_PowerModePreChangeCb)(const PowerManager_PowerState_t currentState, const PowerManager_PowerState_t newState, void* userdata);
typedef void (*PowerManager_DeepSleepTimeoutCb)(const int wakeupTimeout, void* userdata);
typedef void (*PowerManager_NetworkStandbyModeChangedCb)(const bool enabled, void* userdata);
typedef void (*PowerManager_ThermalModeChangedCb)(const PowerManager_ThermalTemperature_t currentThermalLevel, const PowerManager_ThermalTemperature_t newThermalLevel, const float currentTemperature, void* userdata);
typedef void (*PowerManager_RebootBeginCb)(const char* rebootReasonCustom, const char* rebootReasonOther, const char* rebootRequestor, void* userdata);

/** Register for PowerMode changed callback */
uint32_t RegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) PowerMode changed callback */
uint32_t UnRegisterPowerModeChangedCallback(PowerManager_PowerModeChangedCb callback);
/** Register for PowerMode pre-change callback */
uint32_t RegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback, void* userdata);
/** UnRegister (previously registered) PowerMode pre-change callback */
uint32_t UnRegisterPowerModePreChangeCallback(PowerManager_PowerModePreChangeCb callback);
/** Register for PowerMode pre-change callback */
uint32_t RegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback, void* userdata);
/** UnRegister (previously registered) DeepSleep Timeout callback */
uint32_t UnRegisterDeepSleepTimeoutCallback(PowerManager_DeepSleepTimeoutCb callback);
/** Register for Network Standby Mode changed event - only on XIone */
uint32_t RegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) Network Standby Mode changed callback */
uint32_t UnRegisterNetworkStandbyModeChangedCallback(PowerManager_NetworkStandbyModeChangedCb callback);
/** Register for Thermal Mode changed event callback */
uint32_t RegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) Thermal Mode changed event callback */
uint32_t UnRegisterThermalModeChangedCallback(PowerManager_ThermalModeChangedCb callback);
/** Register for reboot start event callback */
uint32_t RegisterRebootBeginCallback(PowerManager_RebootBeginCb callback, void* userdata);
/** UnRegister (previously registered) reboot start event callback */
uint32_t UnRegisterRebootBeginCallback(PowerManager_RebootBeginCb callback);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // POWERMANAGER_CLIENT_H
