#pragma once
#include <vulkan/vulkan.hpp>

namespace Photon
{
	struct SwapchainFrame
	{
		vk::Image image;
		vk::ImageView imageView;
		vk::Framebuffer frameBuffer;
		vk::CommandBuffer commandBuffer;
	};
}