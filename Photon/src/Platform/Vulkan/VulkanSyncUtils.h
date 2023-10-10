#pragma once
#include <vulkan/vulkan.hpp>

namespace Photon
{
	class VulkanSyncUtils
	{
	public:
		static vk::Semaphore MakeSemaphore(vk::Device device);

		static vk::Fence MakeFence(vk::Device device);
	};
	
}