#include "TkSppmTracer.h"
#include "sampler.h"
#include "camera.h"
#include "Scene.hpp"
#include "Material.hpp"
#include "Object.hpp"
#include "TkLowdiscrepancy.h"

namespace tk
{
	SppmTracer::SppmTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, SPPMParam param)
		: RayTracer(spp, maxDepth, numThreads, russianRoulette),
		numRecurse(param.numRecurse),
		radius2(param.radius2),
		globalPhotonMap(param.globalSize, param.globalSample),
		causticPhotonMap(param.causticSize, param.causticSample)
	{
		s32 x = 0, y = 0;
		while (++x * ++y < numRecurse);
		numRecurse = x * y;
		mSampler = new StratifiedSampler(x, y, true);
		/*s32 x = 1, y = mSpp / x;
		while ((y != x) && (y / x != 2))
		{
			x <<= 1;
			y = mSpp / x;
		}
		mSampler = new StratifiedSampler(x, y, true);*/
	}

	void SppmTracer::visualize()
	{
		auto prims = mScene->get_objects();
		for (size_t i = 0; i < prims.size(); ++i)
			prims[i]->drawOutline(Spectrum(0.8, 0.8, 1.0), 0.5);
			//prims[i]->draw(Spectrum(0.8, 0.8, 1.0), 0.6);
		if (shootCausticPhotons > 0)
			causticPhotonMap.render_photons();
		else
			globalPhotonMap.render_photons();
	}

	void SppmTracer::rendering()
	{
		switch (state)
		{
		case Generate:
			if (storedGlobalPhotons < globalPhotonMap.size())
				fprintf(stderr, "\r[Tracer] Generate global photon map... %02d%%", int((Real)(storedGlobalPhotons) / globalPhotonMap.size() * 100));
			else if (storedCausticPhotons < causticPhotonMap.size())
				fprintf(stderr, "\r[Tracer] Generate caustic photon map... %02d%%", int((Real)storedCausticPhotons / causticPhotonMap.size() * 100));
			else
			{
				mNextStart = Point2i(0, 0);
				mJobsDone = 0;
				fprintf(stderr, "\r[Tracer] Generate caustic photon map... 100%%\n");
				fprintf(stderr, "\r[Tracer] Shoot global photons %d\n", shootGlobalPhotons);
				fprintf(stderr, "\r[Tracer] Shoot caustic photons %d\n", shootCausticPhotons);
				globalPhotonMap.buildKdTree();
				if (storedCausticPhotons > 0)
					causticPhotonMap.buildKdTree();
				state = Rendering;
			}
			break;
		case Rendering:
			fprintf(stderr, "\r[Tracer] Iteration %d... %02d%%", curRecurse, int((Real)(mJobsDone) / mJobsCount * 100));
			if (mJobsDone >= mJobsCount)
			{
				fprintf(stderr, "\r[Tracer] Iteration %d... 100%%\n", curRecurse);
				if (++curRecurse < numRecurse)
				{
					storedGlobalPhotons = 0;
					storedCausticPhotons = 0;
					globalPhotonMap.reset();
					causticPhotonMap.reset();
					state = Generate;
				}
				else
				{
					stopRaytracing();
					fprintf(stderr, "\r\n[Tracer] Rendering Done!");
					mState = DONE;
				}
			}
			for (s32 i = 0; i < mEndPos.x * mEndPos.y; ++i)
			{
				const SPPMPixel& p = pp[i];
				Spectrum c = p.Ld / numRecurse + (storedCausticPhotons > 0 ?
					p.causticFlux / (Math::pi * p.causticRadius2 * shootCausticPhotons) : Spectrum::black) +
					p.globalFlux / (Math::pi * p.globalRadius2 * shootGlobalPhotons);
				u32 v = 0;
				v |= (u32)(255 * Math::Pow(Math::Clamp(c.b, 0.0f, 1.0f), 0.6)) << 16;
				v |= (u32)(255 * Math::Pow(Math::Clamp(c.g, 0.0f, 1.0f), 0.6)) << 8;
				v |= (u32)(255 * Math::Pow(Math::Clamp(c.r, 0.0f, 1.0f), 0.6));
				v |= 0xFF000000;
				mFrameBuffer[i] = v;
			}
			break;
		}
		glDrawPixels(mEndPos.x, mEndPos.y, GL_RGBA,
			GL_UNSIGNED_BYTE, mFrameBuffer);
	}

	void SppmTracer::tracePhoton(u64 haltonIdx)
	{
		s32 haltonDim = 0;
		const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
		int numLights = lights.size();
		if (numLights == 0) return;
		int lightIdx = std::min<int>(RadicalInverse(haltonDim++, haltonIdx) * numLights, numLights - 1);
		float pdf = 1.0f / numLights;
		float pos_pdf, dir_pdf;
		Ray r;
		Vector3f normal;
		Vector2f uLight0(RadicalInverse(haltonDim, haltonIdx), RadicalInverse(haltonDim + 1, haltonIdx));
		Vector2f uLight1(RadicalInverse(haltonDim + 2, haltonIdx), RadicalInverse(haltonDim + 3, haltonIdx));
		haltonDim += 4;
		Spectrum power = lights[lightIdx]->sample_Le(uLight0, uLight1, 0, &r, &normal, &pos_pdf, &dir_pdf);

		if (pos_pdf == 0 || dir_pdf == 0)
			return;
		power = power * AbsDot(r.direction, normal) / (pos_pdf * dir_pdf * pdf);
		bool exit = false;
		for (int i = 0; i < mMaxDepth; ++i)
		{
			Intersection isect;
			if (!mScene->intersect(r, &isect))
				break;
			Vector3f wo = -r.direction;
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(Importance);
			bool isDiffuse = m->getType() == DIFFUSE;
			if (isDiffuse)
			{
				int theta = std::min(int(255 * acos(wo.z) * Math::inv_pi), 255);
				int phi = 255 * atan2(wo.y, wo.x) / (2 * Math::pi);
				if (phi < 0)
					phi += 255;
				mutex1.lock();
				exit = !globalPhotonMap.storePhoton(isect.p, power, theta, std::min(phi, 255), storedGlobalPhotons);
				mutex1.unlock();
			}

			if (exit || get_random_float() > mRussianRoulette)
				break;

			Vector3f wi;
			float pdf = 0;
			Vector2f uBsdf(RadicalInverse(haltonDim, haltonIdx), RadicalInverse(haltonDim + 1, haltonIdx));
			Spectrum f = m->sample_f(wo, &wi, isect.n, uBsdf, &pdf);
			if (pdf == 0 || f == Spectrum::black)
				break;
			power = power * f * AbsDot(wi, isect.n) /
				(pdf * mRussianRoulette);
			r = isect.spawnRay(wi);
		}
	}

	void SppmTracer::traceCausticPhoton(u64 haltonIdx)
	{
		s32 haltonDim = 0;
		const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
		int numLights = lights.size();
		if (numLights == 0) return;
		int lightIdx = std::min<int>(RadicalInverse(haltonDim++, haltonIdx) * numLights, numLights - 1);
		float pdf = 1.0f / numLights;
		float pos_pdf, dir_pdf;
		Ray r;
		Vector3f normal;
		Vector2f uLight0(RadicalInverse(haltonDim, haltonIdx), RadicalInverse(haltonDim + 1, haltonIdx));
		Vector2f uLight1(RadicalInverse(haltonDim + 2, haltonIdx), RadicalInverse(haltonDim + 3, haltonIdx));
		haltonDim += 4;
		Spectrum power = lights[lightIdx]->sample_Le(uLight0, uLight1, 0, &r, &normal, &pos_pdf, &dir_pdf);

		if (pos_pdf == 0 || dir_pdf == 0)
			return;
		power = power * AbsDot(r.direction, normal) / (pos_pdf * dir_pdf * pdf);
		bool hasGlossy = false;
		for (int i = 0; i < 64; ++i)
		{
			Intersection isect;
			if (!mScene->intersect(r, &isect))
				break;
			Vector3f wo = -r.direction;
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(Importance);
			bool isDiffuse = m->getType() == DIFFUSE;
			hasGlossy |= !isDiffuse;
			if (isDiffuse)
			{
				if (hasGlossy)
				{
					int theta = std::min(int(255 * acos(wo.z) * Math::inv_pi), 255);
					int phi = 255 * atan2(wo.y, wo.x) / (2 * Math::pi);
					if (phi < 0)
						phi += 255;
					mutex1.lock();
					causticPhotonMap.storePhoton(isect.p, power, theta, std::min(phi, 255), storedCausticPhotons);
					mutex1.unlock();
				}				
				break;
			}

			if (get_random_float() > mRussianRoulette)
				break;

			Vector3f wi;
			float pdf = 0;
			Vector2f uBsdf(RadicalInverse(haltonDim, haltonIdx), RadicalInverse(haltonDim + 1, haltonIdx));
			Spectrum f = m->sample_f(wo, &wi, isect.n, uBsdf, &pdf);
			if (pdf == 0 || f == Spectrum::black)
				break;
			power = power * f * AbsDot(wi, isect.n) /
				(pdf * mRussianRoulette);
			r = isect.spawnRay(wi);
		}
	}

	/*void SppmTracer::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		//std::unique_ptr<FilmTile> filmTile = mFilm->getFilmTile(Bounds2i(start, end));
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				Spectrum L;			
				SPPMPixel& p = pp[x + y * mEndPos.x];
				sampler.startPixel(Point2i(x, y));
				do {
					Vector2f cameraSample = sampler.get2D() + Vector2f(x, y);
					Ray r;
					mCamera->generateRay(cameraSample, sampler.get2D(), &r);
				
					Spectrum coef(1.f, 1.f, 1.f);
					Intersection isect;
					bool hasSpecular = true;
					for (int i = 0; i < mMaxDepth; ++i)
					{
						if (!mScene->intersect(r, &isect))
							break;
						const Material* m = isect.obj->getMaterial();
						m->setTransportMode(Radiance);
						bool isDiffuse = m->getType() == DIFFUSE;
						Vector3f wo = -r.direction;
						if (hasSpecular)
							L += (coef * isect.Le(wo)) / mSpp;
						if (hasSpecular && storedCausticPhotons > 0)
							L += coef * mScene->uniformSampleOneLight(isect, sampler) / mSpp;
						if (isDiffuse)
						{
							float radius2;
							int numPhotons;
							Spectrum Lindir;
							if (hasSpecular && storedCausticPhotons > 0)
							{
								radius2 = p.causticRadius2;
								Lindir = (coef * causticPhotonMap.radiance_estimate(isect, radius2, numPhotons)) / mSpp;
								L += Lindir / (Math::pi * radius2 * shootCausticPhotons);
							}
							else
							{
								radius2 = p.globalRadius2;
								Lindir = (coef * globalPhotonMap.radiance_estimate(isect, radius2, numPhotons)) / mSpp;
								L += Lindir / (Math::pi * radius2 * shootGlobalPhotons);
								break;
							}
						}
						hasSpecular &= (m->getType() & (MIRROR | GLASS)) != 0;
						Vector3f wi;
						float pdf;
						Spectrum f = m->sample_f(wo, &wi, isect.n, sampler.get2D(), &pdf);
						if (f == Spectrum::black || pdf == 0)
							break;
						coef = coef * f * AbsDot(wi, isect.n) / pdf;
						r = isect.spawnRay(wi);
					}
				} while (sampler.startNextSample());
				p.Ld = L;
				p.globalFlux = Spectrum::black;
				//if (p.numPhotons > 60)
					//return Spectrum(p.numPhotons / 100, 0, 0);
				//else if (p.numPhotons > 30)
					//return Spectrum(0, p.numPhotons / 60, 0);
				//else
					//return Spectrum(0, 0, p.numPhotons / 30);
			}
		}
	}*/

	void SppmTracer::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				SPPMPixel& p = pp[x + y * mEndPos.x];
				sampler.startPixelSample(Point2i(x, y), curRecurse);
				Vector2f cameraSample = sampler.get2D() + Vector2f(x, y);
				Ray r;
				mCamera->generateRay(cameraSample, sampler.get2D(), &r);

				Spectrum coef(1.f, 1.f, 1.f);
				Intersection isect;
				bool hasGlossy = true;

				for (int i = 0; i < mMaxDepth; ++i)
				{
					if (!mScene->intersect(r, &isect))
						break;
					const Material* m = isect.obj->getMaterial();
					m->setTransportMode(Radiance);
					bool isDiffuse = m->getType() == DIFFUSE;
					Vector3f wo = -r.direction;
					if (hasGlossy)
						p.Ld += coef * isect.Le(wo);
					if (hasGlossy || !isDiffuse)
						p.Ld += coef * mScene->uniformSampleOneLight(isect, sampler);
					if (isDiffuse)
					{
						float radius2;
						int numPhotons;
						Spectrum Lindir;
						if (hasGlossy && storedCausticPhotons > 0)
						{
							radius2 = p.causticRadius2;
							Lindir = (coef * causticPhotonMap.radiance_estimate(isect, radius2, numPhotons));
							if (p.causticPhotons == 0)
							{
								p.causticRadius2 = radius2;
								p.causticPhotons = numPhotons;
								p.causticFlux = Lindir;
							}
							else
							{
								Real newPhotons = p.causticPhotons + 0.7 * numPhotons;
								Real newRadius2 = p.causticRadius2 * newPhotons / (p.causticPhotons + numPhotons);
								p.causticFlux = (p.causticFlux + Lindir) * newRadius2 / p.causticRadius2;
								p.causticPhotons = newPhotons;
								p.causticRadius2 = newRadius2;
							}
						}
						if (!hasGlossy)
						{
							radius2 = p.globalRadius2;
							Lindir = (coef * globalPhotonMap.radiance_estimate(isect, radius2, numPhotons));
							if (p.globalPhotons == 0)
							{
								p.globalRadius2 = radius2;
								p.globalPhotons = numPhotons;
								p.globalFlux = Lindir;
							}
							else
							{
								float newPhotons = p.globalPhotons + 0.7 * numPhotons;
								float newRadius2 = p.globalRadius2 * newPhotons / (p.globalPhotons + numPhotons);
								p.globalFlux = (p.globalFlux + Lindir) * newRadius2 / p.globalRadius2;
								p.globalPhotons = newPhotons;
								p.globalRadius2 = newRadius2;
							}
							break;
						}
					}
					hasGlossy &= !isDiffuse;
					Vector3f wi;
					float pdf;
					Spectrum f = m->sample_f(wo, &wi, isect.n, sampler.get2D(), &pdf);
					if (f == Spectrum::black || pdf == 0)
						break;
					coef = coef * f * AbsDot(wi, isect.n) / pdf;
					r = isect.spawnRay(wi);
				}
			}
		}
	}

	void SppmTracer::startVisualizing()
	{
		if (shootCausticPhotons > 0)
			causticPhotonMap.update_photons();
		else
			globalPhotonMap.update_photons();
		RayTracer::startVisualizing();
	}

	void SppmTracer::startRaytracing()
	{	
		shootGlobalPhotons = 0;
		shootCausticPhotons = 0;
		storedGlobalPhotons = 0;
		storedCausticPhotons = 0;
		curRecurse = 0;
		globalPhotonMap.reset();
		causticPhotonMap.reset();
		pp.clear();
		pp.resize(mEndPos.x * mEndPos.y, SPPMPixel(radius2));
		state = Generate;
		RayTracer::startRaytracing();
	}

	unsigned long SppmTracer::updateWorkerThread(ThreadHandle* handle)
	{
		s32 idx = handle->getThreadIdx();
		std::unique_ptr<Sampler> tileSampler = mSampler->clone(idx);
		while (mContinueRendering)
		{
			switch (state)
			{
			case Generate:
			{
				s32 haltonIdx = shootGlobalPhotons;
				while (storedGlobalPhotons < globalPhotonMap.size())
				{
					tracePhoton(haltonIdx);
					haltonIdx = ++shootGlobalPhotons;
				}
				haltonIdx = shootCausticPhotons;
				while (storedCausticPhotons < causticPhotonMap.size())
				{
					traceCausticPhoton(haltonIdx + shootGlobalPhotons);
					haltonIdx = ++shootCausticPhotons;
				}
			}
				break;
			case Rendering:
			{
				mutex.lock();
				Point2i start = mNextStart;
				Point2i end = mNextStart + Point2i(mTileSize, mTileSize);
				mNextStart.x += mTileSize;
				if (mNextStart.x >= mEndPos.x)
				{
					mNextStart.x = 0;
					end.x = mEndPos.x;
					mNextStart.y += mTileSize;
				}
				mutex.unlock();
				end.y = std::min(end.y, mEndPos.y);
				traceTile(start, end, *tileSampler);
				++mJobsDone;
			}
				break;
			}					
		}
		return 0;
	}

	void SppmTracer::render(string filename)
	{
		startRaytracing();
		while (1)
		{
			switch (state)
			{
			case Generate:
			{
				s32 haltonIdx = shootGlobalPhotons;
				while (storedGlobalPhotons < globalPhotonMap.size())
				{
					tracePhoton(haltonIdx);
					haltonIdx += ++shootGlobalPhotons;
					fprintf(stderr, "\r[Tracer] Generate global photon map... %02d%%", int((Real)(storedGlobalPhotons) / globalPhotonMap.size() * 100));
				}
				fprintf(stderr, "\r[Tracer] Generate global photon map... 100%%\n");
				haltonIdx = shootCausticPhotons;
				while (storedCausticPhotons < causticPhotonMap.size())
				{
					traceCausticPhoton(haltonIdx + shootGlobalPhotons);
					haltonIdx = ++shootCausticPhotons;
					fprintf(stderr, "\r[Tracer] Generate caustic photon map... %02d%%", int((Real)storedCausticPhotons / causticPhotonMap.size() * 100));
				}
				fprintf(stderr, "\r[Tracer] Generate caustic photon map... 100%%\n");
				mNextStart = Point2i(0, 0);
				mJobsDone = 0;	
				fprintf(stderr, "\r[Tracer] Shoot global photons %d\n", shootGlobalPhotons);
				fprintf(stderr, "\r[Tracer] Shoot caustic photons %d\n", shootCausticPhotons);
				globalPhotonMap.buildKdTree();
				if (storedCausticPhotons > 0)
					causticPhotonMap.buildKdTree();
				state = Rendering;
			}
				break;
			case Rendering:
			{
				mutex.lock();
				Point2i start = mNextStart;
				Point2i end = mNextStart + Point2i(mTileSize, mTileSize);
				mNextStart.x += mTileSize;
				if (mNextStart.x >= mEndPos.x)
				{
					mNextStart.x = 0;
					end.x = mEndPos.x;
					mNextStart.y += mTileSize;
				}
				mutex.unlock();
				end.y = std::min(end.y, mEndPos.y);
				traceTile(start, end, *mSampler);
				fprintf(stderr, "\r[Tracer] Iteration %d... %02d%%", curRecurse, int((Real)(++mJobsDone) / mJobsCount * 100));
				if (mJobsDone >= mJobsCount)
				{
					fprintf(stderr, "\r[Tracer] Iteration %d... 100%%\n", curRecurse);
					if (++curRecurse < numRecurse)
					{
						storedGlobalPhotons = 0;
						storedCausticPhotons = 0;
						globalPhotonMap.reset();
						causticPhotonMap.reset();
						state = Generate;
					}
					else
					{
						stopRaytracing();
						fprintf(stderr, "\r[Tracer] Rendering Done!");
						mState = DONE;
						u8* frame = new u8[mEndPos.x * mEndPos.y * 3];
						s32 offset = 0;
						for (s32 y = mEndPos.y; y-- > 0; )
						{
							for (s32 x = 0; x < mEndPos.x; ++x)
							{
								s32 idx = x + y * mEndPos.x;
								const SPPMPixel& p = pp[idx];
								Spectrum c = p.Ld / numRecurse + (storedCausticPhotons > 0 ?
									p.causticFlux / (Math::pi * p.causticRadius2 * shootCausticPhotons) : Spectrum::black) +
									p.globalFlux / (Math::pi * p.globalRadius2 * shootGlobalPhotons);
								frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.r, 0.0f, 1.0f), 0.6);
								frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.g, 0.0f, 1.0f), 0.6);
								frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.b, 0.0f, 1.0f), 0.6);
							}
						}
						FILE* fp = fopen(filename.c_str(), "wb");
						(void)fprintf(fp, "P6\n%d %d\n255\n", mEndPos.x, mEndPos.y);
						fprintf(stderr, "\n[Tracer] Saving to file: %s... ", filename.c_str());
						fwrite(frame, 1, mEndPos.x * mEndPos.y * 3, fp);
						fprintf(stderr, "Done!\n");
						fclose(fp);
						delete[] frame;
						return;
					}
				}
			}
				break;
			}
		}
	}
}