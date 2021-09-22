#include "window.h"

#include <stdexcept>

#include "spdlog/spdlog.h"

#include "inputManager.h"

Window::Window(int width, int height, std::string name) :
	width{ width }, height{ height }, name{ name } {
	initWindow();
}

Window::~Window() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	InputManager::lastX = static_cast<float>(width / 2);
	InputManager::lastY = static_cast<float>(height / 2);
	glfwSetKeyCallback(window, InputManager::key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, InputManager::mouse_callback);
}

bool Window::shouldClose() {
	return glfwWindowShouldClose(window);
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, window, nullptr, surface)) {
		spdlog::critical("Failed to create window surface");
		throw std::runtime_error("createWindowSurface");
	}
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto newwindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	newwindow->framebufferResized = true;
	newwindow->width = width;
	newwindow->height = height;
}
