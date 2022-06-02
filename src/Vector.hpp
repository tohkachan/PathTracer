//
// Created by LEI XU on 5/13/19.
//
#ifndef RAYTRACING_VECTOR_H
#define RAYTRACING_VECTOR_H

#include "TkMath.h"

namespace tk
{
	class Vector3f {
	public:
		float x, y, z;
		Vector3f() : x(0), y(0), z(0) {}
		Vector3f(float xx) : x(xx), y(xx), z(xx) {}
		Vector3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
		Vector3f operator * (const float &r) const { return Vector3f(x * r, y * r, z * r); }
		Vector3f operator / (const float &r) const { return Vector3f(x / r, y / r, z / r); }

		float norm() { return std::sqrt(x * x + y * y + z * z); }
		Vector3f normalized() {
			float n = std::sqrt(x * x + y * y + z * z);
			return Vector3f(x / n, y / n, z / n);
		}

		Vector3f operator * (const Vector3f &v) const { return Vector3f(x * v.x, y * v.y, z * v.z); }
		Vector3f operator / (const Vector3f &v) const { return Vector3f(x / v.x, y / v.y, z / v.z); }
		Vector3f operator - (const Vector3f &v) const { return Vector3f(x - v.x, y - v.y, z - v.z); }
		Vector3f operator + (const Vector3f &v) const { return Vector3f(x + v.x, y + v.y, z + v.z); }
		Vector3f operator - () const { return Vector3f(-x, -y, -z); }
		Vector3f& operator += (const Vector3f &v) { x += v.x, y += v.y, z += v.z; return *this; }
		friend Vector3f operator * (const float &r, const Vector3f &v)
		{
			return Vector3f(v.x * r, v.y * r, v.z * r);
		}
		friend std::ostream & operator << (std::ostream &os, const Vector3f &v)
		{
			return os << v.x << ", " << v.y << ", " << v.z;
		}
		double       operator[](int index) const;
		float&      operator[](int index);


		static Vector3f Min(const Vector3f &p1, const Vector3f &p2) {
			return Vector3f(std::min(p1.x, p2.x), std::min(p1.y, p2.y),
				std::min(p1.z, p2.z));
		}

		static Vector3f Max(const Vector3f &p1, const Vector3f &p2) {
			return Vector3f(std::max(p1.x, p2.x), std::max(p1.y, p2.y),
				std::max(p1.z, p2.z));
		}
	};
	inline double Vector3f::operator[](int index) const {
		return (&x)[index];
	}
	inline float& Vector3f::operator[](int index)
	{
		return (&x)[index];
	}


	class Vector2f
	{
	public:
		Vector2f() : x(0), y(0) {}
		Vector2f(float xx) : x(xx), y(xx) {}
		Vector2f(float xx, float yy) : x(xx), y(yy) {}
		Vector2f operator * (const float &r) const { return Vector2f(x * r, y * r); }
		Vector2f operator + (const Vector2f &v) const { return Vector2f(x + v.x, y + v.y); }
		Vector2f operator - (const Vector2f &v) const { return Vector2f(x - v.x, y - v.y); }
		float x, y;
	};

	inline Vector3f lerp(const Vector3f &a, const Vector3f& b, const float &t)
	{
		return a * (1 - t) + b * t;
	}

	inline Vector3f normalize(const Vector3f &v)
	{
		float mag2 = v.x * v.x + v.y * v.y + v.z * v.z;
		if (mag2 > 0) {
			float invMag = 1 / sqrtf(mag2);
			return Vector3f(v.x * invMag, v.y * invMag, v.z * invMag);
		}

		return v;
	}

	inline float dotProduct(const Vector3f &a, const Vector3f &b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline Vector3f crossProduct(const Vector3f &a, const Vector3f &b)
	{
		return Vector3f(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	inline Vector3f Abs(const Vector3f &v)
	{
		return Vector3f(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z));
	}

	inline float AbsDot(const Vector3f &a, const Vector3f &b)
	{
		return std::fabs(a.x * b.x + a.y * b.y + a.z * b.z);
	}


	inline void CoordinateSystem(const Vector3f &v1, Vector3f *v2,
		Vector3f *v3) {
		float sign = std::copysign(1.0f, v1.z);
		float a = -1 / (sign + v1.z);
		float b = v1.x * v1.y * a;
		*v2 = Vector3f(1 + sign * v1.x * v1.x * a, sign * b, -sign * v1.x);
		*v3 = Vector3f(b, sign + v1.y * v1.y * a, -v1.y);
	}


	class Point2i
	{
	public:
		int x, y;
		Point2i() : x(0), y(0) {}
		Point2i(int x, int y) : x(x), y(y) {}

		s32  operator[](s32 index)const { return index == 0 ? x : y; }
		s32&  operator[](s32 index) { return index == 0 ? x : y; }

		Point2i& operator=(const Point2i& p)
		{
			x = p.x;
			y = p.y;
			return *this;
		}

		Point2i operator+(const Point2i& rhs)const
		{
			return Point2i(x + rhs.x, y + rhs.y);
		}

		Point2i& operator+=(const Point2i& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		Point2i operator-(const Point2i& rhs)const
		{
			return Point2i(x - rhs.x, y - rhs.y);
		}

		Point2i& operator-=(const Point2i& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		bool operator==(const Point2i& rhs)const
		{
			return x == rhs.x && y == rhs.y;
		}

		bool operator!=(const Point2i& rhs)const
		{
			return x != rhs.x || y != rhs.y;
		}
	};

	class Bounds2i
	{
	public:
		Point2i pMin, pMax;
		Bounds2i()
		{
			int minNum = std::numeric_limits<int>::lowest();
			int maxNum = std::numeric_limits<int>::max();
			pMin = Point2i(maxNum, maxNum);
			pMax = Point2i(minNum, minNum);
		}
		explicit Bounds2i(const Point2i& p) : pMin(p), pMax(p) {}
		Bounds2i(const Point2i& p1, const Point2i& p2)
		{
			pMin = Point2i(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
			pMax = Point2i(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
		}

		bool operator==(const Bounds2i& b)const
		{
			return b.pMin == pMin && b.pMax == pMax;
		}

		bool operator!=(const Bounds2i& b)const
		{
			return b.pMin != pMin || b.pMax != pMax;
		}

		Point2i diagonal()const { return pMax - pMin; }

		int area()const
		{
			Point2i d = pMax - pMin;
			return d.x * d.y;
		}

		int maxExtent()const
		{
			Point2i d = pMax - pMin;
			if (d.x > d.y)
				return 0;
			else
				return 1;
		}

		void intersect(const Bounds2i& b)
		{
			pMin = Point2i(std::max(pMin.x, b.pMin.x), std::max(pMin.y, b.pMin.y));
			pMax = Point2i(std::min(pMax.x, b.pMax.x), std::min(pMax.y, b.pMax.y));
		}
	};
}

#endif