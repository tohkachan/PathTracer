#ifndef __Tk_Bdpt_Tracer_H_
#define __Tk_Bdpt_Tracer_H_

#include "TkRayTracer.h"
#include "Material.hpp"

namespace tk
{
	enum eVertexType
	{
		CAMERA,
		LIGHT,
		SURFACE
	};
	struct PathVertex;
	typedef std::vector<PathVertex> RayPath;
	class BdptTracer : public RayTracer
	{
	protected:
		s32 mDebugS;
		s32 mDebugT;
		bool mDebugNoMIS;

		void visualize();
		void rendering();
		s32 generateCameraSubpath(s32 minLength, const Ray& r, RayPath& path, Sampler& s);
		s32 generateLightSubpath(s32 minLength, RayPath& path, Sampler& s);
		s32 pathWalk(Spectrum beta, Ray r, s32 minDepth, Real pdfFwd, RayPath& path,
			Sampler& s, eTransportMode mode);
		Spectrum connectPath(RayPath& eyePath, RayPath& lightPath, s32 s, s32 t,
			Sampler& sampler, Vector2f *raster);
		Real G(const Intersection& p0, const Intersection& p1)const;
		Real MISWeight(RayPath& eyePath, RayPath& lightPath, s32 s, s32 t,
			PathVertex& sampled);
		void traceTile(Point2i start, Point2i end, Sampler& sampler);
	public:
		BdptTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, s32 debugS, s32 debugT,
			bool debugNoMIS);
	};
}
#endif