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

#include "../include/bluetoothaudiosink.h"

#define CDDA_FRAMERATE (7500 /* cHz */)

#define PRINT(format, ...) printf(format "\n", ##__VA_ARGS__)
#define PRINT_NO_CR(format, ...) printf(format, ##__VA_ARGS__)
#define TRACE(format, ...) fprintf(stderr, "btaudioplayer: " format "\n", ##__VA_ARGS__)
#define ERROR(format, ...) fprintf(stderr, "btaudioplayer: [\033[31merror\033[0m]: " format "\n", ##__VA_ARGS__)


typedef struct {
    const char* file;
    volatile bool connected;
    volatile bool playing;
    volatile bool release;
    volatile bool exit;
    bluetoothaudiosink_format_t format;
    pthread_t thread;
    sem_t connect_sync;
    sem_t playback_sync;
} context_t;

typedef struct {
    char riff[4];
    uint32_t riff_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t type;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t pad1;
    uint16_t pad2;
    uint16_t resolution;
    char data[4];
    uint32_t data_size;
} __attribute__((packed)) wav_header_t;

static volatile bool user_break = false;


static void* playback_task(void *user_data)
{
    context_t *context = (context_t*)user_data;
    assert(context);

    TRACE("WAV streaming thread started");

    if (bluetoothaudiosink_configure(&context->format) != BLUETOOTHAUDIOSINK_SUCCESS) {
        TRACE("Failed to configure Bluetooth audio sink device!");
    }
    else if (bluetoothaudiosink_acquire() != BLUETOOTHAUDIOSINK_SUCCESS) {
        TRACE("Failed to acquire Bluetooth audio sink device!");
    } else {
        TRACE("Starting a playback session...");
        context->release = true;

        if (bluetoothaudiosink_speed(100) == 0) {
            FILE *f = fopen(context->file, "rb+");
            if (f != NULL) {

                const uint16_t bufferSize = ((100UL * context->format.channels * context->format.resolution * (context->format.sample_rate / context->format.frame_rate)) / 8);
                uint8_t *data = alloca(bufferSize);
                uint16_t to_play = 0;

                TRACE("Opened file '%s' (read buffer size: %i bytes)", context->file, bufferSize);

                fseek(f, sizeof(wav_header_t), SEEK_SET);

                while (context->exit == false) {
                    sem_wait(&context->playback_sync);

                    TRACE("Unpaused...");

                    while (context->playing == true) {
                        uint16_t played = 0;

                        if (to_play == 0) {
                            to_play = fread(data, 1, bufferSize, f);
                        }

                        if (bluetoothaudiosink_frame(to_play, data, &played) != 0) {
                            TRACE("Failed to send audio frame!");
                            context->exit = true;
                            break;
                        }

                        if (to_play != bufferSize) {
                            TRACE("EOF reached");
                            context->exit = true;
                            break;
                        }

                        if (user_break == true) {
                            TRACE("User break!");
                            context->exit = true;
                            break;
                        }

                        if (played < to_play) {
                            // we're sending too fast...
                            usleep(1 * 1000);
                        }

                        to_play -= played;
                    }

                    TRACE("Paused...");
                }

                context->playing = false;

                fclose(f);
                TRACE("Closed file '%s'", context->file);
            } else {
                TRACE("Failed to open file '%s'", context->file);
            }

            if (context->release) {
                if (bluetoothaudiosink_speed(0) != 0) {
                    TRACE("Failed to set audio speed 0%%!");
                }

                if (bluetoothaudiosink_relinquish() != 0) {
                    TRACE("Failed to reqlinquish bluetooth audio sink device!");
                }
            }
        } else {
            TRACE("Failed to set audio speed 100%%!");
        }
    }

    TRACE("File streaming thread terminated!");

    return (NULL);
}

static void audio_sink_disconnected(context_t *context)
{
    assert(context);

    if (context->connected) {
        TRACE("Remote device closed. Stopping playback!");
        context->exit = true;
        context->release = false;
        context->playing = false;
        sem_post(&context->playback_sync);
    }
}

static void audio_sink_connected(context_t * context)
{
    assert(context);

    if (!context->playing) {
        context->connected = true;
        sem_post(&context->connect_sync);
    } else {
        context->playing = false;
    }
}

static void audio_sink_playing(context_t * context,  const bool enable)
{
    assert(context);

    if (enable) {
        context->playing = true;
        sem_post(&context->playback_sync);
    }
    else {
        context->playing = false;
    }
}

static void audio_sink_state_changed(const bluetoothaudiosink_state_t state, void *user_data)
{
    switch (state) {
    case BLUETOOTHAUDIOSINK_STATE_UNASSIGNED:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_UNASSIGNED");
        break;
    case BLUETOOTHAUDIOSINK_STATE_CONNECTING:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_CONNECTING");
        break;
    case BLUETOOTHAUDIOSINK_STATE_CONNECTED:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_CONNECTED");
        audio_sink_connected((context_t*)user_data);
        break;
    case BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD");
        break;
    case BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED");
        break;
    case BLUETOOTHAUDIOSINK_STATE_DISCONNECTED:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_DISCONNECTED");
        audio_sink_disconnected((context_t*)user_data);
        break;
    case BLUETOOTHAUDIOSINK_STATE_READY:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_READY");
        audio_sink_playing((context_t*)user_data, false);
        break;
    case BLUETOOTHAUDIOSINK_STATE_STREAMING:
        TRACE("State: BLUETOOTHAUDIOSINK_STATE_STREAMING");
        audio_sink_playing((context_t*)user_data, true);
        break;
    default:
        assert(!"Unmapped enum");
        break;
    }
}

static void audio_sink_operational_state_update(const uint8_t running, void *user_data)
{
    if (running) {
        // Register for the sink updates
        if (bluetoothaudiosink_register_state_changed_callback(audio_sink_state_changed, user_data) != 0) {
            ERROR("Failed to register sink sink update callback!");
        }
    } else {
        TRACE("Bluetooth Audio Sink service is now unavailable");
        audio_sink_disconnected((context_t*)user_data);
    }
}

void ctrl_c_handler(int signal)
{
    // Ctrl+C!
    (void) signal;
    user_break = true;
}


int main(int argc, const char* argv[])
{
    int result = 1;

    PRINT("Plays a .wav file over a Bluetooth speaker device");

    if (argc == 2) {
        FILE *f;
        context_t context;
        memset(&context, 0, sizeof(context));

        context.file = argv[1];
        context.playing = false;

        f = fopen(context.file, "rb+");
        if (f != NULL) {
            wav_header_t header = { 0 };

            if (fread(&header, 1, sizeof(header), f) == sizeof(header)) {
                if (memcmp(header.wave, "WAVE", 4) == 0) {
                    context.format.sample_rate = le32toh(header.sample_rate);
                    context.format.frame_rate = CDDA_FRAMERATE;
                    context.format.channels = (uint8_t) le16toh(header.channels);
                    context.format.resolution = (uint8_t) le16toh(header.resolution);

                    TRACE("Input format: PCM %i Hz, %i bit (signed, little endian), %i channels @ %i.%02i fps",
                        context.format.sample_rate, context.format.resolution, context.format.channels,
                        (context.format.frame_rate/100), (context.format.frame_rate%100));
                }
            }

            fclose(f);
            f = NULL;

            if (((context.format.sample_rate >= 8000) && (context.format.channels == 1)) || ((context.format.channels == 2) && (context.format.resolution == 16))) {
                TRACE("Waiting for Bluetooth audio sink device to be available... (Press Ctrl+C to quit at any time.)");

                setbuf(stdout, NULL);

                sem_init(&context.connect_sync, 0, 0);
                sem_init(&context.playback_sync, 0, 0);

                // Register for the audio sink service updates
                if (bluetoothaudiosink_register_operational_state_update_callback(&audio_sink_operational_state_update, &context) != BLUETOOTHAUDIOSINK_SUCCESS) {
                    ERROR("Failed to register Bluetooths Audio Sink operational callback");
                } else {
                    const uint32_t timeout = 120 /* sec */;
                    struct timespec ts;

                    // Install a Ctrl+C handler
                    struct sigaction handler;
                    handler.sa_handler = ctrl_c_handler;
                    sigemptyset(&handler.sa_mask);
                    handler.sa_flags = 0;
                    sigaction(SIGINT, &handler, NULL);

                    clock_gettime(CLOCK_REALTIME, &ts);
                    ts.tv_sec += timeout;

                    bluetoothaudiosink_init();

                    // Wait for connection...
                    if (sem_timedwait(&context.connect_sync, &ts) != -1) {

                        pthread_create(&context.thread, NULL, playback_task, (void*)&context);

                        while (context.exit == false) {
                            uint32_t time;

                            if (bluetoothaudiosink_time(&time) == BLUETOOTHAUDIOSINK_SUCCESS) {
                                printf("playing %d:%02d.%03d\r", time/60000, (time/1000)%60, time%1000);
                            }

                            usleep(10 * 1000);
                        }

                        TRACE("Exiting...");

                        pthread_join(context.thread, NULL);

                    } else {
                        ERROR("Bluetooth audio sink device not connected in %d seconds, terminating!", timeout);
                    }

                    // And we're done...
                    TRACE("Cleaning up...");
                    bluetoothaudiosink_unregister_state_changed_callback(&audio_sink_state_changed); // this can fail, it's OK
                    bluetoothaudiosink_unregister_operational_state_update_callback(&audio_sink_operational_state_update);
                    bluetoothaudiosink_deinit();
                }
            } else {
                ERROR("Invalid file format!");
            }
        } else {
            ERROR("Failed to open the source file!");
        }
    } else {
        PRINT("arguments:\n%s <file.wav>", argv[0]);
    }

    return (result);
}
