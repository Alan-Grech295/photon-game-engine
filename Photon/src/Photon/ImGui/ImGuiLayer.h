#pragma once
#include "Photon/Layer.h"

namespace Photon
{
	class PHOTON_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;

		void OnRender() override;
		void OnEvent(Event& event) override;
	private:
		class VulkanContext* m_Context = nullptr;
		float m_Time = 0;
	};
}