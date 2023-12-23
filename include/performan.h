// PERFORMAN.H

#ifndef PERFORMAN_H
#define PERFORMAN_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <vector>
#include <thread>

////////////////////////////////// API //////////////////////////////////
	
#define PM_THREAD() Performan::PM_Thread pmThread;
#define PM_SCOPED_FRAME() Performan::PM_FrameScope pmFrameScope(pmThread);
#define PM_SCOPED_EVENT(name) Performan::PM_EventScope pmEventScope(pmFrameScope._frame, name);

namespace Performan {

	////////////////////////////////// Assertion //////////////////////////////////

extern void (*PerformanAssertFunction)(const char*, const char*, const char* file, int line);

#define PERFORMAN_STATIC_ASSERT(condition) static_assert(condition)
#define PERFORMAN_STATIC_ASSERT_MESSAGE(condition, msg) static_assert(condition, msg)

#define PERFORMAN_ASSERT(condition)                                                         \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
		PerformanAssertFunction( #condition, __FUNCTION__, __FILE__, __LINE__ );            \
    }                                                                                       \
} while(0)

void PerformanSetAssertFunction(void (*function)(const char* /*condition*/, const char* /*function*/, const char* /*file*/, int /*line*/));
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
	
	struct ThreadDescription {
		const char* name;
		std::thread::id id;
	};

	struct EventDescription {
		const char* name;
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

	struct PM_Event {
		PM_Event(const char* name)
			: _name(name) {}

		const char* _name;
		std::chrono::time_point<std::chrono::steady_clock> _start;
		std::chrono::time_point<std::chrono::steady_clock> _end;

		void Print() const;
	};

	struct PM_Frame {
		std::vector<PM_Event> _events;

		void Print() const;
	};

	struct PM_Thread {
		std::vector<PM_Frame> _frames;

		void Print() const;
	};
	
	struct PM_EventScope {
		PM_EventScope(PM_Frame& frame, const char* name)
			: _frame(frame)
			, _event(name)
		{
			_event._start = std::chrono::steady_clock::now();
		}

		~PM_EventScope() {
			_event._end = std::chrono::steady_clock::now();
			_frame.get()._events.push_back(std::move(_event));
		}

		std::reference_wrapper<PM_Frame> _frame;
		PM_Event _event;
	};

	struct PM_FrameScope {
		PM_FrameScope(PM_Thread& thread)
			:_thread(thread)
		{
			// init frame start
		}

		~PM_FrameScope()
		{
			_thread.get()._frames.push_back(_frame);
		}

		std::reference_wrapper<PM_Thread> _thread;
		PM_Frame _frame;
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

		void SerializeInt64(int64_t& value);
		void SerializeBytes(uint8_t* value, size_t size);
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

		void SerializeInt64(int64_t& value);
		void SerializeBytes(uint8_t* value, size_t size);
	};
}

#endif // PERFORMAN_H
