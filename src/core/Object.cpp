#include "Object.hpp"
#include "Shape.h"
#include "Bounds3.hpp"

namespace tk
{
	Object::Object(const std::shared_ptr<Shape>& shape, const std::shared_ptr<AreaLight>& areaLight,
		const std::shared_ptr<Material>& material)
		: mShape(shape), mAreaLight(areaLight), mMaterial(material) {}

	bool Object::intersect(const Ray &r, Intersection* isect)const
	{
		if (!mShape->intersect(r, isect)) return false;
		isect->obj = this;
		return true;
	}

	Bounds3 Object::getBounds()const
	{
		return mShape->getBounds();
	}

	Shape* Object::getShape()const
	{
		return mShape.get();
	}

	const AreaLight* Object::getAreaLight()const
	{
		return mAreaLight.get();
	}

	const Material* Object::getMaterial()const
	{
		return mMaterial.get();
	}

	void Object::draw(const Spectrum& c, Real alpha)const
	{
		mShape->draw(c, alpha);
	}

	void Object::drawOutline(const Spectrum& c, Real alpha)const
	{
		mShape->drawOutline(c, alpha);
	}
}

