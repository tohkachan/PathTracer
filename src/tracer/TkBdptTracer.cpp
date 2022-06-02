#include "TkBdptTracer.h"
#include "camera.h"
#include "Scene.hpp"
#include "sampler.h"
#include "Intersection.hpp"
#include "Object.hpp"

#include <GL/glew.h>

namespace tk
{
	template <typename T>
	class ScopedAssignment
	{
	private:
		T *target, backup;
	public:
		ScopedAssignment(T *target = nullptr, T val = T())
			: target(target)
		{
			if (target)
			{
				backup = *target;
				*target = val;
			}
		}

		ScopedAssignment(const ScopedAssignment&) = delete;
		ScopedAssignment& operator=(const ScopedAssignment&) = delete;
		ScopedAssignment& operator=(ScopedAssignment &&other)
		{
			if (target) *target = backup;
			target = other.target;
			backup = other.backup;
			other.target = nullptr;
			return *this;
		}

		~ScopedAssignment()
		{
			if (target) *target = backup;
		}
	};

	struct PathVertex
	{
		eVertexType type;
		Intersection isect;
		Spectrum throughput;
		Real pdfFwd;
		Real pdfRev;
		bool delta;
		bool isSurface;
		const AreaLight* light;
		const Camera* camera;

		PathVertex()
			: pdfFwd(0), pdfRev(0), delta(false), isSurface(false), light(nullptr),
			camera(nullptr) {}
		Spectrum f(const PathVertex& towards, eTransportMode mode)const
		{
			Vector3f wi = normalize(towards.isect.p - isect.p);
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(mode);
			return m->f(isect.wo, wi, isect.n);
		}
		Real convertDensity(const PathVertex& towards, Real pdf)const
		{
			Vector3f d = towards.isect.p - isect.p;
			Real g = dotProduct(d, d);
			if (g == 0) return 0;
			g = 1 / g;
			if (towards.isSurface)
				pdf *= AbsDot(d * Math::Sqrt(g), towards.isect.n);
			return pdf * g;
		}
	};

	BdptTracer::BdptTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette, s32 debugS, s32 debugT, bool debugNoMIS)
		: RayTracer(spp, maxDepth, numThreads, russianRoulette),
		mDebugS(debugS), mDebugT(debugT),
		mDebugNoMIS(debugNoMIS)
	{
		s32 x = 1, y = mSpp / x;
		while ((y != x) && (y / x != 2))
		{
			x <<= 1;
			y = mSpp / x;
		}
		mSampler = new StratifiedSampler(x, y, true);
	}

	void BdptTracer::visualize()
	{

	}

	void BdptTracer::rendering()
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

	s32 BdptTracer::generateCameraSubpath(s32 minLength, const Ray& r, RayPath& path, Sampler& s)
	{
		PathVertex pv;
		Real pdfPos, pdfDir;
		mCamera->Pdf_We(r, &pv.isect.n, &pdfPos, &pdfDir);
		pv.camera = mCamera;
		pv.type = CAMERA;
		pv.isect.p = r.origin;
		pv.throughput = Spectrum::white * (1.0f / pdfPos);
		pv.pdfFwd = pdfPos;
		pv.delta = false; // must be false importance !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		path[0] = pv;
		// throughput = 1!!!! We * L(film->lens) * cosTheta / [pdf(fileArea) * pdf(lens->film)] = L(film->lens)
		Spectrum throughput = pv.throughput * mCamera->We(Ray(r.origin, -r.direction)) * AbsDot(pv.isect.n, r.direction) / pdfDir;
		return pathWalk(throughput, r, minLength, pdfDir, path, s, Radiance) + 1;
	}

	s32 BdptTracer::generateLightSubpath(s32 minLength, RayPath& path, Sampler& s)
	{
		const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
		int numLights = lights.size();
		if (numLights == 0) return 0;
		int idx = std::min<s32>(s.get1D() * numLights, numLights - 1);
		Real pdfChoice = 1.0f / numLights;
		Real pdfPos, pdfDir;
		Ray r;
		Vector3f n;
		Spectrum L = lights[idx]->sample_Le(s.get2D(), s.get2D(), 0, &r, &n, &pdfPos, &pdfDir);
		if (pdfPos == 0 || pdfDir == 0 || L == Spectrum::black) return 0;
		PathVertex pv;
		pv.type = LIGHT;
		pv.light = lights[idx].get();
		pv.isect.p = r.origin;
		pv.isect.n = n;
		pv.pdfFwd = pdfPos * pdfChoice;
		pv.throughput = Spectrum::white * (1.0f / (pdfPos * pdfChoice));
		pv.isSurface = true;
		pv.delta = false; //isDeltaLight ?
		path[0] = pv;
		Spectrum throughput = pv.throughput * L * AbsDot(n, r.direction) / pdfDir;
	
		return pathWalk(throughput, r, minLength, pdfDir, path, s, Importance) + 1;
	}

	s32 BdptTracer::pathWalk(Spectrum throughput, Ray r, s32 minDepth, Real pdfFwd, RayPath& path,
		Sampler& s, eTransportMode mode)
	{
		s32 bounces = 0;
		Real pdfRev = 0;
		Intersection isect;
		while (bounces < mMaxDepth)
		{
			PathVertex pv;
			PathVertex& prev = path[bounces];
			if (!mScene->intersect(r, &isect))
				break;
			pv.throughput = throughput;
			pv.type = SURFACE;
			pv.isect = isect;
			pv.isSurface = true;
			pv.pdfFwd = prev.convertDensity(pv, pdfFwd);
			path[++bounces] = pv;

			float RussianRoulette = bounces < minDepth ? 1.0 : mRussianRoulette;
			if (get_random_float() > RussianRoulette)
				break;
			{
				Vector3f wi;
				const Material* m = isect.obj->getMaterial();
				m->setTransportMode(mode);
				Spectrum f = m->sample_f(isect.wo, &wi, isect.n, s.get2D(), &pdfFwd);
				if (f == Spectrum::black || pdfFwd == 0)
					break;
				throughput = throughput * f * AbsDot(wi, isect.n) / (pdfFwd * RussianRoulette);
				if (std::isnan(throughput.r))
					fprintf(stderr, "\nPostThroughout nan, pdf: %f, f: (%f, %f, %f), material type: %d, bounces: %d\n",
						pdfFwd, f.r, f.g, f.b, m->getType(), bounces);
				if (m->isDelta)
				{
					pv.delta = true;
					pdfRev = pdfFwd = 0;
				}
				else
					pdfRev = m->Pdf(wi, isect.wo, isect.n);
				prev.pdfRev = pv.convertDensity(prev, pdfRev);
				r = isect.spawnRay(wi);
			}
		}
		return bounces;
	}

	Spectrum BdptTracer::connectPath(RayPath& eyePath, RayPath& lightPath, s32 s, s32 t,
		Sampler& sampler, Vector2f *raster)
	{
		Spectrum aL, aE, Cst;
		PathVertex sampled;
		if (s == 0)
		{	
			const PathVertex& vt = eyePath[t - 1];
			aL = Spectrum::white;
			aE = vt.throughput;
			Cst = vt.isect.Le(vt.isect.wo);
		}
		else if (t == 1)
		{
			const PathVertex& vs = lightPath[s - 1];	
			Vector3f wi;
			Real pdf;
			aL = vs.throughput;
			// aE = We / pdf(lensArea), but now pdf(lensArea) is contained in Cst
			aE = mCamera->Sample_Wi(vs.isect, sampler.get2D(), &wi, &sampled.isect, &pdf, raster);
			if (aE == Spectrum::black || mScene->intersectP(sampled.isect.spawnRayTo(vs.isect)))
				return Spectrum::black;
			sampled.camera = mCamera;
			sampled.type = CAMERA;
			sampled.delta = false; //must be false!!!!!!!!!!!!!!!!!!!!!!
			//sampled.pdfFwd = 1.0f / lensArea
			Cst = vs.f(sampled, Importance) / pdf;
			if (vs.isSurface) Cst *= AbsDot(wi, vs.isect.n);
			//if (s == 1)
				//L = vs.throughput * vs.light->L(vs.isect, wi) * sampled.throughput;
		}
		else if (s == 1)
		{
			const PathVertex& vt = eyePath[t - 1];
			const std::vector<std::shared_ptr<AreaLight>>& lights = mScene->get_lights();
			int numLights = lights.size();
			if (numLights == 0) return Spectrum::black;
			int idx = std::min<int>(sampler.get1D() * numLights, numLights - 1);
			Real pdfChoice = 1.0f / numLights;
			Real pdfLight;	// pdfPos * squaredDistance(t, s) / AbsDot(wi, nLight)
			Vector3f wi;
			Spectrum Le = lights[idx]->sample_Li(vt.isect, sampler.get1D(), sampler.get2D(), &sampled.isect, &wi, &pdfLight);
			if (Le == Spectrum::black || mScene->intersectP(sampled.isect.spawnRayTo(vt.isect)))
				return Spectrum::black;
			Real pdfPos, pdfDir;
			lights[idx]->pdf_Le(Ray(sampled.isect.p, -wi), sampled.isect.n, &pdfPos, &pdfDir);
			sampled.pdfFwd = pdfPos * pdfChoice;
			sampled.light = lights[idx].get();
			sampled.type = LIGHT;
			sampled.isSurface = true;
			// aL = 1.0f / (pdfPos * pdfChoice), but now put it in cst
			aL = Spectrum::white;
			aE = vt.throughput;
			Cst = Le * vt.f(sampled, Radiance) / (pdfLight * pdfChoice);
			if (vt.isSurface) Cst *= AbsDot(wi, vt.isect.n);
		}
		else
		{
			const PathVertex& vs = lightPath[s - 1];
			const PathVertex& vt = eyePath[t - 1];
			aL = vs.throughput;
			aE = vt.throughput;
			Real g = G(vs.isect, vt.isect);
			if (g == 0)
				return Spectrum::black;
			Cst = vs.f(vt, Importance) * g * vt.f(vs, Radiance);
		}
		Spectrum L = aL * Cst * aE;
		if (L == Spectrum::black)
			return Spectrum::black;

		Real misWeight = mDebugNoMIS ? 1.0f : MISWeight(eyePath, lightPath, s, t, sampled);
		return L * misWeight;
	}

	Real BdptTracer::G(const Intersection& p0, const Intersection& p1)const
	{
		if (mScene->intersectP(p0.spawnRayTo(p1)))
			return 0;
		Vector3f d = p1.p - p0.p;
		Real g = dotProduct(d, d);
		if (g == 0)
			return 0;
		g = 1.0f / g;
		d = d * Math::Sqrt(g);
		return g * AbsDot(d, p0.n) * AbsDot(d, p1.n);

	}

	Real BdptTracer::MISWeight(RayPath& eyePath, RayPath& lightPath, s32 s, s32 t,
		PathVertex& sampled)
	{
		if (s + t == 2) return 1;

		PathVertex *vs = s > 0 ? &lightPath[s - 1] : nullptr,
			*vt = t > 0 ? &eyePath[t - 1] : nullptr,
			*vsMinus = s > 1 ? &lightPath[s - 2] : nullptr,
			*vtMinus = t > 1 ? &eyePath[t - 2] : nullptr;
		ScopedAssignment<PathVertex> a1;
		if (s == 1)
			a1 = { vs, sampled };
		else if (t == 1)
			a1 = { vt, sampled };

		ScopedAssignment<Real> a2, a3, a4, a5;
		ScopedAssignment<bool> a6, a7;
		a6 = { &vt->delta, false };
		if (s == 0)
		{
			Real pdfChoice, pdfPos, pdfDir;
			pdfChoice = 1.0f / mScene->get_lights().size();
			vt->isect.obj->getAreaLight()->pdf_Le(Ray(vt->isect.p, vt->isect.wo), vt->isect.n, &pdfPos, &pdfDir);

			a2 = { &vt->pdfRev, pdfChoice * pdfPos };
			a3 = { &vtMinus->pdfRev, vt->convertDensity(*vtMinus, pdfDir) };
		}
		else
		{
			a7 = { &vs->delta, false };
			const Vector3f& ps = vs->isect.p;
			const Vector3f& ns = vs->isect.n;
			const Vector3f& pt = vt->isect.p;
			const Vector3f& nt = vt->isect.n;
			Vector3f s2t = pt - ps;
			Real g = 1.0f / dotProduct(s2t, s2t);
			s2t = s2t * Math::Sqrt(g);
			if (s == 1)
			{
				Real pdfPos, pdfDir;
				vs->light->pdf_Le(Ray(ps, s2t), ns, &pdfPos, &pdfDir);
				if (vt->isSurface)
					pdfDir *= AbsDot(nt, s2t);
				a2 = { &vt->pdfRev, pdfDir * g };
			}
			else
			{
				Vector3f s2sMinus = normalize(vsMinus->isect.p - ps);
				const Material* m = vs->isect.obj->getMaterial();
				Real pdfRev = m->Pdf(s2sMinus, s2t, ns);
				if (vt->isSurface)
					pdfRev *= AbsDot(nt, s2t);
				a2 = { &vt->pdfRev, pdfRev * g };
				a5 = { &vsMinus->pdfRev, vs->convertDensity(*vsMinus, m->Pdf(s2t, s2sMinus, ns)) };
			}

			if (t == 1)
			{
				Real pdfPos, pdfDir;
				vt->camera->Pdf_We(Ray(pt, -s2t), nullptr, &pdfPos, &pdfDir);
				if (vs->isSurface)
					pdfDir *= AbsDot(ns, s2t);
				a4 = { &vs->pdfRev, pdfDir * g };
			}
			else
			{
				Vector3f t2tMinus = normalize(vtMinus->isect.p - pt);
				const Material* m = vt->isect.obj->getMaterial();
				Real pdfRev = m->Pdf(t2tMinus, -s2t, nt);
				if (vs->isSurface)
					pdfRev *= AbsDot(ns, s2t);
				a4 = { &vs->pdfRev, pdfRev * g };
				a3 = { &vtMinus->pdfRev, vt->convertDensity(*vtMinus, m->Pdf(-s2t, t2tMinus, nt)) };		
			}
		}

		auto remap0 = [](Real f)->Real { return f != 0 ? f : 1; };

		Real pK = 1.0f;
		Real misWeightSum = 1.0f;
		for (s32 i = t - 1; i > 0; --i)
		{
			pK *= remap0(eyePath[i].pdfRev) / remap0(eyePath[i].pdfFwd);

			if (!eyePath[i].delta && !eyePath[i - 1].delta) misWeightSum += pK * pK;
		}
		pK = 1.0f;
		for (s32 i = s - 1; i >= 0; --i)
		{
			pK *= remap0(lightPath[i].pdfRev) / remap0(lightPath[i].pdfFwd);
			bool test = i > 0 ? lightPath[i - 1].delta : false;
			if (!lightPath[i].delta && !test) misWeightSum += pK * pK;
		}

		return 1.0f / misWeightSum;
	}

	void BdptTracer::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		std::unique_ptr<FilmTile> filmTile = mFilm->getFilmTile(Bounds2i(start, end));
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				Point2i pixel = Point2i(x, y);
				sampler.startPixel(pixel);
				RayPath eyePath(mMaxDepth + 1);
				RayPath lightPath(mMaxDepth + 1);
				do
				{
					Vector2f cameraSample = sampler.get2D() + Vector2f(pixel.x, pixel.y);
					Ray r;
					mCamera->generateRay(cameraSample, sampler.get2D(), &r);
					
					s32 ne = generateCameraSubpath(6, r, eyePath, sampler);
					s32 nl = generateLightSubpath(6, lightPath, sampler);
				
					Spectrum L;
					for (s32 t = 1; t <= ne; ++t)
					{
						for (s32 s = 0; s <= nl; ++s)
						{
							s32 depth = t + s - 2;
							if ((s == 1 && t == 1) || depth < 0 || depth > mMaxDepth * 2)
								continue;
							if (((mDebugS != -1) && (mDebugS != s)) ||
								((mDebugT != -1) && (mDebugT != t)))
								continue;
							Vector2f raster;
							Spectrum Lpath = connectPath(eyePath, lightPath, s, t, sampler, &raster);
							if (Lpath == Spectrum::black)
								continue;
							if (t != 1)
								L += Lpath;
							else
							{
								mutex1.lock();
								mFilm->addSplat(raster, Lpath / mSpp);
								mutex1.unlock();
							}
						}
					}			
					filmTile->addSample(cameraSample, L);
				} while (sampler.startNextSample());
			}
		}
		mutex1.lock();
		mFilm->mergeFilmTile(std::move(filmTile));
		mutex1.unlock();	
	}
}