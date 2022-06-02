#ifndef SAMPLER_H
#define SAMPLER_H

#include "TkPrerequisites.h"
#include "Vector.hpp"

namespace tk
{

	class Sampler
	{
	protected:
		Point2i currentPixel;
		int sampleIdx;
		int samplesPerPixel;
	public:
		Sampler(int samplesPerPixel);

		s32 getSamplesPerPixel()const;
		virtual void startPixel(const Point2i &p);
		virtual void startPixelSample(const Point2i& p, s32 idx, s32 dim = 0);
		virtual float get1D() = 0;
		virtual Vector2f get2D() = 0;
		virtual bool startNextSample();
		virtual std::unique_ptr<Sampler> clone(int numDimensions) = 0;
		
	};

	class StratifiedSampler : public Sampler
	{
	private:
		int xPixelSamples, yPixelSamples, seed;
		bool jitter;
		int dim;
	public:
		StratifiedSampler(int xPixelSamples, int yPixelSamples,
			bool jitter, int seed = 0)
			: Sampler(xPixelSamples * yPixelSamples),
			xPixelSamples(xPixelSamples),
			yPixelSamples(yPixelSamples),
			jitter(jitter),
			seed(seed)
		{
		}

		void startPixel(const Point2i &p);
		void startPixelSample(const Point2i& p, s32 idx, s32 dim);

		float get1D();
		Vector2f get2D();

		virtual bool startNextSample();
		std::unique_ptr<Sampler> clone(int seed);
	};

	class HaltonSampler : public Sampler
	{
	private:
		static constexpr s32 MaxHaltonResolution = 128;
		Point2i baseScales, baseExponents;
		s32 multInverse[2];
		s64 haltonIdx;
		s32 dim;


		HaltonSampler(s32 samplesPerPixel, Point2i baseScales, Point2i baseExponents,
			s32 multInv[2]);
		static u64 multiplicativeInverse(s64 a, s64 n) {
			s64 x, y;
			extendedGCD(a, n, &x, &y);
			return Math::Mod(x, n);
		}

		static void extendedGCD(u64 a, u64 b, s64 *x, s64 *y) {
			if (b == 0) {
				*x = 1;
				*y = 0;
				return;
			}
			s64 d = a / b, xp, yp;
			extendedGCD(b, a % b, &xp, &yp);
			*x = yp;
			*y = xp - (d * yp);
		}
		Real sampleDimension(s32 dim)const;
	public:
		HaltonSampler(s32 samplesPerPixel, Point2i fullResolution, s32 seed = 0);
		void startPixel(const Point2i &p);
		void startPixelSample(const Point2i& p, s32 idx, s32 dim);

		float get1D();
		Vector2f get2D();

		bool startNextSample();
		std::unique_ptr<Sampler> clone(int seed);
	};
}
#endif