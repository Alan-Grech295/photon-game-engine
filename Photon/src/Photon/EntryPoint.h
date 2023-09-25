#pragma once

#ifdef PT_PLATFORM_WINDOWS

extern Photon::Application* Photon::CreateApplication();

int main(int argc, char** argv)
{
	Photon::Log::Init();
	PT_CORE_WARN("Initialized Log");
	PT_INFO("Hello");

	auto app = Photon::CreateApplication();
	app->Run();
	delete app;
}

#endif