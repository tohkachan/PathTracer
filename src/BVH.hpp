//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_BVH_H
#define RAYTRACING_BVH_H

#include "TkPrerequisites.h"
#include "Intersection.hpp"

namespace tk
{
	typedef std::vector<Object*> PrimitiveVec;
	struct BVHBuildNode;
	struct BVHPrimitiveInfo;
	struct LinearBVHNode;

	// BVHAccel Declarations
	inline int leafNodes, totalLeafNodes, totalPrimitives, interiorNodes;
	class BVHAccel {
		typedef std::vector<BVHPrimitiveInfo> PrimitiveInfoVec;
	public:
		// BVHAccel Public Types
		enum class SplitMethod { NAIVE, SAH };

		// BVHAccel Public Methods
		BVHAccel(PrimitiveVec p, int maxPrimsInNode = 1, SplitMethod splitMethod = SplitMethod::SAH);
		//Bounds3 WorldBound() const;
		~BVHAccel();

		bool intersect(const Ray &r, Intersection* isect)const;
		//bool IntersectP(const Ray &ray) const;

		// BVHAccel Private Methods
		BVHBuildNode* recursiveBuild(std::vector<BVHBuildNode>& ns, PrimitiveInfoVec& primInfo, int start, int end, PrimitiveVec& orderedPrims);
		int flattenBVHTree(BVHBuildNode* node, int* offset);

		void draw(s32 nodeIdx, const Spectrum& c, Real alpha)const;
		void drawOutline(s32 nodeIdx, const Spectrum& c, Real alpha)const;
		void drawBounds(s32 nodeIdx, const Spectrum& c, Real alpha)const;

		bool isLeaf(s32 nodeIdx)const;
		s32 getLeftNode(s32 nodeIdx)const;
		s32 getRightNode(s32 nodeIdx)const;

		// BVHAccel Private Data
		const int maxPrimsInNode;
		const SplitMethod splitMethod;
		PrimitiveVec primitives;
		LinearBVHNode* nodes;
	};
}
#endif