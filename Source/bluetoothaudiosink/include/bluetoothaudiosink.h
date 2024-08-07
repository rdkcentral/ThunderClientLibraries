/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#ifndef BLUETOOTHAUDIOSINK_H
#define BLUETOOTHAUDIOSINK_H

#include <stdint.h>

#undef EXTERNAL
#define EXTERNAL __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

#define BLUETOOTHAUDIOSINK_SUCCESS (0)

typedef struct {
    uint32_t sample_rate;
    uint16_t frame_rate;
    uint8_t resolution;
    uint8_t channels;
} bluetoothaudiosink_format_t;

typedef enum {
    BLUETOOTHAUDIOSINK_STATE_UNASSIGNED = 0,
    BLUETOOTHAUDIOSINK_STATE_DISCONNECTED,
    BLUETOOTHAUDIOSINK_STATE_CONNECTING,
    BLUETOOTHAUDIOSINK_STATE_CONNECTED,
    BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD,
    BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED,
    BLUETOOTHAUDIOSINK_STATE_READY,
    BLUETOOTHAUDIOSINK_STATE_STREAMING
} bluetoothaudiosink_state_t;

typedef void (*bluetoothaudiosink_operational_state_update_cb)(const uint8_t running /* 0 == not running */, void *user_data);
typedef void (*bluetoothaudiosink_state_changed_cb)(const bluetoothaudiosink_state_t state, void *user_data);


EXTERNAL uint32_t bluetoothaudiosink_register_operational_state_update_callback(const bluetoothaudiosink_operational_state_update_cb callback, void *user_data);
EXTERNAL uint32_t bluetoothaudiosink_unregister_operational_state_update_callback(const bluetoothaudiosink_operational_state_update_cb callback);

EXTERNAL uint32_t bluetoothaudiosink_register_state_changed_callback(const bluetoothaudiosink_state_changed_cb callback, void *user_data);
EXTERNAL uint32_t bluetoothaudiosink_unregister_state_changed_callback(const bluetoothaudiosink_state_changed_cb callback);

EXTERNAL uint32_t bluetoothaudiosink_state(bluetoothaudiosink_state_t *state);

EXTERNAL uint32_t bluetoothaudiosink_configure(const bluetoothaudiosink_format_t *format);
EXTERNAL uint32_t bluetoothaudiosink_acquire(void);
EXTERNAL uint32_t bluetoothaudiosink_relinquish(void);
EXTERNAL uint32_t bluetoothaudiosink_speed(const int8_t speed);
EXTERNAL uint32_t bluetoothaudiosink_time(uint32_t *out_time_ms);
EXTERNAL uint32_t bluetoothaudiosink_delay(uint32_t *out_delay_samples);
EXTERNAL uint32_t bluetoothaudiosink_frame(const uint16_t length, const uint8_t data[], uint16_t *consumed);

EXTERNAL uint32_t bluetoothaudiosink_init(void);
EXTERNAL uint32_t bluetoothaudiosink_deinit(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BLUETOOTHAUDIOSINK_H
