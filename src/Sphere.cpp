#include "Sphere.hpp"
#include "Intersection.hpp"
#include "EFloat.h"
#include "Bounds3.hpp"
#include "sampling.h"
#include "sphere_drawing.h"

namespace tk
{
	Sphere::Sphere(const Vector3f &c, const float &r)
		: center(c), radius(r), radius2(r * r), area(4 * Math::pi *r *r)
	{}

	bool Sphere::intersect(const Ray &r, Intersection* isect)const
	{
		Vector3f pHit;

		EFloat ox(r.origin.x), oy(r.origin.y), oz(r.origin.z);
		EFloat cx(center.x), cy(center.y), cz(center.z);
		ox = ox - cx;
		oy = oy - cy;
		oz = oz - cz;
		EFloat dx(r.direction.x), dy(r.direction.y), dz(r.direction.z);
		EFloat a = dx * dx + dy * dy + dz * dz;
		EFloat b = (dx * ox + dy * oy + dz * oz) * 2.0f;
		EFloat c = ox * ox + oy * oy + oz * oz - EFloat(radius) * EFloat(radius);

		EFloat t0, t1;
		if (!EFloat::quadratic(a, b, c, t0, t1)) return false;

		if (t0.upperBound() > r.t_max || t1.lowerBound() <= 0) return false;
		EFloat hit = t0;
		if (hit.lowerBound() <= 0)
		{
			hit = t1;
			if (hit.upperBound() > r.t_max) return false;
		}

		pHit = r((float)hit);

		Vector3f dir = pHit - center;
		dir = dir * radius / dir.norm();
		Vector3f pError = Abs(dir) * Math::Gamma(5);

		pHit = dir + center;

		pError = pError * (1 + Math::Gamma(1)) + (Abs(dir) + Abs(center)) * Math::Gamma(1);

		isect->p = pHit;
		isect->pError = pError;
		isect->wo = normalize(-r.direction);
		isect->n = normalize(pHit - center);
		r.t_max = double(hit);
		return true;

		/*Vector3f L = ray.origin - center;
		float a = dotProduct(ray.direction, ray.direction);
		float b = 2 * dotProduct(ray.direction, L);
		float c = dotProduct(L, L) - radius2;
		float t0, t1;
		if (!solveQuadratic(a, b, c, t0, t1)) return result;
		if (t0 < 0) t0 = t1;
		if (t0 < 0) return result;
		result.happened=true;

		result.coords = Vector3f(ray.origin + ray.direction * t0);
		result.normal = normalize(Vector3f(result.coords - center));
		result.m = this->m;
		result.obj = this;
		result.distance = t0;
		return result;*/

	}


	Bounds3 Sphere::getBounds()const
	{
		return Bounds3(Vector3f(center.x - radius, center.y - radius, center.z - radius),
			Vector3f(center.x + radius, center.y + radius, center.z + radius));
	}

	Intersection Sphere::Sample(const Vector2f& sample, float* pdf)const
	{
		Vector3f n = uniformSampleSphere(sample);
		Vector3f p = n * radius;
		Intersection ret;
		ret.n = n;
		ret.p = p + center;
		ret.pError = Abs(p) * Math::Gamma(2) + Abs(center) * Math::Gamma(1);
		*pdf = 1 / area;
		return ret;
	}

	Intersection Sphere::Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const
	{
		return Intersection();
	}

	void Sphere::draw(const Spectrum& c, Real alpha)const
	{
		draw_sphere_opengl(center, radius, c);
	}

	void Sphere::drawOutline(const Spectrum& c, Real alpha)const
	{
		draw_sphere_wireframe_opengl(center, radius, c);
	}
}


