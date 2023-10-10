#pragma once

namespace Photon
{
	enum class RendererAPI
	{
		None = 0, Vulkan = 1
	};

	class Renderer
	{
	public:
		inline static RendererAPI GetAPI() { return s_RendererAPI; }
	private:
		static RendererAPI s_RendererAPI;
	};

}