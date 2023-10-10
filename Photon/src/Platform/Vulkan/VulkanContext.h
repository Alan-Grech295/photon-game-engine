#pragma once
#include "Photon/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Photon
{
	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init();
		virtual void SwapBuffers();
	private:
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
	private:
		GLFWwindow* m_WindowHandle;
		vk::SurfaceKHR m_Surface;
		vk::Device m_Device;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;
	};
}