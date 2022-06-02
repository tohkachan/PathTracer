#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "TkPrerequisites.h"
#include "TkFilm.h"
#include "Threads.h"
#include "TkConfigure.h"

namespace tk
{
	class RayTracer
	{
	protected:
		enum eState
		{
			INIT,
			READY,
			VISUALIZE,
			RENDERING,
			DONE
		};
		eState mState;
		const Scene* mScene;
		Camera* mCamera;
		Film* mFilm;
		u32* mFrameBuffer;

		std::vector<ThreadHandle*> workerThreads;
		LwMutex mutex, mutex1;
		bool mContinueRendering;
		Point2i mNextStart;
		Point2i mEndPos;
		s32 mTileSize;
		s32 mNumThreads;
		s32 mJobsDone;
		s32 mJobsCount;
		s32 mSpp;
		s32 mMaxDepth;
		Real mRussianRoulette;
		Sampler* mSampler;
		virtual void visualize() = 0;
		virtual void rendering() = 0;
		void startWorkerThreads();
		void stopRaytracing();
		virtual void traceTile(Point2i start, Point2i end, Sampler& sampler) = 0;
	public:
		RayTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette);
		virtual ~RayTracer();
		void setScene(Scene* scene);
		void setCamera(Camera* camera);
		void updateScreen();
		void stop();
		void clear();
		virtual void startVisualizing();
		virtual void startRaytracing();
		virtual void keyPress(s32 key) {}
		void saveImage(string filename = "");
		virtual unsigned long updateWorkerThread(ThreadHandle* handle);
		virtual void render(string filename);
	};
}
#endif