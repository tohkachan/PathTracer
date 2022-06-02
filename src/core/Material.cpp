#include "Material.hpp"
#include "scattering.h"
#include "Scene.hpp"
#include "sampling.h"
#include "Intersection.hpp"
#include "TkSpectrum.h"

namespace tk
{
	Material::Material(MaterialType t)
		: m_volume(nullptr)
	{
		m_type = t;
		isDelta = false;
		//m_color = c;
	}

	Material::~Material()
	{
	}

	Spectrum Material::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const
	{
		Vector3f tmp = cosineSampleHemisphere(sample);
		if (tmp.z < 0) tmp.z *= -1;
		*wi = toWorld(tmp, N);
		*pdf = Pdf(wo, *wi, N);
		return f(wo, *wi, N);
	}

	float Material::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		return dotProduct(wo, N) * dotProduct(wi, N) > 0 ? cosineHemispherePdf(dotProduct(wi, N)) : 0;
	}

	Spectrum DiffuseMaterial::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		return dotProduct(wo, N) * dotProduct(wi, N) > 0 ? R * Math::inv_pi : Spectrum::black;
	}

	Real FrDielectric(Real cosThetaI, Real etaI, Real etaT)
	{
		cosThetaI = Math::Clamp(cosThetaI, -1.0f, 1.0f);
		bool entering = cosThetaI > 0.f;
		if (!entering)
		{
			std::swap(etaI, etaT);
			cosThetaI = Math::Abs(cosThetaI);
		}

		Real sinThetaI = Math::Sqrt(tk::max(0.0f, 1 - cosThetaI * cosThetaI));
		Real sinThetaT = etaI / etaT * sinThetaI;
		if (sinThetaT >= 1) return 1;

		Real cosThetaT = Math::Sqrt(tk::max(0.0f, 1 - sinThetaT * sinThetaT));
		Real Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) /
			((etaT * cosThetaI) + (etaI * cosThetaT));
		Real Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) /
			((etaI * cosThetaI) + (etaT * cosThetaT));
		return (Rparl * Rparl + Rperp * Rperp) / 2;
	}

	Spectrum GlassMaterial::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const
	{
		Real pr = FrDielectric(dotProduct(wo, N), etaA, etaB);
		if (sample.x < pr)
		{
			*wi = reflect(wo, N);
			*pdf = pr;
			return R * pr / AbsDot(*wi, N);
		}
		else
		{
			bool entering = dotProduct(wo, N) > 0;
			float etai = entering ? etaA : etaB;
			float etat = entering ? etaB : etaA;

			if (!refract(wo, entering ? N : -N, etai / etat, wi))
				return Spectrum::black;
			Spectrum ft = T * (1 - pr);

			// Account for non-symmetry with transmission to different medium
			if (m_mode == Radiance)
				ft *= (etai * etai) / (etat * etat);
			*pdf = 1 - pr;
			return ft / AbsDot(*wi, N);
		}
	}

	Spectrum MirrorMaterial::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const
	{
		*wi = reflect(wo, N);
		*pdf = 1;
		return R / AbsDot(*wi, N);
	}

	float PlasticMaterial::Lambda(const Vector3f& w, const Vector3f &N)const
	{
		Real cosTheta = dotProduct(w, N);
		Radian theta = Math::ACos(Math::Clamp(cosTheta, -1.0f + 1e-5f, 1.0f - 1e-5f));
		Real a = 1.0f / (alpha * Math::Tan(theta));
		Real ret = 0.5 * (erf(a) - 1.0 + exp(-a * a) / (a * Math::pi));
		if (isnan(ret))
			fprintf(stderr, "\nLambda nan\n");
		return ret;
	}

	float PlasticMaterial::G(const Vector3f& wo, const Vector3f& wi, const Vector3f &N)const
	{
		return 1.0 / (1.0 + Lambda(wi, N) + Lambda(wo, N));
	}

	float PlasticMaterial::D(const Vector3f& wh, const Vector3f &N)const
	{
		float cosh = dotProduct(wh, N);
		float cos2h = cosh * cosh;
		float tan2h = (1 - cos2h) / cos2h;
		double a2 = alpha * alpha;
		Real ret = exp(-tan2h / a2) / (Math::pi * a2 * cos2h * cos2h);
		if (isnan(ret))
			fprintf(stderr, "\nD nan\n");
		return ret;
	}

	float PlasticMaterial::F(float cosi)const
	{	
		return FrDielectric(cosi, etaA, etaB);
	}

	Spectrum PlasticMaterial::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		float coso = dotProduct(wo, N), cosi = dotProduct(wi, N);
		if (coso > 0 && cosi > 0)
		{
			Vector3f Rs, Rp;
			Vector3f eta = Vector3f(0.21646, 0.42833, 1.3284);
			Vector3f k = Vector3f(3.2390, 2.4599, 1.8661);
			Vector3f  eta_k = eta * eta + k * k;
			Rs = (eta_k - 2 * eta * cosi + cosi * cosi) / (eta_k + 2 * eta * cosi + cosi * cosi);
			Rp = (eta_k * cosi * cosi - 2 * eta * cosi + 1) / (eta_k * cosi * cosi + 2 * eta * cosi + 1);
			Vector3f f = (Rs + Rp) / 2.0;
			Vector3f wh = (wo + wi).normalized();
			return R * Spectrum(f.x, f.y, f.z) * G(wo, wi, N) * D(wh, N) / (4.0 * coso * cosi);
		}
		return Spectrum::black;
	}

	Spectrum PlasticMaterial::sample_f(const Vector3f& wo, Vector3f* wi, const Vector3f& N, const Vector2f &sample, float* pdf)const
	{
		Real a2 = alpha * alpha;
		Radian theta = Math::ATan(Math::Sqrt(-a2 * Math::Log(1 - sample.x)));
		Real phi = Math::two_pi * sample.y;
		Real cosh = Math::Cos(theta);
		if (cosh <= 0)
			return Spectrum::black;
		Real cos2h = cosh * cosh;
		Real sin2h = 1 - cos2h;
		Real sinh = Math::Sqrt(sin2h);
		Real tan2h = sin2h / cos2h;

		Vector3f h = { sinh * Math::Cos(phi), sinh * Math::Sin(phi), cosh };
		Vector3f hWorld = normalize(toWorld(h, N));
		if (dotProduct(wo, hWorld) <= 0)
			return Spectrum::black;
		*wi = -wo + 2 * dotProduct(wo, hWorld) * hWorld;
		if (dotProduct(*wi, hWorld) <= 0)
			return Spectrum::black;;
		Real p_h = exp(-tan2h / a2) / (a2 * cos2h * cosh) * Math::inv_pi;
		*pdf = p_h / (4 * dotProduct(*wi, hWorld));
		return f(wo, *wi, N);
	}

	Real PlasticMaterial::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& N)const
	{
		if (dotProduct(wo, N) <= 0 || dotProduct(wi, N) <= 0)
			return 0;
		Vector3f wh = normalize(wo + wi);
		Real cosh = dotProduct(wh, N);
		if (cosh <= 0 || dotProduct(wi, wh) <= 0)
			return 0;
		Real cos2h = cosh * cosh;
		Real tan2h = (1 - cos2h) / cos2h;
		Real a2 = alpha * alpha;
		Real p_h = exp(-tan2h / a2) / (a2 * cos2h * cosh) * Math::inv_pi;
		return p_h / (4 * dotProduct(wi, wh));
	}

	bool BeersLawVolume::integrate(const Scene* scene, const Ray& ray, Spectrum& L, Spectrum& transmittance,
		Spectrum& weight, Ray& outRay)
	{
		/*Intersection isect = scene->intersect(ray);
		if (!isect.happened)
			return false;
		Vector3f p = isect.coords + 0.01 * isect.normal;
		//Vector3f wi = ray.direction;
		L = Vector3f();
		transmittance = Transmittance(p, ray.origin);
		weight = Vector3f(1.f);
		outRay = Ray(p, ray.direction);*/
		return true;
	}

	bool SingleScatterHomogeneousVolume::integrate(const Scene* scene, const Ray& ray, Spectrum& L, Spectrum& transmittance,
		Spectrum& weight, Ray& outRay)
	{
		/*Intersection isect = scene->intersect(ray);
		if (!isect.happened)
			return false;
		Vector3f p = isect.coords + 0.01 * isect.normal;
		Vector3f wi = ray.direction;
		transmittance = Transmittance(p, ray.origin);
		float xi = get_random_float();
		float scatterDist = -logf(1.f - xi * (1.0f - transmittance.x)) / m_extinction.x;
		Vector3f pScatter = ray.origin + scatterDist * wi;
		AnisotropicPhaseBSDF bsdf(0.8);
		Intersection isect_light;
		float lightPdf;
		scene->sampleLight(isect_light, lightPdf);
		Vector3f sampleDir = (isect_light.coords - pScatter).normalized();
		Vector3f f = bsdf.f(-wi, sampleDir, wi);
		//float bsdfPdf = bsdf.Pdf(wi, sampleDir, wi);
		float tmp = dotProduct(-wi, isect_light.normal) /
			dotProduct(isect_light.coords - pScatter, isect_light.coords - pScatter);
		L += isect_light.emit * f * tmp / lightPdf;

		//f = bsdf.sample_f(wi, &sampleDir, wi, Vector2f(get_random_float(), get_random_float()),
		//&bsdfPdf, 0);

		Vector3f Tr(exp(m_extinction.x * -scatterDist), exp(m_extinction.y * -scatterDist),
			exp(m_extinction.z * -scatterDist));

		L = L * (m_extinction * m_scatteringAlbedo * Tr);

		weight = (Vector3f(1.f) - transmittance) / (Tr * m_extinction);
		outRay = Ray(p, wi);*/
		return true;
	}
}