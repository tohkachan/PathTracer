#ifndef PHOTONMAP_H
#define PHOTONMAP_H

#include "TkPrerequisites.h"
#include "neighbor.h"
#include "Bounds3.hpp"
#include <GL/glew.h>

#include "Threads.h"

namespace tk
{ 
	class PhotonMap {
	private:
		std::vector<Photon> all_raw_photons;
		GLuint geometry_array;
		s32 mStoredPhotons;
		s32 sampleCount;

		float cosTheta[256];
		float sinTheta[256];
		float cosPhi[256];
		float sinPhi[256];

		KdTree* root;
		Bounds3 bbox;

		KdTree* balance(std::vector<Photon*>::iterator start,
			std::vector<Photon*>::iterator end, int& idx);
	public:
		PhotonMap(s32 size, s32 sampleCount);
		~PhotonMap();
		s32 size()const { return mStoredPhotons; }
		void reset() { all_raw_photons.clear(); }
		bool storePhoton(Vector3f pos, Spectrum power, u8 theta, u8 phi, s32& num);
		void update_photons();
		/// Organize the photons into some sort of kd-tree
		void buildKdTree();
		void render_photons();
		Spectrum radiance_estimate(const Intersection& isect, float& maxRadius2, int& found)const;
	};
}

#endif