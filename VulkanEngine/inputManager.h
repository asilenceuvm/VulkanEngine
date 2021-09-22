#pragma once

#include <GLFW/glfw3.h>

class InputManager {
private:
	static bool firstMouse;
public:
	static bool keys[350];
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static double lastX, lastY;
	static double xoffset, yoffset;
	
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
};