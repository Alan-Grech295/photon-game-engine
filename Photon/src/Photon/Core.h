#pragma once

#ifdef PT_PLATFORM_WINDOWS
	#ifdef PT_BUILD_DLL
		#define PHOTON_API __declspec(dllexport)
	#else
		#define PHOTON_API __declspec(dllimport)
	#endif
#else
	#error Photon only supports Windows!
#endif

#define BIT(x) (1 << x)