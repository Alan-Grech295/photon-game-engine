#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanFrame.h"

#include "VulkanContext.h"

namespace Photon
{
	class VulkanCommandUtils
	{
	public:
		static vk::CommandPool MakeCommandPool(vk::Device& device, VulkanContext::QueueFamilyIndices& queueFamilyIndices);

		static vk::CommandBuffer MakeCommandBuffer(vk::Device& device, vk::CommandPool& commandPool, std::vector<SwapchainFrame>& frames);
	};
	
}