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
 
#ifndef __OPEN_CDM_ADAPTER_H
#define __OPEN_CDM_ADAPTER_H

#include "open_cdm.h"

struct _GstBuffer;
typedef struct _GstBuffer GstBuffer;

struct _GstCaps;
typedef struct _GstCaps GstCaps;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Performs decryption based on adapter implementation.
 *
 * This method accepts encrypted data and will typically decrypt it out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param buffer Gstreamer buffer containing encrypted data and related meta data. If applicable, decrypted data will be stored here after this call returns.
 * \param subSample Gstreamer buffer containing subsamples size which has been parsed from protection meta data.
 * \param subSampleCount count of subsamples
 * \param IV Gstreamer buffer containing initial vector (IV) used during decryption.
 * \param keyID Gstreamer buffer containing keyID to use for decryption
 *
 * This method handles the Subsample mapping by consolidating all the encrypted data into one buffer before decrypting. This means the Subsample mappings are
 * not passed on to the DRM implementation side.
 *
 * For CBCS support, EncryptionScheme and EncryptionPattern information can be added as part of the ProtectionMeta in the given format below
 *      "cipher-mode"         G_TYPE_STRING   (One of the Four Character Code (FOURCC) Protection schemes as defined in https://www.iso.org/obp/ui/#iso:std:iso-iec:23001:-7:ed-3:v1:en)
 *      "crypt_byte_block"    G_TYPE_UINT     (Present only if cipher-mode is "cbcs")
 *      "skip_byte_block"     G_TYPE_UINT     (Present only cipher-mode is "cbcs")

 * \return Zero on success, non-zero on error.
 */
    EXTERNAL OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
                                                   GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15);

/**
 * \brief Performs decryption based on adapter implementation.
 *
 * This version 3 method accepts encrypted data and will typically decrypt it out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place. 
 * This version assumes all data required is attached as metadata to the buffer. Specification for this data is as follows:
 *
 * Typically, the caller would parse the protection information for a video/audio frame from its input data and use this information to populate the
 * GstStructure info field, which is then encapsulated in a GstProtectionMeta object and attached to the corresponding GstBuffer using the
 * gst_buffer_add_protection_meta function.
 *
 * gst_structure [application/x-cenc]
 *      "iv"                  GST_TYPE_BUFFER
 *      "kid"                 GST_TYPE_BUFFER
 *      "subsample_count"     G_TYPE_UINT
 *      "subsamples"          GST_TYPE_BUFFER
 *      "cipher-mode"         G_TYPE_STRING   (One of the Four Character Code (FOURCC) Protection schemes as defined in https://www.iso.org/obp/ui/#iso:std:iso-iec:23001:-7:ed-3:v1:en)
 *      "crypt_byte_block"    G_TYPE_UINT     (Present only if cipher-mode is "cbcs")
 *      "skip_byte_block"     G_TYPE_UINT     (Present only cipher-mode is "cbcs")
 *
 * This method passes on the subsample mapping to the DRM implementation and assumes that the DRM implementaion will handle the decryption based on subsample mapping.
 *
 * \param session \ref OpenCDMSession instance.
 * \param buffer Gstreamer buffer containing encrypted data and related meta data. If applicable, decrypted data will be stored here after this call returns.
 * \return Zero on success, non-zero on error.
 */
    
    EXTERNAL OpenCDMError opencdm_gstreamer_session_decrypt_buffer(struct OpenCDMSession* session, GstBuffer* buffer, GstCaps* caps);

/**
 * \brief adds SVP related features to the caps structure (only if needed by the platform)
 *
 * \param caps GstCaps to be updated
 *
 * \return Zero on success, non-zero on error.
 */

    EXTERNAL OpenCDMError opencdm_gstreamer_transform_caps(GstCaps** caps);

/**
 * \brief Create DRM session (for actual decrypting of data).
 *
 * Creates an instance of \ref OpenCDMSession using initialization data.
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem DRM system to create the session for.
 * \param licenseType DRM specifc signed integer selecting License Type (e.g.
 * "Limited Duration" for PlayReady).
 * \param initDataType Type of data passed in \ref initData.
 * \param initData Initialization data.
 * \param initDataLength Length (in bytes) of initialization data.
 * \param CDMData CDM data.
 * \param CDMDataLength Length (in bytes) of \ref CDMData.
 * \param callbacks the instance of \ref OpenCDMSessionCallbacks with callbacks to be called on events.
 * \param userData the user data to be passed back to the \ref OpenCDMSessionCallbacks callbacks.
 * \param session Output parameter that will contain pointer to instance of \ref OpenCDMSession.
 * \return Zero on success, non-zero on error.
 */
     EXTERNAL OpenCDMError opencdm_adapter_construct_session(struct OpenCDMSystem* system, const LicenseType licenseType,
                                                    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
                                                    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks, void* userData,
                                                    struct OpenCDMSession** session);

/**
 * Destructs an \ref OpenCDMSession instance.
 * \param system \ref OpenCDMSession instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
     EXTERNAL OpenCDMError opencdm_adapter_destruct_session(struct OpenCDMSession* session);

#ifdef __cplusplus
}
#endif

#endif // __OPEN_CDM_ADAPTER_H
