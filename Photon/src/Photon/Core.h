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

#ifdef PT_ENABLE_ASSERTS
	#define PT_ASSERT(x, ...) { if(!(x)) { PT_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define PT_CORE_ASSERT(x, ...) { if(!(x)) { PT_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PT_ASSERT(x, ...)
	#define PT_CORE_ASSERT(x, ...)
#endif

#ifdef PT_ENABLE_ASSERTS
	#define PT_VALIDATE(x, ...) { if(!(x)) { PT_ERROR("Validation failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define PT_CORE_VALIDATE(x, ...) { if(!(x)) { PT_CORE_ERROR("Validation failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PT_VALIDATE(x, ...) x
	#define PT_CORE_VALIDATE(x, ...) x
#endif

#define BIT(x) (1 << x)