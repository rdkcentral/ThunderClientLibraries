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

#include "../../Module.h"
#include <vault_implementation.h>
#include <cryptalgo/cryptalgo.h>
#include "Vault.h"

namespace Implementation {

    cryptographyvault  vaultId;

    /* Added a default COTR */
    Vault::Vault()
        : _lock()
        , _items()
        , _lastHandle(0)
    {
        Sec_Result sec_res = SecProcessor_GetInstance_Directories(&_secProcHandle, globalDir, appDir);
        if (sec_res != SEC_RESULT_SUCCESS) {
            TRACE_L1(_T("SEC : proccesor instance failed retval= %d\n"),sec_res);
            _secProcHandle = NULL;
        }
        _lastHandle = 0x80000000;
    }

    /* Destructor */
    Vault::~Vault()
    {
        if (_secProcHandle != NULL) {
            SecProcessor_Release(_secProcHandle);
            _secProcHandle = NULL;
        }
	
    }

    /*To release sec processor resource explicitly.Before this call make sure to call Release on hmac, cipher or dh objects if used*/
    void Vault::ProcessorRelease()
    {
       if (_secProcHandle != NULL) {
            SecProcessor_Release(_secProcHandle);
            _secProcHandle = NULL;
        }
    }

    /*********************************************************************
     * @function Size 
     *
     * @brief Gives Size of the blob of an id
     *
     * @param[in] id -  id of blob
     * @param[in] allowSealed -  wether the blob of data is sealed or not
     *
     * @return  size of blob(return USHRT_MAX id data is sealed )
     *
     *********************************************************************/
    uint16_t Vault::Size(const uint32_t id, bool allowSealed) const
    {
        uint16_t size = 0;

        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            if ((allowSealed == true) || (*it).second.IsExportable() == true) {
                IdStore* ids = const_cast<IdStore*>((*it).second.getIdStore());
                Sec_KeyHandle* sec_key_hmac; //check  hmac and  aes id to retreive key size
                Sec_KeyHandle* sec_key_aes;
                TRACE_L2(_T("SEC the retrieved sec obj id: %llu \n"), ids->idHmac);
                Sec_Result sec_res_hmac = SecKey_GetInstance(_secProcHandle, ids->idHmac, &sec_key_hmac);
                Sec_Result sec_res_aes = SecKey_GetInstance(_secProcHandle, ids->idAes, &sec_key_aes);
                if (sec_res_hmac == SEC_RESULT_SUCCESS) {
                    size = SecKey_GetKeyLen(sec_key_hmac);
                    TRACE_L2(_T("SEC:HMAC key size is %d \n"), size);
                    SecKey_Release(sec_key_hmac);
                }
                if (sec_res_aes == SEC_RESULT_SUCCESS) {
                    size = SecKey_GetKeyLen(sec_key_aes);
                    TRACE_L2(_T("SEC:AES key size is %d \n"), size);
                    SecKey_Release(sec_key_aes);
                }
            }
            else {
                TRACE_L1(_T("SEC : blob id 0x%08x is sealed \n"), id);
                size = USHRT_MAX;
            }
        }
        else {
            TRACE_L1(_T("SEC: Failed to look up blob id 0x%08x"), id);
        }
        _lock.Unlock();

        return (size);
    }

    /*********************************************************************
     * @function Import
     *
     * @brief   Store the key/data blob
     *
     * @param[in] size - size of input blob
     * @param[in] blob - input blob of data to be stored
     * @param[in] exportable - blob expotable(true/false)
     *
     * @return id across which the hmac/AES object ids are stored.
     *
     *********************************************************************/
    uint32_t Vault::Import(const uint16_t size, const uint8_t blob[], bool exportable)
    {
        uint32_t id = 0;

        _lock.Lock();
            if (size > 0) {
                id = (_lastHandle + 1);
                if (id != 0) {
                    struct IdStore ids;
                    SEC_BYTE* data = (SEC_BYTE*)blob;
                    Sec_Result sec_res_hmac = SEC_RESULT_FAILURE;
                    Sec_Result sec_res_aes = SEC_RESULT_FAILURE;

                    if (KEYLEN_AES_HMAC_128 == size) { /* 16 byte aes or an hmac key */
                        ids.idAes = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE, SEC_OBJECTID_USER_TOP);
                        if (ids.idAes != 0) {
                            sec_res_aes = SecKey_Provision(_secProcHandle, ids.idAes, SEC_STORAGELOC_RAM, SEC_KEYCONTAINER_RAW_AES_128, data, size);
                        }
                        ids.idHmac = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE, SEC_OBJECTID_USER_TOP);
                        if (ids.idHmac != 0) {
                            sec_res_hmac = SecKey_Provision(_secProcHandle, ids.idHmac, SEC_STORAGELOC_RAM, SEC_KEYCONTAINER_RAW_HMAC_128, data, size);
                        }
                   }

                   else if (KEYLEN_AES_HMAC_256 == size) { /* 32 byte aes or an hmac key */
                        ids.idAes = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE, SEC_OBJECTID_USER_TOP);
                        if (ids.idAes != 0) {
                            sec_res_aes = SecKey_Provision(_secProcHandle, ids.idAes, SEC_STORAGELOC_RAM, SEC_KEYCONTAINER_RAW_AES_256, data, size);
                        }
                        ids.idHmac = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE, SEC_OBJECTID_USER_TOP);
                        if (ids.idHmac != 0) {
                            sec_res_hmac = SecKey_Provision(_secProcHandle, ids.idHmac, SEC_STORAGELOC_RAM, SEC_KEYCONTAINER_RAW_HMAC_256, data, size);
                        }
                   }

                   else if (KEYLEN_HMAC_160  == size) {
                       sec_res_aes = SEC_RESULT_SUCCESS; /* 20 byte hmac key ,no such aes key*/
                       ids.idAes = 0;
                       ids.idHmac = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE, SEC_OBJECTID_USER_TOP);
                       if (ids.idHmac != 0) {
                           sec_res_hmac = SecKey_Provision(_secProcHandle, ids.idHmac, SEC_STORAGELOC_RAM, SEC_KEYCONTAINER_RAW_HMAC_160, data, size);
                       }
                   }

                   if ((sec_res_aes == SEC_RESULT_SUCCESS) && (sec_res_hmac == SEC_RESULT_SUCCESS)) {
                       TRACE_L2(_T("SEC : key is provisioned \n"));

                       _items.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                       std::forward_as_tuple(exportable, ids, size));

                       _lastHandle = id;
                       TRACE_L2(_T("SEC : Added a %s data blob \n"), (exportable ? "clear" : "sealed"));
                   }
                   else if((sec_res_aes != SEC_RESULT_SUCCESS) || (sec_res_hmac != SEC_RESULT_SUCCESS))  {
                       if (sec_res_hmac == SEC_RESULT_SUCCESS) {
                           SecKey_Delete(_secProcHandle, ids.idHmac);
                       }
                       if ((sec_res_aes == SEC_RESULT_SUCCESS) && (ids.idAes != 0)) {
                           SecKey_Delete(_secProcHandle, ids.idAes);
                       }
                       TRACE_L1(_T("SEC :cannot provision key, result for key provision Aes:%d and Hmac:%d "),sec_res_aes,sec_res_hmac);
                   }
               }
               else {
                   TRACE_L1(_T("SEC : id assigned is zero \n"));
               }
           }
           TRACE_L2(_T("SEC : import ends the id assigned is 0x%08x \n"), id);

        _lock.Unlock();

        return(id);

    }

    /*********************************************************************
     * @function ImportNamedKey
     *
     * @brief   Load the persistent key details to vault
     *
     * @param[in] keyFile -keyfile sec object id  to be loaded
     *
     * @return id of vault cross which the hmac/AES object ids are stored.
     *
     *********************************************************************/
    uint32_t Vault::ImportNamedKey(const char keyFile[])
    {
        uint32_t id = 0;

        _lock.Lock();
        SEC_OBJECTID idcheck = 0x0;
        struct IdStore ids;
        string kFile(keyFile);
        string fileName = kFile.substr(0,SEC_ID_SIZE);
        uint64_t secId  = stoull(fileName,0,16);
        if (secId != 0 ) {
            idcheck = secId;
            TRACE_L2(_T("SEC: the obj id from file name is %llx \n"),idcheck);
            Sec_KeyHandle* sec_key_check;
            Sec_Result sec_res = SecKey_GetInstance(_secProcHandle,idcheck, &sec_key_check);
            if( sec_res == SEC_RESULT_SUCCESS ) {
                Sec_KeyType key_type = SecKey_GetKeyType(sec_key_check);
                if((key_type == SEC_KEYTYPE_HMAC_256) ||(key_type == SEC_KEYTYPE_HMAC_128)||(key_type == SEC_KEYTYPE_HMAC_160)) {
                    ids.idHmac = idcheck;
                }
                else if ((key_type == SEC_KEYTYPE_AES_128)||(key_type == SEC_KEYTYPE_AES_256)) {
                    ids.idAes = idcheck;
                }
                else {
                    TRACE_L1(_T("SEC:key type not supported :%d\n"),key_type);
                    SecKey_Release(sec_key_check);
                    _lock.Unlock();
                    return id;
                }
                id = (_lastHandle + 1);
                _items.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                               std::forward_as_tuple(false, ids,SEC_ID_SIZE));

                TRACE_L2(_T("SEC: key placed at %x vault id ,sec obj id %llx \n"),id,idcheck);
                _lastHandle = id;
                SecKey_Release(sec_key_check);
            }
            else {
                TRACE_L1(_T("SEC:key instance  not created for %llx , retVal =  %d\n"),idcheck,sec_res);
            }
        }
        else {
            TRACE_L1(_T("SEC:cannot retrieve valid sec object id from filename %s \n"),kFile.c_str());

        }
        _lock.Unlock();

        return(id);
    }

    /*********************************************************************
     * @function CheckNamedKey
     *
     * @brief   Check if a persistently stored key exists
     *
     * @param[in] keyFile -keyfile to be checked
     *
     * @return ret status -true/false if the key exists
     *
     *********************************************************************/

    bool Vault::CheckNamedKey(const char keyFile[])
    {
        bool ret =false;

        _lock.Lock();
        SEC_OBJECTID idcheck = 0x0;
        string kFile(keyFile);
        string fileName = kFile.substr(0,SEC_ID_SIZE);
        uint64_t secId  = stoull(fileName,0,16);
        if (secId != 0 ) {
            idcheck = secId;
            TRACE_L2(_T("SEC: the obj id from file name is %llx \n"),idcheck);
            Sec_KeyHandle* sec_key_check;
            Sec_Result sec_res = SecKey_GetInstance(_secProcHandle,idcheck, &sec_key_check);
            if( sec_res == SEC_RESULT_SUCCESS ) {
                SecKey_Release(sec_key_check);
                ret =true;
            }
            else {
                TRACE_L1(_T("SEC:key instance  not created for %llx , retVal =  %d\n"),idcheck,sec_res);
            }
        }
        else {
            TRACE_L1(_T("SEC:cannot retrieve valid sec object id from filename %s \n"),kFile.c_str());

        }
        _lock.Unlock();

        return(ret);
    }

     /*********************************************************************
     * @function CreateNamedKey
     *
     * @brief   Create a new AES/HMAC named key
     *
     * @param[in] keyFile -keyfile to be generated
     * @param[in] exportable - blob expotable(true/false)
     * @param[in] keyType -key type (AES/HMAC ,128/160/256)
     *
     * @return id across which the hmac/AES object ids are stored.
     *
     *********************************************************************/
    uint32_t Vault::CreateNamedKey(const char keyFile[] ,bool exportable ,const key_type  keyType)
    {
        uint32_t id = 0;

        _lock.Lock();
        struct IdStore ids;
        Sec_KeyType key = SEC_KEYTYPE_AES_128; //DEFAULT

        switch(keyType) {
            case key_type::AES128 :
                key = SEC_KEYTYPE_AES_128;
                break;
            case key_type::AES256 :
                key = SEC_KEYTYPE_AES_256;
                break;
            case key_type::HMAC128 :
                key = SEC_KEYTYPE_HMAC_128;
                break;
            case key_type::HMAC160 :
                key = SEC_KEYTYPE_HMAC_160;
                break;
            case key_type::HMAC256 :
                key = SEC_KEYTYPE_HMAC_256;
                break;
            default :
                TRACE_L1(_T("SEC :cannot generate any other key types" ));
                _lock.Unlock();
                return (id);
        }

        string kFile(keyFile);
        if(!kFile.empty() && (kFile.length() >= SEC_ID_SIZE)) {
            string keyName = kFile.substr(0,SEC_ID_SIZE);
            uint64_t secId = stoull(keyName,0,16);
            if( secId != 0) {
                TRACE_L2(_T("SEC:the key name/sec id is  %llx \n"),secId);
                Sec_Result res = SecKey_Generate(_secProcHandle,secId,key,SEC_STORAGELOC_FILE);
                if( res == SEC_RESULT_SUCCESS) {
                    id = (_lastHandle + 1);
                    if ((keyType == AES128) || (keyType == AES256)) {
                        ids.idAes = secId;
                    }
                    else {
                        ids.idHmac = secId;
                    }
                    _items.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                                   std::forward_as_tuple(exportable, ids, SEC_ID_SIZE));
                    TRACE_L2(_T("SEC: new  key placed at %x vault id ,sec obj id %llx \n"),id,secId);
                    _lastHandle = id;
                }
                else {
                    TRACE_L1(_T("SEC:new  key cannot be created for sec id %llx  ,retVal %d\n"),secId,res);
                }
            }
            else {
                TRACE_L1(_T("SEC:valid sec id not retreived  from file name %s \n"),kFile.c_str());
            }
        }
        else {
            TRACE_L1(_T("SEC:Invalid File name : %s\n"),kFile.c_str());
        }

        _lock.Unlock();
        return (id);

    }

    /*********************************************************************
     * @function Export
     *
     * @brief   Export the stored blob's secobject id 
     *
     * @param[in] id - id of the blob
     * @param[in] size - size of the blob
     * @param[out] blob - pointer to struct storing both the hmac and aes secobject ids
     * @param[in] allowSealed - blob sealed (true/false)
     *
     * @return size of blob
     *
     *********************************************************************/
    uint16_t Vault::Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed) const
    {
        uint16_t outSize = 0;
        if (size > 0) {
            _lock.Lock();
            auto it = _items.find(id);
            if (it != _items.end()) {
                if (((allowSealed == true) || (*it).second.IsExportable() == true) && ((*it).second.KeyLength() != 0)) {
                    IdStore* ids = const_cast<IdStore*>((*it).second.getIdStore());
                    uint16_t keyLength = (*it).second.KeyLength();
                    Sec_Result sec_res_hmac = SEC_RESULT_FAILURE;
                    Sec_Result sec_res_aes = SEC_RESULT_FAILURE;
                    Sec_KeyHandle* sec_key_hmac;
                    Sec_KeyHandle* sec_key_aes;
                    if ((KEYLEN_HMAC_160 == keyLength) || (KEYLEN_AES_HMAC_128 == keyLength) || (KEYLEN_AES_HMAC_256 == keyLength)) {
                        sec_res_hmac = SecKey_GetInstance(_secProcHandle, ids->idHmac, &sec_key_hmac);
                        sec_res_aes = SecKey_GetInstance(_secProcHandle, ids->idAes, &sec_key_aes);
                        if(sec_res_hmac == SEC_RESULT_SUCCESS) {
                            outSize = SecKey_GetKeyLen(sec_key_hmac);
                            TRACE_L2(_T("SEC:export HMAC key of size %d\n"),outSize);
                            ASSERT(outSize == keyLength);
                            SecKey_Release(sec_key_hmac);
                        }
                        if(sec_res_aes == SEC_RESULT_SUCCESS) {
                            outSize = SecKey_GetKeyLen(sec_key_aes);
                            TRACE_L2(_T("SEC:export AES  key of size %d\n"),outSize);
                            ASSERT(outSize == keyLength);
                            SecKey_Release(sec_key_aes);
                        }

                        memcpy(blob, &ids, sizeof(ids));
                    }
                }
                else {
                    TRACE_L1(_T("Blob id 0x%08x is sealed, can't export"), id);
                }
            }
            else {
                TRACE_L1(_T("SEC : Failed to look up blob id 0x%08x"), id);
            }
            _lock.Unlock();
        }
        return (outSize);
    }

    /*********************************************************************
     * @function Put
     *
     * @brief    store en encrypted blob of data along an id
     *
     * @param[in] size - size of the data blob
     * @param[in] blob - blob array that needs to be stored
     *
     * @return id across which the data blob is stored
     *
     *********************************************************************/
    uint32_t Vault::Put(const uint16_t size, const uint8_t blob[])
    {
        uint32_t id = 0;

        if (size > 0) {
            _lock.Lock();
            id = (_lastHandle + 1);
            if (id != 0) {
                SEC_BYTE store[SEC_KEYCONTAINER_MAX_LEN];
                uint8_t* blob_data = const_cast<uint8_t*>(blob);
                Sec_Result sec_res = SecStore_StoreData(_secProcHandle, SEC_FALSE, SEC_FALSE, (SEC_BYTE*)SEC_UTILS_KEYSTORE_MAGIC,
                    NULL, 0, blob_data, size, store, sizeof(store));
                if (sec_res == SEC_RESULT_SUCCESS) {
                    _items.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                        std::forward_as_tuple(false, store, sizeof(store)));
                    _lastHandle = id;
                    TRACE_L2(_T("SEC: Inserted a sealed data blob of size %i as id 0x%08x"), size, id);
                }
                else {
                    TRACE_L1(_T("SEC :Failed to store blob of id 0x%08x \n"), id);
                }

            }
            _lock.Unlock();
        }

        return (id);
    }

    /*********************************************************************
     * @function Get 
     *
     * @brief    Get back the encrypted data blob stored
     *
     * @param[in] id - id of blob
     * @param[in] size - size of the blob
     * @param[out] blob - data blob stored
     *
     * @return size of data blob got 
     *
     *********************************************************************/
    uint16_t Vault::Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const
    {
        uint16_t outSize = 0;

        if (size > 0) {
            _lock.Lock();
            auto it = _items.find(id);
            if (it != _items.end()) {
                SEC_BYTE* buff = const_cast<SEC_BYTE*>((*it).second.Buffer());
                SEC_SIZE bufLen = (*it).second.Size();
                SecUtils_KeyStoreHeader keystore_header;
                SEC_BYTE dataOut[SEC_KEYCONTAINER_MAX_LEN];
                Sec_Result sec_res = SecStore_RetrieveData(_secProcHandle, SEC_FALSE, &keystore_header, sizeof(keystore_header),
                    dataOut, sizeof(dataOut), buff, bufLen);
                if (sec_res == SEC_RESULT_SUCCESS) {
                    //Copy  to blob
                    outSize = SecStore_GetDataLen(buff);
                    memcpy(blob, dataOut, outSize);
                    TRACE_L2(_T("SEC: Retrieved a sealed data blob id 0x%08x of size %i bytes"), id, outSize);
                }
                else {
                    TRACE_L1(_T("SEC:Failed at sec_retrieve for id 0x%08x and retVal = %d\n"), id,sec_res);
                }
            }
            else {
                TRACE_L1(_T("SEC : Failed to look up blob id 0x%08x"), id);
            }
            _lock.Unlock();
        }
        return(outSize);
    }

   /*********************************************************************
     * @function Delete
     *
     * @brief    Delete the id and its data from map
     *
     * @param[in] id - id of the blob to be deleted 
     *
     * @return true upon succesful delete,else false.
     *
     *********************************************************************/
    bool Vault::Delete(const uint32_t id)
    {
        bool result = false;

        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            IdStore* ids = const_cast<IdStore*>((*it).second.getIdStore());
            if (ids->idAes != 0) {
                SecKey_Delete(_secProcHandle, ids->idAes);
            }
            if (ids->idHmac != 0) {
                SecKey_Delete(_secProcHandle, ids->idHmac);
            }
            _items.erase(it);
            result = true;
        }
        _lock.Unlock();

        return (result);
    }

} // namespace Implementation

extern "C" {

    // Vault
    VaultImplementation* vault_instance(const cryptographyvault id)
    {
        Implementation::Vault* vault = nullptr;
        Implementation::VaultNetflix* netflix;

        switch (id) {
        case CRYPTOGRAPHY_VAULT_NETFLIX:
            netflix = &Implementation::VaultNetflix::NetflixInstance();
            TRACE_L2(_T("SEC :NETFLIX INSTANCE CASE \n"));
            Implementation::vaultId = CRYPTOGRAPHY_VAULT_NETFLIX;
            break;
        case CRYPTOGRAPHY_VAULT_DEFAULT:
           {
                static Implementation::Vault instance;
                vault = &(instance);

            if (vault != nullptr)
                TRACE_L2(_T("SEC :VAULT DEFAULT CASE \n"));
            Implementation::vaultId = CRYPTOGRAPHY_VAULT_DEFAULT; //DEFAULT
            break;
            }
        default:
            TRACE_L1(_T("SEC: Vault not supported: %d"), static_cast<uint32_t>(id));
            break;
        }
        if (id == CRYPTOGRAPHY_VAULT_NETFLIX) {
            return reinterpret_cast<VaultImplementation*>(netflix);
        }
        else {
            return reinterpret_cast<VaultImplementation*>(vault);
        }
    }

    uint16_t vault_size(const VaultImplementation* vault, const uint32_t id)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_size netflix \n"));
            const Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<const Implementation::VaultNetflix*>(vault);
            return(vaultnetflixImpl->Size(id));
        }
        else {
            TRACE_L2(_T("SEC:vault_size generic\n"));
            const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
            return (vaultImpl->Size(id));
        }
    }

    uint32_t vault_import(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_import netflix \n"));
            Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<Implementation::VaultNetflix*>(vault);
            return(vaultnetflixImpl->Import(length, data, true /* imported in clear is always exportable */));
        }
        else {
            TRACE_L2(_T("SEC:vault_import generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            return (vaultImpl->Import(length, data, true /* imported in clear is always exportable */));
        }
    }

    uint16_t vault_export(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
    {
        ASSERT(vault != nullptr);
        uint16_t size = 0;
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_export netflix \n"));
            const Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<const Implementation::VaultNetflix*>(vault);
            size = vaultnetflixImpl->Export(id, max_length, data);
        }
        else {
            TRACE_L2(_T("SEC:vault_export generic\n"));
            const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
            size = vaultImpl->Export(id, max_length, data);
        }
        return size;
    }

    uint32_t vault_set(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_set netflix\n"));
            Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<Implementation::VaultNetflix*>(vault);
            return(vaultnetflixImpl->Put(length, data));
        }
        else {
            TRACE_L2(_T("SEC:vault_set generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            return (vaultImpl->Put(length, data));
        }
    }

    uint16_t vault_get(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
    {
        ASSERT(vault != nullptr);
        uint16_t size = 0;
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_get \n"));
            const Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<const Implementation::VaultNetflix*>(vault);
            size = vaultnetflixImpl->Get(id, max_length, data);
        }
        else {
            TRACE_L2(_T("SEC:vault_get generic \n"));
            const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
            size = vaultImpl->Get(id, max_length, data);
        }
        return size;
    }

    bool vault_delete(VaultImplementation* vault, const uint32_t id)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            TRACE_L2(_T("SEC:vault_delete netflix \n"));
            Implementation::VaultNetflix* vaultnetflixImpl = reinterpret_cast<Implementation::VaultNetflix*>(vault);
            return(vaultnetflixImpl->Delete(id));
        }
        else {
            TRACE_L2(_T("SEC:vault_delete generic \n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            return (vaultImpl->Delete(id));
        }
    }

    uint32_t persistent_key_create( struct VaultImplementation* vault,const char locator[],const key_type keyType,uint32_t* id)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            //NOT IMPLEMENTED FOR SEC_NETFLIX
            return (WPEFramework::Core::ERROR_UNAVAILABLE);
        }
        else {
            TRACE_L2(_T("SEC:persistent_key_create generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            (*id) = vaultImpl->CreateNamedKey(locator,false,keyType) ; //SEC keys are not exportable
            return (((*id)!=0)?(WPEFramework::Core::ERROR_NONE):(WPEFramework::Core::ERROR_GENERAL)) ;
        }
    }

    uint32_t persistent_key_load(struct VaultImplementation* vault,const char locator[],uint32_t* id)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            //NOT IMPLEMENTED FOR SEC_NETFLIX
            return (WPEFramework::Core::ERROR_UNAVAILABLE);
        }
        else {
            TRACE_L2(_T("SEC:persistent_key_load  generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            (*id) = vaultImpl->ImportNamedKey(locator);
            return (((*id)!=0)?(WPEFramework::Core::ERROR_NONE):(WPEFramework::Core::ERROR_GENERAL)) ;
        }

    }

    uint32_t  persistent_key_exists( struct VaultImplementation* vault ,const char locator[],bool* result)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            //NOT IMPLEMENTED FOR SEC_NETFLIX
            return (WPEFramework::Core::ERROR_UNAVAILABLE);
        }
        else {
            TRACE_L2(_T("SEC:persistent_key_exists generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            (*result)= vaultImpl->CheckNamedKey(locator);
            return (WPEFramework::Core::ERROR_NONE);
        }

    }

    uint32_t persistent_flush(struct VaultImplementation* vault)
    {
        ASSERT(vault != nullptr);
        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_DEFAULT) {
            TRACE_L2(_T("SEC:persistent_flush generic\n"));
            Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
            vaultImpl->ProcessorRelease();
            return (WPEFramework::Core::ERROR_NONE);
        }
        else {
            return (WPEFramework::Core::ERROR_UNAVAILABLE);
        }

    }
} // extern "C"
