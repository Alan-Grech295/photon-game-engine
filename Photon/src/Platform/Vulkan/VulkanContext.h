#pragma once
#include "Photon/Renderer/GraphicsContext.h"
#include <optional>

#include "VulkanFrame.h"
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/raytraceKHR_vk.hpp>
#include <nvvk/memallocator_dma_vk.hpp>

struct GLFWwindow;

namespace Photon
{
	struct ObjModel
	{
		uint32_t     nbIndices{ 0 };
		uint32_t     nbVertices{ 0 };
		nvvk::Buffer vertexBuffer;    // Device buffer of all 'Vertex'
		nvvk::Buffer indexBuffer;     // Device buffer of the indices forming triangles
		nvvk::Buffer matColorBuffer;  // Device buffer of array of 'Wavefront material'
		nvvk::Buffer matIndexBuffer;  // Device buffer of array of 'Wavefront material'
	};

	struct ObjInstance
	{
		nvmath::mat4f transform;    // Matrix of the instance
		uint32_t      objIndex{ 0 };  // Model index reference
	};

	struct ObjDesc
	{
		uint32_t txtOffset;
		VkDeviceAddress vertexAddress;
		VkDeviceAddress indexAddress;
		VkDeviceAddress materialAddress;
		VkDeviceAddress materialIndexAddress;

	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		~VulkanContext();
		virtual void Init() override;
		virtual void SwapBuffers() override;
	public:
		vk::DescriptorPool CreateDescriptorPool();

		vk::CommandBuffer CreateSingleTimeCommandBuffer();
		void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);

		// Vulkan Raytracing Tutorial
		auto ObjectToVKGeometry(const ObjModel& model);

		void CreateBottomLevelAS();

		void CreateTopLevelAS();

		// TEMP FUNCS
		void LoadModel(const std::string& filename, nvmath::mat4f transform = nvmath::mat4f(1));

		void CreateTextureImages(const VkCommandBuffer& cmdBuf, const std::vector<std::string>& textures);
	private:
		void RecordDrawCommands(vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
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
	public:
		GLFWwindow* m_WindowHandle;
		vk::SurfaceKHR m_Surface;
		vk::Device m_Device;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		vk::SwapchainKHR m_Swapchain;
		std::vector<SwapchainFrame> m_Frames;
		vk::Format m_Format;
		vk::Extent2D m_Extent;
		SwapchainSupportDetails m_Support;
		uint32_t m_ImageCount;

		QueueFamilyIndices m_QueueFamilyIndices;

		vk::RenderPass m_RasterRenderPass;
		vk::Pipeline m_RasterPipeline;

		// Ray tracing pipeline
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProperties{ };
		vk::RenderPass m_RTRenderPass;
		vk::Pipeline m_RTPipeline;

		nvvk::RaytracingBuilderKHR m_RTBuilder;
		nvvk::ResourceAllocatorDma m_Alloc;

		vk::CommandPool m_CommandPool;
		vk::CommandBuffer m_MainCommandBuffer;

		vk::CommandBuffer* m_CurrentCommandBuffer = nullptr;

		vk::Fence m_InFlightFence;
		vk::Semaphore m_ImageAvailable, m_RenderFinished;


		// TEMP
		std::vector<ObjModel>    m_objModel;   // Model on host
		std::vector<ObjDesc>     m_objDesc;    // Model description for device access
		std::vector<ObjInstance> m_instances;  // Scene model instances
		std::vector<nvvk::Texture> m_Textures;  // vector of all textures of the scene
		nvvk::DebugUtil            m_Debug;  // Utility to name objects
	};
}