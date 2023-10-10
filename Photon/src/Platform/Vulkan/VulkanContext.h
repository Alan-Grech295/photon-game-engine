#pragma once
#include "Photon/Renderer/GraphicsContext.h"

class GLFWwindow;

namespace Photon
{
	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init();
		virtual void SwapBuffers();
	private:
		GLFWwindow* m_WindowHandle;
	};
}