//
// Created by Göksu Güvendiren on 2019-05-14.
//

#pragma once

#include "Light.hpp"
#include "TkSpectrum.h"
#include "Intersection.hpp"

namespace tk
{
	class AreaLight : public Light
	{
	private:
		std::shared_ptr<Shape> shape;
		Spectrum Lemit;
		bool twoSided;
		float area;
	public:
		AreaLight(int numSamples,
			const Spectrum& Le, const std::shared_ptr<Shape>& shape, bool twoSided);
		virtual Spectrum power()const;

		virtual Spectrum sample_Li(const Intersection& ref, Real u0, const Vector2f& u,
			Intersection* it, Vector3f* wi, Real* pdf)const;

		virtual Real pdf_Li(const Intersection& ref, const Vector3f& wi)const;

		virtual Spectrum sample_Le(const Vector2f& u1, const Vector2f& u2, Real time,
			Ray* ray, Vector3f* normalLight, Real* pdfPos, Real* pdfDir)const;
		virtual void pdf_Le(const Ray& ray, const Vector3f& nLight, Real* pdfPos,
			Real* pdfDir)const;

		virtual Spectrum Le(const Ray& r)const;
		Spectrum L(const Intersection& isect, const Vector3f& w)const;
	};
}