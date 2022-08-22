 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "open_cdm_adapter.h"
#include "Module.h"
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/base/gstbytereader.h>
#include "../CapsParser.h"


inline bool mappedBuffer(GstBuffer *buffer, bool writable, uint8_t **data, uint32_t *size)
{
    GstMapInfo map;

    if (!gst_buffer_map (buffer, &map, writable ? GST_MAP_WRITE : GST_MAP_READ)) {
        return false;
    }

    *data = reinterpret_cast<uint8_t* >(map.data);
    *size = static_cast<uint32_t >(map.size);
    gst_buffer_unmap (buffer, &map);

    return true;
}

OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSampleBuffer, const uint32_t subSampleCount,
                                               GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15)
{
    OpenCDMError result (ERROR_INVALID_SESSION);

    printf("opencdm_gstreamer_session_decrypt\n");
    TRACE_L1("opencdm_gstreamer_session");

    if (session != nullptr) {
        GstMapInfo dataMap;
        if (gst_buffer_map(buffer, &dataMap, (GstMapFlags) GST_MAP_READWRITE) == false) {
            printf("Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        GstMapInfo ivMap;
        if (gst_buffer_map(IV, &ivMap, (GstMapFlags) GST_MAP_READ) == false) {
            gst_buffer_unmap(buffer, &dataMap);
            printf("Invalid IV buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        uint8_t *mappedKeyID = nullptr;
        uint32_t mappedKeyIDSize = 0;

        GstMapInfo keyIDMap;
        if (keyID != nullptr) {
           if (gst_buffer_map(keyID, &keyIDMap, (GstMapFlags) GST_MAP_READ) == false) {
               gst_buffer_unmap(buffer, &dataMap);
               gst_buffer_unmap(IV, &ivMap);
               printf("Invalid keyID buffer.\n");
               return (ERROR_INVALID_DECRYPT_BUFFER);
           }

           mappedKeyID = reinterpret_cast<uint8_t* >(keyIDMap.data);
           mappedKeyIDSize =  static_cast<uint32_t >(keyIDMap.size);
        }

        //Set the Encryption Scheme and Pattern to defaults.
        EncryptionScheme encScheme = AesCtr_Cenc;
        EncryptionPattern pattern = {0, 0};

        //Lets try to get Enc Scheme and Pattern from the Protection Metadata.
        GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
        if (protectionMeta != NULL) {
            const char* cipherModeBuf = gst_structure_get_string(protectionMeta->info, "cipher-mode");
            if(g_strcmp0(cipherModeBuf,"cbcs") == 0) {
                encScheme = AesCbc_Cbcs;
            }

            gst_structure_get_uint(protectionMeta->info, "crypt_byte_block", &pattern.encrypted_blocks);
            gst_structure_get_uint(protectionMeta->info, "skip_byte_block", &pattern.clear_blocks);
        }

        uint8_t *mappedData = reinterpret_cast<uint8_t* >(dataMap.data);
        uint32_t mappedDataSize = static_cast<uint32_t >(dataMap.size);
        uint8_t *mappedIV = reinterpret_cast<uint8_t* >(ivMap.data);
        uint32_t mappedIVSize = static_cast<uint32_t >(ivMap.size);
        if (subSampleBuffer != nullptr) {
            GstMapInfo sampleMap;
            if (gst_buffer_map(subSampleBuffer, &sampleMap, GST_MAP_READ) == false) {
                printf("Invalid subsample buffer.\n");
                if (keyID != nullptr) {
                   gst_buffer_unmap(keyID, &keyIDMap);
                }
                gst_buffer_unmap(IV, &ivMap);
                gst_buffer_unmap(buffer, &dataMap);
                return (ERROR_INVALID_DECRYPT_BUFFER);
            }
            uint8_t *mappedSubSample = reinterpret_cast<uint8_t* >(sampleMap.data);
            uint32_t mappedSubSampleSize = static_cast<uint32_t >(sampleMap.size);
            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            uint32_t totalEncrypted = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);
                totalEncrypted += inEncrypted;
            }
            gst_byte_reader_set_pos(reader, 0);

            uint8_t* encryptedData = reinterpret_cast<uint8_t*>(malloc(totalEncrypted));
            uint8_t* encryptedDataIter = encryptedData;

            uint32_t index = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(encryptedDataIter, mappedData + index + inClear, inEncrypted);
                index += inClear + inEncrypted;
                encryptedDataIter += inEncrypted;
            }
            gst_byte_reader_set_pos(reader, 0);


            result = opencdm_session_decrypt(session, encryptedData, totalEncrypted, encScheme, pattern, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize, initWithLast15);
            // Re-build sub-sample data.
            index = 0;
            unsigned total = 0;
            for (uint32_t position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(mappedData + total + inClear, encryptedData + index, inEncrypted);
                index += inEncrypted;
                total += inClear + inEncrypted;
            }

            gst_byte_reader_free(reader);
            free(encryptedData);
            gst_buffer_unmap(subSampleBuffer, &sampleMap);
        } else {
            result = opencdm_session_decrypt(session, mappedData, mappedDataSize, encScheme, pattern, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize, initWithLast15);
        }

        if (keyID != nullptr) {
           gst_buffer_unmap(keyID, &keyIDMap);
        }

        gst_buffer_unmap(IV, &ivMap);
        gst_buffer_unmap(buffer, &dataMap);
    }

    return (result);
}


OpenCDMError opencdm_gstreamer_session_decrypt_buffer(struct OpenCDMSession* session, GstBuffer* buffer, GstCaps* caps) {

    OpenCDMError result (ERROR_INVALID_SESSION);

    if (session != nullptr) {

        uint8_t *mappedData = nullptr;
        uint32_t mappedDataSize = 0;
        if (mappedBuffer(buffer, true, &mappedData, &mappedDataSize) == false) {

            TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        //Check if Protection Metadata is available in Buffer
        GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
        if (protectionMeta != nullptr) {
            const GValue* value;

            //Get Subsample mapping
            unsigned subSampleCount = 0;
            GstBuffer* subSample = nullptr;
            if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subSampleCount)) {
                printf("No Subsample Count.\n");
            }
            uint8_t *mappedSubSample = nullptr;
            uint32_t mappedSubSampleSize = 0;

            if (subSampleCount > 0) {
                value = gst_structure_get_value(protectionMeta->info, "subsamples");
                if (!value) {
                    TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: No subsample buffer.");
                    result = ERROR_INVALID_DECRYPT_BUFFER;
                    goto exit;
                }
                GstBuffer* subSample = gst_value_get_buffer(value);
               if (subSample != nullptr && mappedBuffer(subSample, false, &mappedSubSample, &mappedSubSampleSize) == false) {
                    TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid subsample buffer.");
                    result = ERROR_INVALID_DECRYPT_BUFFER;
                    goto exit;
                }
                ASSERT(mappedSubSampleSize==subSampleCount);
            }

            //Get IV
            value = gst_structure_get_value(protectionMeta->info, "iv");
            if (!value) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing IV buffer.");
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
            GstBuffer* IV = gst_value_get_buffer(value);
            uint8_t *mappedIV = nullptr;    //Set the Encryption Scheme and Pattern to defaults.
            uint32_t mappedIVSize = 0;
            if (mappedBuffer(IV, false, &mappedIV, &mappedIVSize) == false) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid IV buffer.");
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }

            //Get Key ID
            value = gst_structure_get_value(protectionMeta->info, "kid");
            if (!value) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing KeyId buffer.");
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
            GstBuffer* keyID = gst_value_get_buffer(value);
            uint8_t *mappedKeyID = nullptr;
            uint32_t mappedKeyIDSize = 0;
            if (keyID != nullptr && mappedBuffer(keyID, false, &mappedKeyID, &mappedKeyIDSize) == false) {
                TRACE_L1("Invalid keyID buffer.");
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }

            //Get Encryption Scheme and Pattern
            EncryptionScheme encScheme = AesCtr_Cenc;
            EncryptionPattern pattern = {0, 0};
            const char* cipherModeBuf = gst_structure_get_string(protectionMeta->info, "cipher-mode");
            if(g_strcmp0(cipherModeBuf,"cbcs") == 0) {
                encScheme = AesCbc_Cbcs;
            }
            gst_structure_get_uint(protectionMeta->info, "crypt_byte_block", &pattern.encrypted_blocks);
            gst_structure_get_uint(protectionMeta->info, "skip_byte_block", &pattern.clear_blocks);

            //Create a SubSampleInfo Array with mapping
            SubSampleInfo * subSampleInfoPtr = nullptr;
            if (subSample != nullptr) {
                GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
                subSampleInfoPtr = reinterpret_cast<SubSampleInfo*>(malloc(subSampleCount * sizeof(SubSampleInfo)));
                for (unsigned int position = 0; position < subSampleCount; position++) {

                    gst_byte_reader_get_uint16_be(reader, &subSampleInfoPtr[position].clear_bytes);
                    gst_byte_reader_get_uint32_be(reader, &subSampleInfoPtr[position].encrypted_bytes);
                }
                gst_byte_reader_set_pos(reader, 0);
                gst_byte_reader_free(reader);
            }

            //Get Stream Properties from GstCaps
            MediaProperties *spPtr = nullptr;
            MediaProperties streamProperties;
            if(caps != nullptr){
                gchar *capsStr = gst_caps_to_string (caps);
                if (capsStr != nullptr) {
                    WPEFramework::Plugin::CapsParser capsParser;
                    capsParser.Parse(reinterpret_cast<const uint8_t*>(capsStr), strlen(capsStr));
                    streamProperties.height = capsParser.GetHeight();
                    streamProperties.width = capsParser.GetWidth();
                    switch (capsParser.GetMediaType()) {
                        case CDMi::MediaType::Video:
                            streamProperties.media_type = MediaType_Video;
                        break;

                        case CDMi::MediaType::Audio:
                            streamProperties.media_type = MediaType_Audio;
                        break;

                        case CDMi::MediaType::Data:
                            streamProperties.media_type = MediaType_Data;
                        break;

                        default:
                            streamProperties.media_type = MediaType_Unknown;
                        break;
                    }
                    spPtr = &streamProperties;
                    g_free(capsStr);
                } else {
                    printf("Could not convert caps to string\n");
                }
            }

            SampleInfo sampleInfo;
            sampleInfo.subSample = subSampleInfoPtr;
            sampleInfo.subSampleCount = subSampleCount;
            sampleInfo.scheme = encScheme;
            sampleInfo.pattern.clear_blocks = pattern.clear_blocks;
            sampleInfo.pattern.encrypted_blocks = pattern.encrypted_blocks;
            sampleInfo.iv = mappedIV;
            sampleInfo.ivLength = mappedIVSize;
            sampleInfo.keyId = mappedKeyID;
            sampleInfo.keyIdLength = mappedKeyIDSize;

            result = opencdm_session_decrypt_v2(session,
                                                mappedData,
                                                mappedDataSize,
                                                &sampleInfo,
                                                spPtr);

            //Clean up
            if(subSampleInfoPtr != nullptr) {
               free(subSampleInfoPtr);
           }
        } else {
            TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing Protection Metadata.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
        }

    }

exit:
    return (result);
}

