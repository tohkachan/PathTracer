#ifndef MATRIX4_H
#define MATRIX4_H

#include "Quaternion.h"

namespace tk
{
	class Matrix4
	{
	protected:
		union {
			float m[16];
			//_m[row][col]
			float _m[4][4];
		};
	public:
		inline Matrix4() {}
		inline Matrix4(const Matrix4& other) { *this = other; }
		inline Matrix4(float value)
		{
			m[0] = m[5] = m[10] = m[15] = value;
			m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
		}
		inline Matrix4(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33)
		{
			m[0] = m00;
			m[1] = m01;
			m[2] = m02;
			m[3] = m03;
			m[4] = m10;
			m[5] = m11;
			m[6] = m12;
			m[7] = m13;
			m[8] = m20;
			m[9] = m21;
			m[10] = m22;
			m[11] = m23;
			m[12] = m30;
			m[13] = m31;
			m[14] = m32;
			m[15] = m33;
		}
		inline void swap(Matrix4& other)
		{
			std::swap(m[0], other.m[0]);
			std::swap(m[1], other.m[1]);
			std::swap(m[2], other.m[2]);
			std::swap(m[3], other.m[3]);
			std::swap(m[4], other.m[4]);
			std::swap(m[5], other.m[5]);
			std::swap(m[6], other.m[6]);
			std::swap(m[7], other.m[7]);
			std::swap(m[8], other.m[8]);
			std::swap(m[9], other.m[9]);
			std::swap(m[10], other.m[10]);
			std::swap(m[11], other.m[11]);
			std::swap(m[12], other.m[12]);
			std::swap(m[13], other.m[13]);
			std::swap(m[14], other.m[14]);
			std::swap(m[15], other.m[15]);
		}
		/// Operator
		inline float* operator[](size_t row) { return _m[row]; }
		inline const float* operator[](size_t row)const { return _m[row]; }
		inline Matrix4& operator=(const Matrix4& equal)
		{
			if (this == &equal)
				return *this;
			memcpy(m, equal.m, 16 * sizeof(float));
			return *this;
		}
		inline Matrix4 operator*(const Matrix4& rhs)const
		{
			Matrix4 ret;
			//matrix's first row
			ret.m[0] = m[0] * rhs.m[0] + m[1] * rhs.m[4] + m[2] * rhs.m[8] + m[3] * rhs.m[12];
			ret.m[1] = m[0] * rhs.m[1] + m[1] * rhs.m[5] + m[2] * rhs.m[9] + m[3] * rhs.m[13];
			ret.m[2] = m[0] * rhs.m[2] + m[1] * rhs.m[6] + m[2] * rhs.m[10] + m[3] * rhs.m[14];
			ret.m[3] = m[0] * rhs.m[3] + m[1] * rhs.m[7] + m[2] * rhs.m[11] + m[3] * rhs.m[15];
			//matrix's second row
			ret.m[4] = m[4] * rhs.m[0] + m[5] * rhs.m[4] + m[6] * rhs.m[8] + m[7] * rhs.m[12];
			ret.m[5] = m[4] * rhs.m[1] + m[5] * rhs.m[5] + m[6] * rhs.m[9] + m[7] * rhs.m[13];
			ret.m[6] = m[4] * rhs.m[2] + m[5] * rhs.m[6] + m[6] * rhs.m[10] + m[7] * rhs.m[14];
			ret.m[7] = m[4] * rhs.m[3] + m[5] * rhs.m[7] + m[6] * rhs.m[11] + m[7] * rhs.m[15];
			//matrix's third row
			ret.m[8] = m[8] * rhs.m[0] + m[9] * rhs.m[4] + m[10] * rhs.m[8] + m[11] * rhs.m[12];
			ret.m[9] = m[8] * rhs.m[1] + m[9] * rhs.m[5] + m[10] * rhs.m[9] + m[11] * rhs.m[13];
			ret.m[10] = m[8] * rhs.m[2] + m[9] * rhs.m[6] + m[10] * rhs.m[10] + m[11] * rhs.m[14];
			ret.m[11] = m[8] * rhs.m[3] + m[9] * rhs.m[7] + m[10] * rhs.m[11] + m[11] * rhs.m[15];
			//matrix's fourth row
			ret.m[12] = m[12] * rhs.m[0] + m[13] * rhs.m[4] + m[14] * rhs.m[8] + m[15] * rhs.m[12];
			ret.m[13] = m[12] * rhs.m[1] + m[13] * rhs.m[5] + m[14] * rhs.m[9] + m[15] * rhs.m[13];
			ret.m[14] = m[12] * rhs.m[2] + m[13] * rhs.m[6] + m[14] * rhs.m[10] + m[15] * rhs.m[14];
			ret.m[15] = m[12] * rhs.m[3] + m[13] * rhs.m[7] + m[14] * rhs.m[11] + m[15] * rhs.m[15];
			return ret;
		}
		inline Matrix4& operator*=(const Matrix4& rhs)
		{
			//matrix's first row
			float m0 = m[0];
			m[0] = m[0] * rhs.m[0] + m[1] * rhs.m[4] + m[2] * rhs.m[8] + m[3] * rhs.m[12];
			float m1 = m[1];
			m[1] = m0 * rhs.m[1] + m[1] * rhs.m[5] + m[2] * rhs.m[9] + m[3] * rhs.m[13];
			float m2 = m[2];
			m[2] = m0 * rhs.m[2] + m1 * rhs.m[6] + m[2] * rhs.m[10] + m[3] * rhs.m[14];
			m[3] = m0 * rhs.m[3] + m1 * rhs.m[7] + m2 * rhs.m[11] + m[3] * rhs.m[15];

			//matrix's second row
			m0 = m[4];
			m[4] = m[4] * rhs.m[0] + m[5] * rhs.m[4] + m[6] * rhs.m[8] + m[7] * rhs.m[12];
			m1 = m[5];
			m[5] = m0 * rhs.m[1] + m[5] * rhs.m[5] + m[6] * rhs.m[9] + m[7] * rhs.m[13];
			m2 = m[6];
			m[6] = m0 * rhs.m[2] + m1 * rhs.m[6] + m[6] * rhs.m[10] + m[7] * rhs.m[14];
			m[7] = m0 * rhs.m[3] + m1 * rhs.m[7] + m2 * rhs.m[11] + m[7] * rhs.m[15];

			//matrix's third row
			m0 = m[8];
			m[8] = m[8] * rhs.m[0] + m[9] * rhs.m[4] + m[10] * rhs.m[8] + m[11] * rhs.m[12];
			m1 = m[9];
			m[9] = m0 * rhs.m[1] + m[9] * rhs.m[5] + m[10] * rhs.m[9] + m[11] * rhs.m[13];
			m2 = m[10];
			m[10] = m0 * rhs.m[2] + m1 * rhs.m[6] + m[10] * rhs.m[10] + m[11] * rhs.m[14];
			m[11] = m0 * rhs.m[3] + m1 * rhs.m[7] + m2 * rhs.m[11] + m[11] * rhs.m[15];

			//matrix's fourth row
			m0 = m[12];
			m[12] = m[12] * rhs.m[0] + m[13] * rhs.m[4] + m[14] * rhs.m[8] + m[15] * rhs.m[12];
			m1 = m[13];
			m[13] = m0 * rhs.m[1] + m[13] * rhs.m[5] + m[14] * rhs.m[9] + m[15] * rhs.m[13];
			m2 = m[14];
			m[14] = m0 * rhs.m[2] + m1 * rhs.m[6] + m[14] * rhs.m[10] + m[15] * rhs.m[14];
			m[15] = m0 * rhs.m[3] + m1 * rhs.m[7] + m2 * rhs.m[11] + m[15] * rhs.m[15];
			return *this;
		}
		inline Matrix4 operator+(const Matrix4& rhs)const
		{
			Matrix4 ret;
			ret.m[0] = m[0] + rhs.m[0];
			ret.m[1] = m[1] + rhs.m[1];
			ret.m[2] = m[2] + rhs.m[2];
			ret.m[3] = m[3] + rhs.m[3];
			ret.m[4] = m[4] + rhs.m[4];
			ret.m[5] = m[5] + rhs.m[5];
			ret.m[6] = m[6] + rhs.m[6];
			ret.m[7] = m[7] + rhs.m[7];
			ret.m[8] = m[8] + rhs.m[8];
			ret.m[9] = m[9] + rhs.m[9];
			ret.m[10] = m[10] + rhs.m[10];
			ret.m[11] = m[11] + rhs.m[11];
			ret.m[12] = m[12] + rhs.m[12];
			ret.m[13] = m[13] + rhs.m[13];
			ret.m[14] = m[14] + rhs.m[14];
			ret.m[15] = m[15] + rhs.m[15];
			return ret;
		}
		inline Matrix4 operator-(const Matrix4& rhs)const
		{
			Matrix4 ret;
			ret.m[0] = m[0] - rhs.m[0];
			ret.m[1] = m[1] - rhs.m[1];
			ret.m[2] = m[2] - rhs.m[2];
			ret.m[3] = m[3] - rhs.m[3];
			ret.m[4] = m[4] - rhs.m[4];
			ret.m[5] = m[5] - rhs.m[5];
			ret.m[6] = m[6] - rhs.m[6];
			ret.m[7] = m[7] - rhs.m[7];
			ret.m[8] = m[8] - rhs.m[8];
			ret.m[9] = m[9] - rhs.m[9];
			ret.m[10] = m[10] - rhs.m[10];
			ret.m[11] = m[11] - rhs.m[11];
			ret.m[12] = m[12] - rhs.m[12];
			ret.m[13] = m[13] - rhs.m[13];
			ret.m[14] = m[14] - rhs.m[14];
			ret.m[15] = m[15] - rhs.m[15];
			return ret;
		}
		inline bool operator==(const Matrix4& other)const
		{
			if (m[0] != other.m[0] || m[1] != other.m[1] || m[2] != other.m[2] || m[3] != other.m[3] ||
				m[4] != other.m[4] || m[5] != other.m[5] || m[6] != other.m[6] || m[7] != other.m[7] ||
				m[8] != other.m[8] || m[9] != other.m[9] || m[10] != other.m[10] || m[11] != other.m[11] ||
				m[12] != other.m[12] || m[13] != other.m[13] || m[14] != other.m[14] || m[15] != other.m[15])
				return false;
			return true;
		}
		inline bool operator!=(const Matrix4& other)const
		{
			if (m[0] != other.m[0] || m[1] != other.m[1] || m[2] != other.m[2] || m[3] != other.m[3] ||
				m[4] != other.m[4] || m[5] != other.m[5] || m[6] != other.m[6] || m[7] != other.m[7] ||
				m[8] != other.m[8] || m[9] != other.m[9] || m[10] != other.m[10] || m[11] != other.m[11] ||
				m[12] != other.m[12] || m[13] != other.m[13] || m[14] != other.m[14] || m[15] != other.m[15])
				return true;
			return false;
		}
		/// Function
		inline Matrix4 concatenateAffine(const Matrix4& other)const
		{
			return Matrix4(m[0] * other.m[0] + m[1] * other.m[4] + m[2] * other.m[8],
				m[0] * other.m[1] + m[1] * other.m[5] + m[2] * other.m[9],
				m[0] * other.m[2] + m[1] * other.m[6] + m[2] * other.m[10],
				m[0] * other.m[3] + m[1] * other.m[7] + m[2] * other.m[11] + m[3],
				m[4] * other.m[0] + m[5] * other.m[4] + m[6] * other.m[8],
				m[4] * other.m[1] + m[5] * other.m[5] + m[6] * other.m[9],
				m[4] * other.m[2] + m[5] * other.m[6] + m[6] * other.m[10],
				m[4] * other.m[3] + m[5] * other.m[7] + m[6] * other.m[11] + m[7],
				m[8] * other.m[0] + m[9] * other.m[4] + m[10] * other.m[8],
				m[8] * other.m[1] + m[9] * other.m[5] + m[10] * other.m[9],
				m[8] * other.m[2] + m[9] * other.m[6] + m[10] * other.m[10],
				m[8] * other.m[3] + m[9] * other.m[7] + m[10] * other.m[11] + m[11],
				0.f, 0.f, 0.f, 1.0f);
		}
		inline void setTranslation(const Vector3f& translate)
		{
			m[3] = translate.x;
			m[7] = translate.y;
			m[11] = translate.z;
		}
		inline void setScale(const Vector3f& scale)
		{
			m[0] = scale.x;
			m[5] = scale.y;
			m[10] = scale.z;
		}

		Matrix4 transpose(void)const
		{
			return Matrix4(m[0], m[4], m[8], m[12],
				m[1], m[5], m[9], m[13],
				m[2], m[6], m[10], m[14],
				m[3], m[7], m[11], m[15]);
		}
		Matrix4 inverse(void)const;
		/// Calculate the inverse of affine matrix.
		Matrix4 inverseAffine(void)const;
		/// Calculate the primitive matrix which only contains a translation and a rotation.
		//Matrix4 inversePrimitive(void)const;

		void makeTransform(const Vector3f& translate, const Vector3f& scale, const Quaternion& orientation);
		//void makeInverseTransform(const Vector3f& translate, const Vector3f& scale, const Quaternion& orientation);
		//void decomposition(Vector3f& position, Vector3f& scale, Quaternion& orientation);

		/// Rotate the vector through the matrix
		inline Vector3f transformDirectionAffine(const Vector3f& v)const
		{
			return Vector3f(
				m[0] * v.x + m[1] * v.y + m[2] * v.z,
				m[4] * v.x + m[5] * v.y + m[6] * v.z,
				m[8] * v.x + m[9] * v.y + m[10] * v.z);
		}

		Vector3f concatenatePos(const Vector3f& p)const;
		static const Matrix4 zero;
		static const Matrix4 identity;
	};

	inline Matrix4 cameraLookAt(const Vector3f& pos, const Vector3f& target, const Vector3f& up)
	{
		Matrix4 ret(1.0f);
		/*RH:	Vector3 dir = (pos - target).normalize();
			   Vector3 right = up.crossProduct(dir).normalize();*/
			   //Vector3f dir = normalize(pos - target);
		Vector3f dir = normalize(target - pos);
		Vector3f right = normalize(crossProduct(up, dir));
		Vector3f newUp = crossProduct(dir, right);
		//first row
		ret[0][0] = right.x;
		ret[0][1] = right.y;
		ret[0][2] = right.z;
		ret[0][3] = -dotProduct(right, pos);
		//second row
		ret[1][0] = newUp.x;
		ret[1][1] = newUp.y;
		ret[1][2] = newUp.z;
		ret[1][3] = -dotProduct(newUp, pos);
		//third row
		ret[2][0] = dir.x;
		ret[2][1] = dir.y;
		ret[2][2] = dir.z;
		ret[2][3] = -dotProduct(dir, pos);
		return ret;
	}

	inline Matrix4 perspectiveLH(float angle, float ratio, float zNear, float zFar)
	{
		Matrix4 ret(0.0f);
		float h = 1.0f / std::tan(angle * 0.5f);
		float w = h / ratio;

		ret[0][0] = -w;
		ret[1][1] = h;

		ret[2][2] = (zFar + zNear) / (zFar - zNear);
		//ret[2][2] = zFar / (zNear - zFar);
		ret[2][3] = 2.0f * zFar * zNear / (zNear - zFar);
		//ret[2][3] = zFar * zNear / (zNear - zFar);

		ret[3][2] = 1.0f;
		//ret[3][2] = -1.0f;
		return ret;
	}
}

#endif