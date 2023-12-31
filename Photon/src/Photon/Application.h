#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"

namespace Photon
{
	class PHOTON_API Application
	{
	public:
		Application();
		~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;

		LayerStack m_LayerStack;
	};

	// To be define in client
	Application* CreateApplication();
}

