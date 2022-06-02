#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include "Vector.hpp"
#include "TkSpectrum.h"

namespace tk
{
	struct Photon
	{
		Vector3f pos;
		Spectrum power;
		u8 theta, phi;
		s16 split = 4;
		Photon() {}
		Photon(Vector3f pos, Spectrum power, u8 theta, u8 phi)
			: pos(pos), power(power), theta(theta), phi(phi) {}
	};

	struct KdTree
	{
		Photon* pt;
		KdTree* left;
		KdTree* right;
		KdTree() : pt(0), left(0), right(0) {}
	};

	struct NearestPhotons
	{
	public:
		s32 found;
		Vector3f dstPos;
		float *dist2;
		std::vector<Photon*> neighbor_photons;
	};

	void neighbor_search(NearestPhotons* np, KdTree* node, s32 sampleCount);
}

#endif