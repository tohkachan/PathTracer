#include "Triangle.hpp"
#include "Matrix4.h"
#include "EFloat.h"
#include "OBJ_Loader.hpp"
#include "PLY_Loader.h"
#include "BVH.hpp"
#include "TkSpectrum.h"
#include "Object.hpp"
#include "Material.hpp"

#include <GL/glew.h>

namespace tk
{
	bool Triangle::intersect(const Ray &r, Intersection* isect)const
	{
		double u, v, t_tmp = 0;
		Vector3f pvec = crossProduct(r.direction, e2);
		double det = dotProduct(e1, pvec);
		if (fabs(det) < Math::machine_epsilon)
			return false;

		double det_inv = 1. / det;
		Vector3f tvec = r.origin - v0;
		u = dotProduct(tvec, pvec) * det_inv;
		if (u < 0 || u > 1)
			return false;
		Vector3f qvec = crossProduct(tvec, e1);
		v = dotProduct(r.direction, qvec) * det_inv;
		if (v < 0 || u + v > 1)
			return false;
		t_tmp = dotProduct(e2, qvec) * det_inv;

		double w = 1 - u - v;

		// TODO find ray triangle intersection
		if (t_tmp > 0 && t_tmp < r.t_max)
		{
			float xAbsSum = (std::abs(w * v0.x) + std::abs(u * v1.x) + std::abs(v * v2.x));
			float yAbsSum = (std::abs(w * v0.y) + std::abs(u * v1.y) + std::abs(v * v2.y));
			float zAbsSum = (std::abs(w * v0.z) + std::abs(u * v1.z) + std::abs(v * v2.z));
			Vector3f pError = Vector3f(xAbsSum, yAbsSum, zAbsSum) * Math::Gamma(7);
			isect->p = w * v0 + u * v1 + v * v2;
			isect->pError = pError;
			isect->uv = t0 * w + t1 * u + t2 * v;
			isect->wo = normalize(-r.direction);
			isect->n = normal;
			r.t_max = t_tmp;
			return true;
		}
		return false;
	}

	Bounds3 Triangle::getBounds()const { return Union(Bounds3(v0, v1), v2); }

	Intersection Triangle::Sample(const Vector2f& sample, float* pdf)const
	{
		Vector2f b = uniformSampleTriangle(sample);
		Intersection ret;
		ret.p = v0 * b.x + v1 * b.y + v2 * (1 - b.x - b.y);
		ret.n = normal;
		Vector3f pAbsSum = Abs(v0 * b.x) + Abs(v1 * b.y) + Abs(v2 * (1 - b.x - b.y));
		ret.pError = pAbsSum * Math::Gamma(6);
		*pdf = 1 / area;
		return ret;
	}

	Intersection Triangle::Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const
	{
		Intersection ret = Sample(u, pdf);
		Vector3f wi = ret.p - target.p;
		float dist2 = dotProduct(wi, wi);
		if (dist2 == 0)
			*pdf = 0;
		else
		{
			wi = normalize(wi);
			*pdf *= dist2 / AbsDot(ret.n, -wi);
		}
		return ret;
	}

	void Triangle::draw(const Spectrum& c, Real alpha)const
	{
		glColor4f(c.r, c.g, c.b, alpha);
		glBegin(GL_TRIANGLES);

		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
	}
	void Triangle::drawOutline(const Spectrum& c, Real alpha)const
	{
		glColor4f(c.r, c.g, c.b, alpha);
		glBegin(GL_LINE_LOOP);
		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
	}

	MeshTriangle::MeshTriangle(const std::shared_ptr<AreaLight>& areaLight, const std::shared_ptr<Material>& material,
		const string& filename, const Matrix4& tranform)
	{
		Vector3f min_vert = Vector3f{ Math::pos_infinity, Math::pos_infinity, Math::pos_infinity };
		Vector3f max_vert = Vector3f{ Math::neg_infinity, Math::neg_infinity, Math::neg_infinity };

		string extension = filename.substr(filename.size() - 4, 4);
		if (extension == ".obj")
		{
			objl::Loader loader;
			loader.LoadFile(filename);
			area = 0;
			assert(loader.LoadedMeshes.size() == 1);
			auto mesh = loader.LoadedMeshes[0];
			for (int i = 0; i < mesh.Vertices.size(); i += 3) {
				std::array<Vector3f, 3> face_vertices;

				for (int j = 0; j < 3; j++) {
					auto vert = tranform.concatenatePos(Vector3f(mesh.Vertices[i + j].Position.X,
						mesh.Vertices[i + j].Position.Y,
						mesh.Vertices[i + j].Position.Z));
					face_vertices[j] = vert;

					min_vert = Vector3f(std::min(min_vert.x, vert.x),
						std::min(min_vert.y, vert.y),
						std::min(min_vert.z, vert.z));
					max_vert = Vector3f(std::max(max_vert.x, vert.x),
						std::max(max_vert.y, vert.y),
						std::max(max_vert.z, vert.z));
				}

				std::shared_ptr<Shape> shape = std::shared_ptr<Shape>(new Triangle(face_vertices[0], face_vertices[1],
					face_vertices[2]));
				triangles.emplace_back(shape, areaLight, material);
			}
		}
		else if (extension == ".ply")
		{
			auto mesh = createPLYMesh(filename);
			if (mesh)
			{
				for (int i = 0; i < mesh->indexCtr; i += 3) {
					std::array<Vector3f, 3> face_vertices;

					for (int j = 0; j < 3; j++) {
						auto vert = tranform.concatenatePos(mesh->p[mesh->indices[i + j]]);
						face_vertices[j] = vert;

						min_vert = Vector3f(std::min(min_vert.x, vert.x),
							std::min(min_vert.y, vert.y),
							std::min(min_vert.z, vert.z));
						max_vert = Vector3f(std::max(max_vert.x, vert.x),
							std::max(max_vert.y, vert.y),
							std::max(max_vert.z, vert.z));
					}

					std::shared_ptr<Shape> shape = std::shared_ptr<Shape>(new Triangle(face_vertices[0], face_vertices[1],
						face_vertices[2]));
					triangles.emplace_back(shape, areaLight, material);
				}
				delete mesh;
			}
		}

		bounding_box = Bounds3(min_vert, max_vert);

		std::vector<Object*> ptr;
		std::vector<float> tmp(triangles.size());
		int n = 0;
		for (auto& tri : triangles) {
			ptr.push_back(&tri);
			float a = tri.getShape()->getArea();
			tmp[n++] = a;
			area += a;
		}
		distribution = Distribution1D(&tmp[0], n);
		bvh = std::unique_ptr<BVHAccel>(new BVHAccel(ptr));
	}

	bool MeshTriangle::intersect(const Ray &r, Intersection* isect)const
	{
		if (bvh) {
			return bvh->intersect(r, isect);
		}
		return false;
	}

	Intersection MeshTriangle::Sample(const Vector2f& sample, float* pdf)const
	{
		int idx = distribution.sampleDiscrete(get_random_float(), pdf);
		float tmp;
		Intersection ret = triangles[idx].getShape()->Sample(sample, &tmp);
		*pdf *= tmp;
		return ret;
	}

	Intersection MeshTriangle::Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const
	{
		int idx = distribution.sampleDiscrete(u0, pdf);
		float tmp;
		Intersection ret = triangles[idx].getShape()->Sample(target, u0, u, &tmp);
		*pdf *= tmp;
		return ret;
	}

	float MeshTriangle::Pdf(const Intersection& it)const
	{
		return 1.0f / area;//all triangles' area asumption to counterpart
	}

	void MeshTriangle::draw(const Spectrum& c, Real alpha)const
	{
		std::vector<Object>::const_iterator it = triangles.begin();
		std::vector<Object>::const_iterator end = triangles.end();
		while (it != end)
		{
			it->draw(c, alpha);
			++it;
		}
	}
	void MeshTriangle::drawOutline(const Spectrum& c, Real alpha)const
	{
		std::vector<Object>::const_iterator it = triangles.begin();
		std::vector<Object>::const_iterator end = triangles.end();
		while (it != end)
		{
			it->drawOutline(c, alpha);
			++it;
		}
	}
}