#include "BVH.hpp"
#include "Bounds3.hpp"
#include "Object.hpp"


namespace tk
{
	struct BVHBuildNode {
		Bounds3 bounds;
		BVHBuildNode *left;
		BVHBuildNode *right;
		int splitAxis, firstPrimOffset, numPrims;
		void initLeaf(int offset, int n, const Bounds3& b)
		{
			firstPrimOffset = offset;
			numPrims = n;
			bounds = b;
			left = right = 0;

		}

		void initInterior(int axis, BVHBuildNode* l, BVHBuildNode* r)
		{
			left = l;
			right = r;
			bounds = Union(l->bounds, r->bounds);
			splitAxis = axis;
			numPrims = 0;
		}
	};

	struct BVHPrimitiveInfo
	{
		size_t index;
		Bounds3 bounds;
		Vector3f centroid;
		BVHPrimitiveInfo() {}
		BVHPrimitiveInfo(size_t idx, const Bounds3& bounds)
			: index(idx), bounds(bounds), centroid(bounds.Centroid())
		{}
	};

	struct BVHSplitBucket
	{
		int count = 0;
		Bounds3 bounds;
	};

	struct LinearBVHNode
	{
		Bounds3 bounds;
		union
		{
			s32 primitivesOffset;
			s32 secondChildOffset;
		};
		u16 numPrims;
		u8 axis;
		u8 pad[1];
	};

	BVHAccel::BVHAccel(PrimitiveVec p, int maxPrimsInNode,
		SplitMethod splitMethod)
		: maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
		primitives(std::move(p))
	{
		if (primitives.empty())
			return;
		PrimitiveInfoVec primInfo(primitives.size());
		for (size_t i = 0; i < primitives.size(); ++i)
			primInfo[i] = { i , primitives[i]->getBounds() };
		PrimitiveVec orderedPrims;
		orderedPrims.reserve(primitives.size());
		std::vector<BVHBuildNode> buildNodes;
		buildNodes.reserve(1024 * 1024);
		interiorNodes = 0;
		BVHBuildNode* root = recursiveBuild(buildNodes, primInfo, 0, primitives.size(), orderedPrims);
		primitives.swap(orderedPrims);

		nodes = (LinearBVHNode*)malloc(interiorNodes * sizeof(LinearBVHNode));
		int offset = 0;
		flattenBVHTree(root, &offset);
	}

	BVHAccel::~BVHAccel()
	{
		free(nodes);
	}

	BVHBuildNode* BVHAccel::recursiveBuild(std::vector<BVHBuildNode>& ns, PrimitiveInfoVec& primInfo, int start, int end, PrimitiveVec& orderedPrims)
	{
		ns.emplace_back(BVHBuildNode());
		BVHBuildNode* node = &ns.back();
		interiorNodes++;

		// Compute bounds of all primitives in BVH node
		Bounds3 bounds;
		for (int i = start; i < end; ++i)
			bounds = Union(bounds, primInfo[i].bounds);
		int numPrims = end - start;
		if (numPrims == 1) {
			// Create leaf _BVHBuildNode_
			int firsPrimOffset = orderedPrims.size();
			orderedPrims.push_back(primitives[primInfo[start].index]);
			node->initLeaf(firsPrimOffset, numPrims, bounds);
			return node;
		}
		else {
			Bounds3 centroidBounds;
			for (int i = start; i < end; ++i)
				centroidBounds =
				Union(centroidBounds, primInfo[i].centroid);
			int mid = (start + end) / 2;
			int dim = centroidBounds.maxExtent();
			Vector3f diagonal = centroidBounds.Diagonal();

			if (centroidBounds.pMin[dim] == centroidBounds.pMax[dim])
			{
				int firsPrimOffset = orderedPrims.size();
				for (int i = start; i < end; ++i)
				{
					orderedPrims.push_back(primitives[primInfo[i].index]);
				}
				node->initLeaf(firsPrimOffset, numPrims, bounds);
				return node;
			}
			else
			{
				switch (splitMethod)
				{
				case SplitMethod::NAIVE:
				{
					std::nth_element(&primInfo[start], &primInfo[mid], &primInfo[end - 1] + 1,
						[dim](const BVHPrimitiveInfo& lhs, const BVHPrimitiveInfo& rhs) {
						return lhs.centroid[dim] < rhs.centroid[dim]; });
				}
				break;
				case SplitMethod::SAH:
				default:
				{
					if (numPrims <= 2)
					{
						std::nth_element(&primInfo[start], &primInfo[mid], &primInfo[end - 1] + 1,
							[dim](const BVHPrimitiveInfo& lhs, const BVHPrimitiveInfo& rhs) {
							return lhs.centroid[dim] < rhs.centroid[dim]; });
					}
					else
					{
						const int numBuckets = 12;
						BVHSplitBucket buckets[numBuckets];
						for (int i = start; i < end; ++i)
						{
							int b = numBuckets * ((primInfo[i].centroid[dim] - centroidBounds.pMin[dim]) / diagonal[dim]);

							if (b == numBuckets) b = numBuckets - 1;
							buckets[b].count++;
							buckets[b].bounds = Union(buckets[b].bounds, primInfo[i].centroid);
						}
						double cost[numBuckets - 1];
						for (int i = 0; i < numBuckets - 1; ++i)
						{
							Bounds3 b0, b1;
							int count0 = 0, count1 = 0;
							for (int j = 0; j <= i; ++j)
							{
								b0 = Union(b0, buckets[j].bounds);
								count0 += buckets[j].count;
							}

							for (int j = i + 1; j < numBuckets; ++j)
							{
								b1 = Union(b1, buckets[j].bounds);
								count1 += buckets[j].count;
							}

							cost[i] = 1 + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea())
								/ bounds.SurfaceArea();
						}

						double minCost = cost[0];
						int minCostSplitBucket = 0;
						for (int i = 0; i < numBuckets - 1; ++i)
						{
							if (cost[i] < minCost)
							{
								minCost = cost[i];
								minCostSplitBucket = i;
							}
						}

						double leafCost = numPrims;
						if (numPrims > maxPrimsInNode || minCost < leafCost)
						{
							BVHPrimitiveInfo* pMid = std::partition(&primInfo[start],
								&primInfo[end - 1] + 1,
								[=](const BVHPrimitiveInfo& pi) {
								int b = numBuckets * ((pi.centroid[dim] - centroidBounds.pMin[dim]) / diagonal[dim]);
								if (b == numBuckets) b = numBuckets - 1;
								return b <= minCostSplitBucket; });
							mid = pMid - &primInfo[0];
						}
						else
						{
							int firsPrimOffset = orderedPrims.size();
							for (int i = start; i < end; ++i)
							{
								orderedPrims.push_back(primitives[primInfo[i].index]);
							}
							node->initLeaf(firsPrimOffset, numPrims, bounds);
							return node;
						}
					}
				}
				break;
				}

				node->initInterior(dim, recursiveBuild(ns, primInfo, start, mid, orderedPrims),
					recursiveBuild(ns, primInfo, mid, end, orderedPrims));
			}
		}

		return node;
	}

	int BVHAccel::flattenBVHTree(BVHBuildNode* node, int* offset)
	{
		LinearBVHNode* linearNode = &nodes[*offset];
		linearNode->bounds = node->bounds;
		int nextOffset = (*offset)++;
		if (node->numPrims > 0)
		{
			linearNode->primitivesOffset = node->firstPrimOffset;
			linearNode->numPrims = node->numPrims;
		}
		else
		{
			linearNode->axis = node->splitAxis;
			linearNode->numPrims = 0;
			flattenBVHTree(node->left, offset);
			linearNode->secondChildOffset = flattenBVHTree(node->right, offset);
		}
		return nextOffset;
	}

	bool BVHAccel::intersect(const Ray &r, Intersection* isect)const
	{
		if (!nodes) return false;
		bool hit = false;
		std::array<int, 3> dirIsNeg = { r.direction.x > 0, r.direction.y > 0, r.direction.z > 0 };
		s32 toVisitOffset = 0, currentNodeIdx = 0;
		s32 nodesToVisit[64];
		while (true)
		{
			const LinearBVHNode* node = &nodes[currentNodeIdx];
			if (node->bounds.IntersectP(r, r.direction_inv, dirIsNeg))
			{
				if (node->numPrims > 0)
				{
					for (s32 i = 0; i < node->numPrims; ++i)
					{
						if (primitives[node->primitivesOffset + i]->intersect(r, isect))
							hit = true;
					}
					if (toVisitOffset == 0)break;
					currentNodeIdx = nodesToVisit[--toVisitOffset];
				}
				else
				{
					if (dirIsNeg[node->axis])
					{
						nodesToVisit[toVisitOffset++] = currentNodeIdx + 1;
						currentNodeIdx = node->secondChildOffset;
					}
					else
					{
						nodesToVisit[toVisitOffset++] = node->secondChildOffset;
						currentNodeIdx = currentNodeIdx + 1;
					}
				}
			}
			else
			{
				if (toVisitOffset == 0)break;
				currentNodeIdx = nodesToVisit[--toVisitOffset];
			}
		}
		return hit;
	}

	void BVHAccel::draw(s32 nodeIdx, const Spectrum& c, Real alpha)const
	{
		const LinearBVHNode* node = &nodes[nodeIdx];
		if (node->numPrims > 0)
		{
			for (s32 i = 0; i < node->numPrims; ++i)
				primitives[node->primitivesOffset + i]->draw(c, alpha);
		}
		else
		{
			draw(nodeIdx + 1, c, alpha);
			draw(node->secondChildOffset, c, alpha);
		}
	}

	void BVHAccel::drawOutline(s32 nodeIdx, const Spectrum& c, Real alpha)const
	{
		const LinearBVHNode* node = &nodes[nodeIdx];
		if (node->numPrims > 0)
		{
			for (s32 i = 0; i < node->numPrims; ++i)
				primitives[node->primitivesOffset + i]->drawOutline(c, alpha);
		}
		else
		{
			drawOutline(nodeIdx + 1, c, alpha);
			drawOutline(node->secondChildOffset, c, alpha);
		}
	}

	void BVHAccel::drawBounds(s32 nodeIdx, const Spectrum& c, Real alpha)const
	{
		nodes[nodeIdx].bounds.draw(c, alpha);
	}

	bool BVHAccel::isLeaf(s32 nodeIdx)const
	{
		return nodes[nodeIdx].numPrims > 0;
	}

	s32 BVHAccel::getLeftNode(s32 nodeIdx)const
	{
		return nodeIdx + 1;
	}

	s32 BVHAccel::getRightNode(s32 nodeIdx)const
	{
		return nodes[nodeIdx].secondChildOffset;
	}
}