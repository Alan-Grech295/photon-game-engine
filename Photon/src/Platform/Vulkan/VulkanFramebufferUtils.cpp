#include "ptpch.h"
#include "VulkanFramebufferUtils.h"

namespace Photon
{
	void VulkanFramebufferUtils::MakeFrameBuffers(vk::Device device, vk::RenderPass renderPass, vk::Extent2D extent, std::vector<SwapchainFrame>& frames)
	{
		for (auto& frame : frames)
		{
			std::vector<vk::ImageView> attachments = {
				frame.ImageView
			};

			vk::FramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.flags = vk::FramebufferCreateFlags();
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			try
			{
				frame.FrameBuffer = device.createFramebuffer(framebufferInfo);
			}
			catch (vk::SystemError e)
			{
				PT_CORE_ASSERT(false, "Could not create frame buffer ({0})", e.what());
			}
		}
	}
}
