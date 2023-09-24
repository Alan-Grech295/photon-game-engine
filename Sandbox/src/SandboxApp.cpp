#include <Photon.h>

class Sandbox : public Photon::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}


};

Photon::Application* Photon::CreateApplication()
{
	return new Sandbox();
}