//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include "Material.hpp"
#include "sampling.h"
#include "Object.hpp"

Renderer::Renderer(/*const */Scene& scene, int spp, std::string outfile)
	: scene(scene), spp(spp), outfile(outfile), photonmap(&scene)
{
	int num = sqrt(spp);
	sampler = new StratifiedSampler(num, num, 1, true);
}

Renderer::~Renderer()
{
	delete sampler;
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render()
{
	std::vector<Vector3f> framebuffer(scene.width * scene.height);
	Bounds2i box(Point2i(0, 0), Point2i(scene.width, scene.height));

	int count = 1;

	std::vector<SPPMPixel> pp(scene.width * scene.height);

	for (int i = 0; i < pp.size(); ++i)
		pp[i].dist2 = 0.001;

	std::cout << "Each photon used " << sizeof(Photon) << " bytes\nPhoton map size: " << PHOTON_COUNT << "\n";
	for (int i = 0; i < spp; ++i)
	{
		std::cout << "Recurse number: " << i << "\n";
		photonmap.send_photons(*sampler);
		Parrallel::parrallelFor2D(box, [&](Bounds2i b) {
			for (uint32_t j = b.pMin.y; j < b.pMax.y; ++j) {
				for (uint32_t i = b.pMin.x; i < b.pMax.x; ++i) {
					std::unique_ptr<Sampler> tileSampler = sampler->clone(count++);
					Point2i pixel = Point2i(i, j);
					tileSampler->startPixel(pixel);
			
					Vector2f cameraSample = tileSampler->get2D() + Vector2f(pixel.x, pixel.y);
					Ray r = camera->generateRay(cameraSample);
					castRay(r, pp[i + j * scene.width], MAX_DEPTH, *tileSampler);
				}
				if (b.pMax.x >= scene.width)
					UpdateProgress(j / (float)scene.height);
			}});
		UpdateProgress(1.f);
		std::cout << "\n";
	}
	std::cout << "Total shot " << photonmap.totalShootPhotons() << " virtual photons\n";
	for (int i = 0; i < pp.size(); ++i)
	{
		framebuffer[i] = pp[i].Ld / spp + pp[i].tau / (M_PI * pp[i].dist2 * photonmap.totalShootPhotons());
	}

	/*Parrallel::parrallelFor2D(box, [&](Bounds2i b) {
		std::unique_ptr<Sampler> tileSampler = sampler->clone(count++);
		
		for (uint32_t j = b.pMin.y; j < b.pMax.y; ++j) {
			for (uint32_t i = b.pMin.x; i < b.pMax.x; ++i) {
				Point2i pixel = Point2i(i, j);
				tileSampler->startPixel(pixel);

				do {
					Vector2f cameraSample = tileSampler->get2D() + Vector2f(pixel.x, pixel.y);
					Ray r = camera->generateRay(cameraSample);
					framebuffer[i + j * scene.width] += scene.castRay(r, 0, true, *tileSampler) / spp;
				} while (tileSampler->startNextSample());
			}
			if (b.pMax.x >= scene.width)
				UpdateProgress(j / (float)scene.height);
		}});
	UpdateProgress(1.f);
	std::cout << "\n";*/
	

	/*Parrallel::parrallelFor2D(box, [&](Bounds2i b) {
		std::unique_ptr<Sampler> tileSampler = sampler->clone(count++);
		for (uint32_t j = b.pMin.y; j < b.pMax.y; ++j) {
			for (uint32_t i = b.pMin.x; i < b.pMax.x; ++i) {
				Point2i pixel = Point2i(i, j);
				tileSampler->startPixel(pixel);

				std::vector<PathVertex> pathEye;
				std::vector<PathVertex> pathLight;
				do {
					Vector2f cameraSample = tileSampler->get2D() + Vector2f(pixel.x, pixel.y);
					Ray r = camera->generateRay(cameraSample);
					int nCamera = generateCameraSubpath(6, r, pathEye, *tileSampler);
					int nLight = generateLightSubpath(6, pathLight, *tileSampler);
					//if (nCamera >= 3 && nLight >= 0)
					//{
						//Vector3f L = connectPath(pathEye, pathLight, 0, 3);
						//framebuffer[i + j * scene.width] += L / spp;
					//}
					for (int t = 1; t <= nCamera; ++t)
					{
						for (int s = 0; s <= nLight; ++s)
						{
							Vector2f raster;
							Vector3f L = connectPath(pathEye, pathLight, s, t, *tileSampler, &raster);
							if (L.x == 0 && L.y == 0 && L.z == 0)
								continue;
							if (t != 1)
								framebuffer[i + j * scene.width] += L / spp;
							else
							{
								int xx = clamp(raster.x, 0, scene.width - 1);
								int yy = clamp(raster.y, 0, scene.height - 1);
								framebuffer[xx + yy * scene.width] += L / spp;
							}
								
						}
					}
					pathEye.clear();
					pathLight.clear();
				} while (tileSampler->startNextSample());
			}
			if (b.pMax.x >= scene.width)
				UpdateProgress(j / (float)scene.height);
		}});
	UpdateProgress(1.f);
	std::cout << "\n";*/
	/*chunkSize = 20;
	_startWorkerThreads();
	while (startX <= scene.width && startY <= scene.height)
	{
		mutex->lock();	
		uint32_t x = startX;
		uint32_t y = startY;
		uint32_t endX = startX + chunkSize;
		uint32_t endY = startY + chunkSize;
		startX += chunkSize;
		if (startX >= scene.width)
		{
			startX = 0;
			endX = scene.width;
			startY += chunkSize;
			UpdateProgress(startY / (float)scene.height);
		}
		mutex->unlock();
		_render(x, y, endX, endY);
	}

    // change the spp value to change sample ammount   
    UpdateProgress(1.f);

	_stopWorkerThreads();*/
    // save framebuffer to file
    FILE* fp = fopen(outfile.c_str(), "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(framebuffer[i].x, 0, 1), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(framebuffer[i].y, 0, 1), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(framebuffer[i].z, 0, 1), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}

void Renderer::castRay(Ray ray, SPPMPixel& pp, int maxDepth, Sampler& s) const
{
	Vector3f coef(1.f);
	Intersection isect;
	for (int i = 0; i < maxDepth; ++i)
	{	
		if (!scene.intersect(ray, &isect))
			return;
		const Material* m = isect.obj->getMaterial();
		m->setTransportMode(Radiance);
		Vector3f wo = -ray.direction;
		pp.Ld += coef * isect.Le(wo);
		if ((m->getType() & (GLASS | MIRROR)) == 0)
			pp.Ld += coef * scene.uniformSampleOneLight(isect, s);
		if (m->getType() == DIFFUSE)
		{
			float dist2 = pp.dist2;
			int numPhotons;
			coef = coef * photonmap.radiance_estimate(isect, dist2, numPhotons);
			if (numPhotons == 0)
				return;
			if (pp.numPhotons == 0)
			{
				pp.dist2 = dist2;
				pp.numPhotons = numPhotons;
				pp.tau = coef;
			}
			else
			{
				float newPhotons = pp.numPhotons + 0.7 * numPhotons;
				float newDist2 = pp.dist2 * newPhotons / (pp.numPhotons + numPhotons);
				pp.tau = (pp.tau + coef) * newDist2 / pp.dist2;
				pp.numPhotons = newPhotons;
				pp.dist2 = newDist2;
			}
			return;
		}
		Vector3f wi;
		float pdf;
		Vector3f f = m->sample_f(wo, &wi, isect.n, s.get2D(), &pdf);
		if (pdf == 0)
			return;
		coef = coef * f * AbsDot(wi, isect.n) / pdf;
		ray = isect.spawnRay(wi);
	}
}

int Renderer::generateCameraSubpath(int minLength, const Ray& r, std::vector<PathVertex>& pathEye, Sampler& s)
{
	PathVertex camVertex;
	camVertex.cam = camera;
	camVertex.type = CAMERA;
	camVertex.isect.p = r.origin;
	camVertex.beta = Vector3f(1.f);
	pathEye.emplace_back(camVertex);

	float pdfPos, pdfDir;
	camera->Pdf_We(r, &pdfPos, &pdfDir);
	return pathWalk(camVertex.beta, r, minLength, pdfDir, pathEye, s, Radiance) + 1;
}

int Renderer::generateLightSubpath(int minLength, std::vector<PathVertex>& pathLight, Sampler& s)
{
	const std::vector<std::shared_ptr<AreaLight>>& lights = scene.get_lights();
	int numLights = lights.size();
	if (numLights == 0) return 0;
	int lightIdx = std::min<int>(s.get1D() * numLights, numLights - 1);
	float pdf = 1.0f / numLights;
	float pdfPos, pdfDir;
	Ray r;
	Vector3f n;
	Vector3f L = lights[lightIdx]->sample_Le(s.get2D(), s.get2D(), 0, &r, &n, &pdfPos, &pdfDir);

	PathVertex lightVertex;
	lightVertex.type = LIGHT;
	lightVertex.light = lights[lightIdx].get();
	lightVertex.isect.p = r.origin;
	lightVertex.isect.n = n;
	lightVertex.pdfFwd = pdfPos * pdf;
	lightVertex.beta = L * M_PI / (pdfPos * pdf);
	lightVertex.isSurface = true;
	Vector3f power = lightVertex.beta * AbsDot(n, r.direction) /
		(M_PI * pdfDir);
	pathLight.emplace_back(lightVertex);

	return pathWalk(power, r, minLength, pdfDir, pathLight, s, Importance) + 1;
}

int Renderer::pathWalk(Vector3f beta, Ray r, int minDepth, float pdfFwd, std::vector<PathVertex>& pathV,
	Sampler& s, eTransportMode mode)
{
	int bounces = 0;
	float pdfRev = 0;
	Intersection isect;
	while (true)
	{
		PathVertex pv;
		PathVertex& prev = pathV.back();	
		if (!scene.intersect(r, &isect))
			break;
		pv.beta = beta;
		pv.type = SURFACE;
		pv.isect = isect;
		pv.isSurface = true;
		pv.pdfFwd = prev.convertDensity(pv, pdfFwd);
		pathV.emplace_back(pv);
		++bounces;
		
		float RussianRoulette = bounces < minDepth ? 1.0 : 0.7;

		if (s.get1D() > RussianRoulette)
			break;
		{
			Vector3f wi;
			const Material* m = isect.obj->getMaterial();
			m->setTransportMode(mode);
			Vector3f f = m->sample_f(isect.wo, &wi, isect.n, s.get2D(), &pdfFwd);
			if (pdfFwd == 0.f)
				break;
			pdfRev = m->Pdf(wi, isect.wo, isect.n);
			beta = beta * f * AbsDot(wi, isect.n) / (pdfFwd * RussianRoulette);
			if (m->isDelta)
			{
				pv.delta = true;
				pdfRev = pdfFwd = 0;
			}
			prev.pdfRev = pv.convertDensity(prev, pdfRev);
			r = isect.spawnRay(wi);		
		}
	}
	return bounces;
}

float Renderer::G(const Intersection& p0, const Intersection& p1)const
{
	Ray r = p0.spawnRayTo(p1);
	if (!scene.intersectP(r))
	{
		Vector3f d = p1.p - p0.p;
		float g = 1 / dotProduct(d, d);
		d = d * sqrt(g);
		return g * AbsDot(d, p0.n) * AbsDot(-d, p1.n);
	}
	return 0;
}

float Renderer::MISWeight(std::vector<PathVertex>& pathEye, std::vector<PathVertex>& pathL, int s, int t,
	PathVertex& sampled)
{
	if (s + t == 2) return 1;

	PathVertex *qs = s > 0 ? &pathL[s - 1] : nullptr,
		*pt = &pathEye[t - 1],
		*qsMinus = s > 1 ? &pathL[s - 2] : nullptr,
		*ptMinus = t > 1 ? &pathEye[t - 2] : nullptr;

	ScopedAssignment<PathVertex> tmp0;
	if (s == 1)
		tmp0 = { qs, sampled };
	else if (t == 1)
		tmp0 = { pt, sampled };

	ScopedAssignment<bool> tmp1, tmp2;
	tmp1 = { &pt->delta, false };
	if (qs) tmp2 = { &qs->delta, false };

	ScopedAssignment<float> tmp3 = { &pt->pdfRev, s > 0 ? qs->Pdf(qsMinus, *pt) : pt->PdfLightPos(*ptMinus) };

	ScopedAssignment<float> tmp4, tmp5, tmp6;
	if (ptMinus)
		tmp4 = { &ptMinus->pdfRev, s > 0 ? pt->Pdf(qs, *ptMinus) : pt->PdfLightDir(*ptMinus) };
	if (qs)
		tmp5 = { &qs->pdfRev, pt->Pdf(ptMinus, *qs) };
	if (qsMinus)
		tmp6 = { &qsMinus->pdfRev, qs->Pdf(pt, *qsMinus) };

	float sumRi = 0.f;
	auto remap0 = [](float f)->float {return f != 0 ? f : 1; };

	float ri = 1.f;
	for (int i = s - 1; i >= 0; --i)
	{
		ri *= remap0(pathL[i].pdfRev) / remap0(pathL[i].pdfFwd);

		bool test = i > 0 ? pathL[i - 1].delta : false;

		if (!pathL[i].delta && !test) sumRi += ri * ri;
	}

	ri = 1.f;
	for (int i = t - 1; i > 0; --i)
	{
		ri *= remap0(pathEye[i].pdfRev) / remap0(pathEye[i].pdfFwd);

		if (!pathEye[i].delta && !pathEye[i - 1].delta) sumRi += ri * ri;
	}
	//return 1.0 / (s + t);
	return 1.0 / (1.0 + sumRi);
}

Vector3f Renderer::connectPath(std::vector<PathVertex>& pathEye, std::vector<PathVertex>& pathL, int s, int t,
	Sampler& sample, Vector2f *raster)
{
	Vector3f L;
	PathVertex sampled;
	if (s == 0)
	{
		const PathVertex& pv = pathEye[t - 1];
		if (pv.type != CAMERA)
			L = pv.isect.Le(pv.isect.wo);// *pv.beta;
		if (L.x != 0 || L.y != 0 || L.z != 0)
			L = L * pv.beta;
	}
	else if (t == 1)
	{
		const PathVertex& pvL = pathL[s - 1];
		Vector3f wi;
		float pdf;
		Vector3f Wi = camera->Sample_Wi(pvL.isect, sample.get2D(), &wi, &sampled.isect, &pdf, raster);
		if (scene.intersectP(sampled.isect.spawnRayTo(pvL.isect)))
			return 0;
		if (pdf > 0 && (Wi.x != 0 || Wi.y != 0 || Wi.z != 0))
		{
			sampled.cam = camera;
			sampled.type = CAMERA;
			sampled.beta = Wi / pdf;
			L = pvL.beta * pvL.f(sampled, Importance) * sampled.beta;
			if (pvL.isSurface) L = L * AbsDot(wi, pvL.isect.n);
		}
	}
	else if (s == 1)
	{
		const PathVertex& pv = pathEye[t - 1];
		const std::vector<std::shared_ptr<AreaLight>>& lights = scene.get_lights();
		int numLights = lights.size();
		if (numLights == 0) return 0;
		int lightIdx = std::min<int>(sample.get1D() * numLights, numLights - 1);
		float pdf = 1.0f / numLights;
		float pdfLight;
		Vector3f w, p, n;
		Vector3f lightWeight = lights[lightIdx]->sample_Li_tmp(pv.isect, scene, sample.get1D(), sample.get2D(), &w, &p,&n, &pdfLight);
		if (pdfLight > 0 && (lightWeight.x != 0 || lightWeight.y != 0 || lightWeight.z != 0))
		{
			sampled.light = lights[lightIdx].get();
			sampled.beta = lightWeight / (pdf * pdfLight);
			sampled.type = LIGHT;
			sampled.isect.p = p;
			sampled.isect.n = n;
			sampled.pdfFwd = sampled.PdfLightPos(pv);
			L = pv.beta * pv.f(sampled, Radiance) * sampled.beta;
			if (pv.isSurface) L = L * AbsDot(w, pv.isect.n);
		}
	}
	else
	{
		const PathVertex& pvE = pathEye[t - 1];
		const PathVertex& pvL = pathL[s - 1];
		L = pvL.beta * pvL.f(pvE, Importance) * G(pvL.isect, pvE.isect) * pvE.f(pvL, Radiance) * pvE.beta;	
	}

	if (L.x == 0 && L.y == 0 && L.z == 0)
		return L;

	float misWeight = MISWeight(pathEye, pathL, s, t, sampled);
	
	return L * misWeight;
}

Vector3f PathVertex::f(const PathVertex& towards, eTransportMode mode)const
{
	Vector3f wi = normalize(towards.isect.p - isect.p);
	switch (type)
	{
	case CAMERA:
		return 1;
	case LIGHT:		
		if (dotProduct(wi, isect.n) > 0)
			return 1 / M_PI;
		else
			return 0;
	case SURFACE:
		const Material* m = isect.obj->getMaterial();
		m->setTransportMode(mode);
		return m->f(isect.wo, wi, isect.n);
	}
	return Vector3f();
}

float PathVertex::Pdf(const PathVertex* prev, const PathVertex& next)const
{
	if (type == LIGHT)
		return PdfLightDir(next);
	Vector3f wn = next.isect.p - isect.p;
	if (dotProduct(wn, wn) == 0) return 0;
	wn = normalize(wn);
	Vector3f wp;
	if (prev)
	{
		wp = prev->isect.p - isect.p;
		if (dotProduct(wp, wp) == 0) return 0;
		wp = normalize(wp);
	}
	float pdf = 0.f;
	float tmp;
	
	if (type == SURFACE)
	{
		const Material* m = isect.obj->getMaterial();
		m->setTransportMode(Radiance);
		pdf = m->Pdf(wp, wn, isect.n);
	}
	else if (type == CAMERA)
		cam->Pdf_We(isect.spawnRay(wn), &tmp, &pdf);
		
	return convertDensity(next, pdf);
}

float PathVertex::PdfLightPos(const PathVertex& next)const
{
	Vector3f w = next.isect.p - isect.p;
	if (dotProduct(w, w) == 0) return 0;
	w = normalize(w);
	float pdfPos, pdfDir, pdfChoice = 0;
	const AreaLight* l = light? light : isect.obj->getAreaLight();
	pdfChoice = 0.5f;
	l->pdf_Le(Ray(isect.p, w), isect.n, &pdfPos, &pdfDir);
	return pdfPos * pdfChoice;
}

float PathVertex::PdfLightDir(const PathVertex& next)const
{
	Vector3f w = next.isect.p - isect.p;
	if (dotProduct(w, w) == 0) return 0;
	float g = 1 / dotProduct(w, w);
	w = w * sqrt(g);
	float pdfPos, pdfDir;
	const AreaLight* l = light ? light : isect.obj->getAreaLight();
	l->pdf_Le(Ray(isect.p, w), isect.n, &pdfPos, &pdfDir);
	float pdf = pdfDir * g;
	if (next.isSurface)
		pdf *= AbsDot(-w, next.isect.n);
	return pdf;
}

/*void Renderer::_render(uint32_t x, uint32_t y, uint32_t endX, uint32_t endY)
{
	for (uint32_t j = y; j < endY && j < scene.height; ++j) {
		for (uint32_t i = x; i < endX; ++i) {
			// generate primary ray direction
			float xx = (2 * (i + 0.5) / (float)scene.width - 1) *
				imageAspectRatio * scale;
			float yy = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

			Vector3f dir = normalize(Vector3f(-xx, yy, 1));
			for (int k = 0; k < spp; k++) {
				framebuffer[i + j * scene.width] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
			}
		}
	}
}

unsigned long Renderer::_updateWorkerThread(ThreadHandle* handle)
{
	while (runThreads)
	{
		mutex->lock();
		uint32_t x = startX;
		uint32_t y = startY;
		uint32_t endX = startX + chunkSize;
		uint32_t endY = startY + chunkSize;
		startX += chunkSize;
		if (startX >= scene.width)
		{
			startX = 0;
			endX = scene.width;
			startY += chunkSize;
		}
		mutex->unlock();
		_render(x, y, endX, endY);
	}
	return 0;
}

unsigned long updateWorkerThread(ThreadHandle* handle)
{
	Renderer* mgr = reinterpret_cast<Renderer*>(handle->getUserParameter());
	return mgr->_updateWorkerThread(handle);
}

THREAD_DECL(updateWorkerThread);

void Renderer::_startWorkerThreads()
{
	int numThreads = std::max<int>(1, std::thread::hardware_concurrency());
	std::cout << "Number of threads: " << numThreads << std::endl;
	mutex = new LwMutex();
	workerThreads.reserve(numThreads);
	for (size_t i = 0; i < numThreads; ++i)
	{
		ThreadHandle* handle = Threads::createThread(THREAD_GET(updateWorkerThread), i, this);
		workerThreads.push_back(handle);
	}
}

void Renderer::_stopWorkerThreads()
{
	runThreads = false;
	Threads::waitForThreads(workerThreads);
	delete mutex;
	mutex = 0;
	std::vector<ThreadHandle*>::iterator it = workerThreads.begin();
	std::vector<ThreadHandle*>::iterator end = workerThreads.end();
	while (it != end)
		delete *it++;
	workerThreads.clear();
}*/
