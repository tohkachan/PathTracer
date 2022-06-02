#ifndef PREREQUISITE_H
#define PREREQUISITE_H

namespace tk
{
#if defined(_MSC_VER) || (( __BORLANDC__ >= 0x530) && !defined( __STRICT_ANSI__ ))
	typedef unsigned __int8				u8;
	typedef __int8						s8;

	typedef unsigned __int16			u16;
	typedef __int16						s16;

	typedef unsigned __int32			u32;
	typedef __int32						s32;
#else
	typedef unsigned char				u8;
	typedef signed char					s8;

	typedef unsigned short				u16;
	typedef signed short				s16;

	typedef unsigned int				u32;
	typedef signed int					s32;
#endif

#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
	typedef unsigned __int64			u64;
	typedef __int64						s64;
#elif __GNUC__
#	if __WORDSIZE == 64
	typedef unsigned long int			u64;
	typedef long int					s64;
#	else
	__extension__ typedef unsigned long long	u64;
	__extension__ typedef long long				s64;
#	endif
#else
	typedef unsigned long long			u64;
	typedef long long					s64;
#endif
	typedef char				c8;
	typedef float				f32;
	typedef double				f64;

#if TK_DOUBLE_PRECISION
	typedef doule Real;
	typedef u64 RealAsUint;
#else
	typedef float Real;
	typedef u32 RealAsUint;
#endif
}

#include <cassert>
#include <cmath>
#include <ctime>

//! STL containers
#include <vector>
#include <map>
#include <string>
#include <set>
#include <list>
#include <deque>
#include <bitset>

#include <algorithm>
#include <functional>
#include <limits>

#include <fstream>
#include <iosfwd>
#include <sstream>
#include <iostream>

extern "C" {
#	include <sys/types.h>
#	include <sys/stat.h>
}

#undef min
#undef max

namespace tk
{
	typedef std::string string;
	typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stringStream;
}


namespace tk
{
	class Bounds3;
	class Vector3f;
	class Vector2f;
	class Point2i;
	class Bounds2i;
	class Matrix4;
	class Quaternion;
	class Intersection;
	class Ray;
	class Spectrum;
	class Radian;
	class Degree;
	class Math;

	class Scene;
	class Object;
	class Shape;
	class Sphere;
	class Triangle;
	class Material;
	class Light;
	class Camera;
	class AreaLight;
	class BVHAccel;

	class Viewer;
	class Renderer;
	class RayTracer;

	class Sampler;
	class Filter;
	class FilmTile;
	class Film;
}

#endif