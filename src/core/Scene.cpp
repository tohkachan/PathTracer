//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include "Object.hpp"
#include "sampler.h"
#include "Material.hpp"
#include "Intersection.hpp"

namespace tk
{
	Scene::~Scene()
	{
		reset();
	}

	void Scene::buildBVH() {
		this->bvh = std::unique_ptr<BVHAccel>(new BVHAccel(objects, 1));
	}

	bool Scene::intersect(const Ray &r, Intersection* isect)const
	{
		return this->bvh->intersect(r, isect);
	}

	bool Scene::intersectP(const Ray& r)const
	{
		Intersection tmp;
		return this->bvh->intersect(r, &tmp);
	}

	void Scene::reset()
	{
		ObjectPtrVec::iterator it = objects.begin();
		ObjectPtrVec::iterator end = objects.begin();
		while (it != end)
			delete *it++;
		objects.clear();
		lights.clear();
	}

	Spectrum Scene::uniformSampleOneLight(const Intersection& it, Sampler& sampler)const
	{
		s32 numLights = lights.size();
		if (numLights == 0) return Spectrum::black;

		s32 idx = std::min<s32>(sampler.get1D() * numLights, numLights - 1);
		Real pdfChoice = 1.0f / numLights;

		const std::shared_ptr<AreaLight>& l = lights[idx];
		Vector2f uLight = sampler.get2D();
		Vector3f ws;
		Real pdfLight;
		Intersection isect;
		Spectrum Li = l->sample_Li(it, sampler.get1D(), uLight, &isect, &ws, &pdfLight);
		if (Li == Spectrum::black || intersectP(it.spawnRayTo(isect)))
			return Spectrum::black;
		const Material* m = it.obj->getMaterial();
		return Li * m->f(it.wo, ws, it.n) * AbsDot(ws, it.n) / (pdfLight * pdfChoice);
	}
}
