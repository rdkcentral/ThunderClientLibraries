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

typedef enum PowerController_PowerState {
    POWER_STATE_UNKNOWN = 0 /* UNKNOWN */,
    POWER_STATE_OFF = 1 /* OFF */,
    POWER_STATE_STANDBY = 2 /* STANDBY */,
    POWER_STATE_ON = 3 /* ON */,
    POWER_STATE_STANDBY_LIGHT_SLEEP = 4 /* LIGHT_SLEEP */,
    POWER_STATE_STANDBY_DEEP_SLEEP = 5 /* DEEP_SLEEP */
} PowerController_PowerState_t;

typedef enum PowerController_ThermalTemperature {
    THERMAL_TEMPERATURE_UNKNOWN = 0 /* UNKNOWN Thermal Temperature */,
    THERMAL_TEMPERATURE_NORMAL = 1 /* Normal Thermal Temperature */,
    THERMAL_TEMPERATURE_HIGH = 2 /* High Thermal Temperature */,
    THERMAL_TEMPERATURE_CRITICAL = 4 /* Critial Thermal Temperature */
} PowerController_ThermalTemperature_t;

typedef enum PowerController_WakeupSrcType {
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
} PowerController_WakeupSrcType_t;

typedef enum PowerController_WakeupReason {
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
} PowerController_WakeupReason_t;

typedef enum PowerController_SystemMode {
    SYSTEM_MODE_UNKNOWN = 0 /* UNKNOWN */,
    SYSTEM_MODE_NORMAL = 1 /* NORMAL */,
    SYSTEM_MODE_EAS = 2 /* EAS */,
    SYSTEM_MODE_WAREHOUSE = 3 /* WAREHOUSE */
} PowerController_SystemMode_t;

#define POWER_CONTROLLER_ERROR_NONE 0
#define POWER_CONTROLLER_ERROR_GENERAL 1
#define POWER_CONTROLLER_ERROR_UNAVAILABLE 2

/**
 * @brief Initializes the Power Controller.
 *
 * This function creates an instance of the PowerManager plugin client interface and increments the client instance count.
 *
 * @details
 * - If the Power Controller instance does not already exist, it will be created.
 * - The instance count is incremented each time this function is called.
 *
 * @see PowerController_Term
 */
void PowerController_Init();

/**
 * @brief Terminates the Power Controller.
 *
 * This function decrements client instance count attempts to delete Power Controller instance
 *
 * @details
 * - If the controller reference count is greater than one, this function only decrements the count.
 * - When the reference count reaches zero, the controller instance is destroyed, and all associated resources are released (PowerManager plugin client instance).
 * - Ensure that this function is called once for every call to `PowerController_Init`.
 *
 * @see PowerController_Init
 */
void PowerController_Term();

/**
 * @brief Checks if the Power Manager plugin is active & operational
 *
 * This function determines whether the Power Manager interface is operational and ready to handle requests.
 * It can be used to verify the availability of the Power Manager client before initiating operations that depend on it.
 *
 * @return `true` if the Power Manager interface is active and operational, otherwise `false`.
 *
 * @details
 * - Use this function to confirm the operational status of the Power Manager plugin.
 * - Calling this function is NOT MANDATORY but optional
 * - Clients can register for notifications about state changes using `PowerController_RegisterOperationalStateChangeCallback`.
 * - If the Power Manager interface is not active, subsequent Power Manager operations will fail with the error `POWER_CONTROLLER_ERROR_UNAVAILABLE`.
 *
 * @see PowerController_RegisterOperationalStateChangeCallback
 */
bool PowerController_IsOperational();

/** Gets the Power State.*/
// @text getPowerState
// @brief Get Power State
// @param powerState: Get current power state
uint32_t PowerController_GetPowerState(PowerController_PowerState_t* currentState /* @out */, PowerController_PowerState_t* previousState /* @out */);

/** Sets Power State . */
// @text setPowerState
// @brief Set Power State
// @param keyCode: NA for most platfroms, to be depricated
// @param powerState: Set power to this state
// @param reason: null terminated string stating reason for for state change
uint32_t PowerController_SetPowerState(const int keyCode /* @in */, const PowerController_PowerState_t powerstate /* @in */, const char* reason /* @in */);

/** Gets the current Thermal state.*/
// @text getThermalState
// @brief Get Current Thermal State (temperature)
// @param currentTemperature: current temperature
uint32_t PowerController_GetThermalState(float* currentTemperature /* @out */);

/** Sets the Temperature Thresholds.*/
// @text setTemperatureThresholds
// @brief Set Temperature Thresholds
// @param high: high threshold
// @param critical : critical threshold
uint32_t PowerController_SetTemperatureThresholds(float high /* @in */, float critical /* @in */);

/** Gets the current Temperature Thresholds.*/
// @text getTemperatureThresholds
// @brief Get Temperature Thresholds
// @param high: high threshold
// @param critical : critical threshold
uint32_t PowerController_GetTemperatureThresholds(float* high /* @out */, float* critical /* @out */);

/** Sets the current Temperature Grace interval.*/
// @property
// @text PowerController_SetOvertempGraceInterval
// @brief Set Temperature Thresholds
// @param graceInterval: interval in secs?
uint32_t PowerController_SetOvertempGraceInterval(const int graceInterval /* @in */);

/** Gets the grace interval for over-temperature.*/
// @property
// @text PowerController_GetOvertempGraceInterval
// @brief Get Temperature Grace interval
// @param graceInterval: interval in secs?
uint32_t PowerController_GetOvertempGraceInterval(int* graceInterval /* @out */);

/** Set Deep Sleep Timer for later wakeup */
// @property
// @text setDeepSleepTimer
// @brief Set Deep sleep timer for timeOut period
// @param timeOut: deep sleep timeout
uint32_t PowerController_SetDeepSleepTimer(const int timeOut /* @in */);

/** Get Last Wakeup reason */
// @property
// @text getLastWakeupReason
// @brief Get Last Wake up reason
// @param wakeupReason: wake up reason
uint32_t PowerController_GetLastWakeupReason(PowerController_WakeupReason_t* wakeupReason /* @out */);

/** Get Last Wakeup key code */
// @property
// @text getLastWakeupKeyCode
// @brief Get the key code that can be used for wakeup
// @param keycode: Key code for wakeup
uint32_t PowerController_GetLastWakeupKeyCode(int* keycode /* @out */);

/** Request Reboot with PowerManager */
// @text reboot
// @brief Reboot device
// @param rebootRequestor: null terminated string identifier for the entity requesting the reboot.
// @param rebootReasonCustom: custom-defined reason for the reboot, provided as a null terminated string.
// @param rebootReasonOther: null terminated string describing any other reasons for the reboot.
uint32_t PowerController_Reboot(const char* rebootRequestor /* @in */, const char* rebootReasonCustom /* @in */, const char* rebootReasonOther /* @in */);

/** Set Network Standby Mode */
// @property
// @text setNetworkStandbyMode
// @brief Set the standby mode for Network
// @param standbyMode: Network standby mode
uint32_t PowerController_SetNetworkStandbyMode(const bool standbyMode /* @in */);

/** Get Network Standby Mode */
// @text getNetworkStandbyMode
// @brief Get the standby mode for Network
// @param standbyMode: Network standby mode
uint32_t PowerController_GetNetworkStandbyMode(bool* standbyMode /* @out */);

/** Set Wakeup source configuration */
// @text setWakeupSrcConfig
// @brief Set the source configuration for device wakeup
// @param powerMode: power mode
// @param wakeSrcType: source type
// @param config: config
uint32_t PowerController_SetWakeupSrcConfig(const int powerMode /* @in */, const int wakeSrcType /* @in */, int config /* @in */);

/** Get Wakeup source configuration */
// @text getWakeupSrcConfig
// @brief Get the source configuration for device wakeup
// @param powerMode: power mode
// @param srcType: source type
// @param config: config
uint32_t PowerController_GetWakeupSrcConfig(int* powerMode /* @out */, int* srcType /* @out */, int* config /* @out */);

/** Initiate System mode change */
// @text PowerController_SetSystemMode
// @brief System mode change
// @param oldMode: current mode
// @param newMode: new mode
uint32_t PowerController_SetSystemMode(const PowerController_SystemMode_t currentMode /* @in */, const PowerController_SystemMode_t newMode /* @in */);

/** Get Power State before last reboot */
// @text PowerController_GetPowerStateBeforeReboot
// @brief Get Power state before last reboot
// @param powerStateBeforeReboot: power state
uint32_t PowerController_GetPowerStateBeforeReboot(PowerController_PowerState_t* powerStateBeforeReboot /* @out */);

/* Callback data types for event notifications from power manager plugin */
typedef void (*PowerController_OperationalStateChangeCb)(bool isOperational, void* userdata);
typedef void (*PowerController_PowerModeChangedCb)(const PowerController_PowerState_t currentState, const PowerController_PowerState_t newState, void* userdata);
typedef void (*PowerController_PowerModePreChangeCb)(const PowerController_PowerState_t currentState, const PowerController_PowerState_t newState, void* userdata);
typedef void (*PowerController_DeepSleepTimeoutCb)(const int wakeupTimeout, void* userdata);
typedef void (*PowerController_NetworkStandbyModeChangedCb)(const bool enabled, void* userdata);
typedef void (*PowerController_ThermalModeChangedCb)(const PowerController_ThermalTemperature_t currentThermalLevel, const PowerController_ThermalTemperature_t newThermalLevel, const float currentTemperature, void* userdata);
typedef void (*PowerController_RebootBeginCb)(const char* rebootReasonCustom, const char* rebootReasonOther, const char* rebootRequestor, void* userdata);

/* Type defines for callbacks / notifications */
/* userdata in all callbacks are opque, clients can use to have context to callbacks */

/** Register for PowerManager plugin operational state change event callback, for initial state use `PowerController_IsOperational` call */
uint32_t PowerController_RegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback, void* userdata);
/** UnRegister (previously registered) PowerManager plugin operational state change event callback */
uint32_t PowerController_UnRegisterOperationalStateChangeCallback(PowerController_OperationalStateChangeCb callback);
/** Register for PowerMode changed callback */
uint32_t PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) PowerMode changed callback */
uint32_t PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb callback);
/** Register for PowerMode pre-change callback */
uint32_t PowerController_RegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback, void* userdata);
/** UnRegister (previously registered) PowerMode pre-change callback */
uint32_t PowerController_UnRegisterPowerModePreChangeCallback(PowerController_PowerModePreChangeCb callback);
/** Register for PowerMode pre-change callback */
uint32_t PowerController_RegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback, void* userdata);
/** UnRegister (previously registered) DeepSleep Timeout callback */
uint32_t PowerController_UnRegisterDeepSleepTimeoutCallback(PowerController_DeepSleepTimeoutCb callback);
/** Register for Network Standby Mode changed event - only on XIone */
uint32_t PowerController_RegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) Network Standby Mode changed callback */
uint32_t PowerController_UnRegisterNetworkStandbyModeChangedCallback(PowerController_NetworkStandbyModeChangedCb callback);
/** Register for Thermal Mode changed event callback */
uint32_t PowerController_RegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback, void* userdata);
/** UnRegister (previously registered) Thermal Mode changed event callback */
uint32_t PowerController_UnRegisterThermalModeChangedCallback(PowerController_ThermalModeChangedCb callback);
/** Register for reboot start event callback */
uint32_t PowerController_RegisterRebootBeginCallback(PowerController_RebootBeginCb callback, void* userdata);
/** UnRegister (previously registered) reboot start event callback */
uint32_t PowerController_UnRegisterRebootBeginCallback(PowerController_RebootBeginCb callback);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // POWERMANAGER_CLIENT_H
