#include "inputManager.h"

#include <cmath>
#include <iostream>

bool InputManager::keys[350];

void InputManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	keys[key] = action;
}

double InputManager::lastX = 0;
double InputManager::lastY = 0;
double InputManager::xoffset = 0;
double InputManager::yoffset = 0;
bool InputManager::firstMouse = true;

void InputManager::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    xoffset = xpos - lastX;
    yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
}
