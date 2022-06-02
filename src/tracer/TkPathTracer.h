#ifndef __Tk_Path_Tracer_H_
#define __Tk_Path_Tracer_H_

#include "TkRayTracer.h"

namespace tk
{
	class PathTracer : public RayTracer
	{
	protected:
		std::vector<s32> mSelectionHistory;
		void visualize();
		void rendering();
		Spectrum Li(Ray& r, Sampler& sampler, s32 depth = 0)const;
		void traceTile(Point2i start, Point2i end, Sampler& sampler);
	public:
		PathTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette);
		void keyPress(s32 key);
	};
}
#endif