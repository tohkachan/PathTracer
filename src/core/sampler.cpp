#include "sampler.h"
#include "sampling.h"
#include "TkLowdiscrepancy.h"

namespace tk
{
	Sampler::Sampler(int samplesPerPixel)
		: samplesPerPixel(samplesPerPixel)
	{}

	s32 Sampler::getSamplesPerPixel()const
	{
		return samplesPerPixel;
	}

	void Sampler::startPixel(const Point2i &p)
	{
		currentPixel = p;
		sampleIdx = 0;
	}

	void Sampler::startPixelSample(const Point2i& p, s32 idx, s32 dim)
	{
		currentPixel = p;
		sampleIdx = idx;
	}

	bool Sampler::startNextSample()
	{
		return ++sampleIdx < samplesPerPixel;
	}


	void StratifiedSampler::startPixel(const Point2i &p)
	{
		dim = 0;
		Sampler::startPixel(p);
	}

	void  StratifiedSampler::startPixelSample(const Point2i& p, s32 idx, s32 dim)
	{
		this->dim = dim;
		Sampler::startPixelSample(p, idx, dim);
	}

	float StratifiedSampler::get1D()
	{
		u64 hash = Hash(currentPixel, dim, seed);
		s32 stratum = Math::PermutationElement(sampleIdx, samplesPerPixel, hash);
		++dim;
		Real delta = jitter ? get_random_float() : 0.5f;
		return (stratum + delta) / samplesPerPixel;
	}

	Vector2f StratifiedSampler::get2D()
	{
		u64 hash = Hash(currentPixel, dim, seed);
		s32 stratum = Math::PermutationElement(sampleIdx, samplesPerPixel, hash);
		dim += 2;
		s32 x = stratum % xPixelSamples, y = stratum / xPixelSamples;
		Real dx = jitter ? get_random_float() : 0.5f;
		Real dy = jitter ? get_random_float() : 0.5f;
		return { (x + dx) / xPixelSamples, (y + dy) / yPixelSamples };
	}

	bool StratifiedSampler::startNextSample()
	{
		dim = 0;
		return Sampler::startNextSample();
	}

	std::unique_ptr<Sampler> StratifiedSampler::clone(int seed)
	{
		StratifiedSampler* ret = new StratifiedSampler(xPixelSamples, yPixelSamples, jitter, seed);
		return std::unique_ptr<Sampler>(ret);
	}

	//-------------------------------------------------------------------------------------------
	HaltonSampler::HaltonSampler(s32 samplesPerPixel, Point2i fullRes, s32 seed)
		: Sampler(samplesPerPixel)
	{
		// Find radical inverse base scales and exponents that cover sampling area
		for (int i = 0; i < 2; ++i) {
			int base = (i == 0) ? 2 : 3;
			int scale = 1, exp = 0;
			while (scale < std::min(fullRes[i], MaxHaltonResolution)) {
				scale *= base;
				++exp;
			}
			baseScales[i] = scale;
			baseExponents[i] = exp;
		}
		// Compute multiplicative inverses for _baseScales_
		multInverse[0] = multiplicativeInverse(baseScales[1], baseScales[0]);
		multInverse[1] = multiplicativeInverse(baseScales[0], baseScales[1]);
	}

	HaltonSampler::HaltonSampler(s32 samplesPerPixel, Point2i baseScales, Point2i baseExponents,
		s32 multInv[2]) : Sampler(samplesPerPixel),
		baseScales(baseScales), baseExponents(baseExponents)
	{
		multInverse[0] = multInv[0];
		multInverse[1] = multInv[1];
	}
	Real HaltonSampler::sampleDimension(s32 dim)const
	{
		return RadicalInverse(dim, haltonIdx);
	}

	void HaltonSampler::startPixel(const Point2i &p)
	{
		haltonIdx = 0;
		s32 sampleStride = baseScales.x * baseScales.y;
		// Compute Halton sample index for first sample in pixel _p_
		if (sampleStride > 1) {
			Point2i pm(Math::Mod(p.x, MaxHaltonResolution), Math::Mod(p.y, MaxHaltonResolution));
			for (s32 i = 0; i < 2; ++i) {
				u64 dimOffset =
					(i == 0) ? InverseRadicalInverse(pm[i], 2, baseExponents[i])
					: InverseRadicalInverse(pm[i], 3, baseExponents[i]);
				haltonIdx +=
					dimOffset * (sampleStride / baseScales[i]) * multInverse[i];
			}
			haltonIdx %= sampleStride;
		}
		//haltonIdx += sampleIdx * sampleStride;
		dim = 2;
		Sampler::startPixel(p);
	}

	void HaltonSampler::startPixelSample(const Point2i& p, s32 idx, s32 dim)
	{
		haltonIdx = 0;
		s32 sampleStride = baseScales.x * baseScales.y;
		// Compute Halton sample index for first sample in pixel _p_
		if (sampleStride > 1) {
			Point2i pm(Math::Mod(p.x, MaxHaltonResolution), Math::Mod(p.y, MaxHaltonResolution));
			for (s32 i = 0; i < 2; ++i) {
				u64 dimOffset =
					(i == 0) ? InverseRadicalInverse(pm[i], 2, baseExponents[i])
					: InverseRadicalInverse(pm[i], 3, baseExponents[i]);
				haltonIdx +=
					dimOffset * (sampleStride / baseScales[i]) * multInverse[i];
			}
			haltonIdx %= sampleStride;
		}
		haltonIdx += sampleIdx * sampleStride;
		this->dim = std::max(2, dim);
		Sampler::startPixelSample(p, idx, dim);
	}

	float HaltonSampler::get1D()
	{
		if (dim >= PrimeTableSize)
			dim = 2;
		return sampleDimension(dim++);
	}

	Vector2f HaltonSampler::get2D()
	{
		if (dim + 1 >= PrimeTableSize)
			dim = 2;
		s32 d = dim;
		dim += 2;
		return { sampleDimension(dim), sampleDimension(dim + 1) };
	}

	bool HaltonSampler::startNextSample()
	{
		haltonIdx = 0;
		int sampleStride = baseScales.x * baseScales.y;
		// Compute Halton sample index for first sample in pixel _p_
		if (sampleStride > 1) {
			Point2i pm(Math::Mod(currentPixel.x, MaxHaltonResolution), Math::Mod(currentPixel.y, MaxHaltonResolution));
			for (int i = 0; i < 2; ++i) {
				u64 dimOffset =
					(i == 0) ? InverseRadicalInverse(pm[i], 2, baseExponents[i])
					: InverseRadicalInverse(pm[i], 3, baseExponents[i]);
				haltonIdx +=
					dimOffset * (sampleStride / baseScales[i]) * multInverse[i];
			}
			haltonIdx %= sampleStride;
		}

		haltonIdx += (sampleIdx + 1) * sampleStride;
		dim = 2;
		return Sampler::startNextSample();
	}

	std::unique_ptr<Sampler> HaltonSampler::clone(int seed)
	{
		HaltonSampler* ret = new HaltonSampler(samplesPerPixel, baseScales, baseExponents, multInverse);
		return std::unique_ptr<Sampler>(ret);
	}
}