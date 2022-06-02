//
// Created by LEI XU on 5/13/19.
//
#ifndef RAYTRACING_OBJECT_H
#define RAYTRACING_OBJECT_H

#include "TkPrerequisites.h"
#include "Intersection.hpp"

namespace tk
{
	class Object
	{
	private:
		std::shared_ptr<Shape> mShape;
		std::shared_ptr<AreaLight> mAreaLight;
		std::shared_ptr<Material> mMaterial;
	public:
		Object(const std::shared_ptr<Shape>& shape, const std::shared_ptr<AreaLight>& areaLight,
			const std::shared_ptr<Material>& material);
		bool intersect(const Ray &r, Intersection* isect)const;
		Bounds3 getBounds()const;
		Shape* getShape()const;
		const AreaLight* getAreaLight()const;
		const Material* getMaterial()const;

		void draw(const Spectrum& c, Real alpha)const;
		void drawOutline(const Spectrum& c, Real alpha)const;
	};
}

#endif