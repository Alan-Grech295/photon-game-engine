#pragma once
#include <vulkan/vulkan.hpp>
#include "Platform/Windows/WindowsWindow.h"

namespace Photon
{
	class WindowsWindow;

	vk::CommandPool MakeCommandPool(vk::Device& device, WindowsWindow::QueueFamilyIndices& queueFamilyIndices);

	vk::CommandBuffer MakeCommandBuffer(vk::Device& device, vk::CommandPool& commandPool, std::vector<SwapchainFrame>& frames);
}