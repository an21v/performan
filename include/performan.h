// PERFORMAN.H

#ifndef PERFORMAN_H
#define PERFORMAN_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <ratio>
#include <vector>
#include <thread>

////////////////////////////////// API //////////////////////////////////

#define PM_THREAD(name) Performan::SoftPtr<Performan::Thread> pmThread = Performan::Profiler::GetInstance()->AddThread(name);
#define PM_SCOPED_FRAME() Performan::FrameScope pmFrameScope(pmThread);
#define PM_SCOPED_EVENT(name) Performan::EventScope pmEventScope(pmThread, name);

namespace Performan {

#define PERFORMAN_SERIALIZE(stream, ptr, size) (stream).SerializeBytes(static_cast<void*>(ptr), size)

    ////////////////////////////////// Assertion //////////////////////////////////

    extern void (*PerformanAssertFunction)(const char*, const char*, const char* file, int line);

#define PERFORMAN_STATIC_ASSERT(condition) static_assert(condition)
#define PERFORMAN_STATIC_ASSERT_MESSAGE(condition, msg) static_assert(condition, msg)

#define PERFORMAN_ASSERT(condition)                                            \
do                                                                             \
{                                                                              \
    if (!(condition))                                                          \
    {                                                                          \
        PerformanAssertFunction(#condition, __FUNCTION__, __FILE__, __LINE__); \
    }                                                                          \
} while (0)

    void PerformanSetAssertFunction(void (*function)(const char* condition, const char* function, const char* file, int line));
    static void PerformanDefaultAssertHandler(const char* condition, const char* function, const char* file, int line);

    ////////////////////////////////// Allocator //////////////////////////////////

#define PERFORMAN_NEW(allocator, TYPE, ...) new ((allocator).Allocate(sizeof(TYPE))) TYPE(__VA_ARGS__)
#define PERFORMAN_DELETE(allocator, TYPE, p) do { if (p) { (p)->~TYPE(); (allocator).Free( p ); p = nullptr; } } while (0)
#define PERFORMAN_ALLOCATE(allocator, bytes) (allocator).Allocate( (bytes))
#define PERFORMAN_FREE(allocator, p) do { if (p) { (allocator).Free(p); p = nullptr; } } while(0)

    struct Allocator {
        virtual void* Allocate(std::size_t size) = 0;
        virtual void Free(void* ptr) = 0;
    };

    struct DefaultAllocator : public Allocator {
        void* Allocate(std::size_t size) override;
        void Free(void* ptr) override;
    };

    DefaultAllocator& GetDefaultAllocator();

    ////////////////////////////////// Utilities //////////////////////////////////

    // Memory you do not own, do not attempt to delete it.
    template <class T>
    struct SoftPtr
    {
        SoftPtr() = default;
        SoftPtr(T* ptr) : _ptr(ptr) {}

        bool operator==(const SoftPtr& rhs) const { return *this == *rhs; }
        bool operator<(const SoftPtr& rhs) const { return *this < *rhs; }

        T* operator->() const { return _ptr; }
        T& operator*() const { return *_ptr; }
        T* _ptr = nullptr;
    };

    ////////////////////////////////// Profiler //////////////////////////////////

    using PortableNano = std::chrono::duration<int64_t, std::nano>;
    using PortableTimePoint = std::chrono::time_point<std::chrono::steady_clock, PortableNano>;

    struct Event {
        Event() = default;
        Event(const char* name)
            : _name(name) {}

        const char* _name = nullptr;
        PortableTimePoint _start;
        PortableTimePoint _end;

        template <class Stream>
        void Serialize(Stream& stream);
    };

    struct Frame {
        PortableTimePoint _start;
        PortableTimePoint _end;
        uint64_t _frameIdx = 0;

        template <class Stream>
        void Serialize(Stream& stream);
    };

    struct Thread {

        Thread() = default;
        Thread(const char* name)
            : _name(name) {}

        const char* _name = nullptr;
        std::vector<Frame> _frames;
        std::vector<Event> _events;

        template <class Stream>
        void Serialize(Stream& stream);
    };

    struct EventScope {
        EventScope(Thread& thread, const char* name)
            : _thread(&thread)
            , _event(name)
        {
            _event._start = std::chrono::steady_clock::now();
        }

        EventScope(SoftPtr<Thread> frame, const char* name)
            : _thread(frame)
            , _event(name)
        {
            _event._start = std::chrono::steady_clock::now();
        }

        ~EventScope() {
            _event._end = std::chrono::steady_clock::now();
            _thread->_events.push_back(std::move(_event));
        }

        SoftPtr<Thread> _thread;
        Event _event;
    };

    struct FrameScope {
        FrameScope(SoftPtr<Thread> thread)
            :_thread(thread)
        {
            _frame._start = std::chrono::steady_clock::now();
        }

        ~FrameScope()
        {
            _frame._end = std::chrono::steady_clock::now();
            _thread->_frames.push_back(_frame);
        }

        SoftPtr<Thread> _thread;
        Frame _frame;
    };

    class Profiler {
    public:
        // Singleton
        static void CreateInstance();
        static void DestroyInstance();
        static Profiler* GetInstance();

    public:
        void SetAllocator(Allocator* allocator);
        Allocator* GetAllocator() const;

        SoftPtr<Thread> AddThread(const char* name);
        void RemoveThread(SoftPtr<Thread> thread);

    private:
        Profiler() = default;
        ~Profiler();
        Profiler(const Profiler&) = delete;
        void operator=(const Profiler&) = delete;

    private:
        std::vector<Thread*> _threads;
        mutable std::mutex _threadsMtx;

        Allocator* _allocator = nullptr;

        inline static Profiler* _instance = nullptr;
    };

    ////////////////////////////////// Serialization //////////////////////////////////

    template <class Stream>
    void Serialize(Stream& stream, const char*& value);

    template <class Stream, class T>
    void SerializeVector(Stream& stream, std::vector<T>& values);

    class Stream {
    public:
        Stream(Allocator* allocator);
        Stream(Allocator* allocator, uint8_t* buffer, size_t size);

        virtual ~Stream();

        uint8_t* Data() { return _buffer; }
        size_t Offset() const { return _offset; }
        size_t Size() const { return _size; }

        void Resize();
        void Clear();

    protected:
        Allocator* _allocator = nullptr;
        uint8_t* _buffer = nullptr;
        size_t _size = 0;
        size_t _offset = 0;
    };

    class WriteStream : public Stream {
    public:
        enum {
            IsWriting = 1
        };

        enum {
            IsReading = 0
        };

        WriteStream(Allocator* allocator)
            : Stream(allocator) {}

        void SerializeBytes(void* value, size_t size);
    };

    class ReadStream : public Stream {
    public:
        enum {
            IsWriting = 0
        };

        enum {
            IsReading = 1
        };

        ReadStream(Allocator* allocator)
            : Stream(allocator) {}

        ReadStream(Allocator* allocator, uint8_t* buffer, size_t size)
            : Stream(allocator, buffer, size) {}

        void SerializeBytes(void* value, size_t size);
    };

    template<class Stream>
    inline void Thread::Serialize(Stream& stream)
    {
        Performan::Serialize(stream, _name);
        Performan::SerializeVector(stream, _events);
        Performan::SerializeVector(stream, _frames);
    }

    template<class Stream>
    inline void Frame::Serialize(Stream& stream)
    {
        int64_t startCount = 0;
        int64_t endCount = 0;

        if constexpr (Stream::IsWriting)
        {
            startCount = _start.time_since_epoch().count();
            endCount = _end.time_since_epoch().count();
        }

        PERFORMAN_SERIALIZE(stream, &startCount, sizeof(int64_t));
        PERFORMAN_SERIALIZE(stream, &endCount, sizeof(int64_t));
        PERFORMAN_SERIALIZE(stream, &_frameIdx, sizeof(uint64_t));

        if constexpr (Stream::IsReading)
        {
            PortableNano startDuration(startCount);
            PortableNano endDuration(endCount);
            _start = PortableTimePoint(startDuration);
            _end = PortableTimePoint(endDuration);
        }
    }

    template<class Stream>
    inline void Event::Serialize(Stream& stream)
    {
        int64_t startCount = 0;
        int64_t endCount = 0;

        if constexpr (Stream::IsWriting)
        {
            startCount = _start.time_since_epoch().count();
            endCount = _end.time_since_epoch().count();
        }

        PERFORMAN_SERIALIZE(stream, &startCount, sizeof(int64_t));
        PERFORMAN_SERIALIZE(stream, &endCount, sizeof(int64_t));
        Performan::Serialize(stream, _name);

        if constexpr (Stream::IsReading)
        {
            PortableNano startDuration(startCount);
            PortableNano endDuration(endCount);
            _start = PortableTimePoint(startDuration);
            _end = PortableTimePoint(endDuration);
        }
    }

    template<class Stream>
    inline void Serialize(Stream& stream, const char*& value)
    {
        char*& tempValue = const_cast<char*&>(value);
        uint32_t valueLength = 0;

        if constexpr (Stream::IsWriting)
        {
            if (value != nullptr) {
                valueLength = static_cast<uint32_t>(strlen(value));
                valueLength += 1;
            }
        }

        PERFORMAN_SERIALIZE(stream, &valueLength, sizeof(uint32_t));

        if (valueLength == 0) {
            return;
        }

        if constexpr (Stream::IsReading)
        {
            tempValue = new char[valueLength];
            tempValue[valueLength - 1] = '\0';
        }

        PERFORMAN_SERIALIZE(stream, tempValue, valueLength);
    }

    template<class Stream, class T>
    inline void SerializeVector(Stream& stream, std::vector<T>& values) {
        PERFORMAN_ASSERT(values.size() < UINT32_MAX);
        uint32_t size = 0;

        if constexpr (Stream::IsWriting)
        {
            size = static_cast<uint32_t>(values.size());
        }

        PERFORMAN_SERIALIZE(stream, &size, sizeof(uint32_t));

        if constexpr (Stream::IsReading)
        {
            values.resize(size);
        }

        for (uint32_t index = 0; index < size; index++)
        {
            values[index].Serialize(stream);
        }
    }
}

#endif // PERFORMAN_H
