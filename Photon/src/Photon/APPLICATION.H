#pragma once
#include "Core.h"

namespace Photon
{
	class PHOTON_API Application
	{
	public:
		Application();
		~Application();

		void Run();
	};

	// To be define in client
	Application* CreateApplication();
}

