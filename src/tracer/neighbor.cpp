#include "neighbor.h"

namespace tk
{
	void neighbor_search(NearestPhotons* np, KdTree* node, s32 sampleCount)
	{
		if (!node)
			return;
		Photon* p = node->pt;
		Vector3f nodePos = p->pos;
		if (p->split < 3)
		{
			float dist = np->dstPos[p->split] - nodePos[p->split];
			if (dist > 0)
			{
				neighbor_search(np, node->right, sampleCount);
				if (dist * dist < np->dist2[sampleCount])
					neighbor_search(np, node->left, sampleCount);
			}
			else
			{
				neighbor_search(np, node->left, sampleCount);
				if (dist * dist < np->dist2[sampleCount])
					neighbor_search(np, node->right, sampleCount);
			}
		}


		float dist2 = dotProduct(nodePos - np->dstPos, nodePos - np->dstPos);

		if (dist2 < np->dist2[sampleCount])
		{
			if (np->found < sampleCount)
			{
				np->dist2[np->found] = dist2;
				np->neighbor_photons[np->found] = p;
				np->found++;
				if (np->found == sampleCount)
				{
					int half = (np->found - 2) >> 1;
					for (int k = half; k >= 0; --k)
					{
						int parent = k;
						Photon *pp = np->neighbor_photons[k];
						float pdist2 = np->dist2[k];
						while (parent <= half)
						{
							int child = 2 * parent + 1;
							if ((child + 1) < np->found && np->dist2[child] < np->dist2[child + 1])
								child++;
							if (pdist2 > np->dist2[child])
								break;
							np->dist2[parent] = np->dist2[child];
							np->neighbor_photons[parent] = np->neighbor_photons[child];
							parent = child;
						}
						np->dist2[parent] = pdist2;
						np->neighbor_photons[parent] = pp;
					}
					np->dist2[sampleCount] = np->dist2[0];
				}
			}
			else
			{
				//construct a heap for store the neighbor photons
				int half = (np->found - 2) >> 1;
				int parent = 0;
				int child = 1;
				while (parent <= half)
				{
					if ((child + 1) < np->found && np->dist2[child] < np->dist2[child + 1])
						child++;
					if (dist2 > np->dist2[child])
						break;
					np->dist2[parent] = np->dist2[child];
					np->neighbor_photons[parent] = np->neighbor_photons[child];
					parent = child;
					child += child + 1;
				}
				np->dist2[parent] = dist2;
				np->neighbor_photons[parent] = p;
				np->dist2[sampleCount] = np->dist2[0];
			}
		}
	}
}