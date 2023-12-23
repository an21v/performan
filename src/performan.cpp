#include "../include/performan.h"

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
		_instance = PERFORMAN_NEW(GetDefaultAllocator(), Profiler); // User provided allocator
	}

	void Profiler::DestroyInstance()
	{
		PERFORMAN_ASSERT(_instance != nullptr);
		PERFORMAN_DELETE(GetDefaultAllocator(), Profiler, _instance); // User provided allocator
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

	Allocator* Profiler::GetAllocator() const
	{
		if (_allocator) {
			return _allocator;
		}
		
		return &(GetDefaultAllocator());
	}

	void PM_Event::Print() const
	{
		std::cout << "\t\t[ Name: " << _name << ", Start: " << _start.time_since_epoch() << ", End: " << _end.time_since_epoch() << "]\n";
	}

	void PM_Frame::Print() const
	{
		std::cout << "\tEvents:\n";
		for (const PM_Event& event : _events)
		{
			event.Print();
		}
	}

	void PM_Thread::Print() const
	{
		std::cout << "Frames:\n";
		for (const PM_Frame& frame : _frames)
		{
			frame.Print();
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

	void WriteStream::SerializeInt64(int64_t& value) {
		if (_size - _offset < sizeof(int64_t)) {
			Resize();
		}
		memcpy(&_buffer[_offset], &value, sizeof(int64_t));
		_offset += sizeof(int64_t);
	}

	void WriteStream::SerializeBytes(uint8_t* value, size_t size)
	{
		if (_size - _offset < size) {
			Resize();
		}
		memcpy(&_buffer[_offset], value, size);
		_offset += size;
	}

	void ReadStream::SerializeInt64(int64_t& value)
	{
		memcpy(&value, &_buffer[_offset], sizeof(int64_t));
		_offset += sizeof(int64_t);
	}

	void ReadStream::SerializeBytes(uint8_t* value, size_t size)
	{
		memcpy(value, &_buffer[_offset], size);
		_offset += size;
	}
}
