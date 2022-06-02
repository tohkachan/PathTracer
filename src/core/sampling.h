#ifndef SAMPLING_H
#define SAMPLING_H

#include "Vector.hpp"

namespace tk
{
	void stratifiedSample1D(float *data, int numSamples, bool jitter);
	void stratifiedSample2D(Vector2f *data, int xSamples, int ySamples, bool jitter);

	template <typename T>
	void shuffle(T *data, int count, int nDim)
	{
		for (int i = 0; i < count; ++i)
		{
			int other = i + get_random_float() * (count - i);
			for (int j = 0; j < nDim; ++j)
				std::swap(data[nDim * i + j], data[nDim * other + j]);
		}
	}

	Vector3f uniformSampleHemisphere(const Vector2f& u);
	float uniformHemispherePdf();

	Vector3f uniformSampleSphere(const Vector2f& u);
	Real uniformSpherePdf();

	Vector2f uniformSampleTriangle(const Vector2f& u);

	Vector2f concentricSampleDisk(const Vector2f& u);

	inline Vector3f cosineSampleHemisphere(const Vector2f& u)
	{
		Vector2f d = concentricSampleDisk(u);
		float z = sqrt(std::max<float>(0, 1 - d.x * d.x - d.y * d.y));
		return Vector3f(d.x, d.y, z);
	}
	inline float cosineHemispherePdf(float cosTheta) { return cosTheta * Math::inv_pi; }


	inline float balanceHeurisic(int nf, float fPdf, int ng, float gPdf)
	{
		return (nf * fPdf) / (nf * fPdf + ng * gPdf);
	}

	inline float powerHeuristic(int nf, float fPdf, int ng, float gPdf)
	{
		float f = nf * fPdf, g = ng * gPdf;
		return (f * f) / (f * f + g * g);
	}

	/** Distribution of Piecewise-Constant 1D Functions.*/
	struct Distribution1D
	{
		std::vector<float> cdf, func;
		Distribution1D() = default;
		Distribution1D(const float* f, int num)
			: func(f, f + num), cdf(num + 1)
		{
			cdf[0] = 0;
			for (int i = 1; i < num + 1; ++i)
				cdf[i] = cdf[i - 1] + func[i - 1] / num;
			float c = cdf[num];
			for (int i = 1; i < num + 1; ++i)
				cdf[i] /= c;
		}
		int sampleDiscrete(float u, float *pdf = nullptr)const
		{
			int offset = FindInterval(cdf.size(), [&](int idx) {return cdf[idx] <= u; });
			if (pdf) *pdf = cdf[offset + 1] - cdf[offset];
			return offset;
		}
	};
}

#endif