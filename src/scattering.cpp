#include "scattering.h"
#include "sampling.h"

namespace tk
{
	Vector3f BxDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
		const Vector2f& sample, float* pdf, eBxDFType* sampledType)const
	{
		//Vector3f tmp = cosineSampleHemisphere(sample);
		Vector3f dir = uniformSampleHemisphere(sample);
		if (dir.z < 0) dir.z *= -1;

		Vector3f B, C;
		CoordinateSystem(N, &B, &C);
		*wi = dir.x * B + dir.y * C + dir.z * N;
		*pdf = Pdf(wo, *wi, N);
		return f(wo, *wi, N);
	}

	float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		return dotProduct(wo, N) > 0 ? 0.5 * Math::inv_pi : 0;
		//return dotProduct(wo, N) > 0 ? dotProduct(wi, N) / M_PI : 0;
	}

	Vector3f TrivialBSDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
		const Vector2f& sample, float* pdf, eBxDFType* sampledType)const
	{
		*wi = -wo;
		*pdf = 1.0f;
		return Vector3f(1.f) / fabs(dotProduct(*wi, N));
	}

	float AnisotropicPhaseBSDF::calcPdf(float cosTheta)const
	{
		return 0.25 * m_one_minus_g2 / (Math::pi *
			pow(m_one_plus_g2 - 2.0f * m_g * cosTheta, 1.5f));
	}

	AnisotropicPhaseBSDF::AnisotropicPhaseBSDF(float g)
		: BxDF(RBT_DIFFUSE_REFLECTION)
	{
		m_g = Math::Clamp(g, -1.0f, 1.0f);
		m_isotropic = (fabs(m_g) < 0.0001f);
		if (!m_isotropic)
		{
			m_one_plus_g2 = 1.0f + m_g * m_g;
			m_one_minus_g2 = 1.0f - m_g * m_g;
			m_one_over_2g = 0.5f / m_g;
		}
	}

	Vector3f AnisotropicPhaseBSDF::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		if (m_isotropic)
			return Vector3f(0.25 * Math::inv_pi);
		else
		{
			float cosTheta = dotProduct(-wo, wi);
			return Vector3f(calcPdf(cosTheta));
		}
	}

	Vector3f AnisotropicPhaseBSDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
		const Vector2f& sample, float* pdf, eBxDFType* sampledType)const
	{
		if (m_isotropic)
		{
			float z = sample.x - 1.0;
			float r = sqrt(std::max<float>(0, 1.0f - z * z));
			float phi = Math::two_pi * sample.y;
			*wi = Vector3f(r * cos(phi), r * sin(phi), z);
			*pdf = 0.25 * Math::inv_pi;
			return Vector3f(*pdf);
		}
		else
		{
			float phi = Math::two_pi * sample.y;
			float cosTheta = invertCdf(sample.x);
			float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
			Vector3f tmp(sinTheta * sin(phi), sinTheta * cos(phi), cosTheta);
			Vector3f B, C;
			CoordinateSystem(N, &B, &C);
			*wi = tmp.x * B + tmp.y * C + tmp.z * N;
			*pdf = calcPdf(cosTheta);
		}
		return Vector3f(*pdf);
	}

	float AnisotropicPhaseBSDF::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		if (m_isotropic)
			return 0.25 * Math::inv_pi;
		else
		{
			float cosTheta = dotProduct(wo, wi);
			return calcPdf(cosTheta);
		}
	}
}