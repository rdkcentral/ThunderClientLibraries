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

/**
 * @brief Get a @ref deviceinfo_type instance that matches the DeviceInfo implementation.
 * 
 * @param deviceName Name of the implementation
 * @return @ref displayinfo_type instance, NULL if error. 
 */
EXTERNAL struct deviceinfo_type *deviceinfo_instance(const char deviceName[]);

/**
 * @brief Release the @ref deviceinfo_type instance
 * 
 * @param instance Instance of @ref deviceinfo_type
 */
EXTERNAL void deviceinfo_release(struct deviceinfo_type *instance);


/**
 * @brief Get software version (in form version#hashtag)

 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the software version
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_systeminfo_version(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/**
 * @brief Get system uptime in seconds
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return system uptime in seconds, 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_uptime(struct deviceinfo_type* instance);

/**
 * @brief Get total installed system RAM memory (in bytes)
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return total installed RAM (in bytes), 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_totalram(struct deviceinfo_type* instance);

/**
 * @brief Get free system RAM memory(in bytes)
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return free RAM (in bytes), 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_freeram(struct deviceinfo_type* instance);

/**
 * @brief Get total system swap space(in bytes)
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return total swap (in bytes), 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_totalswap(struct deviceinfo_type* instance);

/**
 * @brief Get free system swap space(in bytes)
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return free swap (in bytes), 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_freeswap(struct deviceinfo_type* instance);

/**
 * @brief Get current CPU load (percentage)
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return current CPU load (percentage), 0 on error or invalid connection
 */
EXTERNAL uint64_t deviceinfo_systeminfo_cpuload(struct deviceinfo_type *instance);

/**
 * @brief Get average CPU load(percentage)
 *         -load[0] -> 1 minutes average
 *         -load[1] -> 5 minutes average 
 *         -load[2] -> 15 minutes average         
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param loads MUST BE OF SIZE 3 - array that will contain the loads data.
 */
EXTERNAL void deviceinfo_systeminfo_avgcpuload(struct deviceinfo_type *instance, uint64_t load[]);

/**
 * @brief Get the Host name
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the Host name
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_systeminfo_devicename(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/**
 * @brief Get the Device serial number
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the serial number
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_systeminfo_serialnumber(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/**
 * @brief Get the current system date and time
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the current time
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_systeminfo_time(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/**
 * @brief Get the Interface name
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the interface name
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_addresses_name(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/**
 * @brief Get the Interface MAC address
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the Inteface MAC address
 * @param length Size of the @ref buffer
 */
EXTERNAL void deviceinfo_addresses_mac(struct deviceinfo_type *instance, char buffer[], const uint8_t length);

/** TODO - or single address?
 * @brief Get array of the Interface IP addresses
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @param buffer Buffer that will contain the Inteface IP addresses
 * @param buffer_length Size of the @ref buffer
 * @param length Size of each IP address
 */
EXTERNAL void deviceinfo_addresses_ip(struct deviceinfo_type *instance, char *buffer[], const uint8_t buffer_length, const uint8_t length);

/**
 * @brief Get number of runs
 * 
 * @param instance Instance of @ref deviceinfo_type
 * @return number of runs, 0 on error or invalid connection
 */
EXTERNAL uint32_t deviceinfo_socketinfo_runs(struct deviceinfo_type *instance);


/* TODO???
EXTERNAL uint32_t deviceinfo_socketinfo_open(struct deviceinfo_type *instance);
EXTERNAL uint32_t deviceinfo_socketinfo_link(struct deviceinfo_type *instance);
EXTERNAL uint32_t deviceinfo_socketinfo_exception(struct deviceinfo_type *instance);
EXTERNAL uint32_t deviceinfo_socketinfo_shutdown(struct deviceinfo_type *instance);
*/


#ifdef __cplusplus
} // extern "C"
#endif