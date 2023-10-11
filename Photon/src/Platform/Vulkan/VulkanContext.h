#pragma once
#include "Photon/Renderer/GraphicsContext.h"

#include "VulkanFrame.h"

struct GLFWwindow;

namespace Photon
{
	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init() override;
		virtual void SwapBuffers() override;
	public:
		vk::DescriptorPool CreateDescriptorPool();

		vk::CommandBuffer CreateSingleTimeCommandBuffer();
		void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);
	private:
		void RecordDrawCommands(vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
	public:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool IsComplete()
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapchainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};
	public:
		GLFWwindow* m_WindowHandle;
		vk::SurfaceKHR m_Surface;
		vk::Device m_Device;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		vk::SwapchainKHR m_Swapchain;
		std::vector<SwapchainFrame> m_Frames;
		vk::Format m_Format;
		vk::Extent2D m_Extent;
		SwapchainSupportDetails m_Support;
		uint32_t m_ImageCount;

		QueueFamilyIndices m_QueueFamilyIndices;

		vk::RenderPass m_RenderPass;
		vk::Pipeline m_Pipeline;

		vk::CommandPool m_CommandPool;
		vk::CommandBuffer m_MainCommandBuffer;

		vk::CommandBuffer* m_CurrentCommandBuffer = nullptr;

		vk::Fence m_InFlightFence;
		vk::Semaphore m_ImageAvailable, m_RenderFinished;
	};
}