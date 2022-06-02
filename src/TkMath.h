#ifndef __Tk_Math_H_
#define __Tk_Math_H_

#include "TkPrerequisites.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <random>

namespace tk
{
	inline f32 min(const f32& lhs, const f32& rhs)
	{
		f32 ret;
		_mm_store_ss(&ret, _mm_min_ss(_mm_set_ss(lhs), _mm_set_ss(rhs)));
		return ret;
	}

	inline f32 max(const f32& lhs, const f32& rhs)
	{
		f32 ret;
		_mm_store_ss(&ret, _mm_max_ss(_mm_set_ss(lhs), _mm_set_ss(rhs)));
		return ret;
	}

	inline f64 min(const f64& lhs, const f64& rhs)
	{
		f64 ret;
		_mm_store_sd(&ret, _mm_min_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
		return ret;
	}

	inline f64 max(const f64& lhs, const f64& rhs)
	{
		f64 ret;
		_mm_store_sd(&ret, _mm_max_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
		return ret;
	}

	class Radian
	{
		Real mRad;
	public:
		explicit Radian(Real r = 0) : mRad(r) {}
		Radian(const Degree& d);
		Radian& operator=(const Real& f) { mRad = f; return *this; }
		Radian& operator=(const Radian& r) { mRad = r.mRad; return *this; }

		Real getDegrees()const;
		Real getRadians()const { return mRad; }

		Radian operator+(Real f)const { return Radian(mRad + f); }
		Radian& operator+=(Real f) { mRad += f; return *this; }
		Radian operator*(Real f)const { return Radian(mRad * f); }
		Radian operator*(const Radian& other)const { return Radian(mRad * other.mRad); }
		Radian& operator*=(Real f) { mRad *= f; return *this; }
		Radian operator/(Real f)const { return Radian(mRad / f); }
		Radian& operator/=(Real f) { mRad /= f; return *this; }

		bool operator==(const Radian& other)const { return mRad == other.mRad; }
		bool operator!=(const Radian& other)const { return mRad != other.mRad; }
		bool operator<(const Radian& other)const { return mRad < other.mRad; }
	};
	class Degree
	{
		Real mDeg;
	public:
		explicit Degree(Real d = 0.f) : mDeg(d) {}
		Real getDegrees()const { return mDeg; }
		Real getRadians()const;
	};

	class Math
	{
	public:
		static inline s32 IAbs(s32 i)
		{
			/*s32 mask = i >> 31;
			return (i ^ mask) - mask;*/
			return (i >= 0 ? i : -i);
		}
		static inline s32 ICeil(f32 f) { return s32(ceil(f)); }
		static inline s32 IFloor(f32 f) { return s32(floor(f)); }
		static s32 ISign(s32 i) { return (i > 0 ? +1 : (i < 0 ? -1 : 0)); }
		/** Absolute value function.*/
		static inline Real Abs(Real f) { return Real(fabs(f)); }
		static inline Radian Abs(Radian val) { return Radian(fabs(val.getRadians())); }
		static inline Degree Abs(Degree val) { return Degree(fabs(val.getDegrees())); }
		/** Arc cosine function.*/
		static Radian ACos(Real f) { return Radian(acos(f)); }
		/** Arc sine function.*/
		static Radian ASin(Real f) { return Radian(asin(f)); }
		/** Arc tangent function.*/
		static inline Radian ATan(Real f) { return Radian(atan(f)); }
		/** Arc tangent between two values function.*/
		static inline Radian ATan2(Real fY, Real fX) { return Radian(atan2(fY, fX)); }

		static inline Real Ceil(Real f) { return Real(ceil(f)); }
		static inline Real Floor(Real f) { return Real(floor(f)); }
		static inline bool isNaN(Real f)
		{
			//std::isnan() is C99, not supported by all compilers
			//However NaN always fails this next test, no other number does.
			return f != f;
		}

		static inline Real Log(Real f) { return Real(log(f)); }
		static inline Real Log2(Real f) { return Real(log(f) * inv_log2); }
		static inline Real LogN(Real base, Real f) { return Real(log(f) / log(base)); }
		static inline Real Pow(Real f, Real exponent) { return Real(pow(f, exponent)); }
		static inline Real Exp(Real f) { return Real(exp(f)); }

		static Real Sign(Real f) { return f > 0.0 ? 1.0 : (f < 0.0 ? -1.0 : 0.0); }

		static inline Radian Sign(const Radian& val)
		{
			return Radian(Sign(val.getRadians()));
		}

		static inline Degree Sign(const Degree& val)
		{
			return Degree(Sign(val.getDegrees()));
		}

		static inline Real Cos(const Radian& val)
		{
			return Real(cos(val.getRadians()));
		}

		static inline Real Cos(Real f)
		{
			return Real(cos(f));
		}

		static inline Real Sin(const Radian& val)
		{
			return Real(sin(val.getRadians()));
		}

		static inline Real Sin(Real f)
		{
			return Real(sin(f));
		}

		static inline Real Tan(const Radian& val)
		{
			return Real(tan(val.getRadians()));
		}

		static inline Real Tan(Real f)
		{
			return Real(tan(f));
		}

		/** Squared function.*/
		static inline Real Sqr(Real f) { return f * f; }

		/** Square root function.*/
		static inline Real Sqrt(Real f) { return Real(sqrt(f)); }

		static inline Radian Sqrt(const Radian& val) { return Radian(sqrt(val.getRadians())); }

		static inline Degree Sqrt(const Degree& val) { return Degree(sqrt(val.getDegrees())); }

		static Real InvSqrt(Real f) { return Real(1.0 / sqrt(f)); }

		static inline Real DegreesToRadians(Real degrees) { return degrees * fDeg2Rad; }
		static inline Real RadiansToDegrees(Real radians) { return radians * fRad2Deg; }

		static inline f32 Saturate(f32 t)
		{
			f32 temp = tk::max(t, 0.0f);
			temp = tk::min(temp, 1.0f);
			return temp;
		}

		static inline f64 Saturate(f64 t)
		{
			f64 temp = tk::max(t, 0.0);
			temp = tk::min(temp, 1.0);
			return temp;
		}

		template<typename T>
		static inline T Clamp(T val, T minVal, T maxVal)
		{
			return std::max(std::min(val, maxVal), minVal);
		}

		template<typename T, typename S>
		static inline T Lerp(const T& a, const T& b, const S& w)
		{
			return a * (1 - w) + b * w;
		}

		template <typename T>
		static inline T Mod(T a, T b) {
			T result = a - (a / b) * b;
			return (T)((result < 0) ? result + b : result);
		}

		static Real Gamma(s32 n)
		{
			return (n * machine_epsilon) / (1 - n * machine_epsilon);
		}

		static s32 PermutationElement(u32 i, u32 l, u32 p) {
			u32 w = l - 1;
			w |= w >> 1;
			w |= w >> 2;
			w |= w >> 4;
			w |= w >> 8;
			w |= w >> 16;
			do {
				i ^= p;
				i *= 0xe170893d;
				i ^= p >> 16;
				i ^= (i & w) >> 4;
				i ^= p >> 8;
				i *= 0x0929eb3f;
				i ^= p >> 23;
				i ^= (i & w) >> 1;
				i *= 1 | p >> 27;
				i *= 0x6935fa69;
				i ^= (i & w) >> 11;
				i *= 0x74dcb303;
				i ^= (i & w) >> 2;
				i *= 0x9e501cc3;
				i ^= (i & w) >> 2;
				i *= 0xc860a3df;
				i &= w;
				i ^= i >> 5;
			} while (i >= l);
			return (i + p) % l;
		}

		static const Real pos_infinity;
		static const Real neg_infinity;
		static const Real pi;
		static const Real inv_pi;
		static const Real two_pi;
		static const Real half_pi;
		static const Real fDeg2Rad;
		static const Real fRad2Deg;
		static const Real inv_log2;
		static const Real machine_epsilon;
		static const Real one_minus_epsilon;
	};

	template<>
	static inline f32 Math::Clamp<f32>(f32 val, f32 minVal, f32 maxVal)
	{
		return tk::max(tk::min(val, maxVal), minVal);
	}

	template<>
	static inline f64 Math::Clamp<f64>(f64 val, f64 minVal, f64 maxVal)
	{
		return tk::max(tk::min(val, maxVal), minVal);
	}

	template<>
	static inline f32 Math::Mod<f32>(f32 a, f32 b)
	{
		return std::fmod(a, b);
	}

	template<>
	static inline f64 Math::Mod<f64>(f64 a, f64 b)
	{
		return std::fmod(a, b);
	}

	inline Radian::Radian(const Degree& d) : mRad(d.getRadians())
	{
	}

	inline Real Radian::getDegrees()const
	{
		return Math::RadiansToDegrees(mRad);
	}

	inline Real Degree::getRadians()const
	{
		return Math::DegreesToRadians(mDeg);
	}

	inline Radian operator*(Real lhs, const Radian& rhs)
	{
		return Radian(lhs * rhs.getRadians());
	}

	inline Radian operator/(Real lhs, const Radian& rhs)
	{
		return Radian(lhs / rhs.getRadians());
	}


	template <typename Predicate>
	inline size_t FindInterval(size_t sz, const Predicate &pred) {
		using ssize_t = std::make_signed_t<size_t>;
		ssize_t size = (ssize_t)sz - 2, first = 1;
		while (size > 0) {
			// Evaluate predicate at midpoint and update _first_ and _size_
			size_t half = (size_t)size >> 1, middle = first + half;
			bool predResult = pred(middle);
			first = predResult ? middle + 1 : first;
			size = predResult ? size - (half + 1) : half;
		}
		return (size_t)Math::Clamp<ssize_t>((ssize_t)first - 1, 0, sz - 2);
	}

	inline float get_random_float()
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_real_distribution<float> dist(0.f, 1.f); // distribution in range [1, 6]

		return dist(rng);
	}

	inline unsigned FloatToBits(float f) {
		unsigned ui;
		memcpy(&ui, &f, sizeof(float));
		return ui;
	}

	inline float BitsToFloat(unsigned ui) {
		float f;
		memcpy(&f, &ui, sizeof(unsigned));
		return f;
	}


	inline float NextFloatUp(float v) {
		// Handle infinity and negative zero for _NextFloatUp()_
		if (std::isinf(v) && v > 0.f) return v;
		if (v == -0.f) v = 0.f;

		// Advance _v_ to next higher float
		unsigned ui = FloatToBits(v);
		if (v >= 0)
			++ui;
		else
			--ui;
		return BitsToFloat(ui);
	}

	inline float NextFloatDown(float v) {
		// Handle infinity and positive zero for _NextFloatDown()_
		if (std::isinf(v) && v < 0.) return v;
		if (v == 0.f) v = -0.f;
		unsigned ui = FloatToBits(v);
		if (v > 0)
			--ui;
		else
			++ui;
		return BitsToFloat(ui);
	}

	inline  bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
	{
		float discr = b * b - 4 * a * c;
		if (discr < 0) return false;
		else if (discr == 0) x0 = x1 = -0.5 * b / a;
		else {
			float q = (b > 0) ?
				-0.5 * (b + sqrt(discr)) :
				-0.5 * (b - sqrt(discr));
			x0 = q / a;
			x1 = c / q;
		}
		if (x0 > x1) std::swap(x0, x1);
		return true;
	}
}
#endif