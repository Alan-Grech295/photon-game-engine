#pragma once

#include "Photon/Window.h"

//#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "Platform/Vulkan/Tutorial/Frame.h"
#include "Photon/Renderer/GraphicsContext.h"

namespace Photon
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		// Window attributes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		void InitVulkan();
	private:
		GLFWwindow* m_Window;
		GraphicsContext* m_Context;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;

		// VULKAN VARS
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

		void Render();
	private:	
		void RecordDrawCommands(vk::CommandBuffer& commandBuffer, uint32_t imageIndex);

		struct SwapchainBundle
		{
			vk::SwapchainKHR swapchain;
			std::vector<SwapchainFrame> frames;
			vk::Format format;
			vk::Extent2D extent;
		} m_SwapchainBundle;

		// TODO: Move to abstract renderer
		vk::Instance m_VulkanInstance;
		vk::DebugUtilsMessengerEXT m_DebugMessenger;
		vk::SurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;
		vk::Device m_Device;
		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		vk::RenderPass m_RenderPass;
		vk::Pipeline m_Pipeline;

		vk::CommandPool m_CommandPool;
		vk::CommandBuffer m_MainCommandBuffer;

		vk::Fence m_InFlightFence;
		vk::Semaphore m_ImageAvailable, m_RenderFinished;

		// END VULKAN DATA

	};
}