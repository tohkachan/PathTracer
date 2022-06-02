#include "sampling.h"

namespace tk
{
	void stratifiedSample1D(float *data, int numSamples, bool jitter)
	{
		float dx = 1.0f / numSamples;
		for (int i = 0; i < numSamples; ++i)
		{
			float delta = jitter ? get_random_float() : 0.5f;
			data[i] = min((i + delta) * dx, Math::one_minus_epsilon);
		}
	}

	void stratifiedSample2D(Vector2f *data, int xSamples, int ySamples, bool jitter)
	{
		float dx = 1.0f / xSamples, dy = 1.0f / ySamples;
		for (int y = 0; y < ySamples; ++y)
			for (int x = 0; x < xSamples; ++x)
			{
				float jx = jitter ? get_random_float() : 0.5f;
				float jy = jitter ? get_random_float() : 0.5f;
				data->x = min((x + jx) * dx, Math::one_minus_epsilon);
				data->y = min((y + jy) * dy, Math::one_minus_epsilon);
				++data;
			}
	}

	Vector3f uniformSampleHemisphere(const Vector2f& u)
	{
		float z = u.x;
		float r = sqrt(std::max(0.f, 1.0f - z * z));
		float phi = Math::two_pi * u.y;
		return Vector3f(r * cos(phi), r * sin(phi), z);
	}

	float uniformHemispherePdf()
	{
		return 0.5 * Math::inv_pi;
	}

	Vector3f uniformSampleSphere(const Vector2f& u)
	{
		Real z = 1 - 2 * u.x;
		Real r = std::sqrt(std::max(0.f, 1.0f - z * z));
		Real phi = Math::two_pi * u.y;
		return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
	}
	Real uniformSpherePdf()
	{
		return 0.25 * Math::inv_pi;
	}

	Vector2f uniformSampleTriangle(const Vector2f& u)
	{
		Real su0 = std::sqrt(u.x);
		return Vector2f(1 - su0, u.y * su0);
	}

	Vector2f concentricSampleDisk(const Vector2f& u)
	{
		Vector2f uOffset = u * 2.f - Vector2f(1.0f);
		if (uOffset.x == 0.0f && uOffset.y == 0.f)
			return uOffset;
		float theta, r;
		if (Math::Abs(uOffset.x) > Math::Abs(uOffset.y))
		{
			r = uOffset.x;
			theta = Math::half_pi * 0.5 * (uOffset.y / uOffset.x);
		}
		else
		{
			r = uOffset.y;
			theta = Math::half_pi - Math::half_pi * 0.5 * (uOffset.x / uOffset.y);
		}
		return Vector2f(r * cos(theta), r * sin(theta));
	}

}