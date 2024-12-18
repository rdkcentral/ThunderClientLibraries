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

#ifndef MODULE_NAME
#define MODULE_NAME Compositor_BufferType
#endif

#include <core/core.h>
#include <privilegedrequest/PrivilegedRequest.h>

#include <interfaces/ICompositionBuffer.h>

#include <sys/eventfd.h>
#include <sys/mman.h>

namespace Thunder {

namespace Compositor {

    class EXTERNAL Buffer : public Exchange::ICompositionBuffer, public Core::IResource {
    public:
        static constexpr uint8_t MaxPlanes = 4;

    private:
        // We need some shared space for data to exchange, and to create a lock..
        class EXTERNAL SharedStorage {
        private:
            struct PlaneStorage {
                uint32_t _stride;
                uint32_t _offset;
            };

        public:
            // Do not initialize members for now, this constructor is called after a mmap in the
            // placement new operator above. Initializing them now will reset the original values
            // of the buffer metadata.
            SharedStorage() {};

            void* operator new(size_t stAllocateBlock, int fd)
            {
                void* result = ::mmap(nullptr, stAllocateBlock, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                return (result != MAP_FAILED ? result : nullptr);
            }
            // Somehow Purify gets lost if we do not delete it, overide the delete operator
            void operator delete(void* stAllocateBlock)
            {
                ::munmap(stAllocateBlock, sizeof(struct SharedStorage));
            }

        public:
            SharedStorage(SharedStorage&&) = delete;
            SharedStorage(const SharedStorage&) = delete;
            SharedStorage& operator=(SharedStorage&&) = delete;
            SharedStorage& operator=(const SharedStorage&) = delete;

            SharedStorage(const uint32_t id, const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::ICompositionBuffer::DataType type)
                : _id(id)
                , _width(width)
                , _height(height)
                , _format(format)
                , _modifier(modifier)
                , _type(type)
                , _count(0)
                , _requestRender()
            {
                if (::pthread_mutex_init(&_mutex, nullptr) != 0) {
                    // That will be the day, if this fails...
                    ASSERT(false);
                }
            }
            ~SharedStorage()
            {
                #ifdef __WINDOWS__
                ::CloseHandle(&(_mutex));
                #else
                ::pthread_mutex_destroy(&(_mutex));
                #endif
            }

        public:
            uint32_t Id() const {
                return (_id);
            }
            uint8_t Planes() const {
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
            uint32_t Modifier() const
            {
                return (_modifier);
            }
            uint32_t Stride(const uint8_t index) const
            { // Bytes per row for a plane [(bit-per-pixel/8) * width]
                ASSERT (index < _count)
                return (_planes[index]._stride);
            }
            uint32_t Offset(const uint8_t index) const
            { // Offset of the plane from where the pixel data starts in the buffer.
                ASSERT (index < _count)
                return (_planes[index]._offset);
            }
            void Add(const uint32_t stride, const uint32_t offset)
            {
                ASSERT (_count < (sizeof(_planes)/sizeof(PlaneStorage)))
                _planes[_count]._stride = stride;
                _planes[_count]._offset = offset;
                _count++;
            }
            bool IsRenderRequested()
            {
                return (_requestRender);
            }
            void RequestRender()
            {
                _requestRender = true;
            }
            Exchange::ICompositionBuffer::DataType Type() const
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

        private:
            uint32_t _id;
            uint32_t _width;
            uint32_t _height;
            uint32_t _format;
            uint64_t _modifier;
            Exchange::ICompositionBuffer::DataType _type;
            std::atomic<bool> _requestRender;
            #ifdef __WINDOWS__
            CRITICAL_SECTION _mutex;
            #else
            pthread_mutex_t _mutex;
            #endif
            // This might flutuate between the different implementations
            // although the shared storage space might be shared so
            // always keep this at the end of the data set..
            uint8_t  _count;
            PlaneStorage _planes[MaxPlanes];
        };

        class EXTERNAL Iterator : public Exchange::ICompositionBuffer::IIterator {
        public:
            Iterator() = delete;
            Iterator(Iterator&&) = delete;
            Iterator(const Iterator&) = delete;
            Iterator& operator=(Iterator&&) = delete;
            Iterator& operator=(const Iterator&) = delete;

            Iterator(Buffer& parent)
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
            Buffer& _parent;
            uint8_t _position;
        };

    protected:
        Buffer() 
            : _producedFd(-1)
            , _consumedFd(-1)
            , _iterator(*this)
            , _virtualFd(-1)
            , _storage(nullptr) {
        }

    public:
        /***
         * We need a 64bit according: https://github.com/torvalds/linux/blob/v6.1/fs/eventfd.c#L275
         */
        using EventFrame = uint64_t;

        Buffer(Buffer&&) = delete;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(const uint32_t id, const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::ICompositionBuffer::DataType type)
            : _producedFd(-1)
            , _consumedFd(-1)
            , _iterator(*this)
            , _virtualFd(-1)
            , _storage(nullptr)
        {
            string definition = _T("NotifiableBuffer") + Core::NumberType<uint32_t>(id).Text();
            _virtualFd = ::memfd_create(definition.c_str(), MFD_ALLOW_SEALING | MFD_CLOEXEC);
            if (_virtualFd != -1) {
                int length = sizeof(struct SharedStorage);

                /* Size the file as specified by our struct. */
                if (::ftruncate(_virtualFd, length) != -1) {
                    /* map that file to a memory area we can directly access as a memory mapped file */
                    _storage = new (_virtualFd) SharedStorage(id, width, height, format, modifier, type);
                    if (_storage != nullptr) {
                        _producedFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
                        _consumedFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
                    }
                }
            }
        }
        Buffer(Core::PrivilegedRequest::Container& descriptors)
            : _producedFd(-1)
            , _consumedFd(-1)
            , _iterator(*this)
            , _virtualFd(-1)
            , _storage(nullptr)
        {
            Load(descriptors);
        }
        ~Buffer() override
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
        // Implementation of Exchange::ICompositionBuffer
        // -----------------------------------------------------------------
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
        uint32_t Identifier() const override
        {
            ASSERT(_storage != nullptr);
            return (_storage->Id());
        }
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
        Exchange::ICompositionBuffer::DataType Type() const override
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

                _storage = new (_virtualFd) SharedStorage();
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
            ASSERT(index < (sizeof(_descriptors)/ sizeof(int)));
            _descriptors[index] = ::dup(fd);
            _storage->Add(stride, offset);
        }
        int Producer() const {
            return (_producedFd);
        }
        int Consumer() const {
            return (_consumedFd);
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
        int Descriptor(const uint8_t index) const {
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
        SharedStorage* _storage;

        int _descriptors[MaxPlanes];
    };

    class EXTERNAL ClientBuffer : public Buffer {
    public:
        ClientBuffer(ClientBuffer&&) = delete;
        ClientBuffer(const ClientBuffer&) = delete;
        ClientBuffer& operator=(ClientBuffer&&) = delete;
        ClientBuffer& operator=(const ClientBuffer&) = delete;

        ClientBuffer() : Buffer() { }
        ~ClientBuffer() override = default;

    public:
        void Load(Core::PrivilegedRequest::Container& descriptors) {
            Buffer::Load(descriptors);
        }

        //
        // Implementation of Core::IResource
        // -----------------------------------------------------------------
        Core::IResource::handle Descriptor() const override
        {
            return (Buffer::Consumer());
        }
        uint16_t Events() override
        {
            return (POLLIN);
        }
        void Handle(const uint16_t events) override
        {
            typename Buffer::EventFrame value;

            if (((events & POLLIN) != 0) && (::read(Buffer::Consumer(), &value, sizeof(value)) == sizeof(value))) {
                Action();
            }
        }

        //
        // Implementation of Exchange::ICompositionBuffer
        // -----------------------------------------------------------------
        uint32_t Published() override
        {
            typename Buffer::EventFrame value = 1;
            size_t result = ::write(Buffer::Producer(), &value, sizeof(value));
            return (result != sizeof(value) ? Core::ERROR_ILLEGAL_STATE : Core::ERROR_NONE);
        }
        void Action () override = 0;
    };

    class EXTERNAL CompositorBuffer : public Buffer {
    public:
        CompositorBuffer() = delete;
        CompositorBuffer(CompositorBuffer&&) = delete;
        CompositorBuffer(const CompositorBuffer&) = delete;
        CompositorBuffer& operator=(CompositorBuffer&&) = delete;
        CompositorBuffer& operator=(const CompositorBuffer&) = delete;

        CompositorBuffer(const uint32_t id, const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::ICompositionBuffer::DataType type)
            : Buffer(id, width, height, format, modifier, type) {
        }
        ~CompositorBuffer() override = default;

    public:
        //
        // Implementation of Core::IResource
        // -----------------------------------------------------------------
        Core::IResource::handle Descriptor() const override
        {
            return (Buffer::Producer());
        }
        uint16_t Events() override
        {
            return (POLLIN);
        }
        void Handle(const uint16_t events) override
        {
            typename Buffer::EventFrame value;

            if (((events & POLLIN) != 0) && (::read(Buffer::Producer(), &value, sizeof(value)) == sizeof(value))) {
                Action();
            }
        }

        //
        // Implementation of Exchange::ICompositionBuffer
        // -----------------------------------------------------------------
        uint32_t Published() override
        {
            typename Buffer::EventFrame value = 1;
            size_t result = ::write(Buffer::Consumer(), &value, sizeof(value));
            return (result != sizeof(value) ? Core::ERROR_ILLEGAL_STATE : Core::ERROR_NONE);
        }
        void Action () override = 0;
    };
 
}
}
