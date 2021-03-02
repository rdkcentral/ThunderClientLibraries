#pragma once

#include <stdint.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef DISPLAYINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "deviceinfo.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct deviceinfo_type;

typedef enum deviceinfo_output_resolution_type {
    DEVICEINFO_RESOLUTION_UNKNOWN,
    DEVICEINFO_RESOLUTION_480I,
    DEVICEINFO_RESOLUTION_480P,
    DEVICEINFO_RESOLUTION_576I,
    DEVICEINFO_RESOLUTION_576P,
    DEVICEINFO_RESOLUTION_720P,
    DEVICEINFO_RESOLUTION_1080I,
    DEVICEINFO_RESOLUTION_1080P,
    DEVICEINFO_RESOLUTION_2160P30,
    DEVICEINFO_RESOLUTION_2160P60,
    DEVICEINFO_RESOLUTION_4320P30,
    DEVICEINFO_RESOLUTION_4320P60,
    DISPLAYINFO_RESOLUTION_UNKNOWN
} deviceinfo_output_resolution_t;

/**
 * @brief Get a @ref deviceinfo_type instance that matches the DeviceIdentification implementation
 * 
 * @param name Name of the implementation
 * @return EXTERNAL struct* @ref instance, NULL if error.
 */
EXTERNAL struct deviceinfo_type* deviceinfo_instance(const char name[]);

/**
 * @brief Release the @ref instance
 * 
 * @param instance  Instance of @ref deviceinfo_type
 */
EXTERNAL void deviceinfo_release(struct deviceinfo_type* instance);

/**
 * @brief Get the device chipset name 
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the chipset name
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH
 *         if instance or buffer is null, returns 0
 */
EXTERNAL int16_t deviceinfo_chipset(struct deviceinfo_type* instance, char buffer[], const uint8_t length);

/**
 * @brief Get the device firmware version 
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the firmware version
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH of string
 *         if instance or buffer is null, returns 0
 */
EXTERNAL int16_t deviceinfo_firmware_version(struct deviceinfo_type* instance, char buffer[], const uint8_t length);

/**
 * @brief Get the device ID  
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the device ID
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH of string
 *         returns 0 if ID is not available for the device, or instance or buffer is null.
 * 
 */
EXTERNAL int16_t deviceinfo_id(struct deviceinfo_type* instance, uint8_t buffer[], const uint8_t length);

/**
 * @brief Get the device maximum supported output resolution
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the firmware version
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH of string
 *         if instance or buffer is null, returns 0
 */
EXTERNAL deviceinfo_output_resolution_t deviceinfo_output_resolution(struct deviceinfo_type* instance);

#ifdef __cplusplus
} // extern "C"
#endif