#include "camera.h"
#include "Ray.hpp"
#include "Intersection.hpp"
#include "TkSpectrum.h"
#include "TkFilm.h"
#include "sampling.h"

namespace tk
{
	Camera::Camera(const Matrix4* c2w, const Matrix4& c2s, Real lensRadius, Real focalDistance, Film* film)
		: cameraToWorld(c2w), cameraToScreen(c2s), mFilm(film),
		mLensRadius(lensRadius), mFocalDistance(focalDistance)
	{
		Point2i res = film->getFullResolution();
		Matrix4 s2r(1.f);
		s2r.setScale(Vector3f(res.x * 0.5f, res.y * 0.5f, 1));
		s2r.setTranslation(Vector3f(res.x * 0.5f, res.y * 0.5f, 0));

		Matrix4 c2r = s2r * cameraToScreen;
		rasterToCamera = c2r.inverse();

		Vector3f pMin = rasterToCamera.concatenatePos(Vector3f());
		Vector3f pMax = rasterToCamera.concatenatePos(Vector3f(res.x, res.y, 0));

		// This film raster in (X, X, -1), should in (X, X, 1), but now don't rectify!!!
		pMin = pMin / pMin.z;
		pMax = pMax / pMax.z;
		// filmArea = focalDistance^2 * focusPlaneArea
		mFilmArea = Math::Abs((pMax.x - pMin.x) * (pMax.y - pMin.y));
	}

	void Camera::update(const Matrix4& c2s, Film* film)
	{
		cameraToScreen = c2s;
		mFilm = film;
		Point2i res = film->getFullResolution();
		Matrix4 s2r(1.f);
		s2r.setScale(Vector3f(res.x * 0.5f, res.y * 0.5f, 1));
		s2r.setTranslation(Vector3f(res.x * 0.5f, res.y * 0.5f, 0));

		Matrix4 c2r = s2r * cameraToScreen;
		rasterToCamera = c2r.inverse();

		Vector3f pMin = rasterToCamera.concatenatePos(Vector3f());
		Vector3f pMax = rasterToCamera.concatenatePos(Vector3f(res.x, res.y, 0));
		pMin = pMin / pMin.z;
		pMax = pMax / pMax.z;
		mFilmArea = Math::Abs((pMax.x - pMin.x) * (pMax.y - pMin.y));
	}

	Real Camera::generateRay(const Vector2f& uFilm, const Vector2f& uLens, Ray* r)const
	{
		Vector3f pFilm = Vector3f(uFilm.x, uFilm.y, 0);
		Vector3f d = normalize(rasterToCamera.concatenatePos(pFilm));
		Vector3f o = Vector3f(0.f);
		if (mLensRadius > 0)
		{
			// Sample point on lens
			Vector2f pLens = concentricSampleDisk(uLens) * mLensRadius;
			// Compute point on plane of focus
			Real ft = mFocalDistance / d.z;
			Vector3f pFocus = d * ft;

			o = Vector3f(pLens.x, pLens.y, 0);
			d = normalize(o - pFocus);//should rectify
			//d = normalize(pFocus - o);
		}
		const Matrix4& c2w = *cameraToWorld;
		*r = Ray(c2w.concatenatePos(o), normalize(c2w.transformDirectionAffine(d)));
		return 1;
	}

	Spectrum Camera::We(const Ray& r, Vector2f* pRaster)const
	{
		float cosTheta = dotProduct(r.direction, cameraToWorld->transformDirectionAffine(Vector3f(0, 0, 1)));

		if (cosTheta <= 0)
			return Spectrum::black;

		Vector3f pFocus = r((mLensRadius > 0 ? mFocalDistance : 1) / cosTheta);
		Vector3f raster = rasterToCamera.inverse().concatenatePos(cameraToWorld->inverse().concatenatePos(pFocus));

		if (pRaster) *pRaster = Vector2f(raster.x, raster.y);

		Bounds2i sampleBounds = mFilm->getSampleBounds();
		if (raster.x < sampleBounds.pMin.x || raster.x >= sampleBounds.pMax.x ||
			raster.y < sampleBounds.pMin.y || raster.y >= sampleBounds.pMax.y)
			return Spectrum::black;

		Real lensArea = mLensRadius != 0 ? (Math::pi * mLensRadius * mLensRadius) : 1;
		Real cos2Theta = cosTheta * cosTheta;
		Real w = 1.0 / (mFilmArea * lensArea * cos2Theta * cos2Theta);
		return Spectrum::white * w;
	}

	Spectrum Camera::Sample_Wi(const Intersection& ref, const Vector2f& u, Vector3f* wi,
		Intersection* it, Real* pdf, Vector2f* pRaster)const
	{
		Vector2f pLens = concentricSampleDisk(u) * mLensRadius;
		Vector3f pLensWorld = cameraToWorld->concatenatePos(Vector3f(pLens.x, pLens.y, 0.0f));
		it->n = -cameraToWorld->transformDirectionAffine(Vector3f(0, 0, 1));
		it->p = pLensWorld;
		*wi = pLensWorld - ref.p;
		Real dist = wi->norm();
		*wi = *wi / dist;

		Real lensArea = mLensRadius != 0 ? (Math::pi * mLensRadius * mLensRadius) : 1;
		*pdf = (dist * dist) / (AbsDot(it->n, *wi) * lensArea);
		//return We(it->spawnRay(-*wi), pRaster);
		return We(it->spawnRay(*wi), pRaster);//should rectify
	}

	void Camera::Pdf_We(const Ray& r, Vector3f* nCamera, Real* pdfPos, Real* pdfDir)const
	{
		// this method doesn't check whether p is projected to film plane
		// based on the assumption that caller already cull out the
		// position that is out of camera frustum
		Vector3f n = -cameraToWorld->transformDirectionAffine(Vector3f(0, 0, 1));
		Real cosTheta = dotProduct(r.direction, n);
		if (cosTheta <= 0)
		{
			*pdfPos = *pdfDir = 0;
			return;
		}
		if (nCamera) *nCamera = n;

		*pdfPos = mLensRadius != 0 ? 1.0f / (Math::pi * mLensRadius * mLensRadius) : 1.0f;
		*pdfDir = 1.0f / (mFilmArea * cosTheta * cosTheta * cosTheta);
	}
}