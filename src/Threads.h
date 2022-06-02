#ifndef THREADS_H
#define THREADS_H

#include "Vector.hpp"

namespace tk
{
#if (defined( __WIN32__ ) || defined( _WIN32 ))
	typedef void* HANDLE;

#define THREAD_DECL(func) \
unsigned long __stdcall func##_internal(void* argName)\
{\
	unsigned long ret = 0;\
	ThreadHandle* handle(reinterpret_cast<ThreadHandle*>(argName));\
	try\
	{\
			ret = func(handle);\
	}\
	catch(...)\
	{\
	}\
	delete handle;\
	return ret;\
}
#else
#include <pthread.h>

#define THREAD_DECL(func) \
    void* __attribute__((__cdecl__)) func##_internal(void *argName)\
    {\
        unsigned long ret = 0;\
        ThreadHandle* handle( reinterpret_cast<ThreadHandle*>( argName ) );\
        try {\
            ret = func(handle);\
        }\
        catch(...)\
        {\
        }\
        delete handle;\
		return (void*)ret;\
    }
#endif

	class ThreadHandle
	{
#if (defined( __WIN32__ ) || defined( _WIN32 ))
		HANDLE mHandle;
#else
		pthread_t mHandle;
#endif
		size_t mThreadIdx;
		void* mUserParam;
	public:
		ThreadHandle(size_t threadIdx, void* userParam);
		~ThreadHandle();

		size_t getThreadIdx()const { return mThreadIdx; }
		void* getUserParameter()const { return mUserParam; }
#if (defined( __WIN32__ ) || defined( _WIN32 ))
		void _setOsHandle(HANDLE handle) { mHandle = handle; }
		HANDLE _getOsHandle()const { return mHandle; }
#else
		void _setOsHandle(pthread_t &handle) { mHandle = handle; }
		pthread_t _getOsHandle()const { return mHandle; }
#endif

	};

#if (defined( __WIN32__ ) || defined( _WIN32 ))
	typedef unsigned long(__stdcall *THREAD_FUNC_POINT)(void* lpThreadParam);
#else
	typedef void* (__attribute__((__cdecl__)) *THREAD_FUNC_POINT)(void* lpThreadParam);
#endif

	class Threads
	{
	public:
#define THREAD_GET(func) func##_internal
		static ThreadHandle* createThread(THREAD_FUNC_POINT funcPoint, size_t threadIdx, void* param);
		static void waitForThreads(size_t numThreads, const ThreadHandle** handles);
		static void waitForThreads(const std::vector<ThreadHandle*>& handles);
		static void sleep(unsigned milliseconds);
	};

	class LwMutex
	{
#if (defined( __WIN32__ ) || defined( _WIN32 ))
		__declspec(align(4)) long mCounter;
		void* mSemaphore;
#else
		pthread_mutex_t mMutex;
#endif

	public:
		LwMutex();
		~LwMutex();
		void lock();
		void unlock();
	};
	class Parrallel
	{
	public:
		static void parrallelFor(s32 start, s32 end, std::function<void(s32, s32)> func);
		static void parrallelFor2D(const Bounds2i& extent, std::function<void(Bounds2i)> func);
		static void parrallelInit(s32 numThreads);
	};
}
#endif