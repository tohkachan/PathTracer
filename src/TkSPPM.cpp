#include "TkSPPM.h"
#include "Scene.hpp"
#include "Object.hpp"
#include "Material.hpp"
#include "sampler.h"
#include "Bounds3.hpp"
#include "TkLowdiscrepancy.h"
#include "camera.h"

namespace tk
{
	struct PhotonCache
	{
		Spectrum Phi;
		s32 M;
		PhotonCache() : M(0) {}
	};

	class SpatialHashGrids
	{
	private:
		std::vector<std::vector<PixelData*>> mGrids;
		Bounds3 mGridsBounds;
		s32 mGridsRes[3];
		size_t mHashSize;

		s32 hash(s32 x, s32 y, s32 z)const;

	public:
		SpatialHashGrids(s32 pixelSize);

		void rebuild(std::vector<PixelData>& pixels);
		bool worldToGrid(const Vector3f& p,
			s32* pi)const;
		std::vector<PixelData*>* getGrid(const Vector3f& p);
		
	};

	SpatialHashGrids::SpatialHashGrids(s32 pixelSize)
		: mGrids(pixelSize), mHashSize(pixelSize)
	{

	}

	void SpatialHashGrids::rebuild(std::vector<PixelData>& pixels)
	{
		for (size_t i = 0; i < mGrids.size(); ++i)
			std::vector<PixelData*>().swap(mGrids[i]);
		mGridsBounds = Bounds3();
		Real maxRadius = Math::neg_infinity;
		std::vector<PixelData>::iterator it = pixels.begin();
		std::vector<PixelData>::iterator end = pixels.end();
		while (it != end)
		{
			if (it->throughput != Spectrum::black)
			{
				mGridsBounds = Union(mGridsBounds, it->isect.p - Vector3f(it->radius));
				mGridsBounds = Union(mGridsBounds, it->isect.p + Vector3f(it->radius));
				maxRadius = tk::max(maxRadius, it->radius);
			}
			++it;
		}

		Vector3f diag = mGridsBounds.Diagonal();
		Real maxDiag = diag[mGridsBounds.maxExtent()];
		s32 baseGridRes = (s32)(maxDiag / maxRadius);
		for (int i = 0; i < 3; ++i)
			mGridsRes[i] = std::max((s32)(baseGridRes * diag[i] / maxDiag), 1);

		it = pixels.begin();
		while (it != end)
		{
			if (it->throughput != Spectrum::black)
			{
				Real radius = it->radius;
				s32 pMin[3], pMax[3];
				worldToGrid(it->isect.p - Vector3f(radius), pMin);
				worldToGrid(it->isect.p + Vector3f(radius), pMax);
				for (s32 z = pMin[2]; z <= pMax[2]; ++z)
					for (s32 y = pMin[1]; y <= pMax[1]; ++y)
						for (s32 x = pMin[0]; x <= pMax[0]; ++x)
							mGrids[hash(x, y, z)].push_back(&(*it));
			}
			++it;
		}
	}

	bool SpatialHashGrids::worldToGrid(const Vector3f& p,
		s32* pi)const
	{
		bool inBounds = true;
		Vector3f pg = mGridsBounds.Offset(p);
		for (s32 i = 0; i < 3; ++i) {
			pi[i] = (s32)(mGridsRes[i] * pg[i]);
			inBounds &= (pi[i] >= 0 && pi[i] < mGridsRes[i]);
			pi[i] = Math::Clamp(pi[i], 0, mGridsRes[i] - 1);
		}
		return inBounds;
	}

	std::vector<PixelData*>* SpatialHashGrids::getGrid(const Vector3f& p)
	{
		s32 pi[3];
		if (!worldToGrid(p, pi))
			return nullptr;
		return &mGrids[hash(pi[0], pi[1], pi[2])];
	}

	s32 SpatialHashGrids::hash(s32 x, s32 y, s32 z)const
	{
		return ((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) % mHashSize;
	}

	SPPM::SPPM(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, SPPMParam param)
		: RayTracer(spp, maxDepth, numThreads, russianRoulette),
		numIteration(param.numRecurse),
		radius(param.radius2),
		photonsPerIt(param.globalSize)
	{
		s32 x = 0, y = 0;
		while (++x * ++y < numIteration);
		numIteration = x * y;
		mSampler = new StratifiedSampler(x, y, true);
	}

	void SPPM::visualize()
	{

	}

	void SPPM::rendering()
	{

	}

	void SPPM::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				sampler.startPixelSample(Point2i(x, y), curIteration);
				Vector2f cameraSample = sampler.get2D() + Vector2f(x, y);
				Ray r;
				mCamera->generateRay(cameraSample, sampler.get2D(), &r);
				PixelData& p = mPixels[x + y * mEndPos.x];
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
						p.Ld += coef * isect.Le(wo) / mSpp;
					p.Ld += coef * mScene->uniformSampleOneLight(isect, sampler) / mSpp;
					if (isDiffuse)
					{
						p.isect = isect;
						p.throughput = coef;
						break;
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
			}
		}
	}

	void SPPM::tracePhoton(s32 start, s32 end)
	{
		while (start < end)
		{
			u64 haltonIdx = (u64)curIteration * (u64)photonsPerIt + (start++);
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
			Spectrum weight = lights[lightIdx]->sample_Le(uLight0, uLight1, 0, &r, &normal, &pos_pdf, &dir_pdf);

			if (pos_pdf == 0 || dir_pdf == 0)
				return;
			weight = weight * AbsDot(r.direction, normal) / (pos_pdf * dir_pdf * pdf);
			for (s32 i = 0; i < mMaxDepth; ++i)
			{
				Intersection isect;
				if (!mScene->intersect(r, &isect))
					break;
				Vector3f wo = -r.direction;
				if (i > 0)
				{
					std::vector<PixelData*>* grid = mHashGrids->getGrid(isect.p);
					if (grid != nullptr)
					{
						std::vector<PixelData*>::iterator it = grid->begin();
						std::vector<PixelData*>::iterator end = grid->end();
						while (it != end)
						{
							PixelData* p = *it;
							Real radius = p->radius;
							Vector3f dir = p->isect.p - isect.p;
							if (dotProduct(dir, dir) <= radius * radius)
							{
								mutex1.lock();
								p->M++;
								const Material* m = p->isect.obj->getMaterial();
								m->setTransportMode(Radiance);
								Spectrum f = m->f(p->isect.wo, wo, p->isect.n);
								p->Phi += f * weight;
								mutex1.unlock();
							}
							++it;
						}
					}
				}

				if (get_random_float() > mRussianRoulette)
					break;
				Vector3f wi;
				float pdf = 0;
				Vector2f uBsdf(RadicalInverse(haltonDim, haltonIdx), RadicalInverse(haltonDim + 1, haltonIdx));
				const Material* m = isect.obj->getMaterial();
				m->setTransportMode(Importance);
				Spectrum f = m->sample_f(wo, &wi, isect.n, uBsdf, &pdf);
				if (pdf == 0 || f == Spectrum::black)
					break;
				weight = weight * f * AbsDot(wi, isect.n) / (pdf * mRussianRoulette);
				r = isect.spawnRay(wi);
			}
		}
	}

	void SPPM::updatePixel(s32 start, s32 end)
	{
		while (start < end)
		{
			const Real alpha = 0.7f;
			PixelData& p = mPixels[start++];
			if (p.throughput == Spectrum::black)
				continue;
			if (p.M > 0)
			{
				Real newN = p.N + alpha * p.M;
				Real newR = p.radius * Math::Sqrt(newN / (p.N + p.M));
				p.Tau = (p.Tau + p.throughput * p.Phi) * (newR * newR) / (p.radius * p.radius);
				p.N = newN;
				p.radius = newR;
				p.Phi = Spectrum::black;
				p.M = 0;
			}
			p.throughput = Spectrum::black;
		}
	}

	void SPPM::startRaytracing()
	{
		mPixels.resize(mEndPos.x * mEndPos.y, PixelData(radius));
		if (!mHashGrids)
			mHashGrids = new SpatialHashGrids(mEndPos.x * mEndPos.y);
		state = RayPass;
		curIteration = 0;
		RayTracer::startRaytracing();
	}

	unsigned long SPPM::updateWorkerThread(ThreadHandle* handle)
	{
		s32 idx = handle->getThreadIdx();
		std::unique_ptr<Sampler> tileSampler = mSampler->clone(idx);
		while (mContinueRendering)
		{
			switch (state)
			{
			case RayPass:
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
				if (start.y < end.y)
				{
					traceTile(start, end, *tileSampler);
					++mJobsDone;
				}
			}
				break;
			case PhotonPass:
			{
				mutex.lock();
				s32 start = photonStart;
				photonStart += 1024;
				mutex.unlock();
				s32 end = std::min(start + 1024, photonsPerIt);
				if (start < end)
				{
					tracePhoton(start, end);
					mJobsDone += end - start;
				}				
			}
				break;
			case RenewPass:
			{
				mutex.lock();
				s32 start = pixelStart;
				pixelStart += 4096;
				mutex.unlock();
				s32 end = std::min(start + 4096, mEndPos.x * mEndPos.y);
				if (start < end)
				{
					updatePixel(start, end);
					mJobsDone += end - start;
				}
			}
				break;
			}
		}

		return 0;
	}

	void SPPM::render(string filename)
	{
		startRaytracing();
		while (1)
		{
			switch (state)
			{
			case RayPass:
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
				if (start.y < end.y)
				{
					traceTile(start, end, *mSampler);
					++mJobsDone;
				}
				fprintf(stderr, "\r[Tracer] Ray trace pass... %02d%%", int((Real)(mJobsDone) / mJobsCount * 100));
				if (mJobsDone >= mJobsCount)
				{
					state = Idle;
					fprintf(stderr, "\n");
					mHashGrids->rebuild(mPixels);
					mJobsDone = 0;
					photonStart = 0;
					state = PhotonPass;
				}
			}
				break;
			case PhotonPass:
			{
				mutex.lock();
				s32 start = photonStart;
				photonStart += 1024;
				mutex.unlock();
				s32 end = std::min(start + 1024, photonsPerIt);		
				if (start < end)
				{
					tracePhoton(start, end);
					mJobsDone += end - start;
				}
				fprintf(stderr, "\r[Tracer] Photon trace pass... %02d%%", int((Real)(mJobsDone) / photonsPerIt * 100));
				if (mJobsDone >= photonsPerIt)
				{
					state = Idle;
					fprintf(stderr, "\n");
					mJobsDone = 0;
					pixelStart = 0;
					state = RenewPass;
				}

			}
				break;
			case RenewPass:
			{
				mutex.lock();
				s32 start = pixelStart;
				pixelStart += 4096;
				mutex.unlock();
				s32 end = std::min(start + 4096, mEndPos.x * mEndPos.y);
				if (start < end)
				{
					updatePixel(start, end);
					mJobsDone += end - start;
				}
				if (mJobsDone >= mPixels.size())
				{
					fprintf(stderr, "\r[Tracer] Iteration %d... Done!\n", curIteration);
					if (++curIteration < numIteration)
					{
						state = Idle;
						mJobsDone = 0;
						mNextStart = Point2i(0, 0);
						state = RayPass;
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
								const PixelData& p = mPixels[idx];
								Spectrum c = p.Ld / numIteration + p.Tau / (numIteration * photonsPerIt *
									Math::pi * p.radius * p.radius);
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