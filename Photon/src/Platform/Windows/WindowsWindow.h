#pragma once

#include "Photon/Window.h"

//#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

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

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		struct QueueFamilyIndices
		{
		public:
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool IsComplete()
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		// TODO: Move to abstract renderer
		vk::Instance m_VulkanInstance;
		vk::DebugUtilsMessengerEXT m_DebugMessenger;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::Device m_Device;
		vk::Queue m_GraphicsQueue;

		WindowData m_Data;
	};
}