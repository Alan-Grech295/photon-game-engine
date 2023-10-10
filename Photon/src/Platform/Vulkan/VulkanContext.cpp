#include "ptpch.h"
#include "VulkanContext.h"

#include <GLFW/glfw3.h>

namespace Photon
{
	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void VulkanContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
	}

	void VulkanContext::SwapBuffers()
	{
	}
}

