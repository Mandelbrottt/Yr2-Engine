#pragma once

#if defined (OYL_PLATFORM_WINDOWS)

#else
#	error Oyl3D only supports Windows!
#endif

#define BIND_CALLBACK_1(x) std::bind(&x, this, std::placeholders::_1)
#define BIND_CALLBACK(x) BIND_CALLBACK_1(x)

#if defined(OYL_DEBUG)
#	define ASSERT(x, ...) { if(!x) { LOG_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
#	define VERIFY(x, ...) { if(!x) { LOG_ERROR("Verification failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#	define ASSERT(x) 
#	define VERIFY(x) x
#endif