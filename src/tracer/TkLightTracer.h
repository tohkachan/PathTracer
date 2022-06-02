#ifndef __Tk_Light_Tracer_H_
#define __Tk_Light_Tracer_H_

#include "TkRayTracer.h"
#include "Intersection.hpp"

namespace tk
{
	struct PathVertex
	{
		Spectrum throughput;
		Intersection isect;
		const AreaLight* light;
		const Camera* camera;
		Real pdfFwd;
		Real pdfRev;
		bool delta;
		bool isSurface;

		PathVertex() : throughput(Spectrum::black), light(nullptr), camera(nullptr), delta(false) {}

		PathVertex(const Spectrum& t, const Vector3f& p, const Vector3f& n, const AreaLight* l)
			: throughput(t), light(l)
		{
			isect.p = p;
			isect.n = n;
		}

		PathVertex(const Spectrum& t, const Vector3f& p, const Vector3f& n, const Camera* c)
			: throughput(t), camera(c)
		{
			isect.p = p;
			isect.n = n;
		}
	};
	typedef std::vector<PathVertex> RayPath;
	class LightTracer : public RayTracer
	{
	private:
		void visualize();
		void rendering();
		void traceTile(Point2i start, Point2i end, Sampler& sampler);
		s32 generateLightSubpath(RayPath& path, Sampler& s);
		s32 generateCameraSubpath(RayPath& path, Sampler& s, Point2i pixel);
		void splatFilmT1(Point2i pixel, Sampler& sampler, RayPath& path);
		void splatFilmS1(Point2i pixel, Sampler& sampler, RayPath& path);
	public:
		LightTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette);
	};
}
#endif