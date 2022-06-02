#include "TkFilter.h"
#include "TkMath.h"

namespace tk
{
	Filter::Filter(Real xWidth, Real yWidth)
		: xWidth(xWidth), yWidth(yWidth),
		invWidthX(1.0f / xWidth), invWidthY(1.0f / yWidth)
	{
	}

	//---------------------------------------------------------------------------
	FilterTable::FilterTable(const Filter& filter, s32 tableWidth)
		: mFilterRadius(filter.xWidth, filter.yWidth),
		mInvFilterRadius(filter.invWidthX, filter.invWidthY),
		mTableWidth(tableWidth)
	{
		mTable = new Real[tableWidth * tableWidth];
		Real dx = mFilterRadius.x / mTableWidth;
		Real dy = mFilterRadius.y / mTableWidth;
		Real term = 1.0f / filter.normalizeTerm();
		for (s32 y = 0; y < mTableWidth; ++y)
			for (s32 x = 0; x < mTableWidth; ++x)
				mTable[x + y * mTableWidth] = filter.eval((x + 0.5f) * dx, (y + 0.5f) * dy) * term;
	}

	Real FilterTable::eval(Real x, Real y)const
	{
		s32 iy = std::min(Math::IFloor(Math::Abs(mTableWidth * y * mInvFilterRadius.y)), mTableWidth - 1);
		s32 ix = std::min(Math::IFloor(Math::Abs(mTableWidth * x * mInvFilterRadius.x)), mTableWidth - 1);
		return mTable[ix + iy * mTableWidth];
	}

	FilterTable::~FilterTable()
	{
		delete[] mTable;
	}

	//---------------------------------------------------------------------------
	BoxFilter::BoxFilter(Real xWidth, Real yWidth)
		: Filter(xWidth, yWidth)
	{
	}

	Real BoxFilter::eval(Real x, Real y)const
	{
		return 1.0f;
	}

	Real BoxFilter::normalizeTerm()const
	{
		return 4.0f * xWidth * yWidth;
	}

	//---------------------------------------------------------------------------
	TriangleFilter::TriangleFilter(Real xWidth, Real yWidth)
		: Filter(xWidth, yWidth)
	{
	}

	Real TriangleFilter::eval(Real x, Real y)const
	{
		return max(0, xWidth - Math::Abs(x)) * max(0, yWidth - Math::Abs(y));
	}

	Real TriangleFilter::normalizeTerm()const
	{
		// 4 * integrate (integrate (w - x) * (h - y) over 0-w) over 0-h =
		// w * w * h * h
		return xWidth * xWidth * yWidth * yWidth;
	}

	//---------------------------------------------------------------------------
	GaussianFilter::GaussianFilter(Real xWidth, Real yWidth, Real falloff)
		: Filter(xWidth, yWidth), alpha(falloff),
		expX(Math::Exp(-falloff * xWidth * xWidth)),
		expY(Math::Exp(-falloff * yWidth * yWidth))
	{
	}

	Real GaussianFilter::gaussian(Real v, Real expbase)const
	{
		return max(0, Math::Exp(-alpha * v * v) - expbase);
	}

	Real GaussianFilter::eval(Real x, Real y)const
	{
		return gaussian(x, expX) * gaussian(y, expY);
	}

	Real GaussianFilter::normalizeTerm()const
	{
		// for now just simply use numercal approximation
		// this integration actually involves an erf function
		// that need to be approximated anyway...
		size_t step = 20;
		Real deltaX = xWidth / static_cast<Real>(step);
		Real deltaY = yWidth / static_cast<Real>(step);
		float ret = 0.0f;
		for (size_t i = 0; i < step; ++i) {
			for (size_t j = 0; j < step; ++j) {
				ret += 4.0f * deltaX *deltaY *
					gaussian(i * deltaX, expX) *
					gaussian(j * deltaY, expY);
			}
		}
		return ret;
	}

	//---------------------------------------------------------------------------
	MitchellFilter::MitchellFilter(Real xWidth, Real yWidth, Real b, Real c)
		: Filter(xWidth, yWidth),
		B(b), C(c)
	{
	}

	Real MitchellFilter::eval(Real x, Real y)const
	{
		return Mitchell1D(x * invWidthX) * Mitchell1D(y * invWidthY);
	}

	Real MitchellFilter::normalizeTerm()const
	{
		return 4.0f * ((12 - 9 * B - 6 * C) / 4 +
			(-18 + 12 * B + 6 * C) / 3 + (6 - 2 * B) +
			15 * (-B - 6 * B) / 4 + 7 * (6 * B + 30 * C) / 3 +
			3 * (-12 * B - 48 * C) / 2 + (8 * B + 24 * C)) / 6.0f;
	}

	Real MitchellFilter::Mitchell1D(Real x)const
	{
		x = Math::Abs(x);
		if (x > 1)
			return ((-B - 6 * C) * x * x * x + (6 * B + 30 * C) * x * x +
			(-12 * B - 48 * C) * x + (8 * B + 24 * C)) *
				(1.f / 6.f);
		else
			return ((12 - 9 * B - 6 * C) * x * x * x +
			(-18 + 12 * B + 6 * C) * x * x + (6 - 2 * B)) *
				(1.f / 6.f);
	}
}