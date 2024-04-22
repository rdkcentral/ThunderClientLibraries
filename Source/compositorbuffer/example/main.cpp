/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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
 **/
#include <compositorbuffer/CompositorBufferType.h>

using namespace WPEFramework;

MODULE_NAME_ARCHIVE_DECLARATION
namespace Test {

const char* descriptors[] = {
    _T("/tmp/buffer1.txt"),
    _T("/tmp/buffer2.txt"),
};

const char bridgeConnector[] = _T("/tmp/connector");

class CompositorBuffer : public WPEFramework::Compositor::CompositorBufferType<4>, public Core::IReferenceCounted {
private:
    using BaseClass = WPEFramework::Compositor::CompositorBufferType<4>;

protected:
    CompositorBuffer(const string& callsign, const uint32_t id, 
        const uint32_t width, const uint32_t height, 
        const uint32_t format, const uint64_t modifier, 
        const Exchange::ICompositionBuffer::DataType type)
        : BaseClass(callsign, id, width, height, format, modifier, type)
    {

        printf("Constructing server buffer.\n");

        for (uint8_t index = 0; index < (sizeof(descriptors) / sizeof(const char*)); index++) {
            Core::File file(string(descriptors[index]));
            if (file.Create(Core::File::USER_READ | Core::File::USER_WRITE) == true) {
                BaseClass::Add(static_cast<Core::File::Handle>(file), 0xAAAA + index, 0x5555 + index);
                printf("Opening: [%d] -> name: %s\n", static_cast<Core::File::Handle>(file), descriptors[index]);
            }
        }
        // Let's start monitoring the CompositorBuffer to detect changes
        Core::ResourceMonitor::Instance().Register(*this);
    }

public:
    CompositorBuffer() = delete;
    CompositorBuffer(CompositorBuffer&&) = delete;
    CompositorBuffer(const CompositorBuffer&) = delete;
    CompositorBuffer& operator=(const CompositorBuffer&) = delete;

    static Core::ProxyType<CompositorBuffer> Create(const string& callsign, const uint32_t width, const uint32_t height, const Exchange::ICompositionBuffer::DataType type)
    {
        static uint32_t id = 1;
        uint32_t identifier = Core::InterlockedIncrement(id);
        return (_map.Instance<CompositorBuffer>(callsign, callsign, identifier, width, height, 0xAA, 0x55, type));
    }
    static Core::ProxyType<CompositorBuffer> Find(const string& callsign)
    {
        return (_map.Find(callsign));
    }
    ~CompositorBuffer() override
    {
        // Let's stop monitoring the CompositorBuffer to detect changes
        Core::ResourceMonitor::Instance().Unregister(*this);
        printf("Destructing server buffer.\n");
    }

public:
    // Methods still to be implemented....
    void Render() override
    {
        printf("We need to do our magic here :-)\n");
    }

private:
    static Core::ProxyMapType<string, CompositorBuffer> _map;
};

/* static */ Core::ProxyMapType<string, CompositorBuffer> CompositorBuffer::_map;

class ClientBuffer : public WPEFramework::Compositor::CompositorBufferType<4>, public Core::IReferenceCounted {
private:
    using BaseClass = WPEFramework::Compositor::CompositorBufferType<4>;

protected:
    ClientBuffer(const uint32_t id, Core::PrivilegedRequest::Container& descriptors)
        : BaseClass(id, descriptors)
    {
    }

public:
    ClientBuffer() = delete;
    ClientBuffer(ClientBuffer&&) = delete;
    ClientBuffer(const ClientBuffer&) = delete;
    ClientBuffer& operator=(const ClientBuffer&) = delete;

    static Exchange::ICompositionBuffer* Create(const uint32_t id, Core::PrivilegedRequest::Container& descriptors)
    {
        Core::ProxyType<ClientBuffer> element(Core::ProxyType<ClientBuffer>::Create(id, descriptors));
        Exchange::ICompositionBuffer* result = &(*element);
        result->AddRef();
        return (result);
    }

public:
    // Methods still to be implemented....
    void Render() override
    {
        ASSERT(false);
    }
};

class Dispatcher : public Core::PrivilegedRequest {
public:
    Dispatcher(Dispatcher&&) = delete;
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Dispatcher() = default;
    ~Dispatcher() override = default;

    void Add(const uint32_t id, WPEFramework::Compositor::CompositorBufferType<4>& buffer)
    {
        _id = id;
        _buffer = &buffer;
    }
    void Remove(const uint32_t id)
    {
        if (_id == id) {
            _id = 0;
            _buffer = nullptr;
        }
    }
    uint8_t Service(const uint32_t id, const uint8_t maxSize, int container[]) override
    {
        if ((id == _id) && (_buffer != nullptr)) {
            uint8_t result = _buffer->Descriptors(maxSize, container);

            if (result > 0) {
                printf("Handing out: %d descriptors: [", result);
                for (uint8_t index = 0; index < (result - 1); index++) {
                    printf("%d,", container[index]);
                }
                printf("%d]\n", container[result - 1]);
            }
            return (result);
        }
        return 0;
    }

private:
    uint32_t _id;
    WPEFramework::Compositor::CompositorBufferType<4>* _buffer;
};

} // namespace Test

bool server = false;
uint32_t bufferId = 0;

bool ParseOptions(int argc, const char** argv)
{
    int index = 1;
    bool showHelp = false;

    while ((index < argc) && (showHelp == false)) {

        if (strcmp(argv[index], "-server") == 0) {
            server = true;
        } else if (strcmp(argv[index], "-b") == 0) {

            if (((index + 1) < argc) && (argv[index + 1][0] != '-')) {

                index++;
                bufferId = atoi(argv[index]);
            }
        } else {
            showHelp = true;
        }
        index++;
    }

    if (showHelp == true) {
        printf("Test passing filedescriptos and information over a process boundary.\n");
        printf("%s [-server] [-b <number>]\n", argv[0]);
        printf("  -server      Act as a server.\n");
        printf("  -b <number>  Act as a client and get the buffer associated with this number.\n");
    }

    return (showHelp == false);
}

int main(int argc, const char* argv[])
{
    {
        TCHAR element;
        ParseOptions(argc, argv);
        printf("Running as: [%s]\n", server ? _T("server") : _T("client"));
        Test::Dispatcher bridge;
        Core::ProxyType<Test::CompositorBuffer> serverBuffer;
        
        Exchange::ICompositionBuffer* buffer = nullptr;

        if (server == true) {
            serverBuffer = Test::CompositorBuffer::Create(_T("TestCall"), 1024, 1080, Exchange::ICompositionBuffer::TYPE_RAW);
            buffer = &(*serverBuffer);
            bridge.Open(string(Test::bridgeConnector));
            printf("Server has created a buffer, known as: [%d]\n", bufferId);
            bridge.Add(bufferId, *serverBuffer);
        } else {
            printf("Client instantiated to get information of buffer: [%d]\n", bufferId);
        }

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'O':
                if (buffer == nullptr) {
                    Core::PrivilegedRequest::Container descriptors;
                    printf("Get the buffer interface, Sesame open:\n");
                    if (bridge.Request(1000, string(Test::bridgeConnector), bufferId, descriptors) == Core::ERROR_NONE) {
                        printf("Got the buffer on client side.\n");
                        buffer = Test::ClientBuffer::Create(bufferId, descriptors);
                    } else {
                        printf("Failed to get the buffer.\n");
                    }
                } else if (server == false) {
                    printf("Where done, we put it all in, close the vault, Sesame close:\n");
                    buffer = nullptr;
                }
                break;
            case 'I':
                if (buffer == nullptr) {
                    printf("There are no buffers\n");
                } else {
                    printf("Buffer info:\n");
                    printf("==========================================\n");
                    printf("Width:  %d\n", buffer->Width());
                    printf("Height: %d\n", buffer->Height());
                    printf("Format: %d\n", buffer->Format());
                    printf("Type: %d\n", buffer->Type());
                    WPEFramework::Compositor::CompositorBufferType<4>* info = dynamic_cast<WPEFramework::Compositor::CompositorBufferType<4>*>(buffer);
                    if (info != nullptr) {
                        printf("Dirty:  %s\n", info->IsDirty() ? _T("true") : _T("false"));
                    }
                }
                break;
            case 'W':
                if (buffer == nullptr) {
                    printf("There are no buffers\n");
                } else {
                    Exchange::ICompositionBuffer::IIterator* planes = buffer->Planes(10);

                    if (planes != nullptr) {
                        printf("Iterating ove the planes to write:\n");
                        while (planes->Next() == true) {
                            Exchange::ICompositionBuffer::IPlane* plane = planes->Plane();
                            ASSERT(plane != nullptr);
                            int fd = static_cast<int>(plane->Accessor());
                            printf("Writing to [%d]:\n", fd);
                            ::write(fd, "Hello World !!!\n", 16);
                            ::fsync(fd);
                        }
                    }
                    
                    buffer->Completed(true);
                }
                break;
            case 'Q':
                break;
            case '?':
                printf("Options available:\n");
                printf("=======================================================================\n");
                printf("<O> Open/Close the buffer.\n");
                printf("<I> Information of the buffer.\n");
                printf("<W> Write a the number in a file.\n");
                printf("<?> Have no clue what I can do, tell me.\n");
                break;
            default:
                break;
            }
        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return EXIT_SUCCESS;
}
