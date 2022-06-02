#include "Matrix4.h"
#include "camera.h"
#include "Scene.hpp"
#include "TkRayTracer.h"
#include "TkRenderer.h"
#include "TkViewer.h"
#include "TkLoader.h"
#include <chrono>

using namespace tk;

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Please input the scene file!");
		return 0;
	}
	string inputfile = argv[1];
	Config config;
	config.scene = new Scene();

	load_scene(&config, inputfile.c_str());

	if (config.isWindows)
	{
		Viewer v = Viewer();
		Renderer* r = new Renderer(config);
		v.setRenderer(r);
		v.init(config.width, config.height);
		v.start();
	}
	else
	{
		config.camera.rotate(0, 0);
		config.camera.c2w.setTranslation(config.camera.pos);
		BoxFilter filter(0.5, 0.5);
		//GaussianFilter filter(2.0, 2.0, 2.0);
		//TriangleFilter filter(2.f, 2.f);
		//MitchellFilter filter(2.0, 2.0, 1.f / 3.f, 1.f / 3.f);
		Film* film = new Film(Point2i(config.width, config.height), Vector2f(0), Vector2f(1.0f, 1.0f), &filter);
		Camera* camera = new Camera(&config.camera.c2w, config.camera.perspective(config.width, config.height), 0.00, 2.0, film);
		RayTracer* tracer = config.tracer;
		tracer->setScene(config.scene);
		tracer->setCamera(camera);

		auto start = std::chrono::system_clock::now();
		tracer->render(config.outfilename);
		auto stop = std::chrono::system_clock::now();

		std::cout << "Render complete: \n";
		std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
		std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
		std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
		delete tracer;
		delete camera;
		delete film;
		delete config.scene;
	}
    return 0;
}