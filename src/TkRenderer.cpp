#include "TkRenderer.h"
#include "Scene.hpp"
#include "camera.h"
#include "Object.hpp"
#include "TkRayTracer.h"
//#include "TkImage.h"

#include <GL/glew.h>

#include "TkSpectrum.h"

namespace tk
{
	Renderer::Renderer(Config& config)
		: mMode(EDIT),
		mCameraInfo(config.camera),
		mScene(config.scene),
		mRayTracer(config.tracer),
		mScreenW(config.width),
		mScreenH(config.height)
	{
		mFilter = new BoxFilter(0.5f, 0.5f);
	}

	Renderer::~Renderer()
	{
		if (mRayTracer)
			delete mRayTracer;
		if (mCamera)
			delete mCamera;
		if (mScene)
			delete mScene;
		if (mFilter)
			delete mFilter;
		if (mFilm)
			delete mFilm;
	}

	void Renderer::init()
	{
		mLeftDown = false;
		mRightDown = false;
		mMiddleDown = false;

		// Enable anti-aliasing and circular points.
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

		mMode = EDIT;

		mCameraInfo.minR = 0.05;
		mCameraInfo.maxR = mCameraInfo.r * 2.0f;
		mCameraInfo.rotate(0, 0);

		mFilm = new Film(Point2i(mScreenW, mScreenH), Vector2f(0), Vector2f(1.0f), mFilter);

		mCamera = new Camera(&mCameraInfo.c2w, mCameraInfo.perspective(mScreenW, mScreenH), 0, 1.0, mFilm);
	}

	void Renderer::render(void)
	{
		update_camera();
		switch (mMode)
		{
		case EDIT:
		{
			draw_coordinates();
			auto prims = mScene->get_objects();
			for (size_t i = 0; i < prims.size(); ++i)
				prims[i]->drawOutline(Spectrum(0.8, 0.8, 1.0), 0.5);
		}
		break;
		case VISUALIZE:
		case RENDER:
			mRayTracer->updateScreen();
			break;
		}
	}

	void Renderer::resize(size_t w, size_t h)
	{
		mScreenW = w;
		mScreenH = h;
		Matrix4 c2s = mCameraInfo.perspective(w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(mCameraInfo.fov.getDegrees(), (Real)w / (Real)h,
			mCameraInfo.near, mCameraInfo.far);
	}

	string Renderer::name(void)
	{
		return "PathTracer";
	}

	string Renderer::info(void)
	{
		return "PathTracer";
	}

	void Renderer::cursor_event(float x, float y)
	{
		if (mLeftDown && !mMiddleDown && !mRightDown)
			mouse_left_dragged(x, y);
		else if (!mLeftDown && !mMiddleDown && mRightDown)
			mouse_right_dragged(x, y);
		else if (!mLeftDown && !mMiddleDown && !mRightDown)
			mouse_moved(x, y);
		mMouseX = x;
		mMouseY = y;
	}

	void Renderer::scroll_event(float offset_x, float offset_y)
	{
		if (mMode == RENDER) return;
		mCameraInfo.toward(-offset_y * 0.5);
	}

	void Renderer::mouse_event(int key, int event, unsigned char mods)
	{
		switch (event)
		{
		case 0://release
			switch (key)
			{
			case LEFT:
				mouse_released(LEFT);
				break;
			case RIGHT:
				mouse_released(RIGHT);
				break;
			case MIDDLE:
				mouse_released(MIDDLE);
				break;
			}
			break;
		case 1://press
			switch (key)
			{
			case LEFT:
				mouse_pressed(LEFT);
				break;
			case RIGHT:
				mouse_pressed(RIGHT);
				break;
			case MIDDLE:
				mouse_pressed(MIDDLE);
				break;
			}
			break;
		}
		
	}

	void Renderer::keyboard_event(int key, int event, unsigned char mods)
	{
		switch (mMode)
		{
		case EDIT:
			if (event == 1)
			{
				switch (key)
				{
				case 'v': case 'V':
					mRayTracer->setScene(mScene);
					mRayTracer->setCamera(mCamera);
					mRayTracer->startVisualizing();
					mMode = VISUALIZE;
					break;
				case 'r': case 'R':
					mRayTracer->setScene(mScene);
					mRayTracer->setCamera(mCamera);
					mRayTracer->startRaytracing();
					mMode = RENDER;
					break;
				}
			}
			break;
		case VISUALIZE:
			if (event == 1)
			{
				Vector3f z;
				switch (key)
				{
				case 'e': case 'E':
					mRayTracer->stop();
					mRayTracer->clear();
					mMode = EDIT;
					break;
				case 'r': case 'R':
					mRayTracer->stop();
					mRayTracer->startRaytracing();
					mMode = RENDER;
					break;
				default:
					mRayTracer->keyPress(key);
				}
			}
			break;
		case RENDER:
			if (event == 1)
			{
				switch (key)
				{
				case 'e': case 'E':
					mRayTracer->stop();
					mRayTracer->clear();
					mMode = EDIT;
					mouse_moved(mMouseX, mMouseY);
					break;
				case 'v': case 'V':
					mRayTracer->stop();
					mRayTracer->startVisualizing();
					mMode = VISUALIZE;
					break;
				case 's':case 'S':
					mRayTracer->saveImage("binary.ppm");
					break;
				}
			}
			break;
		}
	}

	void Renderer::mouse_pressed(eMouseButton b)
	{
		switch (b)
		{
		case LEFT:
			mLeftDown = true;
			break;
		case RIGHT:
			mRightDown = true;
			break;
		case MIDDLE:
			mMiddleDown = true;
			break;
		}
	}

	void Renderer::mouse_released(eMouseButton b)
	{
		switch (b)
		{
		case LEFT:
			mLeftDown = false;
			break;
		case RIGHT:
			mRightDown = false;
			break;
		case MIDDLE:
			mMiddleDown = false;
			break;
		}
	}

	void Renderer::mouse_left_dragged(float x, float y)
	{
		if (mMode == RENDER) return;
		float dx = (mMouseX - x) * (Math::pi / mScreenW);
		float dy = (mMouseY - y) * (Math::pi / mScreenH);

		mCameraInfo.rotate(dx, dy);
	}

	void Renderer::mouse_right_dragged(float x, float y)
	{
		if (mMode == RENDER) return;
		float dx = (mMouseX - x) * (Math::pi / mScreenW);
		float dy = (y - mMouseY) * (Math::pi / mScreenH);

		mCameraInfo.offset(dx, dy);
	}

	void Renderer::mouse_moved(float x, float y)
	{


	}

	void Renderer::update_camera()
	{
		GLint view[4];
		glGetIntegerv(GL_VIEWPORT, view);
		if (view[2] != mScreenW || view[3] != mScreenH)
			resize(view[2], view[3]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		mCameraInfo.c2w.setTranslation(mCameraInfo.pos);
		Vector3f up = mCameraInfo.yAxis();
		gluLookAt(mCameraInfo.pos.x, mCameraInfo.pos.y, mCameraInfo.pos.z,
			mCameraInfo.target.x, mCameraInfo.target.y, mCameraInfo.target.z,
			up.x, up.y, up.z);

	}

	void Renderer::draw_coordinates()
	{
		Vector3f target = mCameraInfo.target;
		glDisable(GL_DEPTH_TEST);

		glBegin(GL_LINES);
		glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		glVertex3f(target.x, target.y, target.z);
		glVertex3f(target.x + 1, target.y, target.z);

		glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
		glVertex3f(target.x, target.y, target.z);
		glVertex3f(target.x, target.y + 1, target.z);

		glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
		glVertex3f(target.x, target.y, target.z);
		glVertex3f(target.x, target.y, target.z + 1);

		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		for (int x = 0; x <= 8; ++x) {
			glVertex3f(target.x + x - 4, target.y, target.z - 4);
			glVertex3f(target.x + x - 4, target.y, target.z + 4);
		}
		for (int z = 0; z <= 8; ++z) {
			glVertex3f(target.x - 4, target.y, target.z + z - 4);
			glVertex3f(target.x + 4, target.y, target.z + z - 4);
		}
		glEnd();

		glEnable(GL_DEPTH_TEST);
	}

	void CameraInfo::rotate(float dx, float dy)
	{
		phi = Math::Clamp(phi + dy, 0.f, Math::pi);
		theta += dx;
		Real sinPhi = Math::Sin(phi);
		if (sinPhi == 0)
		{
			phi += Math::machine_epsilon;
			sinPhi = Math::Sin(phi);
		}
		Vector3f dir(r * sinPhi * Math::Sin(theta),
			r * Math::Cos(phi), r * sinPhi * Math::Cos(theta));
		pos = target + dir;
		Vector3f up(0, sinPhi > 0 ? 1 : -1, 0);
		Vector3f right = normalize(crossProduct(up, dir));
		Vector3f newUp = normalize(crossProduct(dir, right));
		Vector3f toward = normalize(dir);
		c2w = {
			right.x, newUp.x, toward.x, 0,
			right.y, newUp.y, toward.y, 0,
			right.z, newUp.z, toward.z, 0,
			0, 0, 0, 1,
		};
	}

	Matrix4 CameraInfo::perspective(float width, float hight)
	{
		Matrix4 c2s(0.0f);
		float h = 1.0f / Math::Tan(fov.getRadians() * 0.5f);
		float w = h * hight / width;

		c2s[0][0] = w;
		c2s[1][1] = h;
		// right
		//c2s[2][2] = far / (far - near);
		//c2s[2][3] = -far * near / (far - near);
		//c2s[3][2] = 1.0f;

		// left
		c2s[2][2] = -far / (far - near);
		c2s[2][3] = -far * near / (far - near);
		c2s[3][2] = -1.0f;
		return c2s;
	}

	void CameraInfo::toward(float dist)
	{
		float newR = Math::Clamp(r - dist, minR, maxR);
		pos = target + ((pos - target) * (newR / r));
		r = newR;
	}

	void CameraInfo::offset(float dx, float dy)
	{
		Vector3f x = { c2w[0][0], c2w[1][0], c2w[2][0] };
		Vector3f y = { c2w[0][1], c2w[1][1], c2w[2][1] };
		Vector3f displace = y * dy + x * dx;
		pos += displace;
		target += displace;
	}

	Vector3f CameraInfo::yAxis()const
	{
		return { c2w[0][1], c2w[1][1], c2w[2][1] };
	}
}