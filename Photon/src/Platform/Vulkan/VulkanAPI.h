#pragma once

#include <vulkan/vulkan.hpp>

namespace Photon
{
	class VulkanAPI
	{
	public:
		static void Init();

	private:
		static bool s_Initialized;

		static vk::Instance s_VulkanInstance;
		static std::vector<const char*> s_SupportedExtensions;
	};
}