#include "TkFilm.h"


namespace tk
{

	FilmTile::FilmTile(const Bounds2i& pixelBounds, const FilterTable& filterTable)
		: mPixelBounds(pixelBounds),
		mFilterTable(filterTable)
	{
		mPixels = std::vector<Pixel>(std::max(0, pixelBounds.area()));
	}
	
	void FilmTile::addSample(const Vector2f& pFilm, Spectrum L)
	{ 
		// Transform continuous space sample to discrete space	
		Vector2f pFilmDiscrete = pFilm - Vector2f(0.5f);
		Vector2f tmp = pFilmDiscrete - mFilterTable.getFilterRadius();
		Point2i p0 = Point2i(Math::ICeil(tmp.x),
			Math::ICeil(tmp.y));
		tmp = pFilmDiscrete + mFilterTable.getFilterRadius();
		Point2i p1 = Point2i(Math::IFloor(tmp.x + 1),
			Math::IFloor(tmp.y + 1));
		p0 = Point2i(std::max(p0.x, mPixelBounds.pMin.x), std::max(p0.y, mPixelBounds.pMin.y));
		p1 = Point2i(std::min(p1.x, mPixelBounds.pMax.x), std::min(p1.y, mPixelBounds.pMax.y));

		s32 w = mPixelBounds.pMax.x - mPixelBounds.pMin.x;
		// Loop over filter support and add sample to pixel arrays
		for (s32 y = p0.y; y < p1.y; ++y) {
			for (s32 x = p0.x; x < p1.x; ++x) {
				// Evaluate filter value at $(x,y)$ pixel
				Real filterWeight = mFilterTable.eval(x - pFilmDiscrete.x, y - pFilmDiscrete.y);

				// Update pixel values with filtered sample contribution	
				s32 offset =
					(x - mPixelBounds.pMin.x) + (y - mPixelBounds.pMin.y) * w;			
				Pixel &pixel = mPixels[offset];
				pixel.contribSum += L * filterWeight;
				pixel.filterWeightSum += filterWeight;
			}
		}
	}

	Film::Film(const Point2i& resolution, Vector2f cropMin, Vector2f cropMax,
		Filter* filter)
		: mFullResolution(resolution),
		mFilter(filter),
		mFilterTable(*filter, 16)
	{
		mCroppedPixelBounds =
			Bounds2i(Point2i(Math::ICeil(resolution.x * cropMin.x),
				Math::ICeil(resolution.y * cropMin.y)),
				Point2i(Math::ICeil(resolution.x * cropMax.x),
					Math::ICeil(resolution.y * cropMax.y)));

		mPixels = new Pixel[mCroppedPixelBounds.area()];
		mSplat = new Spectrum[mCroppedPixelBounds.area()];
	}

	Film::~Film()
	{
		delete[] mPixels;
		delete[] mSplat;
	}

	void Film::clear()
	{
		memset(mPixels, 0, mCroppedPixelBounds.area() * sizeof(Pixel));
		memset(mSplat, 0, mCroppedPixelBounds.area() * sizeof(Spectrum));
	}

	Bounds2i Film::getSampleBounds()const
	{
		Real xWidth = mFilter->xWidth;
		Real yWidth = mFilter->yWidth;
		return { Point2i(Math::IFloor(mCroppedPixelBounds.pMin.x + 0.5f - xWidth),
			Math::IFloor(mCroppedPixelBounds.pMin.y + 0.5f - yWidth)),
			Point2i(Math::ICeil(mCroppedPixelBounds.pMax.x - 0.5f + xWidth),
			Math::ICeil(mCroppedPixelBounds.pMax.y - 0.5f + yWidth)) };
	}

	std::unique_ptr<FilmTile> Film::getFilmTile(const Bounds2i& sampleBounds)
	{
		Point2i p0 = Point2i(Math::ICeil(sampleBounds.pMin.x - 0.5f - mFilter->xWidth),
			Math::ICeil(sampleBounds.pMin.y - 0.5f - mFilter->yWidth));
		Point2i p1 = Point2i(Math::IFloor(sampleBounds.pMax.x - 0.5f + mFilter->xWidth),
			Math::IFloor(sampleBounds.pMax.y - 0.5f + mFilter->yWidth)) + Point2i(1, 1);
		Bounds2i tilePixelBounds = Bounds2i(p0, p1);
		tilePixelBounds.intersect(mCroppedPixelBounds);
		FilmTile* tile = new FilmTile(tilePixelBounds, mFilterTable);
		return std::unique_ptr<FilmTile>(tile);
	}

	void Film::addSplat(const Vector2f& pFilm, Spectrum L)
	{
		s32 x = Math::IFloor(pFilm.x) - mCroppedPixelBounds.pMin.x;
		s32 y = Math::IFloor(pFilm.y) - mCroppedPixelBounds.pMin.y;
		s32 w = mCroppedPixelBounds.pMax.x - mCroppedPixelBounds.pMin.x;
		s32 h = mCroppedPixelBounds.pMax.y - mCroppedPixelBounds.pMin.y;
		if (x < 0 || y < 0 || x >= w || y >= h)
			return;
		mSplat[x + y * w] += L;
	}

	void Film::mergeFilmTile(std::unique_ptr<FilmTile> tile)
	{ 
		s32 w = mCroppedPixelBounds.pMax.x - mCroppedPixelBounds.pMin.x;
		for (s32 y = tile->mPixelBounds.pMin.y; y < tile->mPixelBounds.pMax.y; ++y)
		{
			for (s32 x = tile->mPixelBounds.pMin.x; x < tile->mPixelBounds.pMax.x; ++x)
			{
				const Pixel& tilePixel = tile->getPixel(Point2i(x, y));
				s32 offset =
					(x - mCroppedPixelBounds.pMin.x) + (y - mCroppedPixelBounds.pMin.y) * w;
				mPixels[offset].contribSum += tilePixel.contribSum;
				mPixels[offset].filterWeightSum += tilePixel.filterWeightSum;
			}
		}
	}

	void Film::writeImage(Real splatScale, const string& filename)
	{
		s32 startX = mCroppedPixelBounds.pMin.x, endX = mCroppedPixelBounds.pMax.x;
		s32 startY = mCroppedPixelBounds.pMin.y, endY = mCroppedPixelBounds.pMax.y;
		s32 w = endX - startX;
		s32 h = endY - startY;
		u8* frame = new u8[w * h * 3];
		s32 offset = 0;
		for (s32 y = endY; y-- > 0; )
		{
			for (s32 x = startY; x < endY; ++x)
			{
				s32 idx = (x - startX) + (y - startY) * w;
				Spectrum c = mPixels[idx].contribSum;
				Real filterWeightSum = mPixels[idx].filterWeightSum;
				if (filterWeightSum != 0)
					c = c / filterWeightSum;
				c += mSplat[idx] * splatScale;
				frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.r, 0.0f, 1.0f), 0.6);
				frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.g, 0.0f, 1.0f), 0.6);
				frame[offset++] = (u8)255 * Math::Pow(Math::Clamp(c.b, 0.0f, 1.0f), 0.6);
			}
		}		
		FILE* fp = fopen(filename.c_str(), "wb");
		(void)fprintf(fp, "P6\n%d %d\n255\n", w, h);
		fprintf(stderr, "\n[Tracer] Saving to file: %s... ", filename.c_str());
		fwrite(frame, 1, w * h * 3, fp);
		fprintf(stderr, "Done!\n");
		fclose(fp);
		delete[] frame;
	}

	void Film::setFrame(u32* buffer, Real splatScale)
	{
		Point2i d = mCroppedPixelBounds.diagonal();
		for (s32 i = 0; i < d.x * d.y; ++i)
		{
			Spectrum c = mPixels[i].contribSum;
			Real filterWeightSum = mPixels[i].filterWeightSum;
			if (filterWeightSum != 0)
				c = c / filterWeightSum;
			c += mSplat[i] * splatScale;
			u32 p = 0;
			p |= (u32)(255 * Math::Pow(Math::Clamp(c.b, 0.0f, 1.0f), 0.6)) << 16;
			p |= (u32)(255 * Math::Pow(Math::Clamp(c.g, 0.0f, 1.0f), 0.6)) << 8;
			p |= (u32)(255 * Math::Pow(Math::Clamp(c.r, 0.0f, 1.0f), 0.6));
			p |= 0xFF000000;
			buffer[i] = p;
		}
	}
}