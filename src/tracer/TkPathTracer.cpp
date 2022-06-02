#include "TkPathTracer.h"
#include "sampler.h"
#include "camera.h"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Object.hpp"
#include "Material.hpp"

#include <GL/glew.h>

namespace tk
{
	PathTracer::PathTracer(s32 spp, s32 maxDepth, s32 numThreads, Real russianRoulette)
		: RayTracer(spp, maxDepth, numThreads, russianRoulette)
	{
		s32 x = 1, y = mSpp / x;
		while ((y != x) && (y / x != 2))
		{
			x <<= 1;
			y = mSpp / x;
		}
		mSampler = new StratifiedSampler(x, y, true);
		mSelectionHistory.push_back(0);
	}

	void PathTracer::keyPress(s32 key)
	{
		BVHAccel* bvh = mScene->getBVH();
		s32 cur = mSelectionHistory.back();
		switch (key)
		{
		case 262: //KEYBOARD_RIGHT
			if (!bvh->isLeaf(cur))
				mSelectionHistory.push_back(bvh->getRightNode(cur));
			break;
		case 263: //KEYBOARD_LEFT
			if (!bvh->isLeaf(cur))
				mSelectionHistory.push_back(bvh->getLeftNode(cur));
			break;
		case 265:
			if (cur != 0)
				mSelectionHistory.pop_back();
		default:
			break;
		}
	}

	void PathTracer::visualize()
	{
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1);
		glEnable(GL_DEPTH_TEST);

		// hardcoded color settings
		Spectrum cnode = Spectrum(.5, .5, .5); float cnode_alpha = 0.25f;
		Spectrum cnode_hl = Spectrum(1., .25, .0); float cnode_hl_alpha = 0.6f;
		Spectrum cnode_hl_child = Spectrum::white; float cnode_hl_child_alpha = 0.6f;

		Spectrum cprim_hl_left = Spectrum(.6, .6, 1.); float cprim_hl_left_alpha = 1.f;
		Spectrum cprim_hl_right = Spectrum(.8, .8, 1.); float cprim_hl_right_alpha = 1.f;
		Spectrum cprim_hl_edges = Spectrum::black; float cprim_hl_edges_alpha = 0.5f;

		s32 selected = mSelectionHistory.back();

		// render solid geometry (with depth offset)
		glPolygonOffset(1.0, 1.0);
		glEnable(GL_POLYGON_OFFSET_FILL);

		BVHAccel* bvh = mScene->getBVH();

		if (bvh->isLeaf(selected))
			bvh->draw(selected, cprim_hl_left, cprim_hl_left_alpha);
		else
		{
			bvh->draw(bvh->getLeftNode(selected), cprim_hl_left, cprim_hl_left_alpha);
			bvh->draw(bvh->getRightNode(selected), cprim_hl_right, cprim_hl_right_alpha);
		}

		glDisable(GL_POLYGON_OFFSET_FILL);
		// draw geometry outline
		bvh->drawOutline(0, cprim_hl_edges, cprim_hl_edges_alpha);
		// keep depth buffer check enabled so that mesh occluded bboxes, but
		// disable depth write so that bboxes don't occlude each other.
		glDepthMask(GL_FALSE);
		std::vector<s32> stack;
		stack.push_back(0);
		while (!stack.empty())
		{
			s32 cur = stack.back();
			stack.pop_back();

			bvh->drawBounds(cur, cnode, cnode_alpha);
			if (!bvh->isLeaf(cur))
			{
				stack.push_back(bvh->getLeftNode(cur));
				stack.push_back(bvh->getRightNode(cur));
			}
		}

		if (!bvh->isLeaf(selected))
		{
			bvh->drawBounds(bvh->getLeftNode(selected), cnode_hl_child, cnode_hl_child_alpha);
			bvh->drawBounds(bvh->getRightNode(selected), cnode_hl_child, cnode_hl_child_alpha);
		}

		glLineWidth(3.f);
		bvh->drawBounds(selected, cnode_hl, cnode_hl_alpha);

		glDepthMask(GL_TRUE);
		glPopAttrib();
	}

	void PathTracer::rendering()
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

	void PathTracer::traceTile(Point2i start, Point2i end, Sampler& sampler)
	{
		std::unique_ptr<FilmTile> filmTile = mFilm->getFilmTile(Bounds2i(start, end));
		for (s32 y = start.y; y < end.y; ++y)
		{
			for (s32 x = start.x; x < end.x; ++x)
			{
				Point2i pixel = Point2i(x, y);
				sampler.startPixel(pixel);
				do {
					Vector2f cameraSample = sampler.get2D() + Vector2f(pixel.x, pixel.y);
					Ray r;
					mCamera->generateRay(cameraSample, sampler.get2D(), &r);
					filmTile->addSample(cameraSample, Li(r, sampler));
				} while (sampler.startNextSample());
			}
		}
		mutex1.lock();
		mFilm->mergeFilmTile(std::move(filmTile));
		mutex1.unlock();
	}

	Spectrum PathTracer::Li(Ray& r, Sampler& sampler, s32 depth)const
	{
		Spectrum L(0, 0, 0), beta(1, 1, 1);
		Intersection inter;
		s32 bounces;
		bool test = true;
		for (bounces = 0; bounces < mMaxDepth; ++bounces)
		{
			if (!mScene->intersect(r, &inter))
				break;
			Vector3f wo = -r.direction;
			const Material* m = inter.obj->getMaterial();
			m->setTransportMode(Radiance);
			if (test)
				L += beta * inter.Le(wo);
			L += beta * mScene->uniformSampleOneLight(inter, sampler);
			if (get_random_float() > mRussianRoulette)
				break;
			Vector3f wi;
			float pdf;
			Spectrum f = m->sample_f(wo, &wi, inter.n, sampler.get2D(), &pdf);
			if (pdf == 0 || f == Spectrum::black)
				break;
			if (isnan(pdf) || isnan(f.r))
				fprintf(stderr, "\nmaterial nan\n");
			test = m->getType() & (GLASS | MIRROR);
			beta *= f * AbsDot(wi, inter.n) / (pdf * mRussianRoulette);
			r = inter.spawnRay(wi);
			/*Volume* volume = 0;
			if (m->hasVolume())
			{
				float coso = dotProduct(wo, inter.n);
				bool entered = AbsDot(wi, inter.n) < 0.0f;
				bool transmit = (entered) != (coso < 0);
				if (transmit)
				{
					if (entered)
						r.enterVolume(m);
					else
						r.exitVolume(m);
				}
				volume = !r.m_volumes.empty() ? (*r.m_volumes.rbegin())->getVolume() : nullptr;
				if (volume)
				{
					Spectrum Lv;
					Spectrum transmittance;
					Spectrum weight;
					if (!volume->integrate(mScene, r, Lv, transmittance, weight, r))
						break;
					L += weight * Lv * beta;
					beta *= transmittance;
				}
			}*/
		}
		return L;
	}
}