#ifndef __Tk_Hash_H_
#define __Tk_Hash_H_

#include "TkPrerequisites.h"

namespace tk
{
	// https://github.com/explosion/murmurhash/blob/master/murmurhash/MurmurHash2.cpp
	template <bool isAligned>
	inline u64 MurmurHash64AFlex(const void *key, size_t len,
		u64 seed) {
		//if (isAligned)
			//DCHECK(((uintptr_t)key & 7) == 0);

		const u64 m = 0xc6a4a7935bd1e995ull;
		const s32 r = 47;

		u64 h = seed ^ (len * m);

		const u64 *data = (const u64*)key;
		const u64 *end = data + (len / 8);

		while (data != end) {
			u64 k;
			if constexpr (isAligned)
				k = *data++;
			else
				std::memcpy(&k, data++, sizeof(u64));

			k *= m;
			k ^= k >> r;
			k *= m;

			h ^= k;
			h *= m;
		}

		const u8 *data2 = (const u8 *)data;

		switch (len & 7) {
		case 7:
			h ^= u64(data2[6]) << 48;
		case 6:
			h ^= u64(data2[5]) << 40;
		case 5:
			h ^= u64(data2[4]) << 32;
		case 4:
			h ^= u64(data2[3]) << 24;
		case 3:
			h ^= u64(data2[2]) << 16;
		case 2:
			h ^= u64(data2[1]) << 8;
		case 1:
			h ^= u64(data2[0]);
			h *= m;
		};

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	template <typename T>
	inline u64 MurmurHash64A(const T *key, size_t len, u64 seed) {
		if (alignof(T) >= 8 || ((uintptr_t)key & 7) == 0)
			return MurmurHash64AFlex<true>(key, len, seed);
		else
			return MurmurHash64AFlex<false>(key, len, seed);
	}

	// Hashing Inline Functions
	// http://zimbry.blogspot.ch/2011/09/better-bit-mixing-improving-on.html
	inline u64 MixBits(u64 v);

	inline u64 MixBits(u64 v) {
		v ^= (v >> 31);
		v *= 0x7fb5d329728ea185;
		v ^= (v >> 27);
		v *= 0x81dadef4bc2dd44d;
		v ^= (v >> 33);
		return v;
	}

	template <typename T>
	inline u64 HashBuffer(const T *ptr, size_t size, u64 seed = 0) {
		return MurmurHash64A(ptr, size, seed);
	}

	template <size_t size, typename T>
	inline uint64_t HashBuffer(const T *ptr, u64 seed = 0) {
		return MurmurHash64A(ptr, size, seed);
	}

	template <typename... Args>
	inline u64 Hash(Args... args);

	template <typename... Args>
	inline void hashRecursiveCopy(c8 *buf, Args...);

	template <>
	inline void hashRecursiveCopy(c8 *buf) {}

	template <typename T, typename... Args>
	inline void hashRecursiveCopy(c8 *buf, T v, Args... args) {
		memcpy(buf, &v, sizeof(T));
		hashRecursiveCopy(buf + sizeof(T), args...);
	}

	template <typename... Args>
	inline u64 Hash(Args... args) {
		// C++, you never cease to amaze: https://stackoverflow.com/a/57246704
		constexpr size_t sz = (sizeof(Args) + ... + 0);
		constexpr size_t n = (sz + 7) / 8;
		u64 buf[n];
		hashRecursiveCopy((c8 *)buf, args...);
		return MurmurHash64A(buf, sz, 0);
	}

	template <typename... Args>
	inline Real HashFloat(Args... args) {
		return u32(Hash(args...)) * 0x1p-32f;
	}

}
#endif