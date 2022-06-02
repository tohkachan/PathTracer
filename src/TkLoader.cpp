#include "TkLoader.h"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Material.hpp"
#include "Object.hpp"
#include "AreaLight.hpp"
#include "Matrix4.h"
#include "tracer/TkBdptTracer.h"
#include "tracer/TkSppmTracer.h"
#include "tracer/TkPathTracer.h"
#include "tracer/TkLightTracer.h"
//#include "TkSPPM.h"

#include "tinyxml.h"

namespace tk
{
	// map from strings to materials
	typedef std::map<string, Material*> MaterialMap;
	// map from strings to meshes
	typedef std::map<string, Shape*> ShapeMap;
	// map from strings to triangle vertices
	typedef std::map<string, AreaLight*> AreaLightMap;

	static const char STR_WIDTH[] = "width";
	static const char STR_HEIGHT[] = "height";
	static const char STR_FOV[] = "fov";
	static const char STR_NEAR[] = "near_clip";
	static const char STR_FAR[] = "far_clip";
	static const char STR_POSITION[] = "position";
	static const char STR_SCALE[] = "scale";
	static const char STR_ORIENT[] = "orientation";
	static const char STR_NORMAL[] = "normal";
	static const char STR_TCOORD[] = "tex_coord";
	static const char STR_COLOR[] = "color";
	static const char STR_RADIUS[] = "radius";
	static const char STR_R[] = "R";
	static const char STR_T[] = "T";
	static const char STR_ALPHA[] = "alpha";
	static const char STR_REFRACT[] = "refractive_index";
	static const char STR_TEXTURE[] = "texture";
	static const char STR_NAME[] = "name";
	static const char STR_TYPE[] = "type";
	static const char STR_FILENAME[] = "filename";
	static const char STR_CAMERA[] = "camera";
	static const char STR_LIGHT[] = "light";
	static const char STR_MATERIAL[] = "material";
	static const char STR_MESH[] = "mesh";
	static const char STR_OBJECT[] = "object";
	static const char STR_BUMP[] = "bump";
	static const char STR_TRACER[] = "tracer";
	static const char STR_SPP[] = "spp";
	static const char STR_DEPTH[] = "depth";
	static const char STR_ROULETTE[] = "roulete";
	static const char STR_THREAD[] = "thread";

	static void print_error_header(const TiXmlElement* base)
	{
		std::cout << "ERROR, " << base->Row() << ":" << base->Column() << "; "
			<< "in " << base->Value() << ", ";
	}

	static const TiXmlElement* get_unique_child(const TiXmlElement* parent, bool required, const char* name)
	{
		const TiXmlElement* elem = parent->FirstChildElement(name);

		if (!elem) {
			if (required) {
				print_error_header(parent);
				std::cout << "no '" << name << "' defined.\n";
				throw std::exception();
			}
			else {
				return 0;
			}
		}

		if (elem->NextSiblingElement(name)) {
			print_error_header(elem);
			std::cout << "'" << name << "' multiply defined.\n";
			throw std::exception();
		}

		return elem;
	}

	static void parse_attrib_real(const TiXmlElement* elem, bool required, const char* name, Real* val)
	{
#	if TK_DOUBLE_PRECISION
		int rv = elem->QueryDoubleAttribute(name, val);
#	else
		int rv = elem->QueryFloatAttribute(name, val);
#	endif
		if (rv == TIXML_WRONG_TYPE) {
			print_error_header(elem);
			std::cout << "error parsing '" << name << "'.\n";
			throw std::exception();
		}
		else if (required && rv == TIXML_NO_ATTRIBUTE) {
			print_error_header(elem);
			std::cout << "missing '" << name << "'.\n";
			throw std::exception();
		}
	}

	static void parse_attrib_int(const TiXmlElement* elem, bool required, const char* name, s32* val)
	{
		int rv = elem->QueryIntAttribute(name, val);
		if (rv == TIXML_WRONG_TYPE) {
			print_error_header(elem);
			std::cout << "error parsing '" << name << "'.\n";
			throw std::exception();
		}
		else if (required && rv == TIXML_NO_ATTRIBUTE) {
			print_error_header(elem);
			std::cout << "missing '" << name << "'.\n";
			throw std::exception();
		}
	}

	static void parse_attrib_string(const TiXmlElement* elem, bool required, const char* name, const char** val)
	{
		const char* att = elem->Attribute(name);
		if (!att && required) {
			print_error_header(elem);
			std::cout << "missing '" << name << "'.\n";
			throw std::exception();
		}
		else if (att) {
			*val = att;
		}
	}

	static void parse_attrib_string(const TiXmlElement* elem, bool required, const char* name, string* val)
	{
		const char* att = 0;
		parse_attrib_string(elem, required, name, &att);
		if (att) {
			*val = att;
		}
	}

	template< typename T >
	static void parse_elem(const TiXmlElement* /*elem*/, T* /*val*/)
	{
		throw std::exception();
	}

	template<> void parse_elem<Real>(const TiXmlElement* elem, Real* d)
	{
		parse_attrib_real(elem, true, "v", d);
	}

	template<> void parse_elem<s32>(const TiXmlElement* elem, s32* d)
	{
		int rv = elem->QueryIntAttribute("i", d);
		if (rv == TIXML_WRONG_TYPE) {
			print_error_header(elem);
			std::cout << "error parsing 'i'.\n";
			throw std::exception();
		}
		else if (rv == TIXML_NO_ATTRIBUTE) {
			print_error_header(elem);
			std::cout << "missing 'i'.\n";
			throw std::exception();
		}
	}

	template<> void parse_elem<Spectrum>(const TiXmlElement* elem, Spectrum* c)
	{
		parse_attrib_real(elem, true, "r", &c->r);
		parse_attrib_real(elem, true, "g", &c->g);
		parse_attrib_real(elem, true, "b", &c->b);
	}

	template<> void parse_elem<Vector3f>(const TiXmlElement* elem, Vector3f* vector)
	{
		parse_attrib_real(elem, true, "x", &vector->x);
		parse_attrib_real(elem, true, "y", &vector->y);
		parse_attrib_real(elem, true, "z", &vector->z);
	}

	template<> void parse_elem<Quaternion>(const TiXmlElement* elem, Quaternion* quat)
	{
		Quaternion q;
		Real x, y, z; // axis
		parse_attrib_real(elem, true, "x", &x);
		parse_attrib_real(elem, true, "y", &y);
		parse_attrib_real(elem, true, "z", &z);
		q.setByEuler(x, y, z);
		*quat = q;
	}

	template<typename T>
	static void parse_elem(const TiXmlElement* parent, bool required, const char* name, T* val)
	{
		const TiXmlElement* child = get_unique_child(parent, required, name);
		if (child)
			parse_elem<T>(child, val);			
	}

	template<> void parse_elem<Matrix4>(const TiXmlElement* elem, Matrix4* m)
	{
		Vector3f translate, scale;
		Quaternion ori;
		parse_elem(elem, true, STR_POSITION, &translate);
		parse_elem(elem, false, STR_ORIENT, &ori);
		parse_elem(elem, false, STR_SCALE, &scale);
		m->makeTransform(translate, scale, ori);
	}

	static void parse_camera(const TiXmlElement* elem, CameraInfo* camera)
	{
		// note: we don't load aspect, since it's set by the application
		Real fov;
		parse_elem(elem, true, STR_FOV, &fov);
		camera->fov = Degree(fov);
		parse_elem(elem, true, STR_NEAR, &camera->near);
		parse_elem(elem, true, STR_FAR, &camera->far);
		Vector3f tmp;
		parse_elem(elem, true, STR_POSITION, &tmp);
		camera->theta = tmp.x;
		camera->phi = tmp.y;
		camera->r = tmp.z;
	}

	static void parse_tracer(const TiXmlElement* elem, Config* config)
	{
		string type;
		parse_attrib_string(elem, true, STR_TYPE, &type);
		s32 spp = 16, maxDepth = 32, numThreads = 4;
		Real russianRoulette = 0.8;
		parse_elem(elem, false, STR_SPP, &spp);
		parse_elem(elem, false, STR_DEPTH, &maxDepth);
		parse_elem(elem, false, STR_ROULETTE, &russianRoulette);
		parse_elem(elem, false, STR_THREAD, &numThreads);
		if (type == "pt")
			config->tracer = new PathTracer(spp, maxDepth, numThreads, russianRoulette);
		else if (type == "sppm")
		{
			SPPMParam param;
			parse_elem(elem, true, "recurse", &param.numRecurse);
			parse_elem(elem, true, "gsize", &param.globalSize);
			parse_elem(elem, true, "gsample", &param.globalSample);
			parse_elem(elem, true, "csize", &param.causticSize);
			parse_elem(elem, true, "csample", &param.causticSample);
			parse_elem(elem, true, STR_RADIUS, &param.radius2);
			config->tracer = new SppmTracer(spp, maxDepth, numThreads, russianRoulette, param);
			//config->tracer = new SPPM(spp, maxDepth, numThreads, russianRoulette, param);
		}
		else if (type == "bdpt")
		{
			s32 debug_s = -1, debug_t = -1, noMis = 0;
			parse_elem(elem, false, "debug_s", &debug_s);
			parse_elem(elem, false, "debug_t", &debug_t);
			parse_elem(elem, false, "debug_no_mis", &noMis);
			config->tracer = new BdptTracer(spp, maxDepth, numThreads, russianRoulette,
				debug_s, debug_t, noMis);
		}
		else if (type == "light")
			config->tracer = new LightTracer(spp, maxDepth, numThreads, russianRoulette);
	}

	template< typename T >
	static T parse_lookup_data(std::map<string, T>& tmap, const TiXmlElement* elem, const char* name)
	{
		typename std::map<string, T>::iterator iter;
		string att;
		parse_attrib_string(elem, false, name, &att);
		iter = tmap.find(att);
		if (iter == tmap.end())
		{
			print_error_header(elem);
			std::cout << "No such " << name << " '" << att << "'.\n";
			throw std::exception();
		}
		return iter->second;
	}

	static Material* parse_material(const TiXmlElement* elem, string& name)
	{
		string type;
		parse_attrib_string(elem, true, STR_NAME, &name);
		parse_attrib_string(elem, true, STR_TYPE, &type);

		if (type == "glass")
		{
			float eta = 1.0f;
			Spectrum R = Spectrum::white, T = Spectrum::white;
			parse_elem(elem, false, STR_REFRACT, &eta);
			parse_elem(elem, false, STR_R, &R);
			parse_elem(elem, false, STR_T, &T);
			return new GlassMaterial(R, T, 1.0f, eta);
		}
		else if (type == "mirror")
		{
			Spectrum R = Spectrum::white;
			parse_elem(elem, false, STR_R, &R);
			return new MirrorMaterial(R);
		}
		else if (type == "diffuse")
		{
			Spectrum R = Spectrum::white;
			parse_elem(elem, false, STR_R, &R);
			return new DiffuseMaterial(R);
		}
		else if (type == "plastic")
		{
			float eta = 1.0f, alpha = 0.2f;
			Spectrum R = Spectrum::white;
			/*parse_elem(elem, false, STR_REFRACT, &eta);
			parse_elem(elem, false, STR_R, &R);*/
			parse_elem(elem, false, STR_ALPHA, &alpha);
			return new PlasticMaterial(1.0f, eta, alpha, R);
		}
		return nullptr;
	}

	static MeshTriangle* parse_mesh(const TiXmlElement* elem, string& name)
	{
		string filename;
		Matrix4 m;
		parse_attrib_string(elem, true, STR_NAME, &name);
		parse_attrib_string(elem, false, STR_FILENAME, &filename);
		parse_elem(elem, &m);
		return new MeshTriangle(0, 0, filename, m);
	}

	static AreaLight* parse_light(const TiXmlElement* elem, ShapeMap& shapemap, string& name)
	{
		parse_attrib_string(elem, true, STR_NAME, &name);
		Shape* shape = parse_lookup_data(shapemap, elem, STR_MESH);
		Spectrum Le;
		parse_elem(elem, true, STR_COLOR, &Le);
		return new AreaLight(1, Le, std::shared_ptr<Shape>(shape), false);
	}

	static Object* parse_object(MaterialMap& matmap, ShapeMap& meshmap, AreaLightMap& lightmap, const TiXmlElement* elem)
	{
		string type;
		parse_attrib_string(elem, true, STR_TYPE, &type);
		Shape* shape = nullptr;
		Material* material = parse_lookup_data(matmap, elem, STR_MATERIAL);
		AreaLight* light = nullptr;
		if (type == "light")
		{
			light = parse_lookup_data(lightmap, elem, STR_LIGHT);
			shape = parse_lookup_data(meshmap, elem, STR_MESH);
		}	
		else if (type == "mesh")
			shape = parse_lookup_data(meshmap, elem, STR_MESH);
		else if (type == "sphere")
		{
			Vector3f center;
			Real radius;
			parse_elem(elem, true, STR_POSITION, &center);
			parse_elem(elem, true, STR_RADIUS, &radius);
			shape = new Sphere(center, radius);
		}
		return new Object(std::shared_ptr<Shape>(shape), SharedLight(light), std::shared_ptr<Material>(material));
	}

	bool load_scene(Config* config, const char* filename)
	{
		TiXmlDocument doc(filename);
		const TiXmlElement* root = 0;
		const TiXmlElement* elem = 0;
		MaterialMap materials;
		ShapeMap shapes;
		AreaLightMap areaLights;

		if (!doc.LoadFile())
		{
			std::cout << "ERROR, " << doc.ErrorRow() << ":" << doc.ErrorCol() << "; "
				<< "parse error: " << doc.ErrorDesc() << "\n";
			return false;
		}

		root = doc.RootElement();
		if (!root) {
			std::cout << "No root element.\n";
			return false;
		}
		try
		{
			parse_attrib_string(root, false, STR_FILENAME, &config->outfilename);
			parse_elem(root, false, STR_WIDTH, &config->width);
			parse_elem(root, false, STR_HEIGHT, &config->height);
			parse_elem(root, false, "windows", &config->isWindows);			
			// parse the camera
			elem = get_unique_child(root, true, STR_CAMERA);
			parse_camera(elem, &config->camera);

			elem = get_unique_child(root, true, STR_TRACER);
			parse_tracer(elem, config);

			// parse the materials
			elem = root->FirstChildElement(STR_MATERIAL);
			while (elem)
			{
				string name;
				Material* mat = parse_material(elem, name);
				if (!materials.insert(std::make_pair(name, mat)).second)
				{
					print_error_header(elem);
					std::cout << "Material '" << name << "' multiply defined.\n";
					throw std::exception();
				}
				elem = elem->NextSiblingElement(STR_MATERIAL);
			}

			// parse the shapes
			elem = root->FirstChildElement(STR_MESH);
			while (elem) {
				string name;
				MeshTriangle* mesh = parse_mesh(elem, name);
				// place each mesh in map by it's name, so we can associate geometries
				// with them when loading geometries
				if (!shapes.insert(std::make_pair(name, mesh)).second) {
					print_error_header(elem);
					std::cout << "Mesh '" << name << "' multiply defined.\n";
					throw std::exception();
				}
				elem = elem->NextSiblingElement(STR_MESH);
			}

			// parse the lights
			elem = root->FirstChildElement(STR_LIGHT);
			while (elem)
			{
				string name;
				AreaLight* light = parse_light(elem, shapes, name);
				config->scene->Add(SharedLight(light));
				if (!areaLights.insert(std::make_pair(name, light)).second) {
					print_error_header(elem);
					std::cout << "AreaLight '" << name << "' multiply defined.\n";
					throw std::exception();
				}
				elem = elem->NextSiblingElement(STR_LIGHT);
			}

			elem = root->FirstChildElement(STR_OBJECT);
			while (elem) {
				Object* obj = parse_object(materials, shapes, areaLights, elem);
				config->scene->Add(obj);
				elem = elem->NextSiblingElement(STR_OBJECT);
			}
			config->scene->buildBVH();		
		}
		catch(std::bad_alloc const&)
		{
			std::cout << "Out of memory error while loading scene\n.";
			config->scene->reset();
			return false;
		}
		catch (...)
		{
			config->scene->reset();
			return false;
		}
		
		return true;
	}
}