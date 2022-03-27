#pragma once

#include <core/core.h>
#include <messaging/messaging.h>

#include <iomanip>
#include <iostream>

namespace WPEFramework {
namespace Messaging {
    class EXTERNAL LocalTracer {
    public:
        struct EXTERNAL ICallback {
            virtual void Output(const Core::Messaging::Information& info, const Core::Messaging::IEvent* message) = 0;
        };

    private:
        std::string DispatcherIdentifier()
        {
            string result;
            Core::SystemInfo::GetEnvironment(Core::Messaging::MessageUnit::MESSAGE_DISPACTHER_IDENTIFIER_ENV, result);
            return result;
        }

        std::string DispatcherBasePath()
        {
            string result;
            Core::SystemInfo::GetEnvironment(Core::Messaging::MessageUnit::MESSAGE_DISPATCHER_PATH_ENV, result);
            return result;
        }

    private:
        class WorkerThread : private Core::Thread {
        public:
            WorkerThread(LocalTracer& parent)
                : Core::Thread()
                , _parent(parent)
            {
            }
            void Start()
            {
                Thread::Run();
            }
            void Stop()
            {
                Thread::Block();
            }

        private:
            uint32_t Worker() override
            {
                if (Thread::IsRunning()) {
                    _parent.Dispatch();
                }

                return Core::infinite;
            }

            LocalTracer& _parent;
        };

    public:
        LocalTracer(const LocalTracer&) = delete;
        LocalTracer& operator=(const LocalTracer&) = delete;

        ~LocalTracer()
        {
            _worker.Stop();

            _client.SkipWaiting();
            _client.ClearInstances();
        }

        void Close()
        {
            Core::Messaging::MessageUnit::Instance().Close();

            Core::Directory(_path.c_str()).Destroy();

            LocalTracer*& singleton = LockSingleton(true);

            singleton = nullptr;

            LockSingleton(false);

            delete this;
        }

        static LocalTracer& Open()
        {
            static Core::CriticalSection lock;

            LocalTracer*& singleton = LockSingleton(true);

            if (singleton == nullptr) {

                char path[25];
                strcpy(path, "/tmp/localtracer.XXXXXX");
                mktemp(path);

                Core::Directory(path).CreatePath();

                Core::Messaging::MessageUnit::Instance().IsBackground(false);
                Core::Messaging::MessageUnit::Instance().Open(path);

                singleton = new LocalTracer(path);
            }

            LockSingleton(false);

            return *singleton;
        }

    private:
        static LocalTracer*& LockSingleton(const bool lock)
        {
            static LocalTracer* singleton = nullptr;

            static Core::CriticalSection s_lock;

            (lock == true) ? s_lock.Lock() : s_lock.Unlock();

            return (singleton);
        }

        LocalTracer(const string& path)
            : _adminLock()
            , _path(path)
            , _worker(*this)
            , _client(DispatcherIdentifier(), DispatcherBasePath())
            , _factory()
            , _callback(nullptr)
        {
            _client.AddInstance(0);
            _client.AddFactory(Core::Messaging::MetaData::MessageType::TRACING, &_factory);

            _worker.Start();

            // check if data is already available
            _client.SkipWaiting();
        }

    public:
        uint32_t EnableMessage(const std::string& moduleName, const std::string& categoryName, const bool enable)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            Core::Messaging::MetaData metaData(Core::Messaging::MetaData::MessageType::TRACING, categoryName, moduleName);

            _client.Enable(metaData, enable);

            return Core::ERROR_NONE;
        }

        void Dispatch()
        {
            _client.WaitForUpdates(Core::infinite);
            _client.PopMessagesAndCall([this](const Core::Messaging::Information& info, const Core::ProxyType<Core::Messaging::IEvent>& message) {
                Output(info, message.Origin());
            });
        }

        void Callback(ICallback* callback)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            ASSERT((_callback != nullptr) ^ (callback != nullptr));

            _callback = callback;
        }

    private:
        void Output(const Core::Messaging::Information& info, const Core::Messaging::IEvent* message)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            if (_callback != nullptr) {
                _callback->Output(info, message);
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        const string _path;

        WorkerThread _worker;
        MessageClient _client;
        TraceFactory _factory;

        ICallback* _callback;
    };

    class ConsolePrinter : public Messaging::LocalTracer::ICallback {
    private:
        class IosFlagSaver {
        public:
            explicit IosFlagSaver(std::ostream& _ios)
                : ios(_ios)
                , f(_ios.flags())
            {
            }
            ~IosFlagSaver()
            {
                ios.flags(f);
            }

            IosFlagSaver(const IosFlagSaver&) = delete;
            IosFlagSaver& operator=(const IosFlagSaver&) = delete;

        private:
            std::ostream& ios;
            std::ios::fmtflags f;
        };

    public:
        ConsolePrinter(const ConsolePrinter&) = delete;
        ConsolePrinter& operator=(const ConsolePrinter&) = delete;

        ConsolePrinter(const bool abbreviated)
            : _abbreviated(abbreviated)
        {
        }
        virtual ~ConsolePrinter() = default;

        void Output(const Core::Messaging::Information& info, const Core::Messaging::IEvent* message) override
        {
            IosFlagSaver saveUs(std::cout);

            std::ostringstream output;
            std::string deserializedMessage;

            output.str("");
            output.clear();

            message->ToString(deserializedMessage);

            Core::Time now(info.TimeStamp());

            if (_abbreviated == true) {
                std::string time(now.ToTimeOnly(true));
                output << '[' << time.c_str() << ']'
                       << '[' << info.MessageMetaData().Module() << "]"
                       << '[' << info.MessageMetaData().Category() << "]: "
                       << deserializedMessage << std::endl;
            } else {
                std::string time(now.ToRFC1123(true));
                output << '[' << time.c_str() << "]:[" << Core::FileNameOnly(info.FileName().c_str()) << ':' << info.LineNumber() << "] "
                       << info.MessageMetaData().Category() << ": " << deserializedMessage << std::endl;
            }

            std::cout << output.str() << std::flush;
        }

    private:
        bool _abbreviated;
    };
}
}