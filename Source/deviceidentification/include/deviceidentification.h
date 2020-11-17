#pragma once

#include <stdint.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef DISPLAYINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "deviceidentification.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct deviceidentification_type;

/**
 * @brief Get a @ref deviceidentification_type instance that matches the DeviceIdentification implementation
 * 
 * @param name Name of the implementation
 * @return EXTERNAL struct* @ref instance, NULL if error.
 */
EXTERNAL struct deviceidentification_type* deviceidentification_instance(const char name[]);

/**
 * @brief Release the @ref instance
 * 
 * @param instance  Instance of @ref deviceidentification_type
 */
EXTERNAL void deviceidentification_release(struct deviceidentification_type* instance);

/**
 * @brief Get the device chipset name 
 * 
 * @param instance Instance of @ref deviceidentification_type
 * @param buffer Buffer that will contain the chipset name
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH
 *         if instance or buffer is null, returns 0
 */
EXTERNAL int16_t deviceidentification_chipset(struct deviceidentification_type* instance, char buffer[], const uint8_t length);

/**
 * @brief Get the device firmware version 
 * 
 * @param instance Instance of @ref deviceidentification_type
 * @param buffer Buffer that will contain the firmware version
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH of string
 *         if instance or buffer is null, returns 0
 */
EXTERNAL int16_t deviceidentification_firmware_version(struct deviceidentification_type* instance, char buffer[], const uint8_t length);

/**
 * @brief Get the device ID  
 * 
 * @param instance Instance of @ref deviceidentification_type
 * @param buffer Buffer that will contain the device ID
 * @param length Size of the @ref buffer
 * @return if the buffer is not big enough returns -LENGHTH of required buffer, otherwise return LENGTH of string
 *         returns 0 if ID is not available for the device, or instance or buffer is null.
 * 
 */
EXTERNAL int16_t deviceidentification_id(struct deviceidentification_type* instance, uint8_t buffer[], const uint8_t length);

#ifdef __cplusplus
} // extern "C"
#endif