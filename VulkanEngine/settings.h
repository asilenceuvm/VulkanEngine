#pragma once

namespace Settings {
	#ifdef NDEBUG
		static const bool debugMode = false;
	#else
		static const bool debugMode = true;
	#endif


	static inline int width = 800;
	static inline int height = 600;
}
