#include "Photonmap.h"
#include "Material.hpp"
#include "sampling.h"
#include "Scene.hpp"
#include "Intersection.hpp"
#include "Threads.h"
#include "Object.hpp"
#include "sampler.h"
#include "quickselect.h"

namespace tk
{
//the size in pixels of a photon when rendered in opengl
#define PHOTON_GL_POINT_SIZE 1
//Move photons along their normal by this amount when rendered in opengl
//This increases the likelihood of the photon appearing in front of,
//and not behind, the surface it hit.
#define PHOTON_NORMAL_OFFSET (Real(.1))

	PhotonMap::PhotonMap(s32 size, s32 sampleCount)
		: root(0), mStoredPhotons(size), sampleCount(sampleCount),
		geometry_array(0)
	{
		root = (KdTree*)malloc(sizeof(KdTree) * size);
		all_raw_photons.reserve(size);
		for (int i = 0; i < 256; ++i)
		{
			double radians = double(i) * Math::pi / 255.0;
			cosTheta[i] = cos(radians);
			sinTheta[i] = sin(radians);
			cosPhi[i] = cos(2.0 * radians);
			sinPhi[i] = sin(2.0 * radians);
		}	
	}

	PhotonMap::~PhotonMap() {
		all_raw_photons.clear();
		if (root)
			free(root);
		if (geometry_array)
			glDeleteBuffers(1, &geometry_array);
	}

	bool PhotonMap::storePhoton(Vector3f pos, Spectrum power, u8 theta, u8 phi, s32& num)
	{
		if (all_raw_photons.size() >= mStoredPhotons)
			return false;
		
		all_raw_photons.emplace_back(pos, power, theta, phi);
		Union(bbox, pos);
		num++;
		return true;
	}

	void PhotonMap::update_photons()
	{
		if (!geometry_array)
			glGenBuffers(1, &geometry_array);
		float *temp = new float[mStoredPhotons * 6];
		for (size_t i = 0; i < mStoredPhotons; i++)
		{
			Photon& p = all_raw_photons[i];
			Vector3f pos = p.pos;
			float tmp = sinTheta[p.theta];
			Vector3f out = -Vector3f(tmp * cosPhi[p.phi], tmp * sinPhi[p.phi], cosTheta[p.theta]);
			pos += out * PHOTON_NORMAL_OFFSET;
			temp[6 * i] = pos.x;
			temp[6 * i + 1] = pos.y;
			temp[6 * i + 2] = pos.z;
			Spectrum color = all_raw_photons[i].power;
			temp[6 * i + 3] = color.r;
			temp[6 * i + 4] = color.g;
			temp[6 * i + 5] = color.b;
		}
		glBindBuffer(GL_ARRAY_BUFFER, geometry_array);
		glGetError();
		glBufferData(GL_ARRAY_BUFFER, mStoredPhotons * 6 * sizeof(GLfloat), temp, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		delete[] temp;
	}

	void PhotonMap::render_photons()
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, geometry_array);
		glVertexPointer(3, GL_FLOAT, 6 * sizeof(float), (float*)(sizeof(float) * 0));
		glColorPointer(3, GL_FLOAT, 6 * sizeof(float), (float*)(sizeof(float) * 3));
		glPointSize(PHOTON_GL_POINT_SIZE);
		glDrawArrays(GL_POINTS, 0, (GLsizei)mStoredPhotons);
		glDisableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void PhotonMap::buildKdTree() {
		std::vector<Photon*> tmp(mStoredPhotons);
		for (size_t i = 0; i < mStoredPhotons; ++i)
			tmp[i] = &all_raw_photons[i];
		memset(root, 0, sizeof(KdTree) * mStoredPhotons);
		int idx = 0;
		balance(tmp.begin(), tmp.end(), idx);
	}

	Spectrum PhotonMap::radiance_estimate(const Intersection& isect, float& maxRadius2, int& found)const
	{
		NearestPhotons np;
		np.dstPos = isect.p;
		np.found = 0;
		np.neighbor_photons.resize(sampleCount, 0);
		np.dist2 = (float*)malloc(sizeof(float) * (sampleCount + 1));
		np.dist2[sampleCount] = maxRadius2;

		neighbor_search(&np, root, sampleCount);
		maxRadius2 = np.dist2[sampleCount];
		found = np.found;

		Spectrum ret;
		for (int i = 0; i < np.found; ++i)
		{
			Photon *p = np.neighbor_photons[i];
			float tmp = sinTheta[p->theta];
			Vector3f wi = Vector3f(tmp * cosPhi[p->phi], tmp * sinPhi[p->phi], cosTheta[p->theta]);
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(Radiance);
			ret += p->power * AbsDot(wi, isect.n) * m->f(isect.wo, wi, isect.n);
		}
		free(np.dist2);
		return ret;
	}

	KdTree* PhotonMap::balance(std::vector<Photon*>::iterator start,
		std::vector<Photon*>::iterator end, int& idx)
	{
		KdTree* node = &root[idx];
		int dim = bbox.maxExtent();
		std::vector<Photon*>::iterator median = quick_median<std::vector<Photon*>::iterator, Photon*>(start, end,
			[dim](Photon* lhs, Photon* rhs)->bool {
			return lhs->pos[dim] < rhs->pos[dim];
		});
		(*median)->split = dim;
		node->pt = *median;

		if (median > start)
		{
			++idx;
			if (start < median - 1)
			{
				const float tmp = bbox.pMax[dim];
				bbox.pMax[dim] = (*median)->pos[dim];
				node->left = balance(start, median, idx);
				bbox.pMax[dim] = tmp;
			}
			else
			{
				KdTree* child = &root[idx];
				child->pt = *start;
				node->left = child;
			}
		}

		if (++median != end)
		{
			++idx;
			if (median + 1 != end)
			{
				const float tmp = bbox.pMin[dim];
				bbox.pMin[dim] = (*median)->pos[dim];
				node->right = balance(median, end, idx);
				bbox.pMin[dim] = tmp;
			}
			else {
				KdTree* child = &root[idx];
				child->pt = *median;
				node->right = child;
			}
		}
		return node;
	}
}