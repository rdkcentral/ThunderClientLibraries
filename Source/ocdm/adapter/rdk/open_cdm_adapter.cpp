/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "open_cdm_impl.h"

#include "Module.h"
#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

#include <gst_svp_meta.h>
#include "../CapsParser.h"

//void Log(const char* file, int line, LogLevel ll, const char* fmt, ...);
//void LogTelemetry(const char* file, int line,  int x);
// Log APIs
//#define LOGT(X) LogTelemetry(__FILE__, __LINE__, X)
//#define LOGE(...) Log(__FILE__, __LINE__, CDMi::ERROR,  __VA_ARGS__)

//#define LOGW(...) Log(__FILE__, __LINE__, CDMi::WARNING,  __VA_ARGS__)
//#define LOGI(...) Log(__FILE__, __LINE__, CDMi::INFO, __VA_ARGS__)
//#define LOGD(...) Log(__FILE__, __LINE__, CDMi::DEBUG,  __VA_ARGS__)
//#define LOGV(...) Log(__FILE__, __LINE__, CDMi::VERBOSE,  __VA_ARGS__)

typedef uint32_t SEC_SIZE;
struct My_Sec_OpaqueBufferHandle_struct
{
    SEC_SIZE dataBufSize;
    void *sess;
    SEC_SIZE align;
    int ion_fd;
    int map_fd;
    unsigned int register_shm;
    void *rtkmem_handle;
};

EXTERNAL OpenCDMError opencdm_gstreamer_transform_caps(GstCaps** caps)
{
    OpenCDMError result (ERROR_NONE);

    if(!gst_svp_ext_transform_caps(caps, TRUE))
        result = ERROR_UNKNOWN;

    return (result);
}

bool swapIVBytes(uint8_t *mappedIV,uint32_t mappedIVSize)
{
    uint8_t buf;
    for (uint32_t i = 0; i < mappedIVSize / 2; i++) {
        uint8_t buf = mappedIV[i];
        mappedIV[i] = mappedIV[mappedIVSize - i - 1];
        mappedIV[mappedIVSize - i - 1] = buf;      
    }

    return true;
}

uint32_t opencdm_construct_session_private(struct OpenCDMSession* session, void* &pvtData)
{
    bool success = gst_svp_ext_get_context(&pvtData, Server, (unsigned int)session);
    char buf[25] = { 0 };
    snprintf(buf, 25, "%X", (unsigned int)session);
    ASSERT(session != nullptr);
    session->SetParameter("rpcId", buf);
    return (success ? 0 : 1);
}

uint32_t opencdm_destruct_session_private(struct OpenCDMSession* session, void* &pvtData)
{
    bool success = gst_svp_ext_free_context(pvtData);
    return (success ? 0 : 1);
}

OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
                                                GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15)
{
    OpenCDMError result (ERROR_INVALID_SESSION);
    if (session != nullptr) {
        GstMapInfo dataMap;
        if (gst_buffer_map(buffer, &dataMap, (GstMapFlags) GST_MAP_READWRITE) == false) {
            fprintf(stderr, "Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        media_type mediaType = Data;

		if(subSample == NULL && IV == NULL && keyID == NULL) {
            // no encrypted data, skip decryption...
            // But still need to transform buffer for SVP support
            gst_buffer_svp_transform_from_cleardata(session->SessionPrivateData(), buffer, mediaType);
            gst_buffer_unmap(buffer, &dataMap);
            return(ERROR_NONE);
        }

        GstMapInfo ivMap;
        if (gst_buffer_map(IV, &ivMap, (GstMapFlags) GST_MAP_READ) == false) {
            gst_buffer_unmap(buffer, &dataMap);
            fprintf(stderr, "Invalid IV buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        GstMapInfo keyIDMap;
        if (gst_buffer_map(keyID, &keyIDMap, (GstMapFlags) GST_MAP_READ) == false) {
            gst_buffer_unmap(buffer, &dataMap);
            gst_buffer_unmap(IV, &ivMap);
            fprintf(stderr, "Invalid keyID buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
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
        uint8_t *mappedKeyID = reinterpret_cast<uint8_t* >(keyIDMap.data);
        uint32_t mappedKeyIDSize = static_cast<uint32_t >(keyIDMap.size);

        if (subSample != nullptr) {
            GstMapInfo sampleMap;

            if (gst_buffer_map(subSample, &sampleMap, GST_MAP_READ) == false) {
                fprintf(stderr, "Invalid subsample buffer.\n");
                gst_buffer_unmap(keyID, &keyIDMap);
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
            uint32_t nCount = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);
                totalEncrypted += inEncrypted;
            }
            gst_byte_reader_set_pos(reader, 0);

            if(totalEncrypted > 0)
            {
                uint8_t* svpData;
                gsize dataBlockSize = gst_svp_allocate_data_block(session->SessionPrivateData(), (void**) &svpData, totalEncrypted, totalEncrypted);

                uint8_t* encryptedDataIter = reinterpret_cast<uint8_t *>(gst_svp_header_get_start_of_data(session->SessionPrivateData(), svpData));

                uint32_t index = 0;
                for (unsigned int position = 0; position < subSampleCount; position++) {

                    gst_byte_reader_get_uint16_be(reader, &inClear);

                    gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                    memcpy(encryptedDataIter, mappedData + index + inClear, inEncrypted);
                    index += inClear + inEncrypted;
                    encryptedDataIter += inEncrypted;
                }
                gst_byte_reader_set_pos(reader, 0);

                GstPerf* ocdm_perf = new GstPerf("opencdm_session_decrypt_subsample");
                result = opencdm_session_decrypt(session, svpData, dataBlockSize, encScheme, pattern, mappedIV, mappedIVSize,
                                                 mappedKeyID, mappedKeyIDSize, initWithLast15);
                delete ocdm_perf;

                if(result == ERROR_NONE) {
                    GstPerf* svpTransform_perf1 = new GstPerf("opencdm_svp_transform_subsample");
                    gst_buffer_append_svp_transform(session->SessionPrivateData(), buffer, subSample, subSampleCount, svpData);
                    delete svpTransform_perf1;
                }
                gst_svp_free_data_block(session->SessionPrivateData(), svpData);
            } else {
                // no encrypted data, skip decryption...
                // But still need to transform buffer for SVP support
                gst_buffer_svp_transform_from_cleardata(session->SessionPrivateData(), buffer, mediaType);
                result = ERROR_NONE;
            }
            gst_byte_reader_free(reader);
            gst_buffer_unmap(subSample, &sampleMap);
        } else {
            uint8_t* encryptedData = NULL;
            uint8_t* svpData = NULL;

            uint32_t dataBlockSize = gst_svp_allocate_data_block(session->SessionPrivateData(), (void**) &svpData, mappedDataSize, mappedDataSize);

            // Adjust data start after header
            encryptedData = reinterpret_cast<uint8_t *>(gst_svp_header_get_start_of_data(session->SessionPrivateData(), svpData));

            memcpy(encryptedData, mappedData, mappedDataSize);

            GstPerf* ocdm_perf = new GstPerf("opencdm_session_decrypt_no_subsample");
            result = opencdm_session_decrypt(session, svpData, dataBlockSize, encScheme, pattern, mappedIV, mappedIVSize,
                                             mappedKeyID, mappedKeyIDSize, initWithLast15);
            delete ocdm_perf;

            if(result == ERROR_NONE){
                GstPerf* svpTransform_perf2 = new GstPerf("opencdm_svp_transform_no_subsample");
                gst_buffer_append_svp_transform(session->SessionPrivateData(), buffer, NULL, mappedDataSize, svpData);
                delete svpTransform_perf2;
            }
            gst_svp_free_data_block(session->SessionPrivateData(), svpData);
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

        GstMapInfo dataMap;
        if (gst_buffer_map(buffer, &dataMap, (GstMapFlags) GST_MAP_READWRITE) == false) {

            TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        media_type mediaType = Data;
        uint8_t *mappedData = reinterpret_cast<uint8_t* >(dataMap.data);
        uint32_t mappedDataSize = static_cast<uint32_t >(dataMap.size);

        //Check if Protection Metadata is available in Buffer
        GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
        if (protectionMeta != nullptr) {
            const GValue* value;

            //Get Subsample mapping
            unsigned subSampleCount = 0;
            if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subSampleCount)) {
                TRACE_L1("No Subsample Count.\n");
            }

            GstBuffer* subSample = nullptr;
            GstMapInfo sampleMap;
            uint8_t *mappedSubSample = nullptr;
            uint32_t mappedSubSampleSize = 0;
            if (subSampleCount > 0) {
                value = gst_structure_get_value(protectionMeta->info, "subsamples");
                if (!value) {
                    TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: No subsample buffer.");
                    gst_buffer_unmap(buffer, &dataMap);
                    result = ERROR_INVALID_DECRYPT_BUFFER;
                    goto exit;
                }

                subSample = gst_value_get_buffer(value);
                if (subSample != nullptr && gst_buffer_map(subSample, &sampleMap, GST_MAP_READ) == false) {
                    TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid subsample buffer.");
                    gst_buffer_unmap(buffer, &dataMap);
                    result = ERROR_INVALID_DECRYPT_BUFFER;
                    goto exit;
                }
                mappedSubSample = reinterpret_cast<uint8_t* >(sampleMap.data);
                mappedSubSampleSize = static_cast<uint32_t >(sampleMap.size);
            }

            //Get IV
            value = gst_structure_get_value(protectionMeta->info, "iv");
            if (!value) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing IV buffer.");
                gst_buffer_unmap(buffer, &dataMap);
                gst_buffer_unmap(subSample, &sampleMap);
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
            GstBuffer* IV = gst_value_get_buffer(value);
            GstMapInfo ivMap;
            if (IV != nullptr && gst_buffer_map(IV, &ivMap, (GstMapFlags) GST_MAP_READ) == false) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Invalid IV buffer.");
                gst_buffer_unmap(buffer, &dataMap);
                gst_buffer_unmap(subSample, &sampleMap);
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
            uint8_t *mappedIV = reinterpret_cast<uint8_t* >(ivMap.data);
            uint32_t mappedIVSize = static_cast<uint32_t >(ivMap.size);

            unsigned InitWithLast15 = 0;
            if (!gst_structure_get_uint(protectionMeta->info, "initWithLast15", &InitWithLast15)) {
                TRACE_L3("opencdm_gstreamer_session_decrypt_buffer: Missing initWithLast15 value.");
            }
            if (InitWithLast15 == 1) {
                swapIVBytes(mappedIV,mappedIVSize);
            }

            //Get Key ID
            value = gst_structure_get_value(protectionMeta->info, "kid");
            if (!value) {
                TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing KeyId buffer.");
                gst_buffer_unmap(buffer, &dataMap);
                gst_buffer_unmap(subSample, &sampleMap);
                gst_buffer_unmap(IV, &ivMap);
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }

            GstBuffer* keyID = gst_value_get_buffer(value);
            uint8_t *mappedKeyID = nullptr;
            uint32_t mappedKeyIDSize = 0;
            GstMapInfo keyIDMap;
            if (keyID != nullptr && gst_buffer_map(keyID, &keyIDMap, (GstMapFlags) GST_MAP_READ) == false) {
                TRACE_L1("Invalid keyID buffer.");
                gst_buffer_unmap(buffer, &dataMap);
                gst_buffer_unmap(subSample, &sampleMap);
                gst_buffer_unmap(IV, &ivMap);
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
            mappedKeyID = reinterpret_cast<uint8_t* >(keyIDMap.data);
            mappedKeyIDSize = static_cast<uint32_t >(keyIDMap.size);

            std::string perfString(__FUNCTION__);
            //Get Stream Properties from GstCaps
            MediaProperties *spPtr = nullptr;
            if(caps != nullptr){
                gchar *capsStr = gst_caps_to_string (caps);
                if (capsStr != nullptr) {
                    WPEFramework::Plugin::CapsParser capsParser;
                    MediaProperties streamProperties;
                    capsParser.Parse(reinterpret_cast<const uint8_t*>(capsStr), strlen(capsStr));
                    streamProperties.height = capsParser.GetHeight();
                    streamProperties.width = capsParser.GetWidth();
                    switch (capsParser.GetMediaType()) {
                        case CDMi::MediaType::Video:
                            streamProperties.media_type = MediaType_Video;
                            mediaType = Video;
                            perfString += "_Video";
                        break;

                        case CDMi::MediaType::Audio:
                            streamProperties.media_type = MediaType_Audio;
                            mediaType = Audio;
                            perfString += "_Audio";
                        break;

                        case CDMi::MediaType::Data:
                            streamProperties.media_type = MediaType_Data;
                            mediaType = Data;
                            perfString += "_Data";
                        break;

                        default:
                            streamProperties.media_type = MediaType_Unknown;
                            perfString += "_Unknown";
                        break;
                    }

                    if (subSample == nullptr && IV == nullptr && keyID == nullptr) {
                       perfString += "_clearData";
                    }

                    spPtr = &streamProperties;
                    g_free(capsStr);
                } else {
                    perfString += "_NoGstCaps";
                    TRACE_L1("Could not convert caps to string\n");
                }
            }
            RDKPerf perf(perfString.c_str());

            if (subSample == nullptr && IV == nullptr && keyID == nullptr) {
            // no encrypted data, skip decryption...
            // But still need to transform buffer for SVP support
                gst_buffer_svp_transform_from_cleardata(session->SessionPrivateData(), buffer, mediaType);
                return(ERROR_NONE);
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
            uint32_t total_encrypted_bytes = 0;
            if (subSample != nullptr) {
                GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
                subSampleInfoPtr = reinterpret_cast<SubSampleInfo*>(malloc(subSampleCount * sizeof(SubSampleInfo)));
                for (unsigned int position = 0; position < subSampleCount; position++) {

                    gst_byte_reader_get_uint16_be(reader, &subSampleInfoPtr[position].clear_bytes);
                    gst_byte_reader_get_uint32_be(reader, &subSampleInfoPtr[position].encrypted_bytes);
                    total_encrypted_bytes += subSampleInfoPtr[position].encrypted_bytes;
                }
                gst_byte_reader_set_pos(reader, 0);
                gst_byte_reader_free(reader);
            } else {
                 total_encrypted_bytes = mappedDataSize;
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

            if(total_encrypted_bytes > 0) {
               uint8_t* svpData;
               uint32_t dataBlockSize = gst_svp_allocate_data_block(session->SessionPrivateData(), (void**) &svpData, mappedDataSize, mappedDataSize);

               void * encryptedData = reinterpret_cast<uint8_t *>(gst_svp_header_get_start_of_data(session->SessionPrivateData(), svpData));

                memcpy(encryptedData, mappedData, mappedDataSize);

                GstPerf* ocdm_perf = new GstPerf("opencdm_session_decrypt_v2");
                result = opencdm_session_decrypt_v2(session,
                                                svpData,
                                                dataBlockSize,
                                                &sampleInfo,
                                                spPtr);
                delete ocdm_perf;

               if(result == ERROR_NONE) {
                  GstPerf* svpTransform_perf3 = new GstPerf("opencdm_svp_transform_subsample");
                  gst_buffer_append_svp_transform(session->SessionPrivateData(), buffer, subSample, subSampleCount, svpData, mappedDataSize);
                  delete svpTransform_perf3;
               }
               gst_svp_free_data_block(session->SessionPrivateData(), svpData);
           } else {
               // no encrypted data, skip decryption...
               // But still need to transform buffer for SVP support
               gst_buffer_svp_transform_from_cleardata(session->SessionPrivateData(), buffer, mediaType);
               result = ERROR_NONE;
           }

            //Clean up
            if(subSampleInfoPtr != nullptr) {
               free(subSampleInfoPtr);
            }

            gst_buffer_unmap(buffer, &dataMap);

            if (subSample != nullptr) {
              gst_buffer_unmap(subSample, &sampleMap);
            }

            if (IV != nullptr) {
               gst_buffer_unmap(IV, &ivMap);
            }

            if (keyID != nullptr) {
              gst_buffer_unmap(keyID, &keyIDMap);
            }
        } else {
            TRACE_L1("opencdm_gstreamer_session_decrypt_buffer: Missing Protection Metadata.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
        }

    }

exit:
    return (result);
}
