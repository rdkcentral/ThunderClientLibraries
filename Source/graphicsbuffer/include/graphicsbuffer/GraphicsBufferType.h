/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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
#pragma once

#include <core/core.h>
#include <privilegedrequest/PrivilegedRequest.h>
#include <interfaces/IGraphicsBuffer.h>

#include <sys/eventfd.h>
#include <sys/mman.h>

namespace Thunder {

namespace Graphics {

    template <const uint8_t PLANES>
    class LocalBufferType : public Exchange::IGraphicsBuffer {
    private:
        class Iterator : public Exchange::IGraphicsBuffer::IIterator {
        public:
            Iterator() = delete;
            Iterator(Iterator&&) = delete;
            Iterator(const Iterator&) = delete;
            Iterator& operator=(Iterator&&) = delete;
            Iterator& operator=(const Iterator&) = delete;

            Iterator(LocalBufferType<PLANES>& parent)
                : _parent(parent)
            {
                Reset();
            }
            ~Iterator() override = default;

        public:
            bool IsValid() const override
            {
                return ((_position > 0) && (_position <= _parent.Planes()));
            }
            void Reset() override
            {
                _position = 0;
            }
            bool Next() override
            {
                if (_position <= _parent.Planes()) {
                    _position++;
                }
                return (IsValid());
            }
            int Descriptor() const override
            { // Access to the actual data.
                ASSERT(IsValid() == true);
                return (_parent.Descriptor(_position - 1));
            }
            uint32_t Stride() const override
            { // Bytes per row for a plane [(bit-per-pixel/8) * width]
                ASSERT(IsValid() == true);
                return (_parent.Stride(_position - 1));
            }
            uint32_t Offset() const override
            { // Offset of the plane from where the pixel data starts in the buffer.
                ASSERT(IsValid() == true);
                return (_parent.Offset(_position - 1));
            }

        private:
            LocalBufferType<PLANES>& _parent;
            uint8_t _position;
        };

    public:
        LocalBufferType(LocalBufferType<PLANES>&&) = delete;
        LocalBufferType(const LocalBufferType<PLANES>&) = delete;
        LocalBufferType<PLANES>& operator=(LocalBufferType<PLANES>&&) = delete;
        LocalBufferType<PLANES>& operator=(const LocalBufferType<PLANES>&) = delete;

        LocalBufferType(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::IGraphicsBuffer::DataType type)
            : _lock(false)
            , _width(width)
            , _height(height)
            , _format(format)
            , _modifier(modifier)
            , _type(type)
            , _count(0)
            , _iterator(*this)
        {
        }
        ~LocalBufferType() override
        {
            for (uint8_t index = 0; index < _count; index++) {
                ::close(_planes[index]._fd);
            }
        }

    public:
        //
        // Implementation of Exchange::ILocalBufferType
        // -----------------------------------------------------------------
        // Wait time in milliseconds.
        IIterator* Acquire(const uint32_t waitTimeInMs) override
        {
            // Access to the buffer planes.
            IIterator* result = nullptr;
            if (_lock.Lock(waitTimeInMs) == Core::ERROR_NONE) {
                _iterator.Reset();
                result = &_iterator;
            }
            return (result);
        }
        void Relinquish() override
        {
            _lock.Unlock();
        }
        uint32_t Width() const override
        {
            return (_width);
        }
        uint32_t Height() const override
        {
            return (_height);
        }
        uint32_t Format() const override
        {
            return (_format);
        }
        uint64_t Modifier() const override
        {
            return (_modifier);
        }
        Exchange::IGraphicsBuffer::DataType Type() const override
        {
            return (_type);
        }
        uint8_t Planes() const
        {
            return (_count);
        }

    protected:
        void Add(int fd, const uint32_t stride, const uint32_t offset)
        {
            ASSERT(fd > 0);
            ASSERT(_count < (sizeof(_planes) / sizeof(struct PlaneStorage)));
            _planes[_count]._fd = ::dup(fd);
            _planes[_count]._offset = offset;
            _planes[_count]._stride = stride;
            ++_count;
        }

        void Modifier(const uint64_t modifier)
        {
            _modifier = modifier;
        }

    private:
        uint32_t Stride(const uint8_t index) const
        {
            return (_planes[index]._stride);
        }
        uint32_t Offset(const uint8_t index) const
        {
            return (_planes[index]._offset);
        }
        int Descriptor(const uint8_t index) const
        {
            return (_planes[index]._fd);
        }

    private:
        struct PlaneStorage {
            uint32_t _stride;
            uint32_t _offset;
            int _fd;
        };

        Core::BinarySemaphore _lock;
        uint32_t _width;
        uint32_t _height;
        uint32_t _format;
        uint64_t _modifier;
        Exchange::IGraphicsBuffer::DataType _type;
        uint8_t _count;
        Iterator _iterator;
        PlaneStorage _planes[PLANES];
    };

    template <const uint8_t PLANES>
    class SharedBufferType : public Exchange::IGraphicsBuffer, public Core::IResource {
    private:
        // We need some shared space for data to exchange, and to create a lock..
        template <const uint8_t LAYERS>
        class SharedStorageType {
        private:
            struct PlaneStorage {
                uint32_t _stride;
                uint32_t _offset;
            };
            enum mode : uint8_t {
                IDLE,
                REQUEST,
                RENDERED,
                PUBLISHED,
                DESTROYED
            };

        public:
            // Do not initialize members for now, this constructor is called after a mmap in the
            // placement new operator above. Initializing them now will reset the original values
            // of the buffer metadata.
            SharedStorageType() { };

            void* operator new(size_t stAllocateBlock, int fd)
            {
                void* result = ::mmap(nullptr, stAllocateBlock, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                return (result != MAP_FAILED ? result : nullptr);
            }
            // Somehow Purify gets lost if we do not delete it, override the delete operator
            void operator delete(void* stAllocateBlock)
            {
                ::munmap(stAllocateBlock, sizeof(SharedStorageType<LAYERS>));
            }

        public:
            SharedStorageType(SharedStorageType<LAYERS>&&) = delete;
            SharedStorageType(const SharedStorageType<LAYERS>&) = delete;
            SharedStorageType<LAYERS>& operator=(SharedStorageType<LAYERS>&&) = delete;
            SharedStorageType<LAYERS>& operator=(const SharedStorageType<LAYERS>&) = delete;

            SharedStorageType(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::IGraphicsBuffer::DataType type)
                : _width(width)
                , _height(height)
                , _format(format)
                , _modifier(modifier)
                , _type(type)
                , _command(mode::IDLE)
                , _count(0)
            {
                if (::pthread_mutex_init(&_mutex, nullptr) != 0) {
                    // That will be the day, if this fails...
                    ASSERT(false);
                }
            }
            ~SharedStorageType()
            {
#ifdef __WINDOWS__
                ::CloseHandle(&(_mutex));
#else
                ::pthread_mutex_destroy(&(_mutex));
#endif
            }

        public:
            uint8_t Planes() const
            {
                return (_count);
            }
            uint32_t Width() const
            {
                return (_width);
            }
            uint32_t Height() const
            {
                return (_height);
            }
            uint32_t Format() const
            {
                return (_format);
            }
            uint64_t Modifier() const
            {
                return (_modifier);
            }
            uint32_t Stride(const uint8_t index) const
            { // Bytes per row for a plane [(bit-per-pixel/8) * width]
                ASSERT(index < _count);
                return (_planes[index]._stride);
            }
            uint32_t Offset(const uint8_t index) const
            { // Offset of the plane from where the pixel data starts in the buffer.
                ASSERT(index < _count);
                return (_planes[index]._offset);
            }
            void Add(const uint32_t stride, const uint32_t offset)
            {
                ASSERT(_count < (sizeof(_planes) / sizeof(PlaneStorage)));
                _planes[_count]._stride = stride;
                _planes[_count]._offset = offset;
                _count++;
            }
            bool Request()
            {
                bool result;
                mode set = mode::IDLE;
                if ((result = _command.compare_exchange_strong(set, mode::REQUEST)) == false) {
                    set = mode::REQUEST;
                    result = _command.compare_exchange_strong(set, mode::REQUEST);
                }
                return (result);
            }
            bool Rendered()
            {
                bool result;
                mode set = mode::IDLE;
                if ((result = _command.compare_exchange_strong(set, mode::RENDERED)) == false) {
                    set = mode::RENDERED;
                    if ((result = _command.compare_exchange_strong(set, mode::RENDERED)) == false) {
                        set = mode::PUBLISHED;
                        result = _command.compare_exchange_strong(set, mode::RENDERED);
                    }
                }
                return (result);
            }
            bool Published()
            {
                bool result;
                mode set = mode::IDLE;
                if ((result = _command.compare_exchange_strong(set, mode::PUBLISHED)) == false) {
                    set = mode::RENDERED;
                    if ((result = _command.compare_exchange_strong(set, mode::PUBLISHED)) == false) {
                        set = mode::PUBLISHED;
                        result = _command.compare_exchange_strong(set, mode::PUBLISHED);
                    }
                }
                return (result);
            }
            void Destroyed()
            {
                _command.store(mode::DESTROYED);
            }
            bool IsDestroyed() const
            {
                return (_command == mode::DESTROYED);
            }
            bool IsRequested() const
            {
                mode set = mode::REQUEST;
                return (_command.compare_exchange_strong(set, mode::IDLE));
            }
            bool IsRendered() const
            {
                mode set = mode::RENDERED;
                return (_command.compare_exchange_strong(set, mode::IDLE));
            }
            bool IsPublished() const
            {
                mode set = mode::PUBLISHED;
                return (_command.compare_exchange_strong(set, mode::IDLE));
            }
            Exchange::IGraphicsBuffer::DataType Type() const
            {
                return _type;
            }
            uint32_t Lock(uint32_t timeout)
            {
                timespec structTime;

#ifdef __WINDOWS__
                return (::WaitForSingleObjectEx(&_mutex, timeout, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#else
                clock_gettime(CLOCK_MONOTONIC, &structTime);
                structTime.tv_nsec += ((timeout % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
                structTime.tv_sec += (timeout / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
                structTime.tv_nsec = structTime.tv_nsec % 1000000000;
                int result = pthread_mutex_timedlock(&_mutex, &structTime);
                return (result == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#endif
            }
            uint32_t Unlock()
            {
#ifdef __WINDOWS__
                ::LeaveCriticalSection(&_mutex);
#else
                pthread_mutex_unlock(&_mutex);
#endif
                return (Core::ERROR_NONE);
            }

        protected:
            void Modifier(const uint64_t modifier)
            {
                _modifier = modifier;
            }

        private:
            uint32_t _width;
            uint32_t _height;
            uint32_t _format;
            uint64_t _modifier;
            Exchange::IGraphicsBuffer::DataType _type;
            mutable std::atomic<mode> _command;
#ifdef __WINDOWS__
            CRITICAL_SECTION _mutex;
#else
            pthread_mutex_t _mutex;
#endif
            // This might fluctuate between the different implementations
            // although the shared storage space might be shared so
            // always keep this at the end of the data set..
            uint8_t _count;
            PlaneStorage _planes[LAYERS];
        };

        class Iterator : public Exchange::IGraphicsBuffer::IIterator {
        public:
            Iterator() = delete;
            Iterator(Iterator&&) = delete;
            Iterator(const Iterator&) = delete;
            Iterator& operator=(Iterator&&) = delete;
            Iterator& operator=(const Iterator&) = delete;

            Iterator(SharedBufferType<PLANES>& parent)
                : _parent(parent)
            {
                Reset();
            }
            ~Iterator() override = default;

        public:
            bool IsValid() const override
            {
                return ((_position > 0) && (_position <= _parent.Planes()));
            }
            void Reset() override
            {
                _position = 0;
            }
            bool Next() override
            {
                if (_position <= _parent.Planes()) {
                    _position++;
                }
                return (IsValid());
            }
            int Descriptor() const override
            { // Access to the actual data.
                ASSERT(IsValid() == true);
                return (_parent.Descriptor(_position - 1));
            }
            uint32_t Stride() const override
            { // Bytes per row for a plane [(bit-per-pixel/8) * width]
                ASSERT(IsValid() == true);
                return (_parent.Stride(_position - 1));
            }
            uint32_t Offset() const override
            { // Offset of the plane from where the pixel data starts in the buffer.
                ASSERT(IsValid() == true);
                return (_parent.Offset(_position - 1));
            }

        private:
            SharedBufferType<PLANES>& _parent;
            uint8_t _position;
        };

    protected:
        SharedBufferType()
            : _iterator(*this)
            , _virtualFd(-1)
            , _producedFd(-1)
            , _consumedFd(-1)
            , _storage(nullptr)
        {
        }

    public:
        /***
         * We need a 64bit according: https://github.com/torvalds/linux/blob/v6.1/fs/eventfd.c#L275
         */
        using EventFrame = uint64_t;

        SharedBufferType(SharedBufferType<PLANES>&&) = delete;
        SharedBufferType(const SharedBufferType<PLANES>&) = delete;
        SharedBufferType<PLANES>& operator=(SharedBufferType<PLANES>&&) = delete;
        SharedBufferType<PLANES>& operator=(const SharedBufferType<PLANES>&) = delete;

        SharedBufferType(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::IGraphicsBuffer::DataType type)
            : _iterator(*this)
            , _virtualFd(-1)
            , _producedFd(-1)
            , _consumedFd(-1)
            , _storage(nullptr)
        {
            _virtualFd = ::memfd_create(_T("GraphicsBufferType"), MFD_ALLOW_SEALING | MFD_CLOEXEC);
            if (_virtualFd != -1) {
                int length = sizeof(struct SharedStorageType<PLANES>);

                /* Size the file as specified by our struct. */
                if (::ftruncate(_virtualFd, length) != -1) {
                    /* map that file to a memory area we can directly access as a memory mapped file */
                    _storage = new (_virtualFd) SharedStorageType<PLANES>(width, height, format, modifier, type);
                    if (_storage == nullptr) {
                        ::close(_virtualFd);
                        _virtualFd = -1;
                    } else {
                        _producedFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
                        _consumedFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
                    }
                }
            }
        }
        SharedBufferType(Core::PrivilegedRequest::Container& descriptors)
            : _iterator(*this)
            , _virtualFd(-1)
            , _producedFd(-1)
            , _consumedFd(-1)
            , _storage(nullptr)
        {
            Load(descriptors);
        }
        ~SharedBufferType() override
        {
            if (_producedFd != -1) {
                ::close(_producedFd);
                _producedFd = -1;

                ASSERT(_storage != nullptr);
            }
            if (_consumedFd != -1) {
                ::close(_consumedFd);
                _consumedFd = -1;

                ASSERT(_storage != nullptr);
            }
            // Close all the FileDescriptors handed over to us for the planes.
            for (uint8_t index = 0; index < _storage->Planes(); index++) {
                ::close(_descriptors[index]);
            }
            if (_storage != nullptr) {
                delete _storage;
                _storage = nullptr;
            }
            if (_virtualFd != -1) {
                ::close(_virtualFd);
                _virtualFd = -1;
            }
        }

    public:
        bool IsValid() const
        {
            return (_storage != nullptr);
        }
        uint8_t Descriptors(const uint8_t maxSize, int container[]) const
        {
            ASSERT(IsValid() == true);
            ASSERT(maxSize > 3);
            uint8_t result = 0;

            if (maxSize > 3) {
                container[0] = _virtualFd;
                container[1] = _producedFd;
                container[2] = _consumedFd;
                uint8_t count = std::min(_storage->Planes(), static_cast<uint8_t>(maxSize - 3));
                for (uint8_t index = 0; index < count; index++) {
                    container[index + 3] = _descriptors[index];
                }
                result = 3 + count;
            }
            return (result);
        }

        //
        // Implementation of Exchange::IGraphicsBuffer
        // -----------------------------------------------------------------

        PUSH_WARNING(DISABLE_WARNING_OVERLOADED_VIRTUALS)
        // Wait time in milliseconds.
        IIterator* Acquire(const uint32_t waitTimeInMs) override
        {
            // Access to the buffer planes.
            IIterator* result = nullptr;
            if (_storage->Lock(waitTimeInMs) == Core::ERROR_NONE) {
                _iterator.Reset();
                result = &_iterator;
            }
            return (result);
        }
        void Relinquish() override
        {
            _storage->Unlock();
        }
        POP_WARNING()

        uint32_t Width() const override
        { // Width of the allocated buffer in pixels
            ASSERT(_storage != nullptr);
            return (_storage->Width());
        }
        uint32_t Height() const override
        { // Height of the allocated buffer in pixels
            ASSERT(_storage != nullptr);
            return (_storage->Height());
        }
        uint32_t Format() const override
        { // Layout of a pixel according the fourcc format
            ASSERT(_storage != nullptr);
            return (_storage->Format());
        }
        uint64_t Modifier() const override
        { // Pixel arrangement in the buffer, used to optimize for hardware
            ASSERT(_storage != nullptr);
            return (_storage->Modifier());
        }
        Exchange::IGraphicsBuffer::DataType Type() const override
        {
            ASSERT(_storage != nullptr);
            return (_storage->Type());
        }
        uint8_t Planes() const
        {
            return (_storage->Planes());
        }

    protected:
        void Load(Core::PrivilegedRequest::Container& descriptors)
        {
            ASSERT(descriptors.size() >= 3);

            if (descriptors.size() >= 3) {
                Core::PrivilegedRequest::Container::iterator index(descriptors.begin());

                _virtualFd = index->Move();
                index++;

                ASSERT(_virtualFd != -1);

                _storage = new (_virtualFd) SharedStorageType<PLANES>();
                if (_storage == nullptr) {
                    ::close(_virtualFd);
                } else {
                    _producedFd = index->Move();
                    index++;
                    _consumedFd = index->Move();
                    index++;

                    ASSERT(_producedFd != -1);
                    ASSERT(_consumedFd != -1);

                    uint8_t position = 0;

                    while ((index != descriptors.end()) && (position < _storage->Planes())) {
                        ASSERT(position < (sizeof(_descriptors) / sizeof(int)));
                        _descriptors[position] = index->Move();
                        index++;
                        position++;
                    }
                }
            }
        }
        void Add(int fd, const uint32_t stride, const uint32_t offset)
        {
            uint8_t index = _storage->Planes();
            ASSERT(fd > 0);
            ASSERT(index < (sizeof(_descriptors) / sizeof(int)));
            _descriptors[index] = ::dup(fd);
            _storage->Add(stride, offset);
        }
        void Planes(Core::PrivilegedRequest::Descriptor descriptors[], const uint8_t size VARIABLE_IS_NOT_USED)
        {
            ASSERT(size == _storage->Planes());

            for (uint8_t index = 0; index < _storage->Planes(); index++) {
                _descriptors[index] = descriptors[index].Move();
            }
        }
        int Producer() const
        {
            return (_producedFd);
        }
        int Consumer() const
        {
            return (_consumedFd);
        }
        void Destroyed()
        {
            _storage->Destroyed();
        }
        bool Request()
        {
            return (_storage->Request());
        }
        bool Rendered()
        {
            return (_storage->Rendered());
        }
        bool Published()
        {
            return (_storage->Published());
        }
        bool IsDestroyed() const
        {
            return (_storage->IsDestroyed());
        }
        bool IsRequested() const
        {
            return (_storage->IsRequested());
        }
        bool IsRendered() const
        {
            return (_storage->IsRendered());
        }
        bool IsPublished() const
        {
            return (_storage->IsPublished());
        }

    private:
        uint32_t Stride(const uint8_t index) const
        { // Bytes per row for a plane [(bit-per-pixel/8) * width]
            ASSERT(_storage != nullptr);
            return (_storage->Stride(index));
        }
        uint32_t Offset(const uint8_t index) const
        { // Offset of the plane from where the pixel data starts in the buffer.
            ASSERT(_storage != nullptr);
            return (_storage->Offset(index));
        }
        int Descriptor(const uint8_t index) const
        {
            return (_descriptors[index]);
        }

    private:
        Iterator _iterator;

        // We need a descriptor that is pointing to the virtual memory (shared memory)
        int _virtualFd;

        // We need a descriptor we can wait for to see if we need a render and to see if
        // the render is completed...
        int _producedFd;
        int _consumedFd;

        // From the virtual memory we can map the shared data to a memory area in "our" process.
        SharedStorageType<PLANES>* _storage;

        int _descriptors[PLANES];
    };

    template <const uint8_t PLANES>
    class ClientBufferType : public SharedBufferType<PLANES> {
    public:
        ClientBufferType(ClientBufferType<PLANES>&&) = delete;
        ClientBufferType(const ClientBufferType<PLANES>&) = delete;
        ClientBufferType<PLANES>& operator=(ClientBufferType<PLANES>&&) = delete;
        ClientBufferType<PLANES>& operator=(const ClientBufferType<PLANES>&) = delete;

        ClientBufferType()
            : SharedBufferType<PLANES>()
        {
        }

        ClientBufferType(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::IGraphicsBuffer::DataType type)
            : SharedBufferType<PLANES>(width, height, format, modifier, type)
        {
        }

        ~ClientBufferType() override
        {
            SharedBufferType<PLANES>::Destroyed();
        }

    public:
        void Load(Core::PrivilegedRequest::Container& descriptors)
        {
            SharedBufferType<PLANES>::Load(descriptors);
        }
        bool RequestRender()
        {
            bool requested = true;

            if (SharedBufferType<PLANES>::Request() == false) {
                // Might be that we just got a RENDERED event from the other side
                if (SharedBufferType<PLANES>::IsRendered() == true) {
                    // If so handle it..
                    Rendered();
                }
                // If it was not the Rendered event it must  have been the Published event
                else if (SharedBufferType<PLANES>::IsPublished() == true) {
                    Published();
                }

                // Potentially the code in the Handle method *might* have read
                // the first blocking reason (IsRendered()) and cleared that
                // state, than if the IsPublished() state was not yet handled
                // it might ocurr now before we do a second attempt to set the
                // request.. So potentially it might still fail once!
                if (SharedBufferType<PLANES>::Request() == false) {

                    // This ocurres if the IsRendered() was picked up by the Handle
                    // method in this class, the Publication occurred after we checked
                    // the IsPublished before we reached the second attempt to Request()
                    if (SharedBufferType<PLANES>::IsPublished() == true) {
                        Published();
                    }

                    // Now the request *MUST* succeed!
                    requested = SharedBufferType<PLANES>::Request();

                    ASSERT((requested == true) || (SharedBufferType<PLANES>::IsDestroyed() == true));
                }
            }
            if (requested == true) {
                typename SharedBufferType<PLANES>::EventFrame value = 1;
                requested = (::write(SharedBufferType<PLANES>::Producer(), &value, sizeof(value)) == sizeof(value));
            }

            return (requested);
        }

        //
        // Implementation of Core::IResource
        // -----------------------------------------------------------------
        Core::IResource::handle Descriptor() const override
        {
            return (SharedBufferType<PLANES>::Consumer());
        }
        uint16_t Events() override
        {
            return (POLLIN);
        }
        void Handle(const uint16_t events) override
        {
            typename SharedBufferType<PLANES>::EventFrame value;

            if (((events & POLLIN) != 0) && (::read(SharedBufferType<PLANES>::Consumer(), &value, sizeof(value)) == sizeof(value))) {
                if (SharedBufferType<PLANES>::IsRendered() == true) {
                    Rendered();
                } else if (SharedBufferType<PLANES>::IsPublished() == true) {
                    Published();
                }
            }
        }

        //
        // Methods to retrieve the status of the buffer on client side
        // ----------------------------------------------------------------
        virtual void Rendered() = 0;
        virtual void Published() = 0;
    };

    template <const uint8_t PLANES>
    class ServerBufferType : public SharedBufferType<PLANES> {
    public:
        ServerBufferType(ServerBufferType<PLANES>&&) = delete;
        ServerBufferType(const ServerBufferType<PLANES>&) = delete;
        ServerBufferType<PLANES>& operator=(ServerBufferType<PLANES>&&) = delete;
        ServerBufferType<PLANES>& operator=(const ServerBufferType<PLANES>&) = delete;

        ServerBufferType(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::IGraphicsBuffer::DataType type)
            : SharedBufferType<PLANES>(width, height, format, modifier, type)
        {
        }
        ServerBufferType(const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer)
            : SharedBufferType<PLANES>(buffer)
        {
        }

        ServerBufferType()
            : SharedBufferType<PLANES>()
        {
        }

        ~ServerBufferType() override
        {
            // If we go out of scope, no use for the client
            // to continue rendering. Let him know....
            SharedBufferType<PLANES>::Destroyed();
        }

    public:
        void Load(const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer)
        {
            SharedBufferType<PLANES>::Load(buffer);
        }

        void Load(Core::PrivilegedRequest::Container& descriptors)
        {
            SharedBufferType<PLANES>::Load(descriptors);
        }

        bool Rendered()
        {
            bool requested = true;

            if (SharedBufferType<PLANES>::Rendered() == false) {

                // Might be that we just got a REQUEST event from the other side
                if (SharedBufferType<PLANES>::IsRequested() == true) {
                    // If so handle it..
                    Request();
                }

                // Now the request *MUST* succeed!
                requested = SharedBufferType<PLANES>::Rendered();

                ASSERT((requested == true) || (SharedBufferType<PLANES>::IsDestroyed() == true));
            }

            if (requested == true) {
                typename SharedBufferType<PLANES>::EventFrame value = 1;
                requested = (::write(SharedBufferType<PLANES>::Consumer(), &value, sizeof(value)) == sizeof(value));
            }

            return (requested);
        }
        bool Published()
        {
            bool requested = true;

            if (SharedBufferType<PLANES>::Published() == false) {
                // Might be that we just got a REQUEST event from the other side
                if (SharedBufferType<PLANES>::IsRequested() == true) {
                    // If so handle it..
                    Request();
                }

                // Now the request *MUST* succeed!
                requested = SharedBufferType<PLANES>::Published();

                ASSERT((requested == true) || (SharedBufferType<PLANES>::IsDestroyed() == true));
            }

            if (requested == true) {
                typename SharedBufferType<PLANES>::EventFrame value = 1;
                requested = (::write(SharedBufferType<PLANES>::Consumer(), &value, sizeof(value)) == sizeof(value));
            }

            return (requested);
        }

        //
        // Implementation of Core::IResource
        // -----------------------------------------------------------------
        Core::IResource::handle Descriptor() const override
        {
            return (SharedBufferType<PLANES>::Producer());
        }
        uint16_t Events() override
        {
            return (POLLIN);
        }
        void Handle(const uint16_t events) override
        {
            typename SharedBufferType<PLANES>::EventFrame value;

            if (((events & POLLIN) != 0) && (::read(SharedBufferType<PLANES>::Producer(), &value, sizeof(value)) == sizeof(value))) {
                if (SharedBufferType<PLANES>::IsRequested() == true) {
                    Request();
                }
            }
        }

        //
        // Method called by the client to "Request" a buffer commit
        // ----------------------------------------------------------------
        virtual void Request() = 0;
    };
}
}
