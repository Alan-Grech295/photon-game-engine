#pragma once
#include <memory>

#include "Core.h"

#include "spdlog/spdlog.h"

namespace Photon
{
	class PHOTON_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#define PT_CORE_TRACE(...)   ::Photon::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PT_CORE_INFO(...)    ::Photon::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PT_CORE_WARN(...)    ::Photon::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PT_CORE_ERROR(...)   ::Photon::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PT_CORE_FATAL(...)   ::Photon::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define PT_TRACE(...)   ::Photon::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PT_INFO(...)    ::Photon::Log::GetClientLogger()->info(__VA_ARGS__)
#define PT_WARN(...)    ::Photon::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PT_ERROR(...)   ::Photon::Log::GetClientLogger()->error(__VA_ARGS__)
#define PT_FATAL(...)   ::Photon::Log::GetClientLogger()->fatal(__VA_ARGS__)


