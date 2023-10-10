#pragma once

#include <vulkan/vulkan.hpp>

namespace Photon
{
	struct SwapchainFrame
	{
		vk::Image Image;
		vk::ImageView ImageView;
		vk::Framebuffer FrameBuffer;
		vk::CommandBuffer CommandBuffer;
	};
}