#ifndef __Tk_Lowdiscrepancy_H_
#define __Tk_Lowdiscrepancy_H_

#include "TkPrerequisites.h"
#include "TkMath.h"
#include "TkHash.h"

namespace tk
{

	// Prime Table Declarations
	static constexpr s32 PrimeTableSize = 1000;
	extern const s32 Primes[PrimeTableSize];

	// Low Discrepancy Declarations
	Real RadicalInverse(s32 baseIndex, u64 a);

	// Low Discrepancy Inline Functions
	inline Real RadicalInverse(s32 baseIndex, u64 a) {
		s32 base = Primes[baseIndex];
		Real invBase = (Real)1 / (Real)base, invBaseN = 1;
		u64 reversedDigits = 0;
		while (a) {
			// Extract least significant digit from _a_ and update _reversedDigits_
			u64 next = a / base;
			u64 digit = a - next * base;
			reversedDigits = reversedDigits * base + digit;
			invBaseN *= invBase;
			a = next;
		}
		return tk::min(reversedDigits * invBaseN, Math::one_minus_epsilon);
	}

	inline u64 InverseRadicalInverse(u64 inverse, s32 base,
		s32 nDigits) {
		u64 index = 0;
		for (s32 i = 0; i < nDigits; ++i) {
			u64 digit = inverse % base;
			inverse /= base;
			index = index * base + digit;
		}
		return index;
	}
}
#endif