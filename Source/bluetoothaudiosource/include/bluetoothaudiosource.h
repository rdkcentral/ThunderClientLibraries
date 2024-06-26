/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#ifndef BLUETOOTHAUDIOSOURCE_H
#define BLUETOOTHAUDIOSOURCE_H

#include <stdint.h>

#undef EXTERNAL
#define EXTERNAL __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

#define BLUETOOTHAUDIOSOURCE_SUCCESS (0)

typedef struct {
    uint32_t sample_rate;
    uint16_t frame_rate;
    uint8_t resolution;
    uint8_t channels;
} bluetoothaudiosource_format_t;

typedef enum {
    BLUETOOTHAUDIOSOURCE_STATE_DISCONNECTED = 0,
    BLUETOOTHAUDIOSOURCE_STATE_CONNECTING,
    BLUETOOTHAUDIOSOURCE_STATE_CONNECTED,
    BLUETOOTHAUDIOSOURCE_STATE_CONNECTED_BAD,
    BLUETOOTHAUDIOSOURCE_STATE_READY,
    BLUETOOTHAUDIOSOURCE_STATE_STREAMING
} bluetoothaudiosource_state_t;

typedef void *bluetoothaudiosurce_player_t;

typedef void (*bluetoothaudiosource_operational_state_update_cb)(const uint8_t running /* 0 == not running */, void *user_data);
typedef void (*bluetoothaudiosource_state_changed_cb)(const bluetoothaudiosource_state_t state, void *user_data);
typedef uint32_t (*bluetoothaudiosource_configure_cb)(const bluetoothaudiosource_format_t *format, void *user_data);
typedef uint32_t (*bluetoothaudiosource_acquire_cb)(void *user_data);
typedef uint32_t (*bluetoothaudiosource_relinquish_cb)(void *user_data);
typedef uint32_t (*bluetoothaudiosource_set_speed_cb)(const int8_t speed, void *user_data);
typedef uint32_t (*bluetoothaudiosource_get_time_cb)(uint32_t* time_ms, void *user_data);
typedef uint32_t (*bluetoothaudiosource_get_delay_cb)(uint32_t* delay_samples, void *user_data);
typedef void (*bluetoothaudiosource_frame_cb)(const uint16_t length_bytes, const uint8_t frame[], void *user_data);

typedef struct bluetoothaudiosource_sink_tag {
    bluetoothaudiosource_configure_cb configure_cb;
    bluetoothaudiosource_acquire_cb acquire_cb;
    bluetoothaudiosource_relinquish_cb relinquish_cb;
    bluetoothaudiosource_set_speed_cb set_speed_cb;
    bluetoothaudiosource_get_time_cb get_time_cb;
    bluetoothaudiosource_get_delay_cb get_delay_cb;
    bluetoothaudiosource_frame_cb frame_cb;
} bluetoothaudiosource_sink_t;


EXTERNAL uint32_t bluetoothaudiosource_register_operational_state_update_callback(const bluetoothaudiosource_operational_state_update_cb callback, void *user_data);
EXTERNAL uint32_t bluetoothaudiosource_unregister_operational_state_update_callback(const bluetoothaudiosource_operational_state_update_cb callback);

EXTERNAL uint32_t bluetoothaudiosource_register_state_changed_callback(const bluetoothaudiosource_state_changed_cb callback, void *user_data);
EXTERNAL uint32_t bluetoothaudiosource_unregister_state_changed_callback(const bluetoothaudiosource_state_changed_cb callback);

EXTERNAL uint32_t bluetoothaudiosource_set_sink(const bluetoothaudiosource_sink_t *sink, void *user_data);

EXTERNAL uint32_t bluetoothaudiosource_get_state(bluetoothaudiosource_state_t *out_state);
EXTERNAL uint32_t bluetoothaudiosource_get_device(uint8_t out_address[6]);
EXTERNAL uint32_t bluetoothaudiosource_get_time(uint32_t *out_time_ms);

EXTERNAL uint32_t bluetoothaudiosource_set_speed(const int8_t speed);

EXTERNAL uint32_t bluetoothaudiosource_relinquish(void);

EXTERNAL uint32_t bluetoothaudiosource_dispose(void);

#ifdef __cplusplus
} // extern "C"
#endif

WPEFRAMEWORK_NESTEDNAMESPACE_COMPATIBILIY(BluetoothAudioSourceClient);

#endif // BLUETOOTHAUDIOSOURCE_H
