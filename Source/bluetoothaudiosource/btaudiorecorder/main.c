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


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <alloca.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <endian.h>

#include "../include/bluetoothaudiosource.h"


#define TRACE(format, ...) printf("btaudiorecorder: " format "\n", ##__VA_ARGS__);
#define ERROR(format, ...) fprintf(stderr, "btaudiorecorder: [\033[31merror\033[0m]: " format "\n", ##__VA_ARGS__)

static volatile bool user_break = false;

typedef struct {
    const char* file_name;
    FILE* file;
    bluetoothaudiosource_format_t format;
    bool recording;
    uint32_t recorded_bytes;
    sem_t connect_sync;
    sem_t record_sync;
} context_t;

typedef struct {
    // riff chunk
    char riff[4];
    uint32_t riff_size;
    char wave[4];
    // fmt chunk
    char fmt[4];
    uint32_t fmt_size;
    uint16_t type;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t resolution;
    // data chun
    char data[4];
    uint32_t data_size;
} __attribute__((packed)) wav_header_t;


static uint32_t on_configure_sink(const bluetoothaudiosource_format_t *format, void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (user_data && format) {
        context_t *context = ((context_t *) user_data);

        TRACE("Bluetooth audio source was configured");

        TRACE("Configuring sink to parameters: %d Hz, %d bits, %d channel(s), %d.%02d fps",
                format->sample_rate, format->resolution, format->channels,
                (format->frame_rate / 100), (format->frame_rate % 100));

        // Just cache the stream parameters...
        memcpy(&context->format, format, sizeof(*format));

        result = BLUETOOTHAUDIOSOURCE_SUCCESS;
    }

    return (result);
}

static uint32_t on_acquire_sink(void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (user_data) {
        context_t *context = (context_t *) user_data;

        TRACE("Bluetooth audio source was acquired");

        context->file = fopen(context->file_name, "wb");

        if (context->file) {
            // Fill in the header data accordingly to the source format.
            // We're a simple .WAV writer: we accept anything they throw at us.
            wav_header_t header;
            memset(&header, 0, sizeof(header));
            memcpy(&header.riff, "RIFF", 4);
            memcpy(&header.wave, "WAVE", 4);
            memcpy(&header.fmt, "fmt ", 4);
            memcpy(&header.data, "data", 4);
            header.fmt_size = htole32(16); // "fmt" chunk size
            header.type = htole16(1); // uncompressed linear PCM, signed if > 8 bit, little endian if > 8 bit
            header.channels = htole16(context->format.channels);
            header.sample_rate = htole32(context->format.sample_rate);
            header.resolution = htole16(context->format.resolution);

            // Whatever... :)
            header.byte_rate = htole32((context->format.sample_rate * context->format.resolution * context->format.channels) / 8);
            header.block_align = htole16((context->format.resolution * context->format.channels) / 8);

            // And push the header to the file...
            fwrite(&header, sizeof(header), 1, context->file);

            TRACE("Created wave file '%s'", context->file_name);

            TRACE("Successfully acquired audio sink");

            sem_post(&context->connect_sync);

            result = BLUETOOTHAUDIOSOURCE_SUCCESS;
        }
        else {
            ERROR("Failed to create wave file '%s'!", context->file_name);
        }
    }

    return (result);
}

static uint32_t on_relinquish_sink(void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (user_data) {
        context_t *context = user_data;

        TRACE("Bluetooth audio source was relinquished");

        context->recording = false;

        if (context->file) {
            // Retrospectively write file size back into the WAV header.
            uint32_t size = 0;
            fseek(context->file, 0, SEEK_END);
            size = ftell(context->file);
            size -= 8; // without "RIFF"+size
            size = htole32(size);
            fseek(context->file, 4, SEEK_SET);
            fwrite(&size, sizeof(size), 1, context->file);

            // And data size...
            size = htole32(context->recorded_bytes);
            fseek(context->file, (sizeof(wav_header_t) - 4), SEEK_SET);
            fwrite(&size, sizeof(size), 1, context->file);

            // Over and done!
            fclose(context->file);
            context->file = NULL;

            TRACE("Successfully relinquished audio sink");

            result = BLUETOOTHAUDIOSOURCE_SUCCESS;
        }
    }

    return (result);
}

static uint32_t on_change_sink_speed(const int8_t speed, void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (user_data) {
        context_t *context = user_data;

        TRACE("Bluetooth audio source changed playback speed to %d%%", speed);

        if (speed == 100) {
            context->recording = true;
            result = BLUETOOTHAUDIOSOURCE_SUCCESS;
            sem_post(&context->record_sync);
        }
        else if (speed == 0) {
            result = BLUETOOTHAUDIOSOURCE_SUCCESS;
            context->recording = false;
        }
    }

    return (result);
}

static uint32_t on_get_time(uint32_t *out_time_ms, void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (out_time_ms && user_data) {
        context_t *context = user_data;

        (*out_time_ms) = ((1000ULL * context->recorded_bytes) / (context->format.frame_rate * context->format.channels * (context->format.resolution / 8)));

        result = BLUETOOTHAUDIOSOURCE_SUCCESS;
    }

    return (result);
}

static uint32_t on_get_delay(uint32_t *out_delay_samples, void *user_data)
{
    uint32_t result = ~BLUETOOTHAUDIOSOURCE_SUCCESS;

    if (out_delay_samples && user_data) {

        // This is a disk writer, we're super fast!
        (*out_delay_samples) = 0;

        result = BLUETOOTHAUDIOSOURCE_SUCCESS;
    }

    return (result);
}

static void on_frame_received(const uint16_t length, const uint8_t frame[], void *user_data)
{
    // TRACE("Frame received (%d bytes)", length);

    if (frame && user_data) {
        context_t *context = user_data;

        if (context->file && context->recording) {
            fwrite(frame, length, 1, context->file);
            context->recorded_bytes += length;
        }
    }
}

static bluetoothaudiosource_sink_t callbacks = {
    on_configure_sink,
    on_acquire_sink,
    on_relinquish_sink,
    on_change_sink_speed,
    on_get_time,
    on_get_delay,
    on_frame_received
};

static void state_changed(const bluetoothaudiosource_state_t state, __attribute__((unused)) void *user_data)
{
    assert(user_data != NULL);

    switch (state) {
    case BLUETOOTHAUDIOSOURCE_STATE_DISCONNECTED:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_DISCONNECTED");
        break;
    case BLUETOOTHAUDIOSOURCE_STATE_CONNECTING:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_CONNECTING");
        break;
    case BLUETOOTHAUDIOSOURCE_STATE_CONNECTED:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_CONNECTED");
        break;
    case BLUETOOTHAUDIOSOURCE_STATE_CONNECTED_BAD:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_CONNECTED_BAD");
        break;
    case BLUETOOTHAUDIOSOURCE_STATE_READY:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_READY");
        break;
    case BLUETOOTHAUDIOSOURCE_STATE_STREAMING:
        TRACE("State: BLUETOOTHAUDIOSOURCE_STATE_STREAMING");
        break;
    default:
        assert(!"Unmapped enum");
        break;
    }
}

static void operational_state_update(const uint8_t running, void *user_data)
{
    if (running) {
        TRACE("Bluetooth Audio Source service now available");

        // Register for the source updates.
        if (bluetoothaudiosource_register_state_changed_callback(state_changed, user_data) != 0) {
            ERROR("Failed to register state update callback!");
        }
        else if (bluetoothaudiosource_set_sink(&callbacks, user_data) != 0) {
            ERROR("Failed to set sink callbacks!");
        }
    }
    else {
        TRACE("Bluetooth Audio Source service is now unavailable");
    }
}

void ctrl_c_handler(int signal)
{
    // Ctrl+C!
    (void) signal;
    user_break = true;
}

int main(const int argc, const char* argv[])
{
    int result = 1;

    TRACE("Records incoming Bluetooth audio stream to  a .wav file");

    if (argc != 2) {
        TRACE("arguments:\n\t%s <file>", argv[0]);
    }
    else {
        context_t context;
        memset(&context, 0, sizeof(context));

        context.file_name = argv[1];

        sem_init(&context.connect_sync, 0, 0);
        sem_init(&context.record_sync, 0, 0);

        if (bluetoothaudiosource_register_operational_state_update_callback(&operational_state_update, &context) != BLUETOOTHAUDIOSOURCE_SUCCESS) {
            ERROR("Failed to register Bluetooth Audio Source service operational callback");
        }
        else {
            TRACE("Waiting for audio source to connect...");
            setbuf(stdout, NULL);

            uint32_t timeout = 120 /* sec */;
            struct timespec ts;

            // Install a Ctrl+C handler.
            struct sigaction handler;
            handler.sa_handler = ctrl_c_handler;
            sigemptyset(&handler.sa_mask);
            handler.sa_flags = 0;
            sigaction(SIGINT, &handler, NULL);

            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout;

            // Wait for connection...
            if (sem_timedwait(&context.connect_sync, &ts) != -1) {

                TRACE("Waiting for audio source to start streaming...");

                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                timeout = 600;
                ts.tv_sec += timeout;

                while (!user_break) {
                    // Wait for playback start...
                    if (sem_timedwait(&context.record_sync, &ts) != -1) {
                        TRACE("Recording...");

                        while (context.recording == true) {

                            usleep(50 * 1000);

                            printf("Captured %i kilobytes\r", (context.recorded_bytes / 1024));

                            if ((user_break != false) && (context.recording == true)) {
                                // Ctrl+C!
                                printf("\n");
                                TRACE("User break! Stopping recording...");
                                context.recording = false;
                            }
                        }


                        if (!user_break) {
                            printf("\n");
                            TRACE("Recording paused");
                        }
                    }
                    else if (!user_break) {
                        ERROR("Streaming has not started in %i seconds since connecting, terminating!", timeout);
                        break;
                    }
                }

                TRACE("Recording complete. Exiting...");

                // Let them know we're exiting.
                if (bluetoothaudiosource_relinquish() != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                    TRACE("Failed to relinquish source!");
                }

                // Unregister ourselves.
                if (bluetoothaudiosource_set_sink(NULL, NULL) != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                    TRACE("Failed to unset sink callbacks!");
                }

                result = 0;
            }
            else {
                ERROR("Bluetooth audio source device not connected in %i seconds, terminating!", timeout);
            }

            bluetoothaudiosource_dispose();
        }
    }

    return (result);
}
