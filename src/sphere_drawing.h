#ifndef UTIL_SPHEREDRAWING_H
#define UTIL_SPHEREDRAWING_H

#include "TkPrerequisites.h"

namespace tk
{

/**
 * Draws a sphere with the given position and radius in opengl, using the
 * current modelview/projection matrices and the given color.
 */
void draw_sphere_opengl(const Vector3f& p, double r, const Spectrum& c);

/**
 * Draws a sphere with the given position and radius in opengl, using the
 * current modelview/projection matrices and color/material settings.
 */
void draw_sphere_opengl(const Vector3f& p, double r);

void draw_sphere_wireframe_opengl(const Vector3f& p, double r, const Spectrum& c);
}
#endif