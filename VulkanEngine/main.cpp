#include "engine.h"

#include "spdlog/spdlog.h"


int main() {
	//set global levels to debug
	spdlog::set_level(spdlog::level::debug);

	Engine engine{};
	try {
		engine.run();
	}
	catch (const std::exception& e) {
		spdlog::critical("{}", e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}