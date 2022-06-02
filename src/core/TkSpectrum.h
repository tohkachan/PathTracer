#ifndef __Tc_Spectrum_H_
#define __Tc_Spectrum_H_

#include "TkPrerequisites.h"

namespace tk
{
	class Spectrum
	{
	public:
		Real r, g, b;
		Spectrum() : r(0), g(0), b(0) {}
		Spectrum(Real r, Real g, Real b) : r(r), g(g), b(b) {}
		inline Spectrum operator+(const Spectrum& rhs)const
		{
			return Spectrum(r + rhs.r, g + rhs.g, b + rhs.b);
		}

		inline Spectrum& operator+=(const Spectrum& rhs)
		{
			r += rhs.r;
			g += rhs.g;
			b += rhs.b;
			return *this;
		}

		inline Spectrum operator*(const Spectrum& rhs)const
		{
			return Spectrum(r * rhs.r, g * rhs.g, b * rhs.b);
		}

		inline Spectrum& operator*=(const Spectrum& rhs)
		{
			r *= rhs.r;
			g *= rhs.g;
			b *= rhs.b;
			return *this;
		}

		inline Spectrum operator*(Real scale)const
		{
			return Spectrum(r * scale, g * scale, b * scale);
		}

		inline Spectrum& operator*=(Real scale)
		{
			r *= scale;
			g *= scale;
			b *= scale;
			return *this;
		}

		inline Spectrum operator/(Real scale)const
		{
			return Spectrum(r / scale, g / scale, b / scale);
		}

		inline bool operator==(const Spectrum& rhs)const
		{
			return r == rhs.r && g == rhs.g && b == rhs.b;
		}

		inline bool operator!=(const Spectrum& rhs)const
		{
			return r != rhs.r || g != rhs.g || b != rhs.b;
		}

		inline Real illum()const
		{
			return 0.2126f * r + 0.7152f * g + 0.0722f * b;
		}

		static const Spectrum white;
		static const Spectrum black;
	};
}
#endif