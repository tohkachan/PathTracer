#ifndef __Tk_SPPM_H_
#define __Tk_SPPM_H_

#include "TkRayTracer.h"
#include "Intersection.hpp"
#include "TkSpectrum.h"

namespace tk
{
	struct PixelData
	{
		// Data that got reset after each photon pass iteration
		// accumulation of fs * Phi_p (photon contribution)
		Spectrum Phi;
		// Number of photon hit this pixel in this photon pass
		s32 M;

		Intersection isect;
		Spectrum throughput;
		// Accumulated direct radiance contribution
		Spectrum Ld;
		// Tau_i+1 = (Tau_i + Phi_i) * (Ri+1 / Ri)^2
		Spectrum Tau;
		// Photons that stays through the whole sppm iteration cycle
		Real N;
		// Current photon search radius
		Real radius;

		PixelData(Real radius = 0.02)
			: M(0), N(0), radius(radius) {}
	};

	class SpatialHashGrids;
	class SPPM : public RayTracer
	{
	private:
		enum ePmState
		{
			RayPass,
			PhotonPass,
			RenewPass,
			Idle
		};
		Real radius;
		s32 numIteration;
		s32 photonsPerIt;
		s32 photonStart;
		s32 pixelStart;
		s32 curIteration;
		ePmState state;
		SpatialHashGrids* mHashGrids;
		std::vector<PixelData> mPixels;

		void visualize();
		void rendering();
		void tracePhoton(s32 start, s32 end);
		void traceTile(Point2i start, Point2i end, Sampler& sampler);
		void updatePixel(s32 start, s32 end);
	public:
		SPPM(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, SPPMParam param);
		void startRaytracing();
		unsigned long updateWorkerThread(ThreadHandle* handle);
		void render(string filename);
	};
}
#endif