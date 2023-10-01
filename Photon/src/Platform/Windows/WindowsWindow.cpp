#include "ptpch.h"
#include "WindowsWindow.h"

#include "Photon/Events/ApplicationEvent.h"
#include "Photon/Events/KeyEvent.h"
#include "Photon/Events/MouseEvent.h"

#include <set>
#include <filesystem>

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

		// Creating the Vulkan instance
		InitVulkan();
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

	static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
	{
		vk::SurfaceFormatKHR surfaceFormat;
		for (vk::SurfaceFormatKHR format : formats)
		{
			if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return format;
			}
		}

		return formats[0];
	}

	static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
	{
		for (const vk::PresentModeKHR& presentMode : presentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
				return presentMode;
		}

		return vk::PresentModeKHR::eFifo;
	}

	static vk::Extent2D ChooseSwapchainExtent(uint32_t width, uint32_t height, vk::SurfaceCapabilitiesKHR capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		
		return { std::min(capabilities.maxImageExtent.width, std::max(capabilities.minImageExtent.width, width)),
				 std::min(capabilities.maxImageExtent.height, std::max(capabilities.minImageExtent.height, height)) };
	}

	static std::vector<char> ReadFile(const std::filesystem::path& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			PT_CORE_ERROR("Failed to open file {0}", filename.string());
			return {};
		}

		size_t filesize = (size_t)file.tellg();

		std::vector<char> buffer(filesize);

		file.seekg(0);
		file.read(buffer.data(), filesize);

		file.close();

		return buffer;
	}

	static vk::ShaderModule CreateShaderModule(vk::Device& device, const std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		vk::ShaderModule shaderModule;

		try
		{
			shaderModule = device.createShaderModule(createInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create shader module ({0})", e.what());
		}

		return shaderModule;
	}

	static vk::RenderPass MakeRenderPass(vk::Device device, vk::Format swapchainImageFormat)
	{
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.flags = vk::AttachmentDescriptionFlags();
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.flags = vk::SubpassDescriptionFlags();
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		vk::RenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.flags = vk::RenderPassCreateFlags();
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		try
		{
			return device.createRenderPass(renderPassInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create render pass ({0})", e.what());
		}
	}

	vk::PipelineLayout MakePipelineLayout(vk::Device device)
	{
		vk::PipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.flags = vk::PipelineLayoutCreateFlags();
		layoutInfo.setLayoutCount = 0;
		layoutInfo.pushConstantRangeCount = 0;

		try
		{
			return device.createPipelineLayout(layoutInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create pipeline layout ({0})", e.what());
		}
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


		// Creating the window surface
		VkSurfaceKHR cSurface;
		PT_CORE_VALIDATE(glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, &cSurface) == VK_SUCCESS, "Could not create a Vulkan surface");
		m_Surface = cSurface;

		// Getting queue families
		QueueFamilyIndices indices;
		std::vector<vk::QueueFamilyProperties> queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();

		int i = 0;
		for (auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
			}

			if (m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface))
			{
				indices.presentFamily = i;
			}

			if (indices.IsComplete())
				break;

			i++;
		}

		PT_CORE_ASSERT(indices.IsComplete(), "Not all queue families have been found");

		// Setting logical device
		//Getting unique indices
		std::vector<uint32_t> uniqueIndices;
		uniqueIndices.push_back(indices.graphicsFamily.value());
		if (indices.graphicsFamily.value() != indices.presentFamily.value())
			uniqueIndices.push_back(indices.presentFamily.value());

		float queuePriority = 1.0f;

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t queueFamilyIndex : uniqueIndices)
		{
			queueCreateInfos.emplace_back(
				vk::DeviceQueueCreateFlags(),
				indices.graphicsFamily.value(),
				1, &queuePriority);
		}

		// Device must support swapchain
		std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::PhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = true;

		vk::DeviceCreateInfo logicalDeviceCreateInfo = vk::DeviceCreateInfo(
			vk::DeviceCreateFlags(),
			(uint32_t)queueCreateInfos.size(), queueCreateInfos.data(),
			(uint32_t)layers.size(), layers.data(),
			(uint32_t)deviceExtensions.size(), deviceExtensions.data(),
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
		m_PresentQueue = m_Device.getQueue(indices.presentFamily.value(), 0);

		// Getting swapchain support details
		WindowsWindow::SwapchainSupportDetails support;

		support.capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		PT_CORE_INFO("System supports {} up to {} images", support.capabilities.minImageCount, support.capabilities.maxImageCount);

		support.formats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
		support.presentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);

		// Creating the swapchain
		vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.formats);
		vk::PresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
		vk::Extent2D extent = ChooseSwapchainExtent(m_Data.Width, m_Data.Height, support.capabilities);

		// Increases the image count by 1 to increase framerate
		uint32_t imageCount = std::min(
			support.capabilities.maxImageCount == 0 ? UINT32_MAX : support.capabilities.maxImageCount, // Max Image Count can be 0 when there is no maximum
			support.capabilities.minImageCount + 1
		);

		// Creating the swapchain
		vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			m_Surface,
			imageCount,
			surfaceFormat.format,
			surfaceFormat.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment
		);

		if (indices.graphicsFamily.value() != indices.presentFamily.value())
		{
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		swapchainCreateInfo.preTransform = support.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;

		swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

		try
		{
			m_SwapchainBundle.swapchain = m_Device.createSwapchainKHR(swapchainCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create swapchain ({0})", e.what());
		}

		auto images = m_Device.getSwapchainImagesKHR(m_SwapchainBundle.swapchain);
		m_SwapchainBundle.frames.reserve(images.size());

		for (vk::Image& img : images)
		{
			// Creating image views 
			vk::ImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.image = img;

			viewCreateInfo.viewType = vk::ImageViewType::e2D;
			viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
			viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
			viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
			viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;

			viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;

			viewCreateInfo.format = surfaceFormat.format;

			m_SwapchainBundle.frames.emplace_back(img, m_Device.createImageView(viewCreateInfo));
		}

		m_SwapchainBundle.format = surfaceFormat.format;
		m_SwapchainBundle.extent = extent;

		// Loading example vertex and fragment shader
		// Since the app is being launched from Sandbox the path starts from Sandbox
		auto vertShaderCode = ReadFile("../Photon/src/Photon/Shaders/Vertex.spv");
		auto fragShaderCode = ReadFile("../Photon/src/Photon/Shaders/Fragment.spv");

		vk::ShaderModule vertShaderModule = CreateShaderModule(m_Device, vertShaderCode);
		vk::ShaderModule fragShaderModule = CreateShaderModule(m_Device, fragShaderCode);

		// Creating shader pipeline
		vk::PipelineShaderStageCreateInfo vertShaderPipelineInfo = {};
		vertShaderPipelineInfo.flags = vk::PipelineShaderStageCreateFlags();
		vertShaderPipelineInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertShaderPipelineInfo.module = vertShaderModule;
		vertShaderPipelineInfo.pName = "main";

		// Shader modules were destroyed (should they be?)

		vk::PipelineShaderStageCreateInfo fragShaderPipelineInfo = {};
		fragShaderPipelineInfo.flags = vk::PipelineShaderStageCreateFlags();
		fragShaderPipelineInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragShaderPipelineInfo.module = fragShaderModule;
		fragShaderPipelineInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertShaderPipelineInfo, fragShaderPipelineInfo };

		// Creating the pipeline
		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.flags = vk::PipelineCreateFlags();

		// Vertex Input
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.flags = vk::PipelineVertexInputStateCreateFlags();
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		pipelineInfo.pVertexInputState = &vertexInputInfo;

		// Input Assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
		inputAssemblyInfo.flags = vk::PipelineInputAssemblyStateCreateFlags();
		inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;

		// Viewport and scissor
		vk::Viewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = m_SwapchainBundle.extent.width;
		viewport.height = m_SwapchainBundle.extent.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		vk::Rect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent = m_SwapchainBundle.extent;

		vk::PipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.flags = vk::PipelineViewportStateCreateFlags();
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;
		pipelineInfo.pViewportState = &viewportInfo;

		// Rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.flags = vk::PipelineRasterizationStateCreateFlags();
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = vk::PolygonMode::eFill;
		rasterizer.lineWidth = 1;
		rasterizer.cullMode = vk::CullModeFlagBits::eBack;
		rasterizer.frontFace = vk::FrontFace::eClockwise;
		rasterizer.depthBiasEnable = VK_FALSE;
		pipelineInfo.pRasterizationState = &rasterizer;

		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();

		// Multi-sampling
		vk::PipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.flags = vk::PipelineMultisampleStateCreateFlags();
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
		pipelineInfo.pMultisampleState = &multisampling;

		// Color blend
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = VK_TRUE;
		vk::PipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.flags = vk::PipelineColorBlendStateCreateFlags();
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = vk::LogicOp::eCopy;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
		pipelineInfo.pColorBlendState = &colorBlending;

		// Pipeline layout
		vk::PipelineLayout pipelineLayout = MakePipelineLayout(m_Device);
		pipelineInfo.layout = pipelineLayout;

		// Render Pass
		vk::RenderPass renderPass = MakeRenderPass(m_Device, m_SwapchainBundle.format);
		pipelineInfo.renderPass = renderPass;

		// Extra
		pipelineInfo.basePipelineHandle = nullptr;

		// Make the pipeline
		vk::Pipeline graphicsPipeline;

		try
		{
			graphicsPipeline = (m_Device.createGraphicsPipeline(nullptr, pipelineInfo)).value;
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create graphics pipeline ({0})", e.what());
		}

		m_Device.destroyShaderModule(vertShaderModule);
		m_Device.destroyShaderModule(fragShaderModule);
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

