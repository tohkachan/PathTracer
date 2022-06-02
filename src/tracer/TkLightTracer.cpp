#include "TkLightTracer.h"

#include "Scene.hpp"
#include "camera.h"
#include "sampler.h"
#include "Object.hpp"
#include "Material.hpp"

#include <GL/glew.h>

namespace tk
{
	LightTracer::LightTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette)
		: RayTracer(spp, maxDepth, numThreads, russianRoulette)
	{
		s32 x = 1, y = mSpp / x;
		while ((y != x) && (y / x != 2))
		{
			x <<= 1;
			y = mSpp / x;
		}
		mSampler = new StratifiedSampler(x, y, true);
	}

	void LightTracer::visualize()
	{

	}

	void LightTracer::rendering()
	{
		fprintf(stderr, "\r[Tracer] Rendering...... %02d%%", std::min(int((Real)(mJobsDone) / mJobsCount * 100), 100));
		if (mNextStart.y >= mEndPos.y)
		{
			stopRaytracing();
			mState = DONE;
		}
		mFilm->setFrame(mFrameBuffer);
		glDrawPixels(mEndPos.x, mEndPos.y, GL_RGBA,
			GL_UNSIGNED_BYTE, mFrameBuffer);
	}

	void LightTracer::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				Point2i pixel = Point2i(x, y);
				sampler.startPixel(pixel);
				do {
					RayPath path;
					splatFilmT1(pixel, sampler, path);
				} while (sampler.startNextSample());
			}
		}
	}

	s32 LightTracer::generateLightSubpath(RayPath& path, Sampler& s)
	{
		const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
		int numLights = lights.size();
		if (numLights == 0) return 0;
		int idx = std::min<s32>(s.get1D() * numLights, numLights - 1);
		float pdf = 1.0f / numLights;
		float pdfPos, pdfDir;
		Ray r;
		Vector3f n;
		Spectrum Le = lights[idx]->sample_Le(s.get2D(), s.get2D(), 0, &r, &n, &pdfPos, &pdfDir);
		path.emplace_back(Spectrum::white * (1.0f / (pdfPos * pdf)), r.origin, n, lights[idx].get());
		Spectrum throughput = path[0].throughput * AbsDot(n, r.direction) / pdfDir;
		s32 bounces = 1;
		Intersection isect;
		while (bounces < mMaxDepth)
		{
			if (!mScene->intersect(r, &isect))
				break;
			PathVertex pv;
			pv.throughput = throughput;
			pv.isect = isect;
			path.emplace_back(pv);
			++bounces;

			if (get_random_float() > mRussianRoulette)
				break;
			{
				Vector3f wi;
				const Material* m = isect.obj->getMaterial();
				m->setTransportMode(Importance);
				Real pdfFwd;
				Spectrum f = m->sample_f(isect.wo, &wi, isect.n, s.get2D(), &pdfFwd);
				if (pdfFwd == 0.f)
					break;
				throughput *= f * AbsDot(wi, isect.n) / (pdfFwd * mRussianRoulette);
				r = isect.spawnRay(wi);
			}
		}
		return bounces;
	}

	s32 LightTracer::generateCameraSubpath(RayPath& path, Sampler& s, Point2i pixel)
	{
		Vector2f cameraSample = s.get2D() + Vector2f(pixel.x, pixel.y);
		Ray r;
		mCamera->generateRay(cameraSample, s.get2D(), &r);

		Vector3f nCamera;
		Real pdfPos, pdfDir;
		mCamera->Pdf_We(r, &nCamera, &pdfPos, &pdfDir);
		path.emplace_back(Spectrum::white * (1.0f / pdfPos), r.origin, nCamera, mCamera);
		Spectrum throughput = path[0].throughput * AbsDot(nCamera, r.direction) / pdfDir;
		s32 bounces = 1;
		Intersection isect;
		while (bounces < mMaxDepth)
		{
			if (!mScene->intersect(r, &isect))
				break;
			PathVertex pv;
			pv.throughput = throughput;
			pv.isect = isect;
			path.emplace_back(pv);
			++bounces;

			if (get_random_float() > mRussianRoulette)
				break;
			Vector3f wi;
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(Radiance);
			Real pdfFwd;
			Spectrum f = m->sample_f(isect.wo, &wi, isect.n, s.get2D(), &pdfFwd);
			if (pdfFwd == 0.f)
				break;
			throughput *= f * AbsDot(wi, isect.n) / (pdfFwd * mRussianRoulette);
			r = isect.spawnRay(wi);
		}
		return bounces;
	}

	void LightTracer::splatFilmT1(Point2i pixel, Sampler& sampler, RayPath& path)
	{
		Vector2f cameraSample = sampler.get2D() + Vector2f(pixel.x, pixel.y);
		Ray r;
		mCamera->generateRay(cameraSample, sampler.get2D(), &r);

		Vector3f nCamera;
		Real pdfPos, pdfDir;
		mCamera->Pdf_We(r, &nCamera, &pdfPos, &pdfDir);
		PathVertex v(Spectrum::white * (1.0f / pdfPos), r.origin, nCamera, mCamera);

		s32 ns = generateLightSubpath(path, sampler);

		for (s32 s = 1; s <= ns; ++s)
		{
			const PathVertex& pv = path[s - 1];
			Vector3f wi = v.isect.p - pv.isect.p;
			Real dist2 = dotProduct(wi, wi);
			wi = normalize(wi);
			Vector2f raster;
			Spectrum fsE = mCamera->We(Ray(v.isect.p, wi), &raster);
			if (fsE == Spectrum::black || mScene->intersectP(v.isect.spawnRayTo(pv.isect)))
				continue;
			Spectrum fsL;
			Real G;
			if (s > 1)
			{	
				const Material* m = pv.isect.obj->getMaterial();
				m->setTransportMode(Importance);
				Spectrum f = m->f(pv.isect.wo, wi, pv.isect.n);
				fsL = Spectrum::white * f;
				fsL = f * path[0].light->L(path[0].isect, -path[1].isect.wo);
				G = AbsDot(pv.isect.n, wi) * AbsDot(nCamera, wi) / dist2;
			}
			else
			{
				fsL = pv.light->L(pv.isect, wi);
				G = AbsDot(pv.isect.n, wi) * AbsDot(nCamera, wi) / dist2;
				
			}

			Spectrum contribution = fsL * fsE * G * pv.throughput * v.throughput;
			mutex1.lock();
			mFilm->addSplat(raster, contribution / mSpp);
			mutex1.unlock();
		}
	}

	void LightTracer::splatFilmS1(Point2i pixel, Sampler& sampler, RayPath& path)
	{
		const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
		int numLights = lights.size();
		if (numLights == 0) return;
		int idx = std::min<s32>(sampler.get1D() * numLights, numLights - 1);
		float pdf = 1.0f / numLights;
		float pdfPos, pdfDir;
		Ray r;
		Vector3f n;
		Spectrum Le = lights[idx]->sample_Le(sampler.get2D(), sampler.get2D(), 0, &r, &n, &pdfPos, &pdfDir);

		PathVertex lv(Spectrum::white * (1.0f / (pdfPos * pdf)),
			r.origin, n, lights[idx].get());

		s32 nt = generateCameraSubpath(path, sampler, pixel);

		for (s32 t = 1; t <= nt; ++t)
		{
			const PathVertex& pv = path[t - 1];
		
			if (mScene->intersectP(pv.isect.spawnRayTo(r.origin)))
				continue;
			Spectrum fsE;
			Vector3f wi = r.origin - pv.isect.p;
			Real dist2 = dotProduct(wi, wi);
			wi = normalize(wi);
			Vector2f raster;
			if (t > 1)
			{
				const Material* m = pv.isect.obj->getMaterial();
				m->setTransportMode(Radiance);
				Spectrum f = m->f(pv.isect.wo, wi, pv.isect.n);
				Spectrum We = path[0].camera->We(Ray(path[0].isect.p, path[1].isect.wo), &raster);
				fsE = We * f;
			}
			else
				fsE = mCamera->We(Ray(pv.isect.p, -wi), &raster);
			if (fsE == Spectrum::black)
				continue;
			Spectrum fsL = lights[idx]->L(lv.isect, -wi);
			Real G = AbsDot(pv.isect.n, wi) / dist2;
			G *= AbsDot(n, wi);

			Spectrum contribution = fsL * fsE * G * pv.throughput * lv.throughput;
			mutex1.lock();
			mFilm->addSplat(raster, contribution / mSpp);
			mutex1.unlock();
		}
	}
}