// PERFORMAN.H

#ifndef PERFORMAN_H
#define PERFORMAN_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <ratio>
#include <vector>
#include <thread>

////////////////////////////////// API //////////////////////////////////
	
#define PM_THREAD() Performan::Thread pmThread;
#define PM_SCOPED_FRAME() Performan::FrameScope pmFrameScope(pmThread);
#define PM_SCOPED_EVENT(name) Performan::EventScope pmEventScope(pmFrameScope._frame, name);

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

	////////////////////////////////// Profiler //////////////////////////////////

	class Profiler {
	public:
		// Singleton
		static void CreateInstance();
		static void DestroyInstance();
		static Profiler* GetInstance();

	public:
		void SetAllocator(Allocator* allocator);
		Allocator* GetAllocator() const;


	private:
		Profiler() = default;
		Profiler(const Profiler&) = delete;
		void operator=(const Profiler&) = delete;

	private:
		mutable std::shared_mutex _threadDescriptionMtx;
		mutable std::shared_mutex _eventDescriptionMtx;

		Allocator* _allocator = nullptr;

		inline static Profiler* _instance = nullptr;
	};

	using PortableNano = std::chrono::duration<int64_t, std::nano>;
	using PortableTimePoint = std::chrono::time_point<std::chrono::steady_clock, PortableNano>;

	struct Event {
		Event() = default;
		Event(const char* name)
			: _name(name) {}

		const char* _name;
		PortableTimePoint _start;
		PortableTimePoint _end;

        template <class Stream>
		void Serialize(Stream& stream)
        {
			int64_t startCount = 0;
			int64_t endCount = 0;
			int32_t nameLength = 0;
			char* name = nullptr;

            if constexpr (Stream::IsWriting)
            {
				startCount = _start.time_since_epoch().count();
				endCount = _end.time_since_epoch().count();
				nameLength = strlen(_name);
				nameLength += 1;
				name = const_cast<char*>(_name);
            }

			PERFORMAN_SERIALIZE(stream, &startCount, sizeof(int64_t));
			PERFORMAN_SERIALIZE(stream, &endCount, sizeof(int64_t));
			PERFORMAN_SERIALIZE(stream, &nameLength, sizeof(int32_t));

			if constexpr (Stream::IsReading)
			{
				PortableNano startDuration(startCount);
				PortableNano endDuration(endCount);
				_start = PortableTimePoint(startDuration);
				_end = PortableTimePoint(endDuration);
				name = new char[nameLength];
				name[nameLength - 1] = '\0';
			}

			PERFORMAN_SERIALIZE(stream, name, nameLength);

			if constexpr (Stream::IsReading)
			{
				_name = name;
			}
        }
	};

	struct Frame {
		std::vector<Event> _events;
	};

	struct Thread {
		std::vector<Frame> _frames;
	};
	
	struct EventScope {
		EventScope(Frame& frame, const char* name)
			: _frame(frame)
			, _event(name)
		{
			_event._start = std::chrono::steady_clock::now();
		}

		~EventScope() {
			_event._end = std::chrono::steady_clock::now();
			_frame.get()._events.push_back(std::move(_event));
		}

		std::reference_wrapper<Frame> _frame;
		Event _event;
	};

	struct FrameScope {
		FrameScope(Thread& thread)
			:_thread(thread)
		{
			// init frame start
		}

		~FrameScope()
		{
			_thread.get()._frames.push_back(_frame);
		}

		std::reference_wrapper<Thread> _thread;
		Frame _frame;
	};

	////////////////////////////////// Serialization //////////////////////////////////

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
}

#endif // PERFORMAN_H
