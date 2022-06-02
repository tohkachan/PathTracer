//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H

#include "Ray.hpp"

namespace tk
{
	struct Intersection
	{
		Vector3f p;
		Vector3f pError;
		Vector2f uv;
		Vector3f wo;
		Vector3f n;
		const Object* obj;
		Intersection() {}
		Intersection(const Vector3f& p, const Vector3f& n, const Vector2f& uv,
			const Vector3f& pError, const Vector3f& wo)
			: p(p), pError(pError), n(n), uv(uv), wo(wo) {}

		Ray spawnRay(const Vector3f& dir)const
		{
			Vector3f o = offsetRayOrigin(p, pError, n, dir);
			return Ray(o, dir);
		}

		Ray spawnRayTo(const Vector3f& pos)const
		{
			Vector3f o = offsetRayOrigin(p, pError, n, pos - p);
			Vector3f d = pos - o;
			Ray ret(o, d);
			ret.t_max = Math::one_minus_epsilon;
			return ret;
		}

		Ray spawnRayTo(const Intersection& other)const
		{
			Vector3f o = offsetRayOrigin(p, pError, n, other.p - p);
			Vector3f target = offsetRayOrigin(other.p, other.pError, other.n, o - other.p);
			Vector3f d = target - o;
			Ray ret(o, d);
			ret.t_max = Math::one_minus_epsilon;
			return ret;
		}

		Spectrum Le(const Vector3f& w)const;
	};
}

#endif