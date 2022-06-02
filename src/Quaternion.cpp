#include "Quaternion.h"

#include "Matrix4.h"

namespace tk
{
	const Quaternion Quaternion::zero(0, 0, 0, 0);
	const Quaternion Quaternion::identity(1, 0, 0, 0);

	Vector3f Quaternion::operator*(const Vector3f& vec)const
	{
		//qPq-1 = w^2 * P + 2w * v¡ÁP + (v¡¤P)v - v¡ÁP¡Áv
		//v¡ÁP¡Áv = v^2 * P - (v¡¤P)v
		//qPq-1 = (w^2 - v^2)P + 2w * v¡ÁP + 2(v¡¤P)v	(1)
		//qPq-1 = P + 2w * v¡ÁP + 2 * v¡Á(v¡ÁP)			(2)
		//v = (x, y ,z), P is vector position
		Vector3f vP, vvP;
		Vector3f v(x, y, z);
		vP = crossProduct(v, vec);
		vvP = crossProduct(v, vP);
		vP = vP * 2.0f * w;
		vvP = vvP * 2.0f;
		return vec + vP + vvP;
		/*
		vec3 vCrossP;
		f32 vP, vv;
		vec3 v(x, y, z);
		vCrossP = v.crossProduct(vec);
		vP = v.dotProduct(vec) * 2.0f;
		vv = w * w - v.getLengthSQ();
		vCrossP *= 2.0f * w;
		return vec * vv + vCrossP + v * vP;
		*/

	}

	void Quaternion::setByAngleAxis(const float& theta, const Vector3f& axis)
	{
		float halfTheta(theta * 0.5f);
		Real s = std::sin(halfTheta);
		w = std::cos(halfTheta);
		x = axis.x * s;
		y = axis.y * s;
		z = axis.z * s;
	}

	void Quaternion::setByEuler(const Real& x, const Real& y, const Real& z)
	{
		// pitch = (cp, sp, 0, 0)
		Real pitch(x * 0.5);
		Real sp = std::sin(pitch);
		Real cp = std::cos(pitch);
		// yaw = (cy, 0, sy, 0)
		Real yaw(y * 0.5);
		Real sy = std::sin(yaw);
		Real cy = std::cos(yaw);
		// roll = (cr, 0, 0, sr)
		Real roll(z * 0.5);
		Real sr = std::sin(roll);
		Real cr = std::cos(roll);
		// roll * yaw = (crcy, -srsy, crsy, srcy)
		Real crcy = cr * cy;
		Real srsy = sr * sy;
		Real crsy = cr * sy;
		Real srcy = sr * cy;

		w = crcy * cp + srsy * sp;
		this->x = crcy * sp - srsy * cp;
		this->y = crsy * cp + srcy * sp;
		this->z = srcy * cp - crsy * sp;
		normalize();
	}

	void Quaternion::getMatrix(Matrix4& m)const
	{
		Real fTx = x + x;
		Real fTy = y + y;
		Real fTz = z + z;
		Real fTwx = fTx * w;
		Real fTwy = fTy * w;
		Real fTwz = fTz * w;
		Real fTxx = fTx * x;
		Real fTxy = fTy * x;
		Real fTxz = fTz * x;
		Real fTyy = fTy * y;
		Real fTyz = fTz * y;
		Real fTzz = fTz * z;

		m[0][0] = 1.0f - (fTyy + fTzz);
		m[0][1] = fTxy - fTwz;
		m[0][2] = fTxz + fTwy;
		m[0][3] = 0.0f;

		m[1][0] = fTxy + fTwz;
		m[1][1] = 1.0f - (fTxx + fTzz);
		m[1][2] = fTyz - fTwx;
		m[1][3] = 0.0f;

		m[2][0] = fTxz - fTwy;
		m[2][1] = fTyz + fTwx;
		m[2][2] = 1.0f - (fTxx + fTyy);
		m[2][3] = 0.0f;

		m[3][0] = 0.0f;
		m[3][1] = 0.0f;
		m[3][2] = 0.0f;
		m[3][3] = 1.0f;
	}

	void Quaternion::getTransposeMatrix(Matrix4& m)const
	{
		Real fTx = x + x;
		Real fTy = y + y;
		Real fTz = z + z;
		Real fTwx = fTx * w;
		Real fTwy = fTy * w;
		Real fTwz = fTz * w;
		Real fTxx = fTx * x;
		Real fTxy = fTy * x;
		Real fTxz = fTz * x;
		Real fTyy = fTy * y;
		Real fTyz = fTz * y;
		Real fTzz = fTz * z;

		m[0][0] = 1.0f - (fTyy + fTzz);
		m[0][1] = fTxy + fTwz;
		m[0][2] = fTxz - fTwy;
		m[0][3] = 0.0f;

		m[1][0] = fTxy - fTwz;
		m[1][1] = 1.0f - (fTxx + fTzz);
		m[1][2] = fTyz + fTwx;
		m[1][3] = 0.0f;

		m[2][0] = fTxz + fTwy;
		m[2][1] = fTyz - fTwx;
		m[2][2] = 1.0f - (fTxx + fTyy);
		m[2][3] = 0.0f;

		m[3][0] = 0.0f;
		m[3][1] = 0.0f;
		m[3][2] = 0.0f;
		m[3][3] = 1.0f;
	}

	Vector3f Quaternion::xAxis(void)const
	{
		Real fTy = 2.0f * y;
		Real fTz = 2.0f * z;
		Real fTwy = fTy * w;
		Real fTwz = fTz * w;
		Real fTxy = fTy * x;
		Real fTxz = fTz * x;
		Real fTyy = fTy * y;
		Real fTzz = fTz * z;

		return Vector3f(1.0f - (fTyy + fTzz), fTxy + fTwz, fTxz - fTwy);
	}

	Vector3f Quaternion::yAxis(void)const
	{
		Real fTx = 2.0f * x;
		Real fTy = 2.0f * y;
		Real fTz = 2.0f * z;
		Real fTwx = fTx * w;
		Real fTwz = fTz * w;
		Real fTxx = fTx * x;
		Real fTxy = fTx * y;
		Real fTyz = fTz * y;
		Real fTzz = fTz * z;

		return Vector3f(fTxy - fTwz, 1.0f - (fTxx + fTzz), fTyz + fTwx);
	}

	Vector3f Quaternion::zAxis(void)const
	{
		Real fTx = 2.0f * x;
		Real fTy = 2.0f * y;
		Real fTz = 2.0f * z;
		Real fTwx = fTx * w;
		Real fTwy = fTy * w;
		Real fTxx = fTx * x;
		Real fTxz = fTz * x;
		Real fTyy = fTy * y;
		Real fTyz = fTz * y;

		return Vector3f(fTxz + fTwy, fTyz - fTwx, 1.0f - (fTxx + fTyy));
	}

	Quaternion Quaternion::rotation(const Vector3f& from, const Vector3f& to)
	{
		const f32 cosTheta = dotProduct(from, to);
		if (cosTheta == 1.0f)
			return identity;
		else if (cosTheta == -1.0f)
		{
			Vector3f axis(1.f, 0.0f, 0.0f);
			axis = crossProduct(axis, from);
			if (dotProduct(axis, axis) == 0.0f)
			{
				axis = { 0.0f, 1.f, 0.0f };
				axis = crossProduct(axis, from);
			}
			return Quaternion(0, axis.x, axis.y, axis.z).normalize();
		}
		//c = 2 * cos(theta / 2)
		Real c = sqrtf((1 + cosTheta) * 2);
		Real invs = 1.0f / c;
		Vector3f v = crossProduct(from, to) * invs;
		return Quaternion(c * 0.5f, v.x, v.y, v.z).normalize();
	}

	Quaternion Quaternion::nlerp(const Quaternion& q1, const Quaternion& q2, Real t, bool shortestPath)
	{
		Quaternion ret;
		Real cosTheta = q1.dot(q2);
		if (cosTheta < 0.0f && shortestPath)
			ret = q1 + ((-q2) - q1) * t;
		else
			ret = q1 + (q2 - q1) * t;
		return ret.normalize();
	}

	Quaternion Quaternion::slerp(const Quaternion& q1, const Quaternion& _q2, Real t, bool shortestPath)
	{
		Quaternion q2 = _q2;
		Real cosTheta = q1.dot(_q2);
		//since the q and -q represent the same rotation, the signs of
		//the q1 an q2 are usually chosen such that q1 dot q2 >= 0, 
		//this also ensures that the interpolation takes place over the
		//shortest path
		if (cosTheta < 0.0f && shortestPath)
		{
			cosTheta = -cosTheta;
			q2 = -_q2;
		}
		if (std::abs(cosTheta) < 1 - 1e-03)
		{
			Real sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
			Real theta = std::atan2(sinTheta, cosTheta);
			Real invSinTheta = 1.0f / sinTheta;
			Real scale = std::sin(theta * (1.0f - t)) * invSinTheta;
			Real invScale = std::sin(theta * t) * invSinTheta;
			return q1 * scale + q2 * invScale;
		}
		else
			return (q1 + (q2 - q1) * t).normalize();
	}
}