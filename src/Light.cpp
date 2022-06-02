#include "AreaLight.hpp"
#include "Shape.h"
#include "Scene.hpp"
#include "sampling.h"


namespace tk
{
	AreaLight::AreaLight(int numSamples,
		const Spectrum& Le, const std::shared_ptr<Shape>& shape, bool twoSided)
		: Lemit(Le),
		shape(shape),
		twoSided(twoSided),
		area(shape->getArea())
	{
	}


	Spectrum AreaLight::power()const
	{
		return Lemit * area * Math::pi * (twoSided ? 2 : 1);
	}

	Spectrum AreaLight::sample_Li(const Intersection& ref, Real u0, const Vector2f& u,
		Intersection* it, Vector3f* wi, Real* pdf)const
	{
		Intersection pShape = shape->Sample(ref, u0, u, pdf);
		if (*pdf == 0 || dotProduct(pShape.p - ref.p, pShape.p - ref.p) == 0)
			return Spectrum::black;
		*wi = normalize(pShape.p - ref.p);
		*it = pShape;	
		return L(pShape, -*wi);
	}

	Real AreaLight::pdf_Li(const Intersection& ref, const Vector3f& wi)const
	{
		//TODO:
		return 0;
	}

	Spectrum AreaLight::sample_Le(const Vector2f& u1, const Vector2f& u2, Real time,
		Ray* ray, Vector3f* nLight, Real* pdfPos, Real* pdfDir)const
	{
		Intersection pShape = shape->Sample(u1, pdfPos);
		*nLight = pShape.n;

		Vector3f w;
		if (twoSided)
		{
			Vector2f u = u2;
			if (u.x < 0.5)
			{
				u.x = tk::min(u.x * 2, Math::one_minus_epsilon);
				w = cosineSampleHemisphere(u);
			}
			else
			{
				u.x = tk::min((u.x - 0.5f) * 2, Math::one_minus_epsilon);
				w = cosineSampleHemisphere(u);
				w.z *= -1;
			}
			*pdfDir = 0.5f * cosineHemispherePdf(std::abs(w.z));
		}
		else
		{
			w = cosineSampleHemisphere(u2);
			*pdfDir = cosineHemispherePdf(w.z);
		}

		Vector3f v1, v2, n(pShape.n);
		CoordinateSystem(n, &v1, &v2);
		w = w.x * v1 + w.y * v2 + w.z * n;
		*ray = pShape.spawnRay(w);
		return L(pShape, w);
	}

	void AreaLight::pdf_Le(const Ray& ray, const Vector3f& nLight, Real* pdfPos,
		Real* pdfDir)const
	{
		Intersection it;
		it.p = ray.origin;
		it.n = nLight;
		*pdfPos = shape->Pdf(it);
		Real cosTheta = dotProduct(nLight, ray.direction);
		*pdfDir = cosTheta > 0.0f ? cosineHemispherePdf(cosTheta) : 0;
		//*pdfDir = twoSided ? (0.5 * cosineHemispherePdf(AbsDot(nLight, ray.direction)))
			//: cosineHemispherePdf(AbsDot(nLight, ray.direction));
	}

	Spectrum AreaLight::L(const Intersection& isect, const Vector3f& w)const
	{
		return (twoSided || dotProduct(isect.n, w) > 0) ? Lemit : Spectrum::black;
	}

	Spectrum AreaLight::Le(const Ray& r)const
	{
		return Spectrum::black;
	}
}