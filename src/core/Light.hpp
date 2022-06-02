//
// Created by Göksu Güvendiren on 2019-05-14.
//

#pragma once

#include "TkPrerequisites.h"

namespace tk
{
	class Light
	{
	public:
		Light() = default;

		virtual Spectrum power()const = 0;

		virtual Spectrum sample_Li(const Intersection& ref, Real u0, const Vector2f& u,
			Intersection* it, Vector3f* wi, Real* pdf)const = 0;
		virtual Real pdf_Li(const Intersection& ref, const Vector3f& wi)const = 0;

		virtual Spectrum sample_Le(const Vector2f& u1, const Vector2f& u2, Real time,
			Ray* ray, Vector3f* normalLight, Real* pdfPos, Real* pdfDir)const = 0;
		virtual void pdf_Le(const Ray& ray, const Vector3f& nLight, Real* pdfPos,
			Real* pdfDir)const = 0;

		virtual Spectrum Le(const Ray& r)const = 0;
	};
}