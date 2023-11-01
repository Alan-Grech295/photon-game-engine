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

#include "nvvkhl/appbase_vk.hpp"
#include <nvvk/buffers_vk.hpp>
#include "nvh/alignment.hpp"
#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/buffers_vk.hpp"
#include "nvvk/extensions_vk.hpp"

#include "obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::vector<std::string> defaultSearchPaths = {
	  "D:\\Dev\\Photon\\Sandbox\\models"
};

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

	VulkanContext::~VulkanContext()
	{
		m_Device.waitIdle();

		m_Device.destroyFence(m_InFlightFence);
		m_Device.destroySemaphore(m_ImageAvailable);
		m_Device.destroySemaphore(m_RenderFinished);

		m_Device.destroyCommandPool(m_CommandPool);

		m_Device.destroyPipeline(m_RasterPipeline);
		//m_Device.destroyPipelineLayout();
		m_Device.destroyRenderPass(m_RasterRenderPass);
		
		m_Device.destroyPipeline(m_RTPipeline);
		//m_Device.destroyPipelineLayout();
		m_Device.destroyRenderPass(m_RTRenderPass);

		for (auto& frame : m_Frames) {
			m_Device.destroyImageView(frame.ImageView);
			m_Device.destroyFramebuffer(frame.FrameBuffer);
		}

		m_Device.destroySwapchainKHR(m_Swapchain);
		m_Device.destroy();

		m_RTBuilder.destroy();

		//TEMP
		VulkanAPI::Shutdown();
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
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
		};

		auto& layers = VulkanAPI::GetLayers();

		// Enabling buffer device access feature
		VkPhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
		bufferAddressFeatures.bufferDeviceAddress = true;

		// Enabling acceleration structure feature
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };		bufferAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		accelFeature.accelerationStructure = true;
		accelFeature.pNext = &bufferAddressFeatures;

		// Enabling ray tracing
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
		rtPipelineFeature.rayTracingPipeline = true;
		rtPipelineFeature.pNext = &accelFeature;

		VkPhysicalDeviceFeatures2 features = VulkanAPI::GetPhysicalDevice().getFeatures2();
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features.features.samplerAnisotropy = true;

		features.pNext = &rtPipelineFeature;

		vk::DeviceCreateInfo logicalDeviceCreateInfo = vk::DeviceCreateInfo(
			vk::DeviceCreateFlags(),
			(uint32_t)queueCreateInfos.size(), queueCreateInfos.data(),
			(uint32_t)layers.size(), layers.data(),
			(uint32_t)deviceExtensions.size(), deviceExtensions.data(),
			nullptr
		);

		logicalDeviceCreateInfo.pNext = &features;

		try
		{
			m_Device = physicalDevice.createDevice(logicalDeviceCreateInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create the logical device ({0})", e.what());
		}

		load_VK_EXTENSIONS(VulkanAPI::GetInstance(), vkGetInstanceProcAddr, m_Device, vkGetDeviceProcAddr);

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

		// Creating the graphics pipeline
		{
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
			m_RasterRenderPass = MakeRenderPass(m_Device, m_Format);
			pipelineInfo.renderPass = m_RasterRenderPass;

			// Extra
			pipelineInfo.basePipelineHandle = nullptr;

			// Make the pipeline
			try
			{
				m_RasterPipeline = (m_Device.createGraphicsPipeline(nullptr, pipelineInfo)).value;
			}
			catch (vk::SystemError e)
			{
				PT_CORE_ASSERT(false, "Could not create graphics pipeline ({0})", e.what());
			}

			m_Device.destroyShaderModule(vertShaderModule);
			m_Device.destroyShaderModule(fragShaderModule);
		}

		// Ray tracing pipeline
		{
			m_Alloc.init(VulkanAPI::GetInstance(), m_Device, VulkanAPI::GetPhysicalDevice());

			vk::PhysicalDeviceProperties2 prop2{ };
			prop2.pNext = &m_RTProperties;
			VulkanAPI::GetPhysicalDevice().getProperties2(&prop2);

			m_RTBuilder.setup(m_Device, &m_Alloc, m_QueueFamilyIndices.graphicsFamily.value());

			LoadModel("D:\\Dev\\Photon\\Sandbox\\models\\sphere.obj");

			CreateBottomLevelAS();
			CreateTopLevelAS();
		}

		VulkanFramebufferUtils::MakeFrameBuffers(m_Device, m_RasterRenderPass, m_Extent, m_Frames);

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
		renderPassInfo.renderPass = m_RasterRenderPass;
		renderPassInfo.framebuffer = m_Frames[imageIndex].FrameBuffer;
		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = m_Extent;

		vk::ClearValue clearColor = { std::array<float, 4>({1.0f, 0.5f, 0.25f, 1.0f}) };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_RasterPipeline);

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

	auto VulkanContext::ObjectToVKGeometry(const ObjModel& model)
	{
		vk::DeviceAddress vertexAddress = nvvk::getBufferDeviceAddress(m_Device, model.vertexBuffer.buffer);
		vk::DeviceAddress indexAddress = nvvk::getBufferDeviceAddress(m_Device, model.indexBuffer.buffer);

		uint32_t maxPrimitiveCount = model.nbIndices / 3;

		// Describe buffer as array of VertexObj
		vk::AccelerationStructureGeometryTrianglesDataKHR triangles{};
		triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(VertexObj);

		// Describe index data
		triangles.indexType = vk::IndexType::eUint32;
		triangles.indexData.deviceAddress = indexAddress;

		triangles.maxVertex = model.nbVertices - 1;

		// Identify the above data as containing opaque triangles
		vk::AccelerationStructureGeometryKHR asGeometry{};
		asGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
		asGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
		asGeometry.geometry.triangles = triangles;

		// The array is used to build the BLAS
		vk::AccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = maxPrimitiveCount;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		// BLAS is made of only one geometry
		nvvk::RaytracingBuilderKHR::BlasInput input;
		input.asGeometry.emplace_back(asGeometry);
		input.asBuildOffsetInfo.emplace_back(offset);

		return input;
	}

	void VulkanContext::CreateBottomLevelAS()
	{
		// BLAS - Storing each primitive in a geometry
		std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
		allBlas.reserve(m_objModel.size());
		for (const auto& obj : m_objModel)
		{
			auto blas = ObjectToVKGeometry(obj);

			// We could add more geometry in each BLAS, but we add only one for now
			allBlas.emplace_back(blas);
		}
		m_RTBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}

	void VulkanContext::CreateTopLevelAS()
	{
		std::vector<VkAccelerationStructureInstanceKHR> tlas;
		tlas.reserve(m_instances.size());
		for (const ObjInstance& inst : m_instances)
		{
			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = nvvk::toTransformMatrixKHR(inst.transform);  // Position of the instance
			rayInst.instanceCustomIndex = inst.objIndex;                               // gl_InstanceCustomIndexEXT
			rayInst.accelerationStructureReference = m_RTBuilder.getBlasDeviceAddress(inst.objIndex);
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
			tlas.emplace_back(rayInst);
		}
		m_RTBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}

	//--------------------------------------------------------------------------------------------------
	// Creating all textures and samplers
	//
	void VulkanContext::CreateTextureImages(const VkCommandBuffer& cmdBuf, const std::vector<std::string>& textures)
	{
		VkSamplerCreateInfo samplerCreateInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.maxLod = FLT_MAX;

		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

		// If no textures are present, create a dummy one to accommodate the pipeline layout
		if (textures.empty() && m_Textures.empty())
		{
			nvvk::Texture texture;

			std::array<uint8_t, 4> color{ 255u, 255u, 255u, 255u };
			VkDeviceSize           bufferSize = sizeof(color);
			auto                   imgSize = VkExtent2D{ 1, 1 };
			auto                   imageCreateInfo = nvvk::makeImage2DCreateInfo(imgSize, format);

			// Creating the dummy texture
			nvvk::Image           image = m_Alloc.createImage(cmdBuf, bufferSize, color.data(), imageCreateInfo);
			VkImageViewCreateInfo ivInfo = nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
			texture = m_Alloc.createTexture(image, ivInfo, samplerCreateInfo);

			// The image format must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			nvvk::cmdBarrierImageLayout(cmdBuf, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_Textures.push_back(texture);
		}
		else
		{
			PT_CORE_ASSERT(false, "Textured models are not supported");
		}
	}

	void VulkanContext::LoadModel(const std::string& filename, nvmath::mat4f transform)
	{
		PT_CORE_INFO("Loading File:  {0} \n", filename.c_str());
		ObjLoader loader;
		loader.loadModel(filename);

		// Converting from Srgb to linear
		for (auto& m : loader.m_materials)
		{
			m.ambient = nvmath::pow(m.ambient, 2.2f);
			m.diffuse = nvmath::pow(m.diffuse, 2.2f);
			m.specular = nvmath::pow(m.specular, 2.2f);
		}

		ObjModel model;
		model.nbIndices = static_cast<uint32_t>(loader.m_indices.size());
		model.nbVertices = static_cast<uint32_t>(loader.m_vertices.size());

		// Create the buffers on Device and copy vertices, indices and materials
		nvvk::CommandPool  cmdBufGet(m_Device, m_QueueFamilyIndices.graphicsFamily.value());
		VkCommandBuffer    cmdBuf = cmdBufGet.createCommandBuffer();
		VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VkBufferUsageFlags rayTracingFlags =  // used also for building acceleration structures
			flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		model.vertexBuffer = m_Alloc.createBuffer(cmdBuf, loader.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
		model.indexBuffer = m_Alloc.createBuffer(cmdBuf, loader.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
		model.matColorBuffer = m_Alloc.createBuffer(cmdBuf, loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
		model.matIndexBuffer = m_Alloc.createBuffer(cmdBuf, loader.m_matIndx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
		// Creates all textures found and find the offset for this model
		auto txtOffset = static_cast<uint32_t>(m_Textures.size());
		CreateTextureImages(cmdBuf, loader.m_textures);
		cmdBufGet.submitAndWait(cmdBuf);
		m_Alloc.finalizeAndReleaseStaging();

		std::string objNb = std::to_string(m_objModel.size());
		m_Debug.setObjectName(model.vertexBuffer.buffer, (std::string("vertex_" + objNb)));
		m_Debug.setObjectName(model.indexBuffer.buffer, (std::string("index_" + objNb)));
		m_Debug.setObjectName(model.matColorBuffer.buffer, (std::string("mat_" + objNb)));
		m_Debug.setObjectName(model.matIndexBuffer.buffer, (std::string("matIdx_" + objNb)));

		// Keeping transformation matrix of the instance
		ObjInstance instance;
		instance.transform = transform;
		instance.objIndex = static_cast<uint32_t>(m_objModel.size());
		m_instances.push_back(instance);

		// Creating information for device access
		ObjDesc desc;
		desc.txtOffset = txtOffset;
		desc.vertexAddress = nvvk::getBufferDeviceAddress(m_Device, model.vertexBuffer.buffer);
		desc.indexAddress = nvvk::getBufferDeviceAddress(m_Device, model.indexBuffer.buffer);
		desc.materialAddress = nvvk::getBufferDeviceAddress(m_Device, model.matColorBuffer.buffer);
		desc.materialIndexAddress = nvvk::getBufferDeviceAddress(m_Device, model.matIndexBuffer.buffer);

		// Keeping the obj host model and device description
		m_objModel.emplace_back(model);
		m_objDesc.emplace_back(desc);
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

