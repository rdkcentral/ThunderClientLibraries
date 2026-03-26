/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#define MODULE_NAME GraphicsBufferTestApp 

#include <graphicsbuffer/GraphicsBufferType.h>

using namespace Thunder;

MODULE_NAME_ARCHIVE_DECLARATION
namespace Test {

const char* descriptors[] = {
    _T("/tmp/buffer1.txt"),
    _T("/tmp/buffer2.txt"),
};

const char bridgeConnector[] = _T("/tmp/connector");

class ServerBuffer : public Graphics::ServerBufferType<4> {
private:
    using BaseClass = Graphics::ServerBufferType<4>;

protected:
    ServerBuffer(
        const uint32_t width, const uint32_t height,
        const uint32_t format, const uint64_t modifier,
        const Exchange::IGraphicsBuffer::DataType type)
        : BaseClass(width, height, format, modifier, type)
        , _dirty(false)
    {
        printf("Constructing server buffer.\n");

        for (uint8_t index = 0; index < (sizeof(descriptors) / sizeof(const char*)); index++) {
            Core::File file(string(descriptors[index]));
            if (file.Create(Core::File::USER_READ | Core::File::USER_WRITE) == true) {
                BaseClass::Add(static_cast<Core::File::Handle>(file), 0xAAAA + index, 0x5555 + index);
                printf("Opening: [%d] -> name: %s\n", static_cast<Core::File::Handle>(file), descriptors[index]);
            }
        }
        // Let's start monitoring the ServerBuffer to detect changes
        Core::ResourceMonitor::Instance().Register(*this);
    }

public:
    ServerBuffer() = delete;
    ServerBuffer(ServerBuffer&&) = delete;
    ServerBuffer(const ServerBuffer&) = delete;
    ServerBuffer& operator=(ServerBuffer&&) = delete;
    ServerBuffer& operator=(const ServerBuffer&) = delete;

    static Core::ProxyType<ServerBuffer> Create(const string& callsign, const uint32_t width, const uint32_t height, const Exchange::IGraphicsBuffer::DataType type)
    {
        return (_map.Instance<ServerBuffer>(callsign, width, height, 0xAA, 0x55, type));
    }
    static Core::ProxyType<ServerBuffer> Find(const string& callsign)
    {
        return (_map.Find(callsign));
    }
    ~ServerBuffer() override
    {
        // Let's stop monitoring the ServerBuffer to detect changes
        Core::ResourceMonitor::Instance().Unregister(*this);
        printf("Destructing server buffer.\n");
    }

public:
    bool IsDirty() const
    {
        return (_dirty);
    }
    IIterator* Acquire()
    {
        IIterator* result = BaseClass::Acquire(0);
        if (result != nullptr) {
            _dirty = false;
        }
        return (result);
    }
    void Request() override
    {
        printf("We need to do our magic here :-)\n");
        _dirty = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(4));

        if (Rendered() == true) {
            printf("Request rendered.\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(12));

            if (Published() == true){
                printf("Request published.\n");
            } else {
                printf("Request failed to publish.\n");
            }
        } else {
            printf("Request failed to render.\n");
        };
    }

private:
    std::atomic<bool> _dirty;
    static Core::ProxyMapType<string, ServerBuffer> _map;
};

/* static */ Core::ProxyMapType<string, ServerBuffer> ServerBuffer::_map;

class ClientBuffer : public Graphics::ClientBufferType<4> {
private:
    using BaseClass = Graphics::ClientBufferType<4>;

protected:
    ClientBuffer(Core::PrivilegedRequest::Container& descriptors)
        : BaseClass()
    {
        BaseClass::Load(descriptors);
        // Let's start monitoring the ClientBuffer to detect changes
        Core::ResourceMonitor::Instance().Register(*this);
    }

public:
    ~ClientBuffer() override
    {
        // Let's stop monitoring the ClientBuffer to detect changes
        Core::ResourceMonitor::Instance().Unregister(*this);
        printf("Destructing client buffer.\n");
    }

    ClientBuffer() = delete;
    ClientBuffer(ClientBuffer&&) = delete;
    ClientBuffer(const ClientBuffer&) = delete;
    ClientBuffer& operator=(const ClientBuffer&) = delete;

    static Core::ProxyType<Exchange::IGraphicsBuffer> Create(Core::PrivilegedRequest::Container& descriptors)
    {
        return (Core::ProxyType<Exchange::IGraphicsBuffer>(Core::ProxyType<ClientBuffer>::Create(descriptors)));
    }

public:
    // seems we have been Rendered
    void Rendered() override
    {
        printf("My buffer is rendered but might not be visible yet.\n");
    }
    // seems we have been Rendered
    void Published() override
    {
        printf("My buffer is published so its now visible.\n");
    }
};

class Dispatcher : public Core::PrivilegedRequest {
private:
    class Callback : public Core::PrivilegedRequest::ICallback {
    public:
        Callback() = delete;
        Callback(Callback&&) = delete;
        Callback(const Callback&) = delete;
        Callback& operator=(Callback&&) = delete;
        Callback& operator=(const Callback&) = delete;

        Callback(Dispatcher& parent) : _parent(parent) {}
        ~Callback() override = default;

    public:
        void Request(const uint32_t id, Container& descriptors) override {
           _parent.Request(id, descriptors);
        }
        void Offer(const uint32_t id, Container&& descriptors) override {
           _parent.Offer(id, std::move(descriptors));
        }

    private:
        Dispatcher& _parent;
    };

public:
    Dispatcher(Dispatcher&&) = delete;
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(Dispatcher&&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Dispatcher() 
        : Core::PrivilegedRequest(&_callback) 
        , _callback(*this) {
    }
    ~Dispatcher() override = default;

    void Add(const uint32_t id, Graphics::ServerBufferType<4>& buffer)
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
    uint32_t Request(const uint32_t waitTime, const string& identifier, const uint32_t requestId, Container& fds) {
        return (Core::PrivilegedRequest::Request(waitTime, identifier, requestId, fds));
    }

private:
    void Request(const uint32_t id, Container& descriptors) {
        if ((id == _id) && (_buffer != nullptr)) {
            int container[Core::PrivilegedRequest::MaxDescriptorsPerRequest];
            uint8_t result = _buffer->Descriptors(sizeof(container), container);

            if (result > 0) {
                printf("Handing out: %d descriptors: [", result);
                for (uint8_t index = 0; index < (result - 1); index++) {
                    descriptors.emplace_back(container[index]);
                    printf("%d,", container[index]);
                }
                printf("%d]\n", container[result - 1]);
            }
        }
    }
    void Offer(const uint32_t id VARIABLE_IS_NOT_USED, Container&& descriptors VARIABLE_IS_NOT_USED)
    {
    }

private:
    uint32_t _id;
    Graphics::ServerBufferType<4>* _buffer;
    Callback _callback;
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
        printf("Test passing file descriptors and information over a process boundary.\n");
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
        Core::ProxyType<Test::ServerBuffer> serverBuffer;
        Core::ProxyType<Exchange::IGraphicsBuffer> buffer;

        if (server == true) {
            serverBuffer = Test::ServerBuffer::Create(_T("TestThing"), 1024, 1080, Exchange::IGraphicsBuffer::TYPE_RAW);
            buffer = Core::ProxyType<Exchange::IGraphicsBuffer>(serverBuffer);
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
                if (buffer.IsValid() == false) {
                    Core::PrivilegedRequest::Container descriptors;
                    printf("Get the buffer interface, Sesame open:\n");
                    if (bridge.Request(1000, string(Test::bridgeConnector), bufferId, descriptors) == Core::ERROR_NONE) {
                        printf("Got the buffer on client side.\n");
                        buffer = Test::ClientBuffer::Create(descriptors);
                    } else {
                        printf("Failed to get the buffer.\n");
                    }
                } else if (server == false) {
                    printf("Where done, we put it all in, close the vault, Sesame close:\n");
                    buffer.Release();
                }
                break;
            case 'I':
                if (buffer.IsValid() == false) {
                    printf("There are no buffers\n");
                } else {
                    printf("Buffer info:\n");
                    printf("==========================================\n");
                    printf("Width:  %d\n", buffer->Width());
                    printf("Height: %d\n", buffer->Height());
                    printf("Format: %d\n", buffer->Format());
                    printf("Type: %d\n", buffer->Type());
                    Core::ProxyType<Test::ServerBuffer> info = Core::ProxyType<Test::ServerBuffer>(buffer);
                    if (info != nullptr) {
                        printf("Dirty:  %s\n", info->IsDirty() ? _T("true") : _T("false"));
                    }
                }
                break;
            case 'W':
                if (buffer == nullptr) {
                    printf("There are no buffers\n");
                } else {
                    Exchange::IGraphicsBuffer::IIterator* planes = buffer->Acquire(10);

                    if (planes != nullptr) {
                        printf("Iterating ove the planes to write:\n");
                        while (planes->Next() == true) {
                            int fd = planes->Descriptor();
                            printf("Writing to [%d]:\n", fd);
PUSH_WARNING(DISABLE_WARNING_UNUSED_RESULT)
                            ::write(fd, "Hello World !!!\n", 16);
POP_WARNING()
                            ::fsync(fd);
                        }
                    }

                    buffer->Relinquish();

                    if (server == false) {
                        Core::ProxyType<Graphics::ClientBufferType<4> > info = Core::ProxyType<Graphics::ClientBufferType<4> >(buffer);
                        if (info != nullptr) {
                            printf("Request to render.\n");
                            info->RequestRender();
                        }
                    } else {
                        Core::ProxyType<Graphics::ServerBufferType<4> > info = Core::ProxyType<Graphics::ServerBufferType<4> >(buffer);
                        if (info != nullptr) {
                            printf("Render request handled.\n");
                            info->Rendered();
                            info->Published();
                        }
                    }
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
