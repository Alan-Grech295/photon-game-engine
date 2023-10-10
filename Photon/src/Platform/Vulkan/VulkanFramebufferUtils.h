#pragma once
#include "VulkanFrame.h"
#include "ptpch.h"

namespace Photon
{
	class VulkanFramebufferUtils
	{
	public:
		static void MakeFrameBuffers(vk::Device device, vk::RenderPass renderPass, vk::Extent2D extent, std::vector<SwapchainFrame>& frames);
	};
}