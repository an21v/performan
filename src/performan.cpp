#include "../include/performan.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace Performan {

    ////////////////////////////////// Assertion //////////////////////////////////

    void PerformanSetAssertFunction(void(*function)(const char*, const char*, const char*, int))
    {
        PerformanAssertFunction = function;
    }

    void PerformanDefaultAssertHandler(const char* condition, const char* function, const char* file, int line)
    {
#if defined( _MSC_VER )
        __debugbreak();
#endif
    }

    void (*PerformanAssertFunction)(const char*, const char*, const char* file, int line) = PerformanDefaultAssertHandler;

    ////////////////////////////////// Allocator //////////////////////////////////

    void* DefaultAllocator::Allocate(std::size_t size)
    {
        return malloc(size);
    }

    void DefaultAllocator::Free(void* ptr)
    {
        free(ptr);
    }

    DefaultAllocator& GetDefaultAllocator()
    {
        static DefaultAllocator allocator;
        return allocator;
    }

    ////////////////////////////////// Profiler //////////////////////////////////

    void Profiler::CreateInstance()
    {
        PERFORMAN_ASSERT(_instance == nullptr);
        _instance = PERFORMAN_NEW(GetDefaultAllocator(), Profiler); // Pass user provided allocator
    }

    void Profiler::DestroyInstance()
    {
        PERFORMAN_ASSERT(_instance != nullptr);
        PERFORMAN_DELETE(GetDefaultAllocator(), Profiler, _instance); // Pass user provided allocator
    }

    Profiler* Profiler::GetInstance()
    {
        PERFORMAN_ASSERT(_instance != nullptr);
        return _instance;
    }

    void Profiler::SetAllocator(Allocator* allocator)
    {
        _allocator = allocator;
    }

    void Profiler::StopCapture()
    {
        WriteStream wStream(_allocator);

        if (_saveFct == nullptr)
        {
            return;
        }

        {
            std::scoped_lock lock(_threadsMtx);
            for (SoftPtr<Thread> thread : _threads)
            {
                thread->Serialize(wStream);
            }
        }

        uint32_t size = static_cast<uint32_t>(wStream.Offset());
        _saveFct(wStream.Data(), size);
    }

    Allocator* Profiler::GetAllocator() const
    {
        if (_allocator) {
            return _allocator;
        }

        return &(GetDefaultAllocator());
    }

    SoftPtr<Thread> Profiler::AddThread(const char* name)
    {
        // Dynamically allocate thread
        Thread* th = PERFORMAN_NEW(*_allocator, Thread, name);

        {
            std::scoped_lock lock(_threadsMtx);
            _threads.push_back(th);
        }

        return { th };
    }

    void Profiler::RemoveThread(SoftPtr<Thread> thread)
    {
        std::scoped_lock lock(_threadsMtx);
        auto itFound = std::find_if(_threads.begin(), _threads.end(), [thread](const auto* other) { return other == thread._ptr; });

        if (itFound != _threads.end())
        {
            _threads.erase(itFound);
        }
    }

    Profiler::~Profiler()
    {
        for (auto* thread : _threads) {
            delete thread;
        }
    }

    ////////////////////////////////// Serialization //////////////////////////////////

    Stream::Stream(Allocator* allocator)
        : _allocator(allocator)
    {
    }

    Stream::Stream(Allocator* allocator, uint8_t* buffer, size_t size)
        : _allocator(allocator)
        , _size(size)
    {
        _buffer = (uint8_t*)PERFORMAN_ALLOCATE(*_allocator, size);
        memcpy(_buffer, buffer, size);
    }

    Stream::~Stream()
    {
        Clear();
    }

    void Stream::Resize()
    {
        // 1. Reallocate new internal buffer
        constexpr size_t firstAllocDefaultSize = 1024; // 1024 bytes (1KB)

        size_t allocSize = _size > 0 ? _size * 2 : firstAllocDefaultSize;
        uint8_t* buf = (uint8_t*)PERFORMAN_ALLOCATE(*_allocator, allocSize);

        // 2. Copy previous buffer data
        if (_buffer) {
            memcpy(buf, _buffer, _size);
        }

        _size = allocSize;

        // 3. Swap internal buffer & delete previous
        uint8_t* prevBuf = _buffer;
        _buffer = buf;

        delete[] prevBuf;
    }

    void Stream::Clear()
    {
        _size = 0;
        _offset = 0;

        delete[] _buffer;
        _buffer = nullptr;
    }

    void WriteStream::SerializeBytes(void* value, size_t size)
    {
        if (_size - _offset < size) {
            Resize();
        }
        memcpy(&_buffer[_offset], value, size);
        _offset += size;
    }

    void ReadStream::SerializeBytes(void* value, size_t size)
    {
        memcpy(value, &_buffer[_offset], size);
        _offset += size;
    }
}
