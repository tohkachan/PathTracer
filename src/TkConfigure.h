#ifndef __Tk_Configure_H_
#define __Tk_Configure_H_

#include "TkPrerequisites.h"
#include "Matrix4.h"

namespace tk
{
	struct CameraInfo
	{
		Vector3f pos;
		Vector3f target = Vector3f(0);
		Degree fov;
		Real far;
		Real near;
		Real phi;
		Real theta;
		Real r;
		Real minR;
		Real maxR;
		Matrix4 c2w;
		void rotate(float dx, float dy);
		Matrix4 perspective(float width, float hight);
		void toward(float dist);
		void offset(float dx, float dy);
		Vector3f yAxis()const;
	};

	struct SPPMParam
	{
		s32 numRecurse;
		s32 globalSize;
		s32 globalSample;
		s32 causticSize;
		s32 causticSample;
		Real radius2;
	};

	struct Config
	{
		Scene* scene;
		CameraInfo camera;
		RayTracer* tracer;
		s32 isWindows = 0;
		string outfilename;
		s32 width = 784;
		s32 height = 784;
	};
}
#endif