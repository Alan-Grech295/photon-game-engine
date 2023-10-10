#include "ptpch.h"
#include "VulkanAPI.h"

#include <set>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Photon
{
	bool VulkanAPI::s_Initialized = false;

	vk::Instance VulkanAPI::s_VulkanInstance = { nullptr };
	std::vector<const char*> VulkanAPI::s_DeviceExtensions = std::vector<const char*>();
	std::vector<const char*> VulkanAPI::s_Layers = std::vector<const char*>();

	// Debugging
	vk::DebugUtilsMessengerEXT VulkanAPI::s_DebugMessenger = { nullptr };

	vk::PhysicalDevice VulkanAPI::s_PhysicalDevice = { nullptr };

	// Helper functions
	// Checks whether the extensions are supported by the Vulkan instance
	static bool Supported(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
	{
		// Supported Extensions
		std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
#ifdef PT_DEBUG
		PT_CORE_INFO("Supported Extensions:");
		for (auto& supportedExt : supportedExtensions)
		{
			PT_CORE_INFO("\t{0}", supportedExt.extensionName);
		}
#endif

		// Finding the extension in the extension list
		bool found;
		for (const char* ext : extensions)
		{
			found = false;
			for (auto& supportedExt : supportedExtensions)
			{
				if (strcmp(ext, supportedExt.extensionName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		// Supported layers
		std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();
#ifdef PT_DEBUG
		PT_CORE_INFO("Supported Layers:");
		for (auto& supportedLayer : supportedLayers)
		{
			PT_CORE_INFO("\t{0}", supportedLayer.layerName);
		}
#endif

		// Finding the extension in the extension list
		for (const char* layer : layers)
		{
			found = false;
			for (auto& supportedLayer : supportedLayers)
			{
				if (strcmp(layer, supportedLayer.layerName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}

	static bool CheckDeviceExtensionSupport(const vk::PhysicalDevice& device, const std::vector<const char*>& requestedExtensions)
	{
		std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
		std::set<std::string> extensionsSet;
		for (auto& ext : extensions)
			extensionsSet.insert(ext.extensionName);

		for (auto& reqExt : requestedExtensions)
		{
			if (!extensionsSet.contains(reqExt))
				return false;
		}

		return true;
	}

	static bool IsSuitable(const vk::PhysicalDevice& device, const std::vector<const char*>& requestedExtensions)
	{
		return CheckDeviceExtensionSupport(device, requestedExtensions);
	}

	void VulkanAPI::Init()
	{
		if (s_Initialized)
			return;

		// Finds the instance version supported by the implementation
		uint32_t version;
		vkEnumerateInstanceVersion(&version);

		// Outputs version info
		PT_CORE_INFO("System supports Vulkan Variant: {0}, {1}.{2}.{3}",
			VK_API_VERSION_VARIANT(version), VK_API_VERSION_MAJOR(version),
			VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));

		vk::ApplicationInfo appInfo = vk::ApplicationInfo(
			nullptr,
			version,
			nullptr,
			version,
			version
		);

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef PT_DEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#ifdef PT_DEBUG
		s_Layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

		PT_CORE_ASSERT(Supported(extensions, s_Layers), "Extensions not supported");

		vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			(uint32_t)s_Layers.size(), s_Layers.data(), // enabled layers
			(uint32_t)extensions.size(), extensions.data() // enabled extensions
		);

		try
		{
			s_VulkanInstance = vk::createInstance(instanceCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create Vulkan instance ({0})", e.what());
		}

		vk::DispatchLoaderDynamic dldi(s_VulkanInstance, vkGetInstanceProcAddr);

		// Create Debug Messenger
		vk::DebugUtilsMessengerCreateInfoEXT dmCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
			debugCallback,
			nullptr
		);

		s_DebugMessenger = s_VulkanInstance.createDebugUtilsMessengerEXT(dmCreateInfo, nullptr, dldi);

		// Set physical device
		s_DeviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		std::vector<vk::PhysicalDevice> availableDevices = s_VulkanInstance.enumeratePhysicalDevices();

		PT_CORE_INFO("Devices:");
		for (auto& device : availableDevices)
		{
			auto properties = device.getProperties();
			PT_CORE_INFO("\tDevice Name: {0}", properties.deviceName);

			if (IsSuitable(device, s_DeviceExtensions)) {
				s_PhysicalDevice = device;
				PT_CORE_INFO("\tSelected Physical Device: {0}", s_PhysicalDevice.getProperties().deviceName);
				break;
			}
		}

		s_Initialized = true;

	}

	vk::SurfaceKHR VulkanAPI::CreateSurface()
	{
		return vk::SurfaceKHR();
	}

	void VulkanAPI::Shutdown()
	{
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanAPI::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::string type = vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageType));

		switch (messageSeverity)
		{
		case (int)vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			PT_CORE_ERROR("VK Validation Layer ({0}): {1}", type, pCallbackData->pMessage);
			break;
		case (int)vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			PT_CORE_WARN("VK Validation Layer ({0}): {1}", type, pCallbackData->pMessage);
			break;
		case (int)vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			PT_CORE_INFO("VK Validation Layer ({0}): {1}", type, pCallbackData->pMessage);
			break;
		default:
			PT_CORE_TRACE("VK Validation Layer ({0}): {1}", type, pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
}

