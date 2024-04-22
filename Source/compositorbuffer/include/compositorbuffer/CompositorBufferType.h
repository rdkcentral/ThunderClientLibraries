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

namespace WPEFramework {

namespace Compositor {

    template <const uint8_t PLANES>
    class CompositorBufferType : public Exchange::ICompositionBuffer, public Core::IResource {
    private:
        /***
         * We need a 64bit according: https://github.com/torvalds/linux/blob/v6.1/fs/eventfd.c#L275
         */
        using EventFrame = uint64_t;

        // We need some shared space for data to exchange, and to create a lock..
        class SharedStorage {
        private:
            struct PlaneStorage {
                uint32_t _stride;
                uint32_t _offset;
            };

        public:
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
            SharedStorage& operator=(const SharedStorage&) = delete;

            // FIXME:
            // Do not initialize members for now, this constructor is called after a mmap in the
            // placement new operator above. Initializing them now will reset the original values
            // of the buffer metadata.
            SharedStorage() {};

            SharedStorage(const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::ICompositionBuffer::DataType type)
                : _width(width)
                , _height(height)
                , _format(format)
                , _modifier(modifier)
                , _type(type)
                , _dirty()
                , _copyOfDirty(false)
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
                return (_planes[index]._stride);
            }
            uint32_t Offset(const uint8_t index) const
            { // Offset of the plane from where the pixel data starts in the buffer.
                return (_planes[index]._offset);
            }
            void Add(uint8_t index, const uint32_t stride, const uint32_t offset)
            {
                _planes[index]._stride = stride;
                _planes[index]._offset = offset;
            }
            bool Dirty()
            {
                _copyOfDirty = false;
                return (_dirty.test_and_set() == false);
            }
            bool IsDirty()
            {
                return (_copyOfDirty);
            }
            void Set()
            {
                _dirty.clear();
                _copyOfDirty = true;
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
            uint32_t _width;
            uint32_t _height;
            uint32_t _format;
            uint64_t _modifier;
            Exchange::ICompositionBuffer::DataType _type;
            PlaneStorage _planes[PLANES];
#ifdef __WINDOWS__
            CRITICAL_SECTION _mutex;
#else
            pthread_mutex_t _mutex;
#endif
            std::atomic_flag _dirty;
            bool _copyOfDirty;
        };

        class Iterator : public Exchange::ICompositionBuffer::IIterator {
        private:
            class PlaneImplementation : public Exchange::ICompositionBuffer::IPlane {
            public:
                PlaneImplementation(PlaneImplementation&&) = delete;
                PlaneImplementation(const PlaneImplementation&) = delete;
                PlaneImplementation& operator=(const PlaneImplementation&) = delete;

                PlaneImplementation()
                    : _parent(nullptr)
                    , _index(-1)
                {
                }
                ~PlaneImplementation() override = default;

            public:
                void Define(CompositorBufferType<PLANES>& parent, const uint8_t index)
                {
                    _parent = &parent;
                    _index = index;
                }
                buffer_id Accessor() const override
                { // Access to the actual data.
                    ASSERT(_parent != nullptr);
                    return (_parent->Accessor(_index));
                }
                uint32_t Stride() const override
                { // Bytes per row for a plane [(bit-per-pixel/8) * width]
                    ASSERT(_parent != nullptr);
                    ASSERT(_parent->_storage != nullptr);
                    return (_parent->_storage->Stride(_index));
                }
                uint32_t Offset() const override
                { // Offset of the plane from where the pixel data starts in the buffer.
                    ASSERT(_parent != nullptr);
                    ASSERT(_parent->_storage != nullptr);
                    return (_parent->_storage->Offset(_index));
                }

            private:
                CompositorBufferType<PLANES>* _parent;
                uint8_t _index;
            };

        public:
            Iterator() = delete;
            Iterator(Iterator&&) = delete;
            Iterator(const Iterator&) = delete;
            Iterator& operator=(const Iterator&) = delete;

            Iterator(CompositorBufferType<PLANES>& parent)
                : _parent(parent)
            {
                // Fill our elements
                for (uint8_t index = 0; index < PLANES; index++) {
                    _planes[index].Define(_parent, index);
                }

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
            IPlane* Plane() override
            {
                ASSERT(IsValid() == true);
                return (&(_planes[_position - 1]));
            }

        private:
            CompositorBufferType<PLANES>& _parent;
            PlaneImplementation _planes[PLANES];
            uint8_t _position;
        };

    public:
        CompositorBufferType() = delete;
        CompositorBufferType(const CompositorBufferType<PLANES>&) = delete;
        CompositorBufferType<PLANES>& operator=(const CompositorBufferType<PLANES>&) = delete;

        CompositorBufferType(const string& callsign, const uint32_t id, const uint32_t width, const uint32_t height, const uint32_t format, const uint64_t modifier, const Exchange::ICompositionBuffer::DataType type)
            : _id(id)
            , _planeCount(0)
            , _iterator(*this)
            , _virtualFd(-1)
            , _eventFd(-1)
            , _storage(nullptr)
        {
            string definition = _T("NotifiableBuffer") + callsign;
            _virtualFd = ::memfd_create(definition.c_str(), MFD_ALLOW_SEALING | MFD_CLOEXEC);
            if (_virtualFd != -1) {
                int length = sizeof(struct SharedStorage);

                /* Size the file as specified by our struct. */
                if (::ftruncate(_virtualFd, length) != -1) {
                    /* map that file to a memory area we can directly access as a memory mapped file */
                    _storage = new (_virtualFd) SharedStorage(width, height, format, modifier, type);
                    if (_storage != nullptr) {
                        _eventFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
                    }
                }
            }
        }
        CompositorBufferType(const uint32_t id, Core::PrivilegedRequest::Container& descriptors)
            : _id(id)
            , _planeCount(0)
            , _iterator(*this)
            , _virtualFd(-1)
            , _eventFd(-1)
            , _storage(nullptr)
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
                    _eventFd = index->Move();
                    index++;

                    ASSERT(_eventFd != -1);

                    while ((index != descriptors.end()) && (_planeCount < PLANES)) {
                        _planes[_planeCount] = index->Move();
                        index++;
                        _planeCount++;
                    }
                }
            }
        }

        ~CompositorBufferType() override
        {
            if (_eventFd != -1) {
                ::close(_eventFd);
                _eventFd = -1;

                ASSERT(_storage != nullptr);
            }
            if (_storage != nullptr) {
                delete _storage;
                _storage = nullptr;
            }
            if (_virtualFd != -1) {
                ::close(_virtualFd);
                _virtualFd = -1;
            }

            // Close all the FileDescriptors handed over to us for the planes.
            for (uint8_t index = 0; index < _planeCount; index++) {
                ::close(_planes[index]);
            }
        }

    public:
        bool IsValid() const
        {
            return (_eventFd != -1);
        }
        uint8_t Descriptors(const uint8_t maxSize, int container[]) const
        {
            ASSERT(IsValid() == true);
            ASSERT(maxSize > 2);
            uint8_t result = 0;

            if (maxSize > 2) {
                container[0] = _virtualFd;
                container[1] = _eventFd;
                uint8_t count = std::min(_planeCount, static_cast<uint8_t>(maxSize - 2));
                for (uint8_t index = 0; (index < count); index++) {
                    container[index + 2] = _planes[index];
                }
                result = 2 + count;
            }
            return (result);
        }

        //
        // Implementation of Core::IResource
        // -----------------------------------------------------------------
        handle Descriptor() const override
        {
            return (_eventFd);
        }
        uint16_t Events() override
        {
            return (POLLIN);
        }
        void Handle(const uint16_t events) override
        {
            EventFrame value;

            if (((events & POLLIN) != 0) && (::read(_eventFd, &value, sizeof(EventFrame)) == sizeof(EventFrame))) {
                if (_storage->Dirty() == true) {
                    Render();
                }
            }
        }

        //
        // Implementation of Exchange::ICompositionBuffer
        // -----------------------------------------------------------------
        // Wait time in milliseconds.
        IIterator* Planes(const uint32_t waitTimeInMs) override
        { // Access to the buffer planes.
            if (Lock(waitTimeInMs) == Core::ERROR_NONE) {
                _iterator.Reset();
                return (&_iterator);
            }
            return (nullptr);
        }
        virtual uint32_t Completed(const bool dirty)
        {
            Unlock();
            if (dirty == true) {
                EventFrame value = 1;
                _storage->Set();
                size_t result = ::write(_eventFd, &value, sizeof(value));
                return (result != sizeof(value) ? Core::ERROR_ILLEGAL_STATE : Core::ERROR_NONE);
            }
            return (Core::ERROR_NONE);
        }

        uint32_t Identifier() const override
        {
            return (_id);
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
            return (_planeCount);
        }
        bool IsDirty() const
        {
            ASSERT(_storage != nullptr);
            return (_storage->IsDirty());
        }

    protected:
        void Add(int fd, const uint32_t stride, const uint32_t offset)
        {
            ASSERT(fd > 0);
            ASSERT((_planeCount + 1) <= PLANES);
            _storage->Add(_planeCount, stride, offset);
            _planes[_planeCount] = ::dup(fd);
            _planeCount++;
        }

    private:
        buffer_id Accessor(const uint8_t index) const
        { // Access to the actual data.
            ASSERT(index < _planeCount);
            return (_planes[index]);
        }
        uint32_t Lock(const uint32_t timeout)
        {
            return (_storage->Lock(timeout));
        }
        uint32_t Unlock()
        {
            return (_storage->Unlock());
        }

    private:
        uint32_t _id;
        uint8_t _planeCount;
        Iterator _iterator;

        // We need a descriptor that is pointing to the virtual memory (shared memory)
        int _virtualFd;

        // We need a descriptor we can wait for and that can be signalled
        int _eventFd;

        // From the virtual memory we can map the shared data to a memory area in "our" process.
        SharedStorage* _storage;

        int _planes[PLANES];
    };

}
}
