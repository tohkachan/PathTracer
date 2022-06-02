#ifndef __Tk_Filter_H_
#define __Tk_Filter_H_

#include "TkPrerequisites.h"
#include "Vector.hpp"

namespace tk
{
	class Filter
	{
	public:
		const Real xWidth, yWidth;
		const Real invWidthX, invWidthY;

		Filter(Real xWidth, Real yWidth);
		virtual Real eval(Real x, Real y)const = 0;
		virtual Real normalizeTerm()const = 0;
	};

	class FilterTable
	{
	private:
		Vector2f mFilterRadius, mInvFilterRadius;
		Real* mTable;
		s32 mTableWidth;
	public:
		FilterTable(const Filter& filter, s32 tableWidth);
		~FilterTable();
		Real eval(Real x, Real y)const;
		const Vector2f& getFilterRadius()const { return mFilterRadius; }
	};

	class BoxFilter : public Filter
	{
	public:
		BoxFilter(Real xWidth, Real yWidth);
		Real eval(Real x, Real y)const;
		Real normalizeTerm()const;
	};

	class TriangleFilter : public Filter
	{
	public:
		TriangleFilter(Real xWidth, Real yWidth);
		Real eval(Real x, Real y)const;
		Real normalizeTerm()const;
	};

	class GaussianFilter : public Filter
	{
	private:
		Real alpha;
		Real expX, expY;

		Real gaussian(Real v, Real expbase)const;
	public:
		GaussianFilter(Real xWidth, Real yWidth, Real falloff);
		Real eval(Real x, Real y)const;
		Real normalizeTerm()const;
	};

	class MitchellFilter : public Filter
	{
	private:
		Real B, C;
	public:
		MitchellFilter(Real xWidth, Real yWidth, Real b, Real c);
		Real eval(Real x, Real y)const;
		Real normalizeTerm()const;
		Real Mitchell1D(Real x)const;
	};
}
#endif