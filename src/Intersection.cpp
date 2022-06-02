#include "Intersection.hpp"
#include "Object.hpp"
#include "AreaLight.hpp"
#include "TkSpectrum.h"

namespace tk
{
	 Spectrum Intersection::Le(const Vector3f& w)const
	{
		const AreaLight* light = obj->getAreaLight();
		return light ? light->L(*this, w) : Spectrum::black;
	}
}
