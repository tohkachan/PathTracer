#pragma once

#include "Shape.h"
#include "sampling.h"
#include "Bounds3.hpp"

namespace tk
{
	class Triangle : public Shape
	{
	public:
		Vector3f v0, v1, v2; // vertices A, B ,C , counter-clockwise order
		Vector3f e1, e2;     // 2 edges v1-v0, v2-v0;
		Vector2f t0, t1, t2; // texture coords
		Vector3f normal;
		float area;

		Triangle(Vector3f _v0, Vector3f _v1, Vector3f _v2)
			: v0(_v0), v1(_v1), v2(_v2)
		{
			e1 = v1 - v0;
			e2 = v2 - v0;
			normal = normalize(crossProduct(e1, e2));
			area = crossProduct(e1, e2).norm()*0.5f;
		}

		bool intersect(const Ray &r, Intersection* isect)const;
		Bounds3 getBounds()const override;
		Intersection Sample(const Vector2f& sample, float* pdf)const;
		Intersection Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const;
		float getArea()const {
			return area;
		}

		void draw(const Spectrum& c, Real alpha)const;
		void drawOutline(const Spectrum& c, Real alpha)const;
	};

	class MeshTriangle : public Shape
	{
	public:
		MeshTriangle(const std::shared_ptr<AreaLight>& areaLight, const std::shared_ptr<Material>& material,
			const string& filename, const Matrix4& tranform);

		Bounds3 getBounds()const { return bounding_box; }

		bool intersect(const Ray &r, Intersection* isect)const;
		Intersection Sample(const Vector2f& sample, float* pdf)const;
		Intersection Sample(const Intersection& target, float u0, const Vector2f& u, float* pdf)const;

		virtual float Pdf(const Intersection& it)const;

		float getArea()const {
			return area;
		}

		void draw(const Spectrum& c, Real alpha)const;
		void drawOutline(const Spectrum& c, Real alpha)const;
		Bounds3 bounding_box;
		std::unique_ptr<Vector3f[]> vertices;
		u32 numTriangles;
		std::unique_ptr<u32[]> vertexIndex;
		std::unique_ptr<Vector2f[]> stCoordinates;

		std::vector<Object> triangles;
		Distribution1D distribution;

		std::unique_ptr<BVHAccel> bvh;
		float area;

	};
}