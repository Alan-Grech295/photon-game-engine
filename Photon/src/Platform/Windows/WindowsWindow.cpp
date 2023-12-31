#include "ptpch.h"
#include "WindowsWindow.h"

#include "Photon/Events/ApplicationEvent.h"
#include "Photon/Events/KeyEvent.h"
#include "Photon/Events/MouseEvent.h"

#include <set>

namespace Photon
{
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		PT_CORE_ERROR("GLFW Error {0}: {1}", error, description);
	}

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		PT_CORE_INFO("Creating Window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			PT_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			// Resizing breaks the swapchain so it is disabled for now
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			glfwSetErrorCallback(GLFWErrorCallback);

			// Creating the Vulkan instance
			InitVulkan();

			s_GLFWInitialized = true;
		}

		m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Data);

		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scanCode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				
			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) 
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}

	/* START VULKAN FUNCTIONS */

	static bool supported(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
	{
		// Supported Extensions
		std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
#ifdef PT_DEBUG
		PT_CORE_INFO("Supported Extensions:");
		for(auto& supportedExt : supportedExtensions)
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

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

	static bool IsSuitable(const vk::PhysicalDevice& device)
	{
		static const std::vector<const char*> requestedExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		return CheckDeviceExtensionSupport(device, requestedExtensions);
	}

	void WindowsWindow::InitVulkan()
	{
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

		PT_CORE_ASSERT(supported(extensions, layers), "Extensions not supported");

		vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			(uint32_t)layers.size(), layers.data(), // enabled layers
			(uint32_t)extensions.size(), extensions.data() // enabled extensions
		);

		try
		{
			m_VulkanInstance = vk::createInstance(instanceCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create Vulkan instance ({0})", e.what());
		}

		vk::DispatchLoaderDynamic dldi(m_VulkanInstance, vkGetInstanceProcAddr);
		
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

		m_DebugMessenger = m_VulkanInstance.createDebugUtilsMessengerEXT(dmCreateInfo, nullptr, dldi);

		// Set physical device
		std::vector<vk::PhysicalDevice> availableDevices = m_VulkanInstance.enumeratePhysicalDevices();

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

		// Getting queue families
		QueueFamilyIndices indices;
		std::vector<vk::QueueFamilyProperties> queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();

		int i = 0;
		for (auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
				indices.presentFamily = i;
			}

			if (indices.IsComplete())
				break;

			i++;
		}

		PT_CORE_ASSERT(indices.IsComplete(), "Not all queue families have been found");

		// Setting logical device
		float queuePriority = 1.0f;

		vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			indices.graphicsFamily.value(),
			1, &queuePriority
		);

		vk::PhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = true;

		vk::DeviceCreateInfo logicalDeviceCreateInfo = vk::DeviceCreateInfo(
			vk::DeviceCreateFlags(),
			1, &queueCreateInfo,
			(uint32_t)layers.size(), layers.data(),
			0, nullptr,
			&deviceFeatures
		);

		try
		{
			m_Device = m_PhysicalDevice.createDevice(logicalDeviceCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create the logical device ({0})", e.what());
		}

		// Getting graphics queue from device
		m_GraphicsQueue = m_Device.getQueue(indices.graphicsFamily.value(), 0);
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}
}

