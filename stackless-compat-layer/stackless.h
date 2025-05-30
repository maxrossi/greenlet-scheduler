#pragma once

// Stackless Python compatibility layer
// Define USE_GREENLET_STACKLESS to use greenlet implementation instead of native stackless
// Example: #define USE_GREENLET_STACKLESS

#ifdef USE_GREENLET_STACKLESS
	// Use greenlet-based implementation
	#include "stackless_greenlet.h"
#else
	// Native Stackless Python implementation
	#define Py_BUILD_CORE
	#include <stackless_api.h>

namespace stackless
{
	namespace python = boost::python;
	using namespace python;

	static PyObject* taskletExit = PyExc_TaskletExit;

	class tasklet : public object
	{
	public:
		explicit tasklet(object_cref callable)
			: object(detail::new_reference(PyTasklet_New(0, callable.ptr())))
		{}

		void run()
		{
			if (PyTasklet_Run(this->ptr()))
				throw_error_already_set();
		}

		void remove()
		{
			if (PyTasklet_Remove(this->ptr()))
				throw_error_already_set();
		}

		void insert()
		{
			if (PyTasklet_Insert(this->ptr()))
				throw_error_already_set();
		}

		void raise_exception(PyObject* excClass, object_cref args)
		{
			if (PyTasklet_RaiseException(this->ptr(), excClass, args.ptr()))
				throw_error_already_set();
		}

		void kill()
		{
			if (PyTasklet_Kill(this->ptr()))
				throw_error_already_set();
		}

		bool set_atomic(bool flag)
		{
			return PyTasklet_SetAtomic(this->ptr(), flag);
		}

		bool set_ignore_nesting(bool flag)
		{
			return PyTasklet_SetIgnoreNesting(this->ptr(), flag);
		}

		bool get_atomic()
		{
			return PyTasklet_GetAtomic(this->ptr());
		}

		bool get_ignore_nesting()
		{
			return PyTasklet_GetIgnoreNesting(this->ptr());
		}

		bool main()
		{
			return PyTasklet_IsMain(this->ptr());
		}

		bool current()
		{
			return PyTasklet_IsCurrent(this->ptr());
		}

		int get_recursion_depth()
		{
			return PyTasklet_GetRecursionDepth(this->ptr());
		}

		int get_nesting_level()
		{
			return PyTasklet_GetNestingLevel(this->ptr());
		}

		bool alive()
		{
			return PyTasklet_Alive(this->ptr());
		}

		bool paused()
		{
			return PyTasklet_Paused(this->ptr());
		}

		bool scheduled()
		{
			return PyTasklet_Scheduled(this->ptr());
		}

		bool restorable()
		{
			return PyTasklet_Restorable(this->ptr());
		}

		PyTaskletObject* ptr()
		{
			return (PyTaskletObject*)object::ptr();
		}

		BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(tasklet, object)
	};

	inline tasklet current()
	{
		return tasklet(detail::new_reference(PyStackless_GetCurrent()));
	}

	class channel : public object
	{
	public:
		channel()
			: object(detail::new_reference(PyChannel_New(0)))
		{}

		void send()
		{
			assert (balance() < 0 || !current().main());
			if (PyChannel_Send(this->ptr(), Py_None))
				throw_error_already_set();
		}

		void send(object_cref arg)
		{
			assert (balance() < 0 || !current().main());
			if (PyChannel_Send(this->ptr(), arg.ptr()))
				throw_error_already_set();
		}

		void send_exception(PyObject* excClass)
		{
			assert (balance() < 0 || !current().main());
			if (PyChannel_SendException(this->ptr(), excClass, Py_None))
				throw_error_already_set();
		}

		void send_exception(PyObject* excClass, object_cref args)
		{
			assert (balance() < 0 || !current().main());
			if (PyChannel_SendException(this->ptr(), excClass, args.ptr()))
				throw_error_already_set();
		}

		object receive()
		{
			assert (balance() > 0 || !current().main());
			return object(detail::new_reference(PyChannel_Receive(this->ptr())));
		}

		object queue()
		{
			return object(detail::new_reference(PyChannel_GetQueue(this->ptr())));
		}

		void open()
		{
			// there is no PyChannel_Open() function, why???
			this->ptr()->flags.closing = 0;
		}

		void close()
		{
			// there is no PyChannel_Close() function, why???
			this->ptr()->flags.closing = 1;
		}

		bool closing()
		{
			return PyChannel_GetClosing(this->ptr());
		}

		bool closed()
		{
			return PyChannel_GetClosed(this->ptr());
		}

		enum Preference
		{
			prefer_receiver = -1,
			no_preference = 0,
			prefer_sender = 1
		};

		Preference preference()
		{
			return Preference(PyChannel_GetPreference(this->ptr()));
		}

		void preference(Preference pref)
		{
			PyChannel_SetPreference(this->ptr(), pref);
		}

		bool schedule_all()
		{
			return PyChannel_GetScheduleAll(this->ptr());
		}

		void schedule_all(bool value)
		{
			PyChannel_SetScheduleAll(this->ptr(), value);
		}

		int balance()
		{
			return PyChannel_GetBalance(this->ptr());
		}

		PyChannelObject* ptr()
		{
			return (PyChannelObject*)object::ptr();
		}

		BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(channel, object)
	};

	inline int get_run_count()
	{
		int ret = PyStackless_GetRunCount();
		if (ret < 0)
			throw_error_already_set();
		return ret;
	}

	inline object run(long timeout = 0)
	{
		return object(detail::new_reference(PyStackless_RunWatchdog(timeout)));
	}

	inline object schedule(bool remove = false)
	{
		return object(detail::new_reference(PyStackless_Schedule(Py_None, remove)));
	}

	inline object schedule(object const& retval, bool remove = false)
	{
		return object(detail::new_reference(PyStackless_Schedule(retval.ptr(), remove)));
	}

	inline void exec_in_tasklet(object const& callable)
	{
		tasklet t(callable);
		t();
	}

	template <class T1>
	inline void exec_in_tasklet(object const& callable, T1 const& param1)
	{
		tasklet t(callable);
		t(param1);
	}

	template <class T1, class T2>
	inline void exec_in_tasklet(object const& callable, T1 const& param1, T2 const& param2)
	{
		tasklet t(callable);
		t(param1, param2);
	}

	template <class T1, class T2, class T3>
	inline void exec_in_tasklet(object const& callable, T1 const& param1, T2 const& param2, T3 const& param3)
	{
		tasklet t(callable);
		t(param1, param2, param3);
	}

	class atomic_guard
	{
		bool m_previous;

	public:
		atomic_guard()
		{
			m_previous = current().set_atomic(true);
		}
		~atomic_guard()
		{
			current().set_atomic(m_previous);
		}
	};
}

namespace boost
{
	namespace python
	{
		namespace converter
		{
			template <>
			struct object_manager_traits<stackless::tasklet>
				: pytype_object_manager_traits<&PyTasklet_Type, stackless::tasklet>
			{
			};

			template <>
			struct object_manager_traits<stackless::channel>
				: pytype_object_manager_traits<&PyChannel_Type, stackless::channel>
			{
			};
		}
	}
}

#endif // USE_GREENLET_STACKLESS
