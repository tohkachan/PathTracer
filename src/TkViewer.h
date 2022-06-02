#ifndef __Tk_Viewer_H_
#define __Tk_Viewer_H_

#include "TkPrerequisites.h"

#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace tk
{
#define DEFAULT_W 800
#define DEFAULT_H 600
	class Viewer
	{
	public:
	//private:
		// HDPI display
		static bool HDPI;

		// framerate related timeers
		static s32 framecount;
		static std::chrono::time_point<std::chrono::system_clock> sys_last;
		static std::chrono::time_point<std::chrono::system_clock> sys_curr;


		// info toggle
		static bool showInfo;

		static GLFWwindow* window;
		static size_t buffer_w;
		static size_t buffer_h;

		// user space renderer
		static Renderer* renderer;

		// window event callbacks
		static void err_callback(int error, const char* description);
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void resize_callback(GLFWwindow* window, int width, int height);
		static void cursor_callback(GLFWwindow* window, double xpos, double ypos);
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	public:
		Viewer(void);

		//Viewer(const char* title);

		/**
		 * Destructor.
		 * Destroys the viewer instance and frees memory.
		 * Note that this does not change the user space renderer.
		 */
		~Viewer(void);

		/**
		 * Initialize the viewer.
		 * This will open up a window and install all the event handlers
		 * and make the viewer ready for drawing.
		 */
		void init(size_t w, size_t h);

		/**
		 * Start the drawing loop of the viewer.
		 * Once called this will block until the viewer is close.
		 */
		void start(void);

		/**
		 * Set a user space renderer.
		 * The viewer will use the given user space renderer in drawing.
		 * \param renderer The user space renderer to use in the viewer.
		 */
		void setRenderer(Renderer *renderer);

		void resize(int w, int h) {
			glfwSetWindowSize(window, w, h);
			resize_callback(window, w, h);
		}

		static void update(void);

		static void drawInfo(void);
	};
}

#endif