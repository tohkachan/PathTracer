#include "Threads.h"
#include <thread>
#include <mutex>

namespace tk
{
#if (defined( __WIN32__ ) || defined( _WIN32 ))
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

	ThreadHandle::ThreadHandle(size_t threadIdx, void* userParam)
		: mHandle(0), mThreadIdx(threadIdx), mUserParam(userParam)
	{
	}

	ThreadHandle::~ThreadHandle()
	{
		if (mHandle)
			CloseHandle(mHandle);
		mHandle = 0;
	}

	ThreadHandle* Threads::createThread(THREAD_FUNC_POINT funcPoint, size_t threadIdx, void* param)
	{
		ThreadHandle* arg = new ThreadHandle(threadIdx, param);
		ThreadHandle* ret(new ThreadHandle(threadIdx, param));
		HANDLE handle = CreateThread(0, 0, funcPoint, arg, 0, 0);
		ret->_setOsHandle(handle);
		return ret;
	}

	void Threads::waitForThreads(size_t numThreads, const ThreadHandle** handles)
	{
		HANDLE hThreads[128];
		for (size_t i = 0; i < numThreads; ++i)
			hThreads[i] = handles[i]->_getOsHandle();
		WaitForMultipleObjects(numThreads, hThreads, true, INFINITE);
	}

	void Threads::waitForThreads(const std::vector<ThreadHandle*>& handles)
	{
		if (!handles.empty())
		{
			HANDLE hThreads[128];
			for (size_t i = 0; i < handles.size(); ++i)
				hThreads[i] = handles[i]->_getOsHandle();
			WaitForMultipleObjects(handles.size(), hThreads, true, INFINITE);
		}
	}

	void Threads::sleep(unsigned milliseconds)
	{
		Sleep(milliseconds);
	}

	LwMutex::LwMutex()
		: mCounter(0)
	{
		mSemaphore = CreateSemaphore(NULL, 0, 1, NULL);//unsigned state
	}

	LwMutex::~LwMutex()
	{
		CloseHandle(mSemaphore);
	}

	void LwMutex::lock()
	{
		if (_InterlockedIncrement(&mCounter) > 1)
			WaitForSingleObject(mSemaphore, INFINITE);
	}

	void LwMutex::unlock()
	{
		if (_InterlockedDecrement(&mCounter) > 0)
			ReleaseSemaphore(mSemaphore, 1, NULL);
	}
#else
	ThreadHandle::ThreadHandle(size_t threadIdx, void* userParam)
		: mThreadIdx(threadIdx), mUserParam(userParam)
	{
	}

	ThreadHandle::~ThreadHandle()
	{
	}

	ThreadHandle* Threads::createThread(THREAD_FUNC_POINT funcPoint, size_t threadIdx, void* param)
	{
		ThreadHandle* threadArg(new ThreadHandle(threadIdx, param));
		ThreadHandle* ret(new ThreadHandle(threadIdx, param));
		pthread_t threadId;
		pthread_create(&threadId, NULL, funcPoint, threadArg);
		ret->_setOsHandle(threadId);
		return ret;
	}

	void Threads::waitForThreads(size_t numThreads, const ThreadHandle** handles)
	{
		for (size_t i = 0; i < numThreads; ++i)
			pthread_join(handles[i]->_getOsHandle(), NULL);
	}

	void Threads::waitForThreads(const std::vector<ThreadHandle*>& handles)
	{
		if (!handles.empty())
		{
			for (size_t i = 0; i < handles.size(); ++i)
				pthread_join(handles[i]->_getOsHandle(), NULL);
		}
	}

	void Threads::sleep(unsigned milliseconds)
	{
		timespec timeToSleep;
		timeToSleep.tv_nsec = (milliseconds % 1000) * 1000000;
		timeToSleep.tv_sec = milliseconds / 1000;
		nanosleep(&timeToSleep, 0);
	}

	LwMutex::LwMutex()
	{
		pthread_mutex_init(&mMutex, 0);
	}

	LwMutex::~LwMutex()
	{
		pthread_mutex_destroy(&mMutex);
	}

	void LwMutex::lock()
	{
		pthread_mutex_lock(&mMutex);
	}

	void LwMutex::unlock()
	{
		pthread_mutex_unlock(&mMutex);
	}
#endif
	class ParallelJob
	{
	private:
		friend class ThreadPool;
		int activeWorkers = 0;
		ParallelJob* prev = nullptr;
		ParallelJob* next = nullptr;
		bool removed = false;
	public:
		virtual ~ParallelJob() {}
		virtual bool haveWork(void)const = 0;
		virtual void run(std::unique_lock<std::mutex>* lock) = 0;
		bool finished(void)const { return !haveWork() && activeWorkers == 0; }
	};

	class ThreadPool
	{
	private:
		std::vector<std::thread> threads;
		mutable std::mutex mutex;
		bool shutdown = false;
		ParallelJob* jobRoot = nullptr;
		std::condition_variable cv;
		void _workerFunc(int threadIdx);
	public:
		explicit ThreadPool(int numThreads);
		~ThreadPool();
		size_t size()const { return threads.size(); }
		std::unique_lock<std::mutex> addParrallelJob(ParallelJob* job);
		void removeParallelJob(ParallelJob* job);
		void workOrWait(std::unique_lock<std::mutex>* lock);
	};

	ThreadPool::ThreadPool(int numThreads)
	{
		for (int i = 1; i < numThreads; ++i)
			threads.push_back(std::thread(&ThreadPool::_workerFunc, this, i));
	}

	ThreadPool::~ThreadPool()
	{
		if (threads.empty())
			return;
		{
			std::lock_guard<std::mutex> lock(mutex);
			shutdown = true;
			cv.notify_all();
		}

		for (std::thread& thread : threads)
			thread.join();
	}

	void ThreadPool::_workerFunc(int threadIdx)
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (!shutdown)
			workOrWait(&lock);
	}

	std::unique_lock<std::mutex> ThreadPool::addParrallelJob(ParallelJob* job)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (jobRoot != nullptr)
			jobRoot->prev = job;
		job->next = jobRoot;
		jobRoot = job;
		cv.notify_all();
		return lock;
	}


	void ThreadPool::removeParallelJob(ParallelJob* job)
	{
		if (job->prev != nullptr)
			job->prev->next = job->next;
		else
			jobRoot = job->next;
		if (job->next != nullptr)
			job->next->prev = job->prev;
		job->removed = true;
	}

	void ThreadPool::workOrWait(std::unique_lock<std::mutex>* lock)
	{
		ParallelJob* job = jobRoot;
		while ((job != nullptr) && !job->haveWork())
			job = job->next;
		if (job != nullptr)
		{
			job->activeWorkers++;
			job->run(lock);
			lock->lock();
			job->activeWorkers--;
			if (job->finished())
				cv.notify_all();
		}
		else
			cv.wait(*lock);
	}

	static std::unique_ptr<ThreadPool> threadPool;

	class ParallelForLoop1D : public ParallelJob
	{
	private:
		std::function<void(int, int)> func;
		int nextIdx, endIdx;
		int chunkSize;
	public:
		ParallelForLoop1D(int startIdx, int endIdx, int chunkSize, std::function<void(int, int)> func)
			: func(std::move(func)),
			nextIdx(startIdx),
			endIdx(endIdx),
			chunkSize(chunkSize) {}

		bool haveWork()const { return nextIdx < endIdx; }
		void run(std::unique_lock<std::mutex>* lock);
	};

	void ParallelForLoop1D::run(std::unique_lock<std::mutex>* lock)
	{
		int start = nextIdx;
		int end = std::min(start + chunkSize, endIdx);
		nextIdx = end;

		if (!haveWork())
			threadPool->removeParallelJob(this);
		lock->unlock();
		func(start, end);
	}

	class ParallelForLoop2D : public ParallelJob
	{
	private:
		std::function<void(Bounds2i)> func;
		const Bounds2i extent;
		Point2i nextStart;
		int chunkSize;
	public:
		ParallelForLoop2D(const Bounds2i& extent, int chunkSize, std::function<void(Bounds2i)> func)
			: func(std::move(func)),
			extent(extent),
			nextStart(extent.pMin),
			chunkSize(chunkSize) {}

		bool haveWork()const { return nextStart.y < extent.pMax.y; }
		void run(std::unique_lock<std::mutex>* lock);
	};

	void ParallelForLoop2D::run(std::unique_lock<std::mutex>* lock)
	{
		Point2i end = nextStart + Point2i(chunkSize, chunkSize);
		Bounds2i b(nextStart, end);
		b.intersect(extent);
		nextStart.x += chunkSize;
		if (nextStart.x >= extent.pMax.x)
		{
			nextStart.x = extent.pMin.x;
			nextStart.y += chunkSize;
		}

		if (!haveWork())
			threadPool->removeParallelJob(this);
		lock->unlock();
		func(b);
	}

	void Parrallel::parrallelFor(s32 start, s32 end, std::function<void(s32, s32)> func)
	{
		int runThreads = threadPool->size() + 1;
		int chunkSize = std::max<int>(1, (end - start) / (8 * runThreads));
		if (end - start < chunkSize)
		{
			func(start, end);
			return;
		}

		ParallelForLoop1D loop(start, end, chunkSize, std::move(func));

		std::unique_lock<std::mutex> lock = threadPool->addParrallelJob(&loop);

		while (!loop.finished())
			threadPool->workOrWait(&lock);
	}

	void Parrallel::parrallelFor2D(const Bounds2i& extent, std::function<void(Bounds2i)> func)
	{
		int runThreads = threadPool->size() + 1;
		int tileSize = Math::Clamp(int(sqrt(extent.diagonal().x * extent.diagonal().y /
			(8 * runThreads))), 1, 32);
		ParallelForLoop2D loop(extent, tileSize, std::move(func));
		std::unique_lock<std::mutex> lock = threadPool->addParrallelJob(&loop);
		while (!loop.finished())
			threadPool->workOrWait(&lock);
	}

	void Parrallel::parrallelInit(s32 numThreads)
	{
		if (numThreads <= 0)
			numThreads = std::max<s32>(1, std::thread::hardware_concurrency());
		threadPool = std::make_unique<ThreadPool>(numThreads);
	}

}