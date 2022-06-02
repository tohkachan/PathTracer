#ifndef __Tk_Sppm_Tracer_H_
#define __Tk_Sppm_Tracer_H_

#include "TkRayTracer.h"
#include "Photonmap.h"

namespace tk
{
	struct SPPMPixel
	{
		Spectrum Ld;
		Spectrum causticFlux, globalFlux;
		Real causticPhotons, globalPhotons;
		Real causticRadius2, globalRadius2;

		SPPMPixel(Real radius2 = 0.05)
			: causticRadius2(radius2), globalRadius2(radius2),
			causticPhotons(0), globalPhotons(0) {}
	};

	class SppmTracer : public RayTracer
	{
	protected:
		enum eSppmState
		{
			Idle,
			Generate,
			Rendering
		};
		eSppmState state;
		s32 shootGlobalPhotons;
		s32 shootCausticPhotons;
		s32 storedGlobalPhotons;
		s32 storedCausticPhotons;
		s32 numRecurse;
		s32 curRecurse;
		PhotonMap globalPhotonMap;
		PhotonMap causticPhotonMap;		
		Real radius2;
		std::vector<SPPMPixel> pp;

		void visualize();
		void rendering();
		void tracePhoton(u64 haltonIdx);
		void traceCausticPhoton(u64 haltonIdx);
		void traceTile(Point2i start, Point2i end, Sampler& sampler);
	public:
		SppmTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, SPPMParam param);
		void startVisualizing();
		void startRaytracing();
		unsigned long updateWorkerThread(ThreadHandle* handle);
		void render(string filename);
	};
}
#endif