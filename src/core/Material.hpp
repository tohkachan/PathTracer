//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "TkPrerequisites.h"
#include "TkSpectrum.h"
#include "Vector.hpp"

namespace tk
{
	enum MaterialType { DIFFUSE = 0x1, MIRROR = 0x2, GLASS = 0x4, PLASTIC = 0x8 };

	enum eTransportMode { Radiance, Importance };

	class Volume
	{
	public:
		Volume() {}
		virtual ~Volume() {}
		virtual bool integrate(const Scene* scene, const Ray& ray, Spectrum& L, Spectrum& transmittance,
			Spectrum& weight, Ray& outRay) = 0;
		virtual Spectrum Transmittance(const Vector3f& p0, const Vector3f& p1) = 0;
	};

	class BeersLawVolume : public Volume
	{
	private:
		Spectrum m_absorption = Spectrum(0.02f, 0.02f, 0.02f);
	public:
		virtual bool integrate(const Scene* scene, const Ray& ray, Spectrum& L, Spectrum& transmittance,
			Spectrum& weight, Ray& outRay);

		virtual Spectrum Transmittance(const Vector3f& p0, const Vector3f& p1)
		{
			float distance = (p0 - p1).norm();
			return Spectrum(exp(m_absorption.r * -distance), exp(m_absorption.g * -distance),
				exp(m_absorption.b * -distance));
		}
	};

	class SingleScatterHomogeneousVolume : public Volume
	{
	private:
		Spectrum m_extinction = Spectrum(0.01f, 0.01f, 0.01f);
		Spectrum m_scatteringAlbedo = Spectrum(0.6f, 0.6f, 0.6f);
	public:
		virtual bool integrate(const Scene* scene, const Ray& ray, Spectrum& L, Spectrum& transmittance,
			Spectrum& weight, Ray& outRay);

		virtual Spectrum Transmittance(const Vector3f& p0, const Vector3f& p1)
		{
			float distance = (p0 - p1).norm();
			return Spectrum(exp(m_extinction.r * -distance), exp(m_extinction.g * -distance),
				exp(m_extinction.b * -distance));
		}
	};

	class Material {
	public:
		MaterialType m_type;
		Volume* m_volume;
		mutable eTransportMode m_mode;
		bool isDelta;

		Material(MaterialType t = DIFFUSE);
		~Material();
		inline MaterialType getType()const { return m_type; }
		inline void setTransportMode(eTransportMode mode = Radiance)const { m_mode = mode; }
		inline Vector3f getColorAt(double u, double v) { return Vector3f(); }

		bool hasVolume()const { return m_volume ? true : false; }
		Volume* getVolume() { return m_volume; }

		inline Vector3f reflect(const Vector3f &wo, const Vector3f &N)const
		{
			return 2 * dotProduct(wo, N) * N - wo;
		}

		inline bool refract(const Vector3f& wi, const Vector3f& n, float eta, Vector3f* wt)const
		{
			float cosThetaI = dotProduct(wi, n);
			float sin2ThetaI = std::max<float>(0, 1 - cosThetaI * cosThetaI);
			float sin2ThetaT = eta * eta * sin2ThetaI;

			if (sin2ThetaT >= 1) return false;
			float cosThetaT = sqrt(1 - sin2ThetaT);
			*wt = -wi * eta + n * (eta * cosThetaI - cosThetaT);
			return true;
		}

		Vector3f toWorld(const Vector3f &a, const Vector3f &N)const
		{
			Vector3f B, C;
			CoordinateSystem(N, &B, &C);
			return a.x * B + a.y * C + a.z * N;
		}

		virtual Spectrum f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const = 0;
		virtual Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const;
		virtual float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
	};

	class DiffuseMaterial : public Material
	{
	private:
		Spectrum R;
	public:
		DiffuseMaterial(const Spectrum& R)
			: Material(DIFFUSE), R(R) {}
		Spectrum f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
	};

	class GlassMaterial : public Material
	{
	private:
		Spectrum R, T;
		float etaA, etaB;
	public:
		GlassMaterial(const Spectrum &R, const Spectrum &T, float etaA,
			float etaB)
			: Material(GLASS), R(R), T(T), etaA(etaA), etaB(etaB)
		{
			isDelta = true;
		}
		Spectrum f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return Spectrum::black; }
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const;
		float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return 0; }
	};

	class MirrorMaterial : public Material
	{
	private:
		Spectrum R;
	public:
		MirrorMaterial(const Spectrum &R)
			: Material(MIRROR), R(R)
		{
			isDelta = true;
		}
		Spectrum f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return Spectrum::black; }
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const;
		float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const { return 0; }
	};

	class PlasticMaterial : public Material
	{
	private:
		float etaA, etaB;
		float alpha;
		Spectrum R;
		float Lambda(const Vector3f& w, const Vector3f &N)const;
		float G(const Vector3f& wo, const Vector3f& wi, const Vector3f &N)const;
		float D(const Vector3f& wh, const Vector3f &N)const;
		float F(float cosi)const;
	public:
		PlasticMaterial(float etaA, float etaB, float alpha, Spectrum R)
			: Material(PLASTIC), etaA(etaA), etaB(etaB), alpha(alpha), R(R) {}
		Spectrum f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const;
		float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const;
	};
}

#endif