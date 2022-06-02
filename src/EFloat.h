#ifndef EFLOAT_H
#define EFLOAT_H

#include "TkMath.h"

namespace tk
{
	class EFloat
	{
	protected:
		float v, low, high;
	public:
		EFloat() {}
		EFloat(const EFloat& other)
			: v(other.v), low(other.low), high(other.high)
		{}
		EFloat(float v, float err = 0.0f) : v(v)
		{
			if (err == 0.0f)
				low = high = v;
			else
			{
				low = NextFloatDown(v - err);
				high = NextFloatUp(v + err);
			}
		}
		EFloat operator-()const
		{
			EFloat ret;
			ret.v = -v;
			ret.low = -high;
			ret.high = -low;
			return ret;
		}
		explicit operator float()const { return v; }
		explicit operator double()const { return v; }
		EFloat& operator=(const EFloat& rhs)
		{
			if (&rhs != this)
			{
				v = rhs.v;
				low = rhs.low;
				high = rhs.high;
			}
			return *this;
		}
		EFloat operator+(EFloat rhs)const
		{
			EFloat ret;
			ret.v = v + rhs.v;
			ret.low = NextFloatDown(low + rhs.low);
			ret.high = NextFloatUp(high + rhs.high);
			return ret;
		}
		EFloat operator-(EFloat rhs)const
		{
			EFloat ret;
			ret.v = v - rhs.v;
			ret.low = NextFloatDown(low - rhs.high);
			ret.high = NextFloatUp(high - rhs.low);
			return ret;
		}
		EFloat operator*(EFloat rhs)const
		{
			EFloat ret;
			ret.v = v * rhs.v;
			float prod[4] = {
				low * rhs.low, high * rhs.low,
				low * rhs.high, high * rhs.high
			};
			ret.low = NextFloatDown(std::min(std::min(prod[0], prod[1]), std::min(prod[2], prod[3])));
			ret.high = NextFloatUp(std::max(std::max(prod[0], prod[1]), std::max(prod[2], prod[3])));
			return ret;
		}

		EFloat operator/(EFloat rhs)const
		{
			EFloat ret;
			ret.v = v / rhs.v;
			if (rhs.low < 0 && rhs.high > 0)
			{
				ret.low = -std::numeric_limits<float>::infinity();
				ret.high = std::numeric_limits<float>::infinity();
			}
			else
			{
				float div[4] = {
								low / rhs.low, high / rhs.low,
								low / rhs.high, high / rhs.high
				};
				ret.low = NextFloatDown(std::min(std::min(div[0], div[1]), std::min(div[2], div[3])));
				ret.high = NextFloatUp(std::max(std::max(div[0], div[1]), std::max(div[2], div[3])));
			}
			return ret;
		}
		bool operator==(EFloat rhs)const { return v == rhs.v; }

		float getAbsError()const { return NextFloatUp(std::max(high - v, v - low)); }

		float upperBound()const { return high; }
		float lowerBound()const { return low; }
		static inline EFloat sqrt(EFloat f)
		{
			EFloat ret;
			ret.v = std::sqrt(f.v);
			ret.low = NextFloatDown(std::sqrt(f.low));
			ret.high = NextFloatUp(std::sqrt(f.high));
			return ret;
		}
		static inline EFloat abs(EFloat f)
		{
			if (f.low >= 0)
				return f;
			else if (f.high <= 0)
				return -f;
			else
			{
				EFloat ret;
				ret.v = std::fabs(f.v);
				ret.low = 0;
				ret.high = std::max(-f.low, f.high);
				return ret;
			}
		}
		static inline bool quadratic(EFloat a, EFloat b, EFloat c, EFloat& t0, EFloat& t1)
		{
			float discrim = b.v * b.v - 4.0f * a.v * c.v;
			if (discrim < 0) return false;
			float rootDiscrim = std::sqrt(discrim);

			EFloat fRootDiscrim(rootDiscrim, Math::machine_epsilon * rootDiscrim);

			EFloat q;
			if ((float)b < 0)
				q = (b - fRootDiscrim) * (-0.5f);
			else
				q = (b + fRootDiscrim) * (-0.5f);
			t0 = q / a;
			// q1 * q2 = b^2 - b^2 + 4ac / 4a = c
			t1 = c / q;
			if ((float)t0 > (float)t1) std::swap(t0, t1);
			return true;
		}
	};
}

#endif