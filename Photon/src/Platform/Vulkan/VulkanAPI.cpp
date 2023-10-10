#include "ptpch.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Photon
{
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

		std::vector<const char*> layers;

#ifdef PT_DEBUG
		layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

		PT_CORE_ASSERT(Supported(extensions, layers), "Extensions not supported");

		vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			(uint32_t)layers.size(), layers.data(), // enabled layers
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

		m_DebugMessenger = s_VulkanInstance.createDebugUtilsMessengerEXT(dmCreateInfo, nullptr, dldi);

		// Set physical device
		std::vector<vk::PhysicalDevice> availableDevices = s_VulkanInstance.enumeratePhysicalDevices();

		PT_CORE_INFO("Devices:");
		for (auto& device : availableDevices)
		{
			auto properties = device.getProperties();
			PT_CORE_INFO("\tDevice Name: {0}", properties.deviceName);

			if (IsSuitable(device)) {
				m_PhysicalDevice = device;
				PT_CORE_INFO("\tSelected Physical Device: {0}", m_PhysicalDevice.getProperties().deviceName);
				break;
			}
		}

		s_Initialized = true;

	}
}

