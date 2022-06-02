#ifndef QUATERNION_H
#define QUATERNION_H

#include "Vector.hpp"

namespace tk
{
	class Quaternion
	{
	private:
		float w, x, y, z;
	public:
		inline Quaternion() : w(1.0f), x(0), y(0), z(0) {}
		inline Quaternion(const Quaternion& other)
			: w(other.w), x(other.x), y(other.y), z(other.z)
		{}
		inline Quaternion(Real w, Real x, Real y, Real z) : w(w), x(x), y(y), z(z) {}
		inline Quaternion(const float& theta, const Vector3f& rotationAxis)
		{
			setByAngleAxis(theta, rotationAxis);
		}
		inline void swap(Quaternion& other)
		{
			std::swap(w, other.w);
			std::swap(x, other.x);
			std::swap(y, other.y);
			std::swap(z, other.z);
		}
		/// Operator
		inline Quaternion operator-()const
		{
			return Quaternion(-w, -x, -y, -z);
		}
		inline Quaternion& operator=(const Quaternion& rhs)
		{
			w = rhs.w;
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			return *this;
		}
		inline Quaternion& operator*=(const Real scalar)
		{
			w *= scalar;
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		inline Quaternion operator*(const Quaternion& rhs)const
		{
			return Quaternion(
				w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
				w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
				w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
				w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w);
		}
		inline Quaternion operator*(const Real scalar)const
		{
			return Quaternion(scalar * w, scalar * x, scalar * y, scalar * z);
		}
		inline Quaternion operator+(const Quaternion& rhs)const
		{
			return Quaternion(rhs.w + w, rhs.x + x, rhs.y + y, rhs.z + z);
		}
		inline Quaternion operator-(const Quaternion& rhs)const
		{
			return Quaternion(w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z);
		}
		Vector3f operator*(const Vector3f& v)const;
		inline bool operator==(const Quaternion& other)const
		{
			return (w == other.w) && (x == other.x) &&
				(y == other.y) && (z == other.z);
		}
		inline bool operator!=(const Quaternion& other)const
		{
			return (w != other.w) || (x != other.x) ||
				(y != other.y) || (z != other.z);
		}
		/// Function
		Real dot(const Quaternion& other)const
		{
			return w * other.w + x * other.x + y * other.y + y * other.y;
		}
		Quaternion& normalize()
		{
			Real l = std::sqrt(w * w + x * x + y * y + z * z);
			Real invL = 1.0f / l;
			return (*this *= invL);
		}
		Quaternion conjugate(void)const
		{
			return Quaternion(w, -x, -y, -z);
		}
		Quaternion inverse(void)const
		{
			Real norm = w * w + x * x + y * y + z * z;
			if (norm > 0.0f)
			{
				Real invNorm = 1.0f / norm;
				return Quaternion(w * invNorm, -x * invNorm, -y * invNorm, -z * invNorm);
			}
			else
				return zero;
		}
		void setByAngleAxis(const float& theta, const Vector3f& rotationAxis);
		/// Set new quaternion based on euler angles.
		void setByEuler(const Real& x, const Real& y, const Real& z);
		/// Calculate from quaternion to rotation matrix.
		void getMatrix(Matrix4& outMat)const;
		/// Calculate from quaternion to rotation transposed matrix.
		void getTransposeMatrix(Matrix4& outMat)const;
		Vector3f xAxis(void)const;
		Vector3f yAxis(void)const;
		Vector3f zAxis(void)const;
		/// Get the quaternion which rotate the current direction to the target direction.
		static Quaternion rotation(const Vector3f& from, const Vector3f& to);
		/// Linear iterpolation function.
		static Quaternion nlerp(const Quaternion& q1, const Quaternion& q2, Real t, bool shortestPath = false);
		/// Spherical linear iterpolation function.
		static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, Real t, bool shortestPath = false);
		/// Spherical quadratic interplkation function.
		/*static Quaternion squad(const Quaternion& q1, const Quaternion& q2, const Quaternion& q3,
			const Quaternion& q4, Real t, bool shortestPath = false);*/


		static const Quaternion zero;
		static const Quaternion identity;
	};
}

#endif