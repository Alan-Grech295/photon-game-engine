#pragma once
#include "Photon/Events/Event.h"

namespace Photon
{
	class GraphicsContext
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		void SetEventCallback(const EventCallbackFn& callback) { m_EventCallbackFn = callback; }

	protected:
		EventCallbackFn m_EventCallbackFn;
	};
}