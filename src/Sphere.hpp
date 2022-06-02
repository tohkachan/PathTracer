//
// Created by LEI XU on 5/13/19.
//

#ifndef RAYTRACING_SPHERE_H
#define RAYTRACING_SPHERE_H

#include "Shape.h"
#include "Vector.hpp"

namespace tk
{
	class Sphere : public Shape {
	public:
		Vector3f center;
		float radius, radius2;
		float area;
		Sphere(const Vector3f &c, const float &r);

		bool intersect(const Ray &r, Intersection* isect)const;
		Bounds3 getBounds()const;
		Intersection Sample(const Vector2f& sample, float* pdf)const;
		Intersection Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const;
		float getArea()const { return area; }
		void draw(const Spectrum& c, Real alpha)const;
		void drawOutline(const Spectrum& c, Real alpha)const;
	};
}
#endif