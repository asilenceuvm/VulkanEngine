#pragma once

#include <vector>
#include <algorithm>
#include <csignal>
#include <filesystem>

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

#include "spdlog/spdlog.h"

#include "engine.h"
#include "utils.h"
#include "assetManager.h"

namespace PythonManager {
	//python methods to interact with engine
	static PyObject* change_scale(PyObject* self, PyObject* args) {
		char* tag;
		float scaleX, scaleY, scaleZ;
		if (PyArg_ParseTuple(args, "sfff", &tag, &scaleX, &scaleY, &scaleZ)) {
			auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
			auto index = std::distance(Engine::gameObjects.begin(), it);
			if (index < 0 || index >= Engine::gameObjects.size()) {
				spdlog::critical("No such tag exists {}", tag);
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
		if (PyArg_ParseTuple(args, "sfff", &tag, &translationX, &translationY, &translationZ)) {
			auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
			auto index = std::distance(Engine::gameObjects.begin(), it);
			if (index < 0 || index >= Engine::gameObjects.size()) {
				spdlog::critical("No such tag exists {}", tag);
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
		if (PyArg_ParseTuple(args, "sfff", &tag, &rotationX, &rotationY, &rotationZ)) {
			auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == tag; });
			auto index = std::distance(Engine::gameObjects.begin(), it);
			if (index < 0 || index >= Engine::gameObjects.size()) {
				spdlog::critical("No such tag exists {}", tag);
			}
			else {
				Engine::gameObjects[index].transform.rotation = glm::vec3(rotationX, rotationY, rotationZ);
			}
		}

		return PyLong_FromLong(0);
	}

	static PyObject* add_game_object(PyObject* self, PyObject* args) {	
		char* modelName;
		char* objName;
		if (PyArg_ParseTuple(args, "ss", &modelName, &objName)) {
			Engine::addGameObject(AssetManager::models[modelName], objName);
		}
		
		return PyLong_FromLong(0);
	}

	static PyObject* add_model(PyObject* self, PyObject* args) {
		char* modelToAdd;
		char* modelFilepath;
		char* modelTexture;
		if (PyArg_ParseTuple(args, "sss", &modelToAdd, &modelFilepath, & modelTexture)) {
			Engine::modelToAdd = modelToAdd;
			Engine::modelFilepath = modelFilepath;
			Engine::modelTexture = modelTexture;
		}

		return PyLong_FromLong(0);
	}

	static PyObject* change_light_pos(PyObject* self, PyObject* args) {
		float posX, posY, posZ;
		if (PyArg_ParseTuple(args, "fff", &posX, &posY, &posZ)) {
			Engine::lightPos = glm::vec3(posX, posY, posZ);
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

	static PyObject* get_cur_image(PyObject* self, PyObject* args) {
		PyObject* cur_image;
		Engine::takeImage = true;
		while (Engine::takeImage == true) {}
		PyObject* listObj = PyList_New(Engine::curImage.size());
		PyObject* rValues = PyList_New(Engine::curImage[0].size());
		PyObject* gValues = PyList_New(Engine::curImage[1].size());
		PyObject* bValues = PyList_New(Engine::curImage[2].size());
		spdlog::debug("{}", Engine::curImage[0].size());
		//rValues = Py_BuildValue("[items]", Engine::curImage[0]);
		//gValues = Py_BuildValue("[items]", Engine::curImage[1]);
		//bValues = Py_BuildValue("[items]", Engine::curImage[2]);

		for (auto it = Engine::curImage[0].begin(); it != Engine::curImage[0].end(); it++){
			PyList_Append(rValues, Py_BuildValue("c", *it));
		}
		for (auto it = Engine::curImage[1].begin(); it != Engine::curImage[1].end(); it++){
			PyList_Append(gValues, Py_BuildValue("c", *it));
		}
		for (auto it = Engine::curImage[2].begin(); it != Engine::curImage[2].end(); it++){
			PyList_Append(bValues, Py_BuildValue("c", *it));
		}
		/*for (int i = 0; i < Engine::curImage[0].size() / 2; ++i) {
			//spdlog::debug("{}", Engine::curImage[0][i]);
			//spdlog::debug("{}", *Engine::curImage[0][i]);
			//PyObject* val = PyBytes_FromString(Engine::curImage[0][i]);
			//PyObject* val = PyLong_FromUnsignedLong(*Engine::curImage[0][i]);
			?/PyList_SetItem(rValues, i, val);
		}
		for (int i = 0; i < Engine::curImage[1].size() / 2; i++) {
			//PyObject* val2 = PyBytes_FromString(Engine::curImage[1][i]);
			PyObject* val2 = PyLong_FromUnsignedLong(*Engine::curImage[0][i]);
			PyList_SetItem(gValues, i, val2);
		}
		for (int i = 0; i < Engine::curImage[2].size() / 2; i++) {
			//PyObject* val3 = PyBytes_FromString(Engine::curImage[2][i]);
			PyObject* val3 = PyLong_FromUnsignedLong(*Engine::curImage[0][i]);
			PyList_SetItem(bValues, i, val3);
		}*/
		PyList_SetItem(listObj, 0, rValues);
		PyList_SetItem(listObj, 1, gValues);
		PyList_SetItem(listObj, 2, bValues);
		//cur_image = Engine::curImage;
		return listObj;
		//return cur_image;
	}

	static PyObject* make_art(PyObject* self, PyObject* args) {
		Engine::takeImage = true;
		while (Engine::takeImage == true) {}


		char* styleImage;
		if (PyArg_ParseTuple(args, "s", &styleImage)) {
			std::string pystring = std::string("tfu.generate_image(hub_module, tfu.load_style_image('") + std::string(styleImage) + std::string("'))");
			PyRun_SimpleString(pystring.c_str());
		}

		return PyLong_FromLong(0);
	}
	//helper methods python/c++ interaction
	static struct PyMethodDef methods[] = {
		{ "change_scale", change_scale, METH_VARARGS, "test print method"},
		{ "change_translation", change_translation, METH_VARARGS, "test print method"},
		{ "change_rotation", change_rotation, METH_VARARGS, "test print method"},
		{ "add_game_object", add_game_object, METH_VARARGS, "test print method"},
		{ "add_model", add_model, METH_VARARGS, "test print method"},
		{ "get_tags", get_tags, METH_VARARGS, "test print method"},
		{ "get_cur_image", get_cur_image, METH_VARARGS, "test print method"},
		{ "make_art", make_art, METH_VARARGS, "test print method"},
		{ "change_light_pos", change_light_pos, METH_VARARGS, "test print method"},
		{ NULL, NULL, 0, NULL }
	};

	static struct PyModuleDef modDef = {
		PyModuleDef_HEAD_INIT, "engine", NULL, -1, methods,
		NULL, NULL, NULL, NULL
	};

	static PyObject* PyInit_engine(void) {
		return PyModule_Create(&modDef);
	}


	void sighandler(int s) {
		std::cout << "ctrl c" << std::endl;
	}

	static void runPython() {
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


	static void runPythonScript(std::string scriptname, std::string methodname, std::vector<std::string> args = {}) {
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		PyObject* pName, * pModule, * pFunc;
		PyObject* pArgs, * pValue;
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
					//printf("Result of call: %ld\n", PyLong_AsLong(pValue));
					Py_DECREF(pValue);
				}
				else {
					Py_DECREF(pFunc);
					Py_DECREF(pModule);
					PyErr_Print();
					spdlog::critical("Call failed");
				}
			}
			else {
				if (PyErr_Occurred())
					PyErr_Print();
				spdlog::critical("Cannot find function {}", methodname);
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
		}
		else {
			PyErr_Print();
			spdlog::critical("Failed to load module {}", scriptname);
		}
		PyGILState_Release(gstate);
	}



	static void initPython() {
		//init python
		Py_Initialize();
		std::thread(runPython).detach();

		//temp example call for python script
		PyRun_SimpleString("import sys");
		PyRun_SimpleString("sys.path.append('.')");
		PyRun_SimpleString("sys.path.append('./scripts')");
		PyRun_SimpleString("import tensorflow");
		PyRun_SimpleString("import tensorflow_hub as hub");
		PyRun_SimpleString("hub_handle = 'https://tfhub.dev/google/magenta/arbitrary-image-stylization-v1-256/2'");
		PyRun_SimpleString("hub_module = hub.load(hub_handle)");
		PyRun_SimpleString("import tensorflow_utils as tfu");
		//PyRun_SimpleString("sys.path.append('./scripts')");
		//PyRun_SimpleString("import numpy");
		//std::vector<std::string> args;
		//args.push_back("1");
		//args.push_back("2");
		//runPythonScript("multiply", "sub", args);
		runPythonScript("startup", "startup");
	}

	static void runUpdates() {
		//get all python files in scripts directory
		std::vector<std::string> scripts;
		std::string path = "./scripts";
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (Utils::hasEnding(entry.path().u8string(), ".py")) {
				// -3 and +1 are for \\(script) and .py ending
				std::string script = entry.path().u8string().substr(path.size() + 1, entry.path().u8string().size());
				script = script.substr(0, script.size() - 3);
				scripts.push_back(script);
			}
		}

		for (const auto& script : scripts) {
			runPythonScript(script, "update");
		}
	}
};
