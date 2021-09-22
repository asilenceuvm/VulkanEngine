#pragma once

#include <memory>
#include <vector>

#include "window.h"
#include "device.h"
#include "gameObject.h"
#include "renderer.h"

class Engine {
public:
	int width = 800;
	int height = 600;
	static std::vector<GameObject> gameObjects;

	Engine();
	~Engine();

	//delete copy constructors
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();
private:
	Window window{width, height, "Vulkan"};
	Device device{ window };
	Renderer renderer{ window, device };


	void loadGameObjects();

	void initPython();
	void runPythonScript(std::string scriptname, std::string methodname, std::vector<std::string> args);
};

