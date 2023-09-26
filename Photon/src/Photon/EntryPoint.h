#pragma once

#ifdef PT_PLATFORM_WINDOWS

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

extern Photon::Application* Photon::CreateApplication();

int main(int argc, char** argv)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Photon::Log::Init();

	auto app = Photon::CreateApplication();
	app->Run();
	delete app;

	/*_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();*/
}

#endif