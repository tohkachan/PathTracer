#ifndef CAMERA_H
#define CAMERA_H

#include "Matrix4.h"
#include "TkSpectrum.h"
#include "Ray.hpp"
#include "Intersection.hpp"

namespace tk
{
	class Camera
	{
	private:
		const Matrix4* cameraToWorld;
		Matrix4 cameraToScreen, rasterToCamera;
		Film* mFilm;
		Real mFilmArea;
		Real mLensRadius;
		Real mFocalDistance;
	public:
		Camera(const Matrix4* c2w, const Matrix4& c2s, Real lensRadius, Real focalDistance, Film* film);
		void update(const Matrix4& c2s, Film* film);

		Film* getFilm() { return mFilm; }

		Real generateRay(const Vector2f& uFilm, const Vector2f& uLens, Ray* r)const;

		// Now is error, ray direction is inverse
		Spectrum We(const Ray& r, Vector2f* pRaster = nullptr)const;

		Spectrum Sample_Wi(const Intersection& ref, const Vector2f& u, Vector3f* wi,
			Intersection* it, Real* pdf, Vector2f* pRaster)const;

		void Pdf_We(const Ray& r, Vector3f* n, Real* pdfPos, Real* pdfDir)const;
	};
}
#endif