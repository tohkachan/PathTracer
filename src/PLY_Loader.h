#ifndef PLY_LOADER_H
#define PLY_LOADER_H

#include "Vector.hpp"

namespace tk
{
	struct CallbackContext
	{
		Vector3f* p;
		Vector3f* n;
		Vector2f* uv;
		int* indices;
		int* faceIndices;
		int indexCtr, faceIndexCtr;
		int face[4];
		bool error;
		int vertexCount;

		CallbackContext() :
			p(nullptr),
			n(nullptr),
			uv(nullptr),
			indices(nullptr),
			faceIndices(nullptr),
			indexCtr(0),
			faceIndexCtr(0),
			error(false),
			vertexCount(0)
		{}
		~CallbackContext()
		{
			if (p)
				delete[] p;
			if (n)
				delete[] n;
			if (uv)
				delete[] uv;
			if (indices)
				delete[] indices;
			if (faceIndices)
				delete[] faceIndices;
		}
	};



	CallbackContext* createPLYMesh(string path);
}

#endif