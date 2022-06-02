#include "Bounds3.hpp"

#include <GL/glew.h>

namespace tk
{
	void Bounds3::draw(Spectrum c, Real alpha)const
	{
		glColor4f(c.r, c.g, c.b, alpha);

		// top
		glBegin(GL_LINE_STRIP);
		glVertex3d(pMax.x, pMax.y, pMax.z);
		glVertex3d(pMax.x, pMax.y, pMin.z);
		glVertex3d(pMin.x, pMax.y, pMin.z);
		glVertex3d(pMin.x, pMax.y, pMax.z);
		glVertex3d(pMax.x, pMax.y, pMax.z);
		glEnd();

		// bottom
		glBegin(GL_LINE_STRIP);
		glVertex3d(pMin.x, pMin.y, pMin.z);
		glVertex3d(pMin.x, pMin.y, pMax.z);
		glVertex3d(pMax.x, pMin.y, pMax.z);
		glVertex3d(pMax.x, pMin.y, pMin.z);
		glVertex3d(pMin.x, pMin.y, pMin.z);
		glEnd();

		// side
		glBegin(GL_LINES);
		glVertex3d(pMax.x, pMax.y, pMax.z);
		glVertex3d(pMax.x, pMin.y, pMax.z);
		glVertex3d(pMax.x, pMax.y, pMin.z);
		glVertex3d(pMax.x, pMin.y, pMin.z);
		glVertex3d(pMin.x, pMax.y, pMin.z);
		glVertex3d(pMin.x, pMin.y, pMin.z);
		glVertex3d(pMin.x, pMax.y, pMax.z);
		glVertex3d(pMin.x, pMin.y, pMax.z);
		glEnd();
	}
}