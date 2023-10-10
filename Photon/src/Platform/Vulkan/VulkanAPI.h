#pragma once

#include <vulkan/vulkan.hpp>

namespace Photon
{
	class VulkanAPI
	{
	public:
		static void Init();

		static vk::SurfaceKHR CreateSurface();

		inline static vk::Instance& GetInstance() { return s_VulkanInstance; }
		inline static vk::PhysicalDevice& GetPhysicalDevice() { return s_PhysicalDevice; }
		inline static const std::vector<const char*>& GetDeviceExtensions() { return s_DeviceExtensions; }
		inline static const std::vector<const char*>& GetLayers() { return s_Layers; }

		static void Shutdown();
	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);
	public:

	private:
		static bool s_Initialized;

		static vk::Instance s_VulkanInstance;
		static std::vector<const char*> s_DeviceExtensions;
		static std::vector<const char*> s_Layers;

		// Debugging
		static vk::DebugUtilsMessengerEXT s_DebugMessenger;

		static vk::PhysicalDevice s_PhysicalDevice;

	};
}