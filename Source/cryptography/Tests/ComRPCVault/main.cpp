#include <iostream>
#include <iomanip>

#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <cryptography.h>

namespace Thunder = WPEFramework;

int main(int argc, char *argv[])
{
#ifdef __WINDOWS__
    const string nodeId("127.0.0.1:62005");
#else
    const string nodeId("/tmp/svalbard");
#endif

    {
        Thunder::Cryptography::ICryptography *cryptography = Thunder::Cryptography::ICryptography::Instance(nodeId);

        if (cryptography != nullptr)
        {
            Thunder::Cryptography::IVault *vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);
            cryptography->Release();

            char keyPress;
            uint32_t id = 0;

            const char data[] = "super secret string of secets";

            do
            {
                keyPress = toupper(getchar());

                switch (keyPress)
                {
                case 'O':
                {
                    break;
                }
                case 'I':
                {
                    if (id == 0)
                    {
                        id = vault->Import(sizeof(data), reinterpret_cast<const uint8_t*>(data));
                        std::cout << "Imported data, ID: " << std::setw(8) << std::hex << id << std::endl;
                    }
                    break;
                }
                case 'E':
                {
                    uint32_t base = 0x80000001;

                    while(vault->Size(base) > 0){
                        uint8_t data[vault->Size(base)];

                        vault->Export(base, sizeof(data), data);

                        string hexData;
                        Thunder::Core::ToHexString(data, sizeof(data), hexData);

                        std::cout << "ID: " << std::setw(8) << std::hex << base << std::dec << " Data:\"" << hexData << "\"" << std::endl;

                        base++;
                    }

                    break;
                }
                
                case 'S':
                {
                    uint16_t size = vault->Size(id);
                    std::cout << "Size of dataID: " << std::setw(8) << std::hex << id << std::dec << " is " << size << "bytes" << std::endl;
                    break;
                }
                case 'Q':
                    break;
                default:
                    break;
                };
            } while (keyPress != 'Q');
        }
    }

    Thunder::Core::Singleton::Dispose();

    return 0;
}
