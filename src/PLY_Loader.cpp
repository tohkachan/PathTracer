#include "PLY_Loader.h"
#include "rply.h"

namespace tk
{
	void rply_message_callback(p_ply ply, const char* message)
	{
	}

	int rply_vertex_callback(p_ply_argument arg)
	{
		float** buffers;
		long index, flags;
		ply_get_argument_user_data(arg, (void**)&buffers, &flags);
		ply_get_argument_element(arg, nullptr, &index);

		int bufferIdx = (flags & 0xF00) >> 8;
		int stride = (flags & 0x0F0) >> 4;
		int offset = flags & 0x00F;

		float* buffer = buffers[bufferIdx];
		if (buffer)
			buffer[index * stride + offset] = (float)ply_get_argument_value(arg);
		return 1;
	}

	int rply_face_callback(p_ply_argument arg)
	{
		CallbackContext* context;
		long flags;
		ply_get_argument_user_data(arg, (void**)&context, &flags);
		if (flags == 0)
		{
			long length, value_index;
			ply_get_argument_property(arg, nullptr, &length, &value_index);
			if (length != 3 && length != 4)
			{
				return 1;
			}
			else if (value_index < 0)
				return 1;
			if (value_index >= 0)
			{
				int value = (int)ply_get_argument_value(arg);
				if (value < 0 || value >= context->vertexCount)
				{
					context->error = true;
				}
				context->face[value_index] = value;
			}

			if (value_index == length - 1)
			{
				for (int i = 0; i < 3; ++i)
					context->indices[context->indexCtr++] = context->face[i];
				if (length == 4)
				{
					context->indices[context->indexCtr++] = context->face[3];
					context->indices[context->indexCtr++] = context->face[0];
					context->indices[context->indexCtr++] = context->face[2];
				}
			}
		}
		else
		{
			context->faceIndices[context->faceIndexCtr++] = (int)ply_get_argument_value(arg);
		}
		return 1;
	}

	CallbackContext* createPLYMesh(string path)
	{
		p_ply ply = nullptr;
		ply = ply_open(path.c_str(), rply_message_callback, 0, nullptr);
		if (!ply)
			return nullptr;
		if (!ply_read_header(ply))
			return nullptr;
		p_ply_element element = nullptr;
		long vertexCount = 0, faceCount = 0;
		while ((element = ply_get_next_element(ply, element)) != nullptr)
		{
			const char* name;
			long nInstances;
			ply_get_element_info(element, &name, &nInstances);
			if (!strcmp(name, "vertex"))
				vertexCount = nInstances;
			else if (!strcmp(name, "face"))
				faceCount = nInstances;
		}
		if (vertexCount == 0 || faceCount == 0)
			return nullptr;
		CallbackContext* context = new CallbackContext();

		if (ply_set_read_cb(ply, "vertex", "x", rply_vertex_callback, context, 0x030) &&
			ply_set_read_cb(ply, "vertex", "y", rply_vertex_callback, context, 0x031) &&
			ply_set_read_cb(ply, "vertex", "z", rply_vertex_callback, context, 0x032))
		{
			context->p = new Vector3f[vertexCount];
		}
		else
		{
			delete context;
			return nullptr;
		}


		if (ply_set_read_cb(ply, "vertex", "nx", rply_vertex_callback, context, 0x130) &&
			ply_set_read_cb(ply, "vertex", "ny", rply_vertex_callback, context, 0x131) &&
			ply_set_read_cb(ply, "vertex", "nz", rply_vertex_callback, context, 0x132))
		{
			context->n = new Vector3f[vertexCount];
		}

		if ((ply_set_read_cb(ply, "vertex", "u", rply_vertex_callback, context, 0x220) &&
			ply_set_read_cb(ply, "vertex", "v", rply_vertex_callback, context, 0x221)) ||
			(ply_set_read_cb(ply, "vertex", "s", rply_vertex_callback, context, 0x220) &&
				ply_set_read_cb(ply, "vertex", "t", rply_vertex_callback, context, 0x221)) ||
				(ply_set_read_cb(ply, "vertex", "texture_u", rply_vertex_callback, context, 0x220) &&
					ply_set_read_cb(ply, "vertex", "texture_v", rply_vertex_callback, context, 0x221)) ||
					(ply_set_read_cb(ply, "vertex", "texture_s", rply_vertex_callback, context, 0x220) &&
						ply_set_read_cb(ply, "vertex", "texture_t", rply_vertex_callback, context, 0x221)))
		{
			context->uv = new Vector2f[vertexCount];
		}

		context->indices = new int[faceCount * 6];
		context->vertexCount = vertexCount;

		ply_set_read_cb(ply, "face", "vertex_indices", rply_face_callback, context, 0);
		if (ply_set_read_cb(ply, "face", "face_indices", rply_face_callback, context, 1))
			context->faceIndices = new int[faceCount];
		if (!ply_read(ply))
		{
			ply_close(ply);
			delete context;
			return nullptr;
		}
		ply_close(ply);
		if (context->error)
		{
			delete context;
			return nullptr;
		}
		return context;
	}
}