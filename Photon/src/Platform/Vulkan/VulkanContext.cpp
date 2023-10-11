#include "ptpch.h"
#include "VulkanContext.h"

#include "VulkanAPI.h"

#include "VulkanCommandUtils.h"
#include "VulkanFramebufferUtils.h"
#include "VulkanSyncUtils.h"
#include "Photon/Events/ApplicationEvent.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>

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


	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void VulkanContext::Init()
	{
		VulkanAPI::Init();

		int width, height;
		glfwGetWindowSize(m_WindowHandle, &width, &height);

		// Vulkan
		// Creating the window surface
		VkSurfaceKHR cSurface;
		PT_CORE_VALIDATE(glfwCreateWindowSurface(VulkanAPI::GetInstance(), m_WindowHandle, nullptr, &cSurface) == VK_SUCCESS, "Could not create a Vulkan surface");
		m_Surface = cSurface;

		// Getting queue families
		vk::PhysicalDevice& physicalDevice = VulkanAPI::GetPhysicalDevice();
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

		int i = 0;
		for (auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				m_QueueFamilyIndices.graphicsFamily = i;
			}

			if (physicalDevice.getSurfaceSupportKHR(i, m_Surface))
			{
				m_QueueFamilyIndices.presentFamily = i;
			}

			if (m_QueueFamilyIndices.IsComplete())
				break;

			i++;
		}

		PT_CORE_ASSERT(m_QueueFamilyIndices.IsComplete(), "Not all queue families have been found");

		// Setting logical device
		//Getting unique indices
		std::vector<uint32_t> uniqueIndices;
		uniqueIndices.push_back(m_QueueFamilyIndices.graphicsFamily.value());
		if (m_QueueFamilyIndices.graphicsFamily.value() != m_QueueFamilyIndices.presentFamily.value())
			uniqueIndices.push_back(m_QueueFamilyIndices.presentFamily.value());

		float queuePriority = 1.0f;

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t queueFamilyIndex : uniqueIndices)
		{
			queueCreateInfos.emplace_back(
				vk::DeviceQueueCreateFlags(),
				m_QueueFamilyIndices.graphicsFamily.value(),
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
		m_GraphicsQueue = m_Device.getQueue(m_QueueFamilyIndices.graphicsFamily.value(), 0);
		m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndices.presentFamily.value(), 0);

		// Getting swapchain support details
		m_Support.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		PT_CORE_INFO("System supports {} up to {} images", m_Support.capabilities.minImageCount, m_Support.capabilities.maxImageCount);

		m_Support.formats = physicalDevice.getSurfaceFormatsKHR(m_Surface);
		m_Support.presentModes = physicalDevice.getSurfacePresentModesKHR(m_Surface);

		// Creating the swapchain
		vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(m_Support.formats);
		vk::PresentModeKHR presentMode = ChoosePresentMode(m_Support.presentModes);
		vk::Extent2D extent = ChooseSwapchainExtent(width, height, m_Support.capabilities);

		// Increases the image count by 1 to increase framerate
		m_ImageCount = std::min(
			m_Support.capabilities.maxImageCount == 0 ? UINT32_MAX : m_Support.capabilities.maxImageCount, // Max Image Count can be 0 when there is no maximum
			m_Support.capabilities.minImageCount + 1
		);

		// Creating the swapchain
		vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			m_Surface,
			m_ImageCount,
			surfaceFormat.format,
			surfaceFormat.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment
		);

		if (m_QueueFamilyIndices.graphicsFamily.value() != m_QueueFamilyIndices.presentFamily.value())
		{
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value() };
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		swapchainCreateInfo.preTransform = m_Support.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;

		swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

		try
		{
			m_Swapchain = m_Device.createSwapchainKHR(swapchainCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create swapchain ({0})", e.what());
		}

		auto images = m_Device.getSwapchainImagesKHR(m_Swapchain);
		m_Frames.reserve(images.size());

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

			m_Frames.emplace_back(img, m_Device.createImageView(viewCreateInfo));
		}

		m_Format = surfaceFormat.format;
		m_Extent = extent;

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
		viewport.width = m_Extent.width;
		viewport.height = m_Extent.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		vk::Rect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent = m_Extent;

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
		m_RenderPass = MakeRenderPass(m_Device, m_Format);
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

		VulkanFramebufferUtils::MakeFrameBuffers(m_Device, m_RenderPass, m_Extent, m_Frames);

		m_CommandPool = VulkanCommandUtils::MakeCommandPool(m_Device, m_QueueFamilyIndices);
		m_MainCommandBuffer = VulkanCommandUtils::MakeCommandBuffer(m_Device, m_CommandPool, m_Frames);

		m_InFlightFence = VulkanSyncUtils::MakeFence(m_Device);
		m_ImageAvailable = VulkanSyncUtils::MakeSemaphore(m_Device);
		m_RenderFinished = VulkanSyncUtils::MakeSemaphore(m_Device);
	}

	void VulkanContext::SwapBuffers()
	{
		m_Device.waitForFences(1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
		m_Device.resetFences(1, &m_InFlightFence);

		uint32_t imageIndex = m_Device.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_ImageAvailable, nullptr).value;

		vk::CommandBuffer& commandBuffer = m_Frames[imageIndex].CommandBuffer;

		commandBuffer.reset();

		RecordDrawCommands(commandBuffer, imageIndex);

		vk::SubmitInfo submitInfo = {};
		vk::Semaphore waitSemaphores[] = { m_ImageAvailable };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vk::Semaphore signalSemaphores[] = { m_RenderFinished };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		try
		{
			m_GraphicsQueue.submit(submitInfo, m_InFlightFence);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ERROR("Failed to submit draw command buffer ({0})", e.what());
		}

		vk::PresentInfoKHR presentInfo = {};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		vk::SwapchainKHR swapchains[] = { m_Swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;

		m_PresentQueue.presentKHR(presentInfo);
	}

	void VulkanContext::RecordDrawCommands(vk::CommandBuffer& commandBuffer, uint32_t imageIndex)
	{
		m_CurrentCommandBuffer = &commandBuffer;

		vk::CommandBufferBeginInfo beginInfo = {};
		try
		{
			commandBuffer.begin(beginInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not being recording command buffer ({0})", e.what());
		}

		vk::RenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_Frames[imageIndex].FrameBuffer;
		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = m_Extent;

		vk::ClearValue clearColor = { std::array<float, 4>({1.0f, 0.5f, 0.25f, 1.0f}) };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);

		commandBuffer.draw(3, 1, 0, 0);

		AppRenderEvent event;
		m_EventCallbackFn(event);

		commandBuffer.endRenderPass();

		try
		{
			commandBuffer.end();
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Failed recording command buffer ({0})", e.what());
		}

		m_CurrentCommandBuffer = nullptr;
	}

	vk::DescriptorPool VulkanContext::CreateDescriptorPool()
	{
		vk::DescriptorPoolSize poolSizes[] =
		{
			{ vk::DescriptorType::eCombinedImageSampler, 1 },
		};
		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
		poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = poolSizes;

		try
		{
			return m_Device.createDescriptorPool(poolInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create descriptor pool ({0})", e.what());
		}
	}

	vk::CommandBuffer VulkanContext::CreateSingleTimeCommandBuffer()
	{
		vk::CommandBufferAllocateInfo allocInfo = {};
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		vk::CommandBuffer commandBuffer = m_Device.allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo = {};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		commandBuffer.begin(beginInfo);

		return commandBuffer;
	}

	void VulkanContext::EndSingleTimeCommands(vk::CommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		vk::SubmitInfo submitInfo{};
		submitInfo.sType = vk::StructureType::eSubmitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		m_GraphicsQueue.submit(submitInfo);
		m_GraphicsQueue.waitIdle();

		std::vector<vk::CommandBuffer> commandBuffers = { commandBuffer };

		m_Device.freeCommandBuffers(m_CommandPool, commandBuffers);
	}
}

