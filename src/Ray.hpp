//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_RAY_H
#define RAYTRACING_RAY_H
#include "Vector.hpp"

namespace tk
{
	struct Ray {
		//Destination = origin + t*direction
		Vector3f origin;
		Vector3f direction, direction_inv;
		double t;//transportation time,
		mutable double t_max;

		Ray() = default;

		Ray(const Vector3f& ori, const Vector3f& dir, const double _t = 0.0) : origin(ori), direction(dir), t(_t) {
			direction_inv = Vector3f(1. / direction.x, 1. / direction.y, 1. / direction.z);
			t_max = std::numeric_limits<double>::max();

		}

		Vector3f operator()(float t) const { return origin + direction * t; }

		friend std::ostream &operator<<(std::ostream& os, const Ray& r){
			os<<"[origin:="<<r.origin<<", direction="<<r.direction<<", time="<< r.t<<"]\n";
			return os;
		}

		void enterVolume(const Material* b) { m_volumes.push_back(b); }
		void exitVolume(const Material* b)
		{
			m_volumes.erase(std::remove(m_volumes.begin(), m_volumes.end(), b),
				m_volumes.end());
		}

		std::vector<const Material*> m_volumes;
	};

	inline Vector3f offsetRayOrigin(const Vector3f& p, const Vector3f& pError,
		const Vector3f& n, const Vector3f& w)
	{
		float d = dotProduct(Abs(n), pError);
		Vector3f offset = d * n;
		if (dotProduct(w, n) < 0)
			offset = -offset;
		Vector3f po = p + offset;
		for (int i = 0; i < 3; ++i)
		{
			if (offset[i] > 0) po[i] = NextFloatUp(po[i]);
			else if (offset[i] < 0) po[i] = NextFloatDown(po[i]);
		}
		return po;
	}
}
#endif //RAYTRACING_RAY_H
