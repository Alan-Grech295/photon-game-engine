#pragma once

#ifdef PT_PLATFORM_WINDOWS

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//
//#ifdef PT_DEBUG
//#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
//// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
//// allocations to be of _CLIENT_BLOCK type
//#else
//#define DBG_NEW new
//#endif

extern Photon::Application* Photon::CreateApplication();

int main(int argc, char** argv)
{
	/*_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(1342);
	_CrtMemState s1;
	_CrtMemState s2;
	_CrtMemState s3;
	_CrtMemCheckpoint(&s1);*/

	Photon::Log::Init();

	auto app = Photon::CreateApplication();
	app->Run();
	delete app;

	/*_CrtMemCheckpoint(&s2);

	if (_CrtMemDifference(&s3, &s1, &s2))
		_CrtMemDumpStatistics(&s3);*/

	/*_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();*/
}

#endif