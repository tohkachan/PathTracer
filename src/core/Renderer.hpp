//
// Created by goksu on 2/25/20.
//
#include "Scene.hpp"
#include "Threads.h"
#include "Photonmap.h"
#include "sampler.h"
#include "camera.h"

#pragma once

struct SPPMPixel
{
	
	Vector3f Ld;
	Vector3f tau;
	int numPhotons;
	float dist2;

	SPPMPixel()
		: Ld(0), dist2(0), numPhotons(0){}
};

enum eVertexType
{
	CAMERA,
	LIGHT,
	SURFACE
};

enum eTransportMode;

struct PathVertex
{
	eVertexType type;
	Intersection isect;
	Vector3f beta;
	float pdfFwd;
	float pdfRev;
	bool delta;
	bool isSurface;
	const AreaLight* light;
	const Camera* cam;

	PathVertex()
		: pdfFwd(0), pdfRev(0), delta(false), isSurface(false), light(nullptr),
	cam(nullptr){}
	Vector3f f(const PathVertex& towards, eTransportMode mode)const;
	float convertDensity(const PathVertex& towards, float pdf)const
	{
		Vector3f d = towards.isect.p - isect.p;
		float g = 1 / dotProduct(d, d);
		if (towards.isSurface)
			pdf *= AbsDot(d * sqrt(g), towards.isect.n);
		return pdf * g;
	}

	float Pdf(const PathVertex* prev, const PathVertex& next)const;

	float PdfLightPos(const PathVertex& next)const;

	float PdfLightDir(const PathVertex& next)const;
};

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

struct TmpRaster
{
	Vector2f raster;
	Vector3f radiance;
	TmpRaster(Vector2f r, Vector3f ra)
		: raster(r), radiance(ra) {}
};

class Renderer
{
public:
	Camera* camera;
	Renderer(/*const */Scene& scene, int spp, std::string outfile);
	~Renderer();
    void Render();
	int generateCameraSubpath(int minLength, const Ray& r, std::vector<PathVertex>& pathEye, Sampler& s);
	int generateLightSubpath(int minLength, std::vector<PathVertex>& pathLight, Sampler& s);
	int pathWalk(Vector3f beta, Ray r, int minDepth, float pdfFwd, std::vector<PathVertex>& pathV,
		Sampler& s, eTransportMode mode);
	Vector3f connectPath(std::vector<PathVertex>& pathEye, std::vector<PathVertex>& pathL, int s, int t,
		Sampler& sample, Vector2f *raster);
	void castRay(Ray ray, SPPMPixel& pp, int maxDepth, Sampler& s) const;
	//unsigned long _updateWorkerThread(ThreadHandle* handle);
	float G(const Intersection& p0, const Intersection& p1)const;
	float MISWeight(std::vector<PathVertex>& pathEye, std::vector<PathVertex>& pathL, int s, int t,
		PathVertex& sampled);
private:
	//uint32_t startX, startY;
	//uint32_t chunkSize;
	int spp;
	//bool runThreads;
	/*const */Scene& scene;
	/*std::vector<ThreadHandle*> workerThreads;
	std::vector<Vector3f> framebuffer;
	LwMutex* mutex;
	float imageAspectRatio;
	float scale;
	Vector3f eye_pos;*/
	std::string outfile;
	PhotonMap photonmap;
	LwMutex mutex;
	Sampler* sampler;

	//void _startWorkerThreads();
	//void _stopWorkerThreads();
	//void _render(uint32_t x, uint32_t y, uint32_t endX, uint32_t endY);
};
