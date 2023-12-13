// PERFORMAN.H

#ifndef PERFORMAN_H
#define PERFORMAN_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

#include <iostream>

// API
	
#define PM_THREAD() Performan::PM_Thread pmThread;

#define PM_SCOPED_FRAME() Performan::PM_FrameScope pmFrameScope(pmThread);

#define PM_SCOPED_EVENT(name) Performan::PM_EventScope pmEventScope(pmFrameScope._frame, name);

namespace Performan {

	// Base structures

	struct PM_Event {
		PM_Event(const char* name)
			: _name(name) {}

		const char* _name;
		std::chrono::time_point<std::chrono::steady_clock> _start;
		std::chrono::time_point<std::chrono::steady_clock> _end;

		void Print() const
		{
			using namespace std::literals;
			std::cout << "\t\t[ Name: " << _name << ", Start: " << _start.time_since_epoch() << ", End: " << _end.time_since_epoch() << "]\n";
		}
	};

	struct PM_Frame {
		std::vector<PM_Event> _events;

		void Print() const 
		{
			std::cout << "\tEvents:\n";
			for (const PM_Event& event: _events)
			{
				event.Print();
			}
		}
	};

	struct PM_Thread {
		std::vector<PM_Frame> _frames;

		void Print() const
		{
			std::cout << "Frames:\n";
			for (const PM_Frame& frame: _frames) 
			{
				frame.Print();
			}
		}
	};

	// Scoped structures
	
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
}

#endif // PERFORMAN_H
