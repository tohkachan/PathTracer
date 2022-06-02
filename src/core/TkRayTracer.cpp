#include "TkRayTracer.h"
#include "sampler.h"
#include "camera.h"

#include <GL/glew.h>
#include <thread>

namespace tk
{
	RayTracer::RayTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette)
		: mState(INIT),
		mNumThreads(numThreads),
		mMaxDepth(maxDepth),
		mRussianRoulette(russianRoulette),
		mScene(nullptr),
		mCamera(nullptr),
		mFrameBuffer(nullptr),
		mSampler(nullptr)
	{
		spp--;
		spp |= spp >> 1;
		spp |= spp >> 2;
		spp |= spp >> 4;
		spp |= spp >> 8;
		spp |= spp >> 16;
		mSpp = spp + 1;
	}

	RayTracer::~RayTracer()
	{
		if (mFrameBuffer)
			delete[] mFrameBuffer;
		delete mSampler;
	}

	void RayTracer::setScene(Scene* scene)
	{
		if (mState != INIT)
			return;
		mScene = scene;
	}

	void RayTracer::setCamera(Camera* cam)
	{
		if (mState != INIT && mState != READY)
			return;
		mCamera = cam;
		mFilm = mCamera->getFilm();
		mEndPos = mFilm->getFullResolution();
		mFrameBuffer = new u32[mEndPos.x * mEndPos.y];
		mState = READY;
	}

	void RayTracer::updateScreen()
	{
		switch (mState)
		{
		case INIT:
		case READY:
			break;
		case VISUALIZE:
			visualize();
			break;
		case RENDERING:
			rendering();
			break;
		case DONE:
			glDrawPixels(mEndPos.x, mEndPos.y, GL_RGBA,
				GL_UNSIGNED_BYTE, mFrameBuffer);
			break;
		}

	}

	void RayTracer::stop()
	{
		switch (mState)
		{
		case INIT:
		case READY:
			break;
		case VISUALIZE:
			mState = READY;
			break;
		case RENDERING:
			stopRaytracing();
		case DONE:
			mState = READY;
			break;
		}
	}

	void RayTracer::clear()
	{
		/*if (mState != READY) return;
		mScene = nullptr;
		mCamera = nullptr;
		mFrameBuffer.resize(0, 0);
		mState = INIT;*/
	}

	void RayTracer::startVisualizing()
	{
		if (mState != READY)
			return;
		mState = VISUALIZE;
	}

	void RayTracer::startRaytracing()
	{
		if (mState != READY)
			return;
		mState = RENDERING;
		mNextStart = Point2i(0, 0);
		mTileSize = 20;
		mJobsDone = 0;
		mJobsCount = (mEndPos.x * mEndPos.y + mTileSize * mTileSize - 1) / (mTileSize * mTileSize);
		mFilm->clear();
		memset(mFrameBuffer, 0, sizeof(u32) * mEndPos.x * mEndPos.y);
		mContinueRendering = true;
		startWorkerThreads();
	}

	void RayTracer::saveImage(string filename)
	{
		if (mState != DONE) return;
		if (filename == "")
		{
			time_t rawtime;
			time(&rawtime);

			time_t t = time(nullptr);
			tm *lt = localtime(&t);
			stringStream ss;
			ss << "_screenshot_" << lt->tm_mon + 1 << "-" << lt->tm_mday << "_"
				<< lt->tm_hour << "-" << lt->tm_min << "-" << lt->tm_sec << ".ppm";
			filename = ss.str();	
		}
		mFilm->writeImage(1.0, filename);
	}

	void RayTracer::render(string filename)
	{
		startRaytracing();
		while (mNextStart.y < mEndPos.y)
		{
			mutex.lock();
			Point2i start = mNextStart;
			Point2i end = mNextStart + Point2i(mTileSize, mTileSize);
			mNextStart.x += mTileSize;
			if (mNextStart.x >= mEndPos.x)
			{
				mNextStart.x = 0;
				end.x = mEndPos.x;
				mNextStart.y += mTileSize;
			}
			mutex.unlock();
			end.y = std::min(end.y, mEndPos.y);
			traceTile(start, end, *mSampler);
			fprintf(stderr, "\r[Tracer] Rendering...... %02d%%", std::min(int((Real)(++mJobsDone) / mJobsCount * 100), 100));
		}
		stopRaytracing();
		mState = DONE;
		saveImage(filename);
	}

	unsigned long RayTracer::updateWorkerThread(ThreadHandle* handle)
	{
		std::unique_ptr<Sampler> tileSampler = mSampler->clone(handle->getThreadIdx());
		while (mContinueRendering)
		{
			mutex.lock();
			Point2i start = mNextStart;
			Point2i end = mNextStart + Point2i(mTileSize, mTileSize);
			mNextStart.x += mTileSize;
			if (mNextStart.x >= mEndPos.x)
			{
				mNextStart.x = 0;
				end.x = mEndPos.x;
				mNextStart.y += mTileSize;
			}
			mutex.unlock();
			end.y = std::min<s32>(end.y, mEndPos.y);
			traceTile(start, end, *tileSampler);
			++mJobsDone;
		}
		return 0;
	}

	unsigned long updateWorkerThread(ThreadHandle* handle)
	{
		RayTracer* tracer = reinterpret_cast<RayTracer*>(handle->getUserParameter());
		return tracer->updateWorkerThread(handle);
	}

	THREAD_DECL(updateWorkerThread);

	void RayTracer::startWorkerThreads()
	{
		int numThreads = mNumThreads > 0 ? mNumThreads : std::thread::hardware_concurrency();
		fprintf(stderr, "\r[Tracer] Number of threads: %d, spp: %d\n", numThreads, mSpp);
		workerThreads.reserve(numThreads);
		for (size_t i = 0; i < numThreads; ++i)
		{
			ThreadHandle* handle = Threads::createThread(THREAD_GET(updateWorkerThread), i + 1, this);
			workerThreads.push_back(handle);
		}
	}

	void RayTracer::stopRaytracing()
	{
		mContinueRendering = false;
		Threads::waitForThreads(workerThreads);
		std::vector<ThreadHandle*>::iterator it = workerThreads.begin();
		std::vector<ThreadHandle*>::iterator end = workerThreads.end();
		while (it != end)
			delete *it++;
		workerThreads.clear();
	}
}