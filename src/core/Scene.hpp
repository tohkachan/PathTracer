//
// Created by Göksu Güvendiren on 2019-05-14.
//

#pragma once

#include "AreaLight.hpp"
#include "BVH.hpp"

namespace tk
{
	typedef std::shared_ptr<AreaLight> SharedLight;
	typedef std::vector<SharedLight> SharedLightVec;
	typedef std::vector<Object*> ObjectPtrVec;
	class Scene
	{
	private:
		std::unique_ptr<BVHAccel> bvh;
		ObjectPtrVec objects;
		SharedLightVec lights;
	public:
		Scene(){}
		~Scene();

		void Add(Object* obj) { objects.push_back(obj); }
		void Add(const SharedLight& light) { lights.push_back(std::move(light)); }

		const ObjectPtrVec& get_objects() const { return objects; }
		const SharedLightVec&  get_lights() const { return lights; }
		bool intersect(const Ray &r, Intersection* isect)const;
		bool intersectP(const Ray& r)const;
		void reset();	
		void buildBVH();
		BVHAccel* getBVH()const { return bvh.get(); }
		Spectrum uniformSampleOneLight(const Intersection& it, Sampler& sampler)const;
	};
}

