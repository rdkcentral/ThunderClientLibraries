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

#include <stdlib.h>
#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

OpenCDMError opencdm_gstreamer_session_decrypt_v2(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSampleBuffer, const uint32_t subSampleCount,
                                               const EncryptionScheme encScheme, const EncryptionPattern pattern,
                                               GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15)
{
    OpenCDMError result (ERROR_INVALID_SESSION);

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

OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
                                                   GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15){
    //Set the Encryption Scheme and Pattern to defaults. 
    EncryptionScheme encScheme = AesCtr_Cenc;
    EncryptionPattern pattern = {0};

    //Lets try to get Enc Scheme and Pattern from the Protection Metadata.
    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
    if (protectionMeta != NULL) {
        if (gst_structure_has_name(protectionMeta->info, "application/x-cbcs")) {
             encScheme = AesCbc_Cbcs;
        }

        gst_structure_get_uint(protectionMeta->info, "crypt_byte_block", &pattern.encrypted_blocks);
        gst_structure_get_uint(protectionMeta->info, "skip_byte_block", &pattern.clear_blocks);
    }

    return(opencdm_gstreamer_session_decrypt_v2(session, buffer, subSample, subSampleCount, encScheme, pattern, IV, keyID, initWithLast15));
}
