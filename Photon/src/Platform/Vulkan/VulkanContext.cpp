#include "ptpch.h"
#include "VulkanContext.h"

#include "VulkanAPI.h"

#include <GLFW/glfw3.h>

namespace Photon
{
	// Helper functions
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


	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void VulkanContext::Init()
	{
		VulkanAPI::Init();
		glfwMakeContextCurrent(m_WindowHandle);

		// Vulkan
		// Creating the window surface
		VkSurfaceKHR cSurface;
		PT_CORE_VALIDATE(glfwCreateWindowSurface(VulkanAPI::GetInstance(), m_WindowHandle, nullptr, &cSurface) == VK_SUCCESS, "Could not create a Vulkan surface");
		m_Surface = cSurface;

		// Getting queue families
		QueueFamilyIndices indices;
		vk::PhysicalDevice& physicalDevice = VulkanAPI::GetPhysicalDevice();
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

		int i = 0;
		for (auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
			}

			if (physicalDevice.getSurfaceSupportKHR(i, m_Surface))
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

		auto& layers = VulkanAPI::GetLayers();

		vk::DeviceCreateInfo logicalDeviceCreateInfo = vk::DeviceCreateInfo(
			vk::DeviceCreateFlags(),
			(uint32_t)queueCreateInfos.size(), queueCreateInfos.data(),
			(uint32_t)layers.size(), layers.data(),
			(uint32_t)deviceExtensions.size(), deviceExtensions.data(),
			&deviceFeatures
		);

		try
		{
			m_Device = physicalDevice.createDevice(logicalDeviceCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create the logical device ({0})", e.what());
		}

		// Getting graphics queue from device
		m_GraphicsQueue = m_Device.getQueue(indices.graphicsFamily.value(), 0);
		m_PresentQueue = m_Device.getQueue(indices.presentFamily.value(), 0);

		// Getting swapchain support details
		SwapchainSupportDetails support;

		support.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		PT_CORE_INFO("System supports {} up to {} images", support.capabilities.minImageCount, support.capabilities.maxImageCount);

		support.formats = physicalDevice.getSurfaceFormatsKHR(m_Surface);
		support.presentModes = physicalDevice.getSurfacePresentModesKHR(m_Surface);

		// Creating the swapchain
		vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.formats);
		vk::PresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
		vk::Extent2D extent = ChooseSwapchainExtent(m_WindowHandle->, m_Data.Height, support.capabilities);

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
		colorBlendAttachment.blendEnable = VK_FALSE;
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
		m_RenderPass = MakeRenderPass(m_Device, m_SwapchainBundle.format);
		pipelineInfo.renderPass = m_RenderPass;

		// Extra
		pipelineInfo.basePipelineHandle = nullptr;

		// Make the pipeline
		try
		{
			m_Pipeline = (m_Device.createGraphicsPipeline(nullptr, pipelineInfo)).value;
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create graphics pipeline ({0})", e.what());
		}

		m_Device.destroyShaderModule(vertShaderModule);
		m_Device.destroyShaderModule(fragShaderModule);

		MakeFrameBuffers(m_Device, m_RenderPass, m_SwapchainBundle.extent, m_SwapchainBundle.frames);

		m_CommandPool = MakeCommandPool(m_Device, indices);
		m_MainCommandBuffer = MakeCommandBuffer(m_Device, m_CommandPool, m_SwapchainBundle.frames);

		m_InFlightFence = MakeFence(m_Device);
		m_ImageAvailable = MakeSemaphore(m_Device);
		m_RenderFinished = MakeSemaphore(m_Device);
	}

	void VulkanContext::SwapBuffers()
	{
	}
}

