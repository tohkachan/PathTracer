#ifndef __Tk_Film_H_
#define __Tk_Film_H_

#include "TkPrerequisites.h"
#include "TkFilter.h"
#include "TkSpectrum.h"

namespace tk
{
	struct Pixel
	{
		Spectrum contribSum;
		Real filterWeightSum;
	};

	class Film
	{
	private:
		Point2i mFullResolution;
		Filter* mFilter;
		FilterTable mFilterTable;
		Pixel* mPixels;
		Spectrum* mSplat;
		Bounds2i mCroppedPixelBounds;
	public:
		Film(const Point2i& resolution, Vector2f cropMin, Vector2f cropMax,
			Filter* filter);
		~Film();
		void clear();
		Point2i getFullResolution()const { return mFullResolution; }
		Bounds2i getSampleBounds()const;

		std::unique_ptr<FilmTile> getFilmTile(const Bounds2i& sampleBounds);
		void addSplat(const Vector2f& pFilm, Spectrum L);
		void mergeFilmTile(std::unique_ptr<FilmTile> tile);
		void writeImage(Real splatScale = 1, const string& filename = "");

		void setFrame(u32* buffer, Real splatScale = 1);

	};

	class FilmTile
	{
	private:
		Bounds2i mPixelBounds;
		const FilterTable& mFilterTable;
		std::vector<Pixel> mPixels;
		friend class Film;
	public:
		FilmTile(const Bounds2i& pixelBounds, const FilterTable& filterTable);
		void addSample(const Vector2f& pFilm, Spectrum L);
		const Pixel& getPixel(const Point2i &p)const
		{
			int width = mPixelBounds.pMax.x - mPixelBounds.pMin.x;
			int offset =
				(p.x - mPixelBounds.pMin.x) + (p.y - mPixelBounds.pMin.y) * width;
			return mPixels[offset];
		}
	};
}
#endif