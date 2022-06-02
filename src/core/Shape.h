#ifndef SHAPE_H
#define SHAPE_H

#include "TkPrerequisites.h"

namespace tk
{
	class Shape
	{
	public:
		Shape() = default;
		virtual bool intersect(const Ray &r, Intersection* isect)const = 0;
		virtual Bounds3 getBounds()const = 0;
		virtual float getArea()const = 0;
		virtual Intersection Sample(const Vector2f& sample, float* pdf)const = 0;
		virtual Intersection Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const = 0;
		virtual float Pdf(const Intersection&)const { return 1 / getArea(); }

		virtual void draw(const Spectrum& c, Real alpha)const = 0;
		virtual void drawOutline(const Spectrum& c, Real alpha)const = 0;
	};
}

#endif