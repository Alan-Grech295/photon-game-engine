#include "ptpch.h"
#include "ImGuiLayer.h"
#include "Photon/Application.h"
#include "Platform/Vulkan/VulkanAPI.h"

#include "GLFW/glfw3.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/ImGui/ImGuiVulkanRenderer.h"
#include "Platform/Vulkan/ImGui/ImGuiGLFWRenderer.h"
#include "imgui.h"

namespace Photon 
{
	static void CheckVKResult(VkResult err)
	{
		if (err == 0)
			return;

		PT_CORE_ERROR("[Vulkan] Error: VKResult = {0}", (int)err);

		if (err < 0)
			abort();
	}

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		m_Context = (VulkanContext*)Application::Get().GetWindow().GetContext();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

		// Keymap?

		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Application::Get().GetWindow().GetNativePtr(), true);

		ImGui_ImplVulkan_InitInfo info;
		info.Instance = VulkanAPI::GetInstance();
		info.DescriptorPool = m_Context->CreateDescriptorPool();
		info.Device = m_Context->m_Device;
		info.PhysicalDevice = VulkanAPI::GetPhysicalDevice();
		info.ImageCount = m_Context->m_ImageCount;
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		info.QueueFamily = m_Context->m_QueueFamilyIndices.graphicsFamily.value();
		info.Queue = m_Context->m_GraphicsQueue;
		info.MinImageCount = 2;
		info.Subpass = 0;
		info.PipelineCache = nullptr;
		info.Allocator = nullptr;
		info.ColorAttachmentFormat = VK_FORMAT_UNDEFINED;
		info.UseDynamicRendering = false;
		info.CheckVkResultFn = CheckVKResult;

		ImGui_ImplVulkan_Init(&info, m_Context->m_RenderPass);

		vk::CommandBuffer commandBuffer = m_Context->CreateSingleTimeCommandBuffer();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		m_Context->EndSingleTimeCommands(commandBuffer);
		
		m_Context->m_Device.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void ImGuiLayer::OnDetach()
	{
		m_Context->m_Device.waitIdle();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnRender()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		float time = (float)glfwGetTime();
		io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool show = true;
		ImGui::ShowDemoWindow(&show);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *m_Context->m_CurrentCommandBuffer);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

	}

	void ImGuiLayer::OnEvent(Event& event)
	{

	}
}

