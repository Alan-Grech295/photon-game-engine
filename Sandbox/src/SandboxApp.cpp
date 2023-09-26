#include <Photon.h>

class ExampleLayer : public Photon::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{}

	void OnUpdate() override
	{
		PT_INFO("ExampleLayer::Update");
	}

	void OnEvent(Photon::Event& e) override
	{
		PT_CORE_TRACE("{0}", e.ToString());
	}
};

class Sandbox : public Photon::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}


};

Photon::Application* Photon::CreateApplication()
{
	return new Sandbox();
}