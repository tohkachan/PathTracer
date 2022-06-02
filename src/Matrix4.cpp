#include "Matrix4.h"

namespace tk
{
	const Matrix4 Matrix4::zero(0.0f);
	const Matrix4 Matrix4::identity(1.0f);

	Matrix4 Matrix4::inverse(void)const
	{
		Real m00 = m[0], m01 = m[1], m02 = m[2], m03 = m[3];
		Real m10 = m[4], m11 = m[5], m12 = m[6], m13 = m[7];
		Real m20 = m[8], m21 = m[9], m22 = m[10], m23 = m[11];
		Real m30 = m[12], m31 = m[13], m32 = m[14], m33 = m[15];

		Real v0 = m20 * m31 - m21 * m30;
		Real v1 = m20 * m32 - m22 * m30;
		Real v2 = m20 * m33 - m23 * m30;
		Real v3 = m21 * m32 - m22 * m31;
		Real v4 = m21 * m33 - m23 * m31;
		Real v5 = m22 * m33 - m23 * m32;

		Real t00 = v5 * m11 - v4 * m12 + v3 * m13;
		Real t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
		Real t20 = v4 * m10 - v2 * m11 + v0 * m13;
		Real t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

		Real invDet = 1.0f / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

		Real d00 = t00 * invDet;
		Real d10 = t10 * invDet;
		Real d20 = t20 * invDet;
		Real d30 = t30 * invDet;

		Real d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		Real d11 = (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		Real d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		Real d31 = (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m10 * m31 - m11 * m30;
		v1 = m10 * m32 - m12 * m30;
		v2 = m10 * m33 - m13 * m30;
		v3 = m11 * m32 - m12 * m31;
		v4 = m11 * m33 - m13 * m31;
		v5 = m12 * m33 - m13 * m32;

		Real d02 = (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		Real d12 = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		Real d22 = (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		Real d32 = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m21 * m10 - m20 * m11;
		v1 = m22 * m10 - m20 * m12;
		v2 = m23 * m10 - m20 * m13;
		v3 = m22 * m11 - m21 * m12;
		v4 = m23 * m11 - m21 * m13;
		v5 = m23 * m12 - m22 * m13;

		Real d03 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		Real d13 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		Real d23 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		Real d33 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		return Matrix4(
			d00, d01, d02, d03,
			d10, d11, d12, d13,
			d20, d21, d22, d23,
			d30, d31, d32, d33);
	}

	Matrix4 Matrix4::inverseAffine(void)const
	{
		Real m10 = m[4], m11 = m[5], m12 = m[6];
		Real m20 = m[8], m21 = m[9], m22 = m[10];
		Real t00 = m11 * m22 - m12 * m21;
		Real t10 = m12 * m20 - m10 * m22;
		Real t20 = m10 * m21 - m11 * m20;
		Real m00 = m[0], m01 = m[1], m02 = m[2];
		Real invDet = 1.0f / (m00 * t00 + m01 * t10 + m02 * t20);
		m00 *= invDet;
		m01 *= invDet;
		m02 *= invDet;
		Real r00 = invDet * t00;
		Real r01 = m02 * m21 - m01 * m22;
		Real r02 = m01 * m12 - m02 * m11;

		Real r10 = invDet * t10;
		Real r11 = m00 * m22 - m02 * m20;
		Real r12 = m02 * m10 - m00 * m12;

		Real r20 = invDet * t20;
		Real r21 = m01 * m20 - m00 * m21;
		Real r22 = m00 * m11 - m01 * m10;

		t00 = m[3], t10 = m[7], t20 = m[11];
		Real r03 = -1.0f * (m[0] * t00 + m[1] * t10 + m[2] * t20);
		Real r13 = -1.0f * (m[4] * t00 + m[5] * t10 + m[6] * t20);
		Real r23 = -1.0f * (m[8] * t00 + m[9] * t10 + m[10] * t20);
		return Matrix4(
			r00, r01, r02, r03,
			r10, r11, r12, r13,
			r20, r21, r22, r23,
			0, 0, 0, 1);
	}

	void Matrix4::makeTransform(const Vector3f& translate, const Vector3f& scale, const Quaternion& orientation)
	{
		// Order is scale, rotate, translate
		Matrix4 rot;
		orientation.getMatrix(rot);

		m[0] = scale.x * rot.m[0]; m[1] = scale.y * rot.m[1]; m[2] = scale.z * rot.m[2]; m[3] = translate.x;
		m[4] = scale.x * rot.m[4]; m[5] = scale.y * rot.m[5]; m[6] = scale.z * rot.m[6]; m[7] = translate.y;
		m[8] = scale.x * rot.m[8]; m[9] = scale.y * rot.m[9]; m[10] = scale.z * rot.m[10]; m[11] = translate.z;

		m[12] = m[13] = m[14] = 0; m[15] = 1;
	}

	Vector3f Matrix4::concatenatePos(const Vector3f& p)const
	{
		Real xp = _m[0][0] * p.x + _m[0][1] * p.y + _m[0][2] * p.z + _m[0][3];
		Real yp = _m[1][0] * p.x + _m[1][1] * p.y + _m[1][2] * p.z + _m[1][3];
		Real zp = _m[2][0] * p.x + _m[2][1] * p.y + _m[2][2] * p.z + _m[2][3];
		Real wp = _m[3][0] * p.x + _m[3][1] * p.y + _m[3][2] * p.z + _m[3][3];
		if (wp == 1)
			return Vector3f(xp, yp, zp);
		else
			return Vector3f(xp, yp, zp) / wp;
	}
}