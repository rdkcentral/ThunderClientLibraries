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
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <bluetoothaudiosink.h>

#define CDDA_FRAMERATE (7500 /* cHz */)

#define TRACE(format, ...) fprintf(stderr, "btaudioplayer: " format "\n", ##__VA_ARGS__)


typedef struct {
    const char* file;
    bool do_acquire;
    bool playing;
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

    TRACE("WAV streaming thread started");

    if (bluetoothaudiosink_acquire(&context->format) != 0) {
        TRACE("Failed to acquire Bluetooth audio sink device!");
    } else {
        TRACE("Starting a playback session...");
        context->do_acquire = false;

        if (bluetoothaudiosink_speed(100) == 0) {
            FILE *f = fopen(context->file, "rb+");
            if (f != NULL) {

                const uint16_t bufferSize = ((100UL * context->format.channels * context->format.resolution * (context->format.sample_rate / context->format.frame_rate)) / 8);
                uint8_t *data = alloca(bufferSize);
                uint16_t to_play = 0;

                TRACE("Opened file '%s' (read buffer size: %i bytes)", context->file, bufferSize);

                fseek(f, sizeof(wav_header_t), SEEK_SET);

                sem_post(&context->playback_sync);
                context->playing = true;

                while (context->playing == true) {
                    uint16_t played = 0;

                    if (to_play == 0) {
                        to_play = fread(data, 1, bufferSize, f);
                    }

                    if (bluetoothaudiosink_frame(to_play, data, &played) != 0) {
                        TRACE("Failed to send audio frame!");
                        break;
                    }

                    if (to_play != bufferSize) {
                        TRACE("EOF reached");
                        break;
                    }

                    if (played < to_play) {
                        // we're sending too fast...
                        usleep(1 * 1000);
                    }

                    to_play -= played;
                }

                context->playing = false;

                fclose(f);
                TRACE("Closed file '%s'", context->file);
            } else {
                TRACE("Failed to open file '%s'", context->file);
            }

            if (bluetoothaudiosink_speed(0) != 0) {
                TRACE("Failed to set audio speed 0%%!");
            }

            if (bluetoothaudiosink_relinquish() != 0) {
                TRACE("Failed to reqlinquish bluetooth audio sink device!");
            }
        } else {
            TRACE("Failed to set audio speed 100%%!");
        }
    }

    TRACE("File streaming thread terminated!");

    return (NULL);
}

static void audio_sink_connected(context_t * context)
{
    if ((context->playing == false) && (context->do_acquire == true)) {
        sem_post(&context->connect_sync);
    }
}

static void audio_sink_disconnected(context_t *context)
{
    if (context->playing == true) {
        // Device disconnected abruptly, stop the playback thread, too...
        context->playing = false;
    }
}

static void audio_sink_state_update(void *user_data)
{
    bluetoothaudiosink_state_t state = BLUETOOTHAUDIOSINK_STATE_UNKNOWN;

    if (bluetoothaudiosink_state(&state) == 0) {
        switch (state) {
        case BLUETOOTHAUDIOSINK_STATE_UNASSIGNED:
            TRACE("Bluetooth audio sink is currently unassigned!");
            break;
        case BLUETOOTHAUDIOSINK_STATE_CONNECTED:
            TRACE("Bluetooth audio sink is now connected!");
            audio_sink_connected((context_t*)user_data);
            break;
        case BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD_DEVICE:
            TRACE("Invalid device connected - cant't play!");
            break;
        case BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED:
            TRACE("Restricted Bluetooth audio device connected - won't play!");
            break;
        case BLUETOOTHAUDIOSINK_STATE_DISCONNECTED:
            TRACE("Bluetooth Audio sink is now disconnected!");
            audio_sink_disconnected((context_t*)user_data);
            break;
        case BLUETOOTHAUDIOSINK_STATE_READY:
            TRACE("Bluetooth Audio sink now ready!");
            break;
        case BLUETOOTHAUDIOSINK_STATE_STREAMING:
            TRACE("Bluetooth Audio sink is now streaming!");
            break;
        default:
            break;
        }
    }
}

static void audio_sink_operational_state_update(const uint8_t running, void *user_data)
{
    if (running) {
        bluetoothaudiosink_state_t state = BLUETOOTHAUDIOSINK_STATE_UNKNOWN;
        bluetoothaudiosink_state(&state);

        if (state == BLUETOOTHAUDIOSINK_STATE_UNKNOWN) {
            TRACE("Unknown Bluetooth Audio Sink failure!");
        } else {
            TRACE("Bluetooth Audio Sink service now available");

            // Register for the sink updates
            if (bluetoothaudiosink_register_state_update_callback(audio_sink_state_update, user_data) != 0) {
                TRACE("Failed to register sink sink update callback!");
            }
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

    TRACE("Plays a .wav file over a Bluetooth speaker device");

    if (argc == 2) {
        FILE *f;
        context_t context;
        memset(&context, 0, sizeof(context));

        context.file = argv[1];
        context.do_acquire = true;
        context.playing = false;

        f = fopen(context.file, "rb+");
        if (f != NULL) {
            wav_header_t header = { 0 };

            if (fread(&header, 1, sizeof(header), f) == sizeof(header)) {
                if (memcmp(header.wave, "WAVE", 4) == 0) {
                    context.format.sample_rate = header.sample_rate;
                    context.format.frame_rate = CDDA_FRAMERATE;
                    context.format.channels = header.channels;
                    context.format.resolution = header.resolution;

                    TRACE("Input format: PCM %i Hz, %i bit (signed, little endian), %i channels @ %i.%02i fps",
                        context.format.sample_rate, context.format.resolution, context.format.channels,
                        (context.format.frame_rate/100), (context.format.frame_rate%100));
                }
            }

            fclose(f);
            f = NULL;

            if (((context.format.sample_rate >= 8000) && (context.format.channels == 1)) || ((context.format.channels == 2) && (context.format.resolution == 16))) {
                TRACE("Waiting for Bluetooth audio sink device to be available... (Press Ctrl+C to quit at any time.)");

                sem_init(&context.connect_sync, 0, 0);
                sem_init(&context.playback_sync, 0, 0);

                // Register for the audio sink service updates
                if (bluetoothaudiosink_register_operational_state_update_callback(&audio_sink_operational_state_update, &context) != 0) {
                    TRACE("Failed to register Bluetooths Audio Sink operational callback");
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

                    // Wait for connection...
                    if (sem_timedwait(&context.connect_sync, &ts) != -1) {
                        struct timespec ts;
                        clock_gettime(CLOCK_REALTIME, &ts);
                        ts.tv_sec += 2;

                        pthread_create(&context.thread, NULL, playback_task, (void*)&context);

                        // Wait for playback start...
                        if (sem_timedwait(&context.playback_sync, &ts) != -1) {
                            TRACE("Playing...");

                            while (context.playing == true) {
                                uint32_t playtime = 0;

                                bluetoothaudiosink_time(&playtime);
                                TRACE("Time: %02i:%02i:%03i\r", ((playtime / 1000) / 60), ((playtime / 1000) % 60), playtime % 1000);

                                usleep(50 * 1000);

                                if ((user_break != false) && (context.playing == true)) {
                                    // Ctrl+C!
                                    TRACE("User break! Stopping playback...");
                                    context.playing = false;
                                }
                            }

                            result = 0;
                        } else {
                            TRACE("Failed to start playback!");
                        }

                        pthread_join(context.thread, NULL);
                    } else {
                        TRACE("Bluetooth audio sink device not connected in %i seconds, terminating!", timeout);
                    }

                    // And we're done...
                    TRACE("Cleaning up...");
                    bluetoothaudiosink_unregister_state_update_callback(&audio_sink_state_update); // this can fail, it's OK
                    bluetoothaudiosink_unregister_operational_state_update_callback(&audio_sink_operational_state_update);
                    bluetoothaudiosink_dispose();
                }
            } else {
                TRACE("Invalid file format!");
            }
        } else {
            TRACE("Failed to open the source file!");
        }
    } else {
        TRACE("arguments:\n%s <file.wav>", argv[0]);
    }

    return (result);
}
