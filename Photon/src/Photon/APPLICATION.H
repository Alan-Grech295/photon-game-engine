#pragma once
#include "Core.h"
#include "Window.h"

namespace Photon
{
	class PHOTON_API Application
	{
	public:
		Application();
		~Application();

		void Run();

	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	// To be define in client
	Application* CreateApplication();
}

