#include "engine.h"

#include <stdexcept>
#include <iostream>
#include <thread>
#include <math.h>
#include <csignal>

#include "spdlog/spdlog.h"

// logic for python in debug mode
// little bit hacky but should do the trick
#ifdef _DEBUG
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#endif

#include "python/Python.h"

#ifdef _DEBUG_WAS_DEFINED
#define _DEBUG
#undef _DEBUG_WAS_DEFINED
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/string_cast.hpp"

#include "renderManager.h"
#include "camera.h"
#include "inputManager.h"


std::vector<GameObject> Engine::gameObjects;
static PyObject* engineError;

//python methods to interact with engine
static PyObject* change_scale(PyObject* self, PyObject* args) {
	char* tag;
    float scaleX, scaleY, scaleZ;
	if(PyArg_ParseTuple(args, "sfff", &tag, &scaleX, &scaleY, &scaleZ)) {
		auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
		auto index = std::distance(Engine::gameObjects.begin(), it);
		if (index < 0 || index >= Engine::gameObjects.size()) {
			spdlog::critical("No such tag exists");
		}
		else {
			Engine::gameObjects[index].transform.scale = glm::vec3(scaleX, scaleY, scaleZ);
		}
	}

	return PyLong_FromLong(0);
}

static PyObject* change_translation(PyObject* self, PyObject* args) {
	char* tag;
    float translationX, translationY, translationZ;
	if(PyArg_ParseTuple(args, "sfff", &tag, &translationX, &translationY, &translationZ)) {
		auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
		auto index = std::distance(Engine::gameObjects.begin(), it);
		if (index < 0 || index >= Engine::gameObjects.size()) {
			spdlog::critical("No such tag exists");
		}
		else {
			Engine::gameObjects[index].transform.translation = glm::vec3(translationX, translationY, translationZ);
		}
	}

	return PyLong_FromLong(0);
}

static PyObject* change_rotation(PyObject* self, PyObject* args) {
	char* tag;
    float rotationX, rotationY, rotationZ;
	if(PyArg_ParseTuple(args, "sfff", &tag, &rotationX, &rotationY, &rotationZ)) {
		auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
		auto index = std::distance(Engine::gameObjects.begin(), it);
		if (index < 0 || index >= Engine::gameObjects.size()) {
			spdlog::critical("No such tag exists");
		}
		else {
			Engine::gameObjects[index].transform.rotation = glm::vec3(rotationX, rotationY, rotationZ);
		}
	}

	return PyLong_FromLong(0);
}

static PyObject* get_tags(PyObject* self, PyObject* args) {
	PyObject* listObj = PyList_New(Engine::gameObjects.size());
	for (int i = 0; i < Engine::gameObjects.size(); i++) {
		PyObject* tag = PyBytes_FromString(Engine::gameObjects[i].getTag().c_str());
		PyList_SetItem(listObj, i, tag);
	}
	return listObj;
}

//helper methods python/c++ interaction
static struct PyMethodDef methods[] = {
	{ "change_scale", change_scale, METH_VARARGS, "test print method"},
	{ "change_translation", change_translation, METH_VARARGS, "test print method"},
	{ "change_rotation", change_rotation, METH_VARARGS, "test print method"},
	{ "get_tags", get_tags, METH_VARARGS, "test print method"},
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef modDef = {
	PyModuleDef_HEAD_INIT, "engine", NULL, -1, methods, 
	NULL, NULL, NULL, NULL
};

static PyObject* PyInit_engine(void) {
	return PyModule_Create(&modDef);
}

Engine::Engine() {
	loadGameObjects();

	//tell python where to find c++ interaction methods 
	PyImport_AppendInittab("engine", &PyInit_engine);
}

Engine::~Engine() {
	gameObjects.clear();
}

//temp
std::unique_ptr<Model> createCubeModel(Device& device, glm::vec3 offset) {
	std::vector<Model::Vertex> vertices{

     // left face (white)
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},

      // right face (yellow)
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},

      // top face (orange, remember y axis points down)
      {{-.5f, -.5f, -.5f}, {0.8f, 0.1f, 0.8f}},
      {{.5f, -.5f, .5f},   {0.8f, 0.1f, 0.8f}},
      {{-.5f, -.5f, .5f},  {0.8f, 0.1f, 0.8f}},
      {{-.5f, -.5f, -.5f}, {0.8f, 0.1f, 0.8f}},
      {{.5f, -.5f, -.5f},  {0.8f, 0.1f, 0.8f}},
      {{.5f, -.5f, .5f},   {0.8f, 0.1f, 0.8f}},

      // bottom face (red)
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},

      // nose face (blue)
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},

      // tail face (green)
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},

	};
	for (auto& v : vertices) {
	v.position += offset;
	}
	return std::make_unique<Model>(device, vertices);
}

void Engine::loadGameObjects() {
	std::shared_ptr<Model> model = createCubeModel(device, {.0f, .0f, .0f});
	//auto cube = GameObject::createGameObject();
	//cube.model = model;
	//cube.transform.translation = {.0f, .0f, .5f};
	//cube.transform.scale = {.5f, .5f, .5f};
	//cube.transform.rotation = { 0.f, 0.5f, 0.f };
	//gameObjects.push_back(std::move(cube));

	//auto cube2 = GameObject::createGameObject();
	//cube2.model = model;
	//cube2.transform.translation = {.5f, .5f, 1.f};
	//cube2.transform.scale = {.5f, .5f, .5f};
	//cube2.transform.rotation = { 0.f, 0.5f, 0.f };
	//gameObjects.push_back(std::move(cube2));

	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			auto cube = GameObject::createGameObject("cube" + std::to_string(i) + std::to_string(j));
			cube.model = model;
			cube.transform.translation = { .2f + (i * .2f), .2f + (j * .2f), .5f };
			cube.transform.scale = { .2f, .2f, .2f };
			cube.transform.rotation = { 0.f, 0.5f, 0.f };
			gameObjects.push_back(std::move(cube));
		}
	}
}

void sighandler(int s) {
    std::cout << "ctrl c" << std::endl;
}

void runPython() {
    
	std::signal(SIGINT, sighandler);
	std::cout << "Python Terminal: " << std::endl;

	std::string s;
	std::cout << ">> ";
	while (std::getline(std::cin, s)) {
		PyRun_SimpleString(s.c_str());
		std::cout << ">> ";
	}

	Py_Finalize();
}


void Engine::runPythonScript(std::string scriptname, std::string methodname, std::vector<std::string> args = {}) {
	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    pName = PyUnicode_DecodeFSDefault(scriptname.c_str());

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, methodname.c_str());

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(args.size());
            for (i = 0; i < args.size(); ++i) {
                pValue = PyLong_FromLong(atoi(args[i].c_str()));
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
					spdlog::critical("Cannot convert argument {}", args[i]);
                }
                PyTuple_SetItem(pArgs, i, pValue);
            }
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            } else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
				spdlog::critical("Call failed");
            }
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
				spdlog::critical("Cannot find function {}", methodname);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
		spdlog::critical("Failed to load module {}", scriptname);
    }
	PyGILState_Release(gstate);
}


void Engine::initPython() {
	//init python
    Py_Initialize();
	std::thread(runPython).detach();

	//temp example call for python script
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('.')");
	//PyRun_SimpleString("sys.path.append('./scripts')");
	//PyRun_SimpleString("import numpy");
	std::vector<std::string> args;
	args.push_back("1");
	args.push_back("2");
	//runPythonScript("multiply", "sub", args);
	runPythonScript("startup", "startup");
}

void Engine::render() {
	glfwPollEvents();
	if (auto commandBuffer = renderer.beginFrame()) {
		renderer.beginSwapChainRenderPass(commandBuffer);
		renderManager.renderGameObjects(commandBuffer, gameObjects, camera);
		renderer.endSwapChainRenderPass(commandBuffer);
		renderer.endFrame();
	}
}

void Engine::update() {
	camera.setProjection(glm::radians(45.f),  renderer.getAspectRatio(), 0.1f, 10.f);
	if (InputManager::keys[GLFW_KEY_W]) {
		camera.moveCamForward(.05f);
	}
	if (InputManager::keys[GLFW_KEY_S]) {
		camera.moveCamBack(.05f);
	}
	if (InputManager::keys[GLFW_KEY_A]) {
		camera.moveCamLeft(.05f);
	}
	if (InputManager::keys[GLFW_KEY_D]) {
		camera.moveCamRight(.05f);
	}
	camera.rotateCamera(InputManager::xoffset, InputManager::yoffset, 0.1f);
	InputManager::xoffset = 0;
	InputManager::yoffset = 0;
}

void Engine::run() {   
	initPython();

	//starting values for camera
	InputManager::xoffset = 0;
	InputManager::yoffset = 0;
	camera.rotateCamera(-90, 0, 1);

	double lastTime = glfwGetTime(), timer = lastTime;
	double deltaTime = 0, nowTime = 0;
	int frames = 0, updates = 0;
	const double delta = 1.0 / 120.0;
    
	while (!window.shouldClose()) {
		//get time
		nowTime = glfwGetTime();
		deltaTime += (nowTime - lastTime) / delta;
		lastTime = nowTime;

		//update at delta
		while (deltaTime >= 1.0) {
			update();
			updates++;
			deltaTime--;
		}

		render();
		frames++;

		//reset and output fps
		if (glfwGetTime() - timer > 1.0) {
			timer++;
			//std::cout << "FPS: " << frames << " Updates:" << updates << std::endl;
			updates = 0, frames = 0;
		}
	}

	vkDeviceWaitIdle(device.device());
}
