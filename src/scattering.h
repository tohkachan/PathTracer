#ifndef SCATTERING_H
#define SCATTERING_H

#include "Vector.hpp"

namespace tk
{
	enum eBxDFType
	{
		RBT_REFLECTION = 0x1,
		RBT_TRANSMISSION = 0x2,
		RBT_DIFFUSE = 0x4,
		RBT_GLOSSY = 0x8,
		RBT_SPECULAR = 0x10,
		RBT_DIFFUSE_REFLECTION = RBT_DIFFUSE | RBT_REFLECTION,
		RBT_DIFFUSE_TRANSMISSION = RBT_DIFFUSE | RBT_TRANSMISSION,
		RBT_GLOSSY_REFLECTION = RBT_GLOSSY | RBT_REFLECTION,
		RBT_GLOSSY_TRANSMISSION = RBT_GLOSSY | RBT_TRANSMISSION,
		RBT_SPECULAR_REFLECTION = RBT_SPECULAR | RBT_REFLECTION,
		RBT_SPECULAR_TRANSMISSION = RBT_SPECULAR | RBT_TRANSMISSION,
		RBT_ALL = RBT_REFLECTION | RBT_TRANSMISSION | RBT_DIFFUSE | RBT_GLOSSY | RBT_SPECULAR
	};

	class BxDF
	{
	public:
		const eBxDFType mType;
		BxDF(eBxDFType type) : mType(type) {}
		virtual ~BxDF() {}
		bool matchFlags(eBxDFType flag)const { return (mType & flag) == mType; }
		virtual Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const = 0;
		virtual Vector3f sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
			const Vector2f& sample, float* pdf, eBxDFType* sampledType = nullptr)const;
		virtual float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
	};

	class TrivialBSDF : public BxDF
	{
	public:
		TrivialBSDF()
			: BxDF(RBT_SPECULAR_TRANSMISSION) {}
		Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return Vector3f(); }
		Vector3f sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
			const Vector2f& sample, float* pdf, eBxDFType* sampledType)const;
		float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return 0; }
	};

	class AnisotropicPhaseBSDF : public BxDF
	{
	private:
		float m_g, m_one_plus_g2, m_one_minus_g2, m_one_over_2g;
		bool m_isotropic;
		float calcPdf(float cosTheta)const;
		float invertCdf(float u)const
		{
			float t = (m_one_minus_g2) / (1.0f - m_g + 2.0f * m_g * u);
			return m_one_over_2g * (m_one_plus_g2 - t * t);
		}
	public:
		AnisotropicPhaseBSDF(float g);
		Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
		Vector3f sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N,
			const Vector2f& sample, float* pdf, eBxDFType* sampledType)const;
		float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
	};
}
#endif