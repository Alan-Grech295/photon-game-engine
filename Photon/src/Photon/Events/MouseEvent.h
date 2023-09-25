#pragma once
#include "Event.h"

#include <sstream>

namespace Photon
{
	class PHOTON_API MouseButtonEvent : public Event
	{
	public:
		inline int GetMouseButton() { return m_Button; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << GetName() << ": " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_CATEGORY(EventCategoryMouseButton | EventCategoryInput)
	protected:
		MouseButtonEvent(int button)
			: m_Button(button)
		{}

	protected:
		int m_Button;
	};

	class PHOTON_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button)
		{}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class PHOTON_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button)
		{}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

	class PHOTON_API MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y)
			: m_MouseX(x), m_MouseY(y)
		{}

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }

		std::string ToString() const
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};

	class PHOTON_API MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset)
		{}

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		std::string ToString() const
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_XOffset << ", " << m_YOffset;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset, m_YOffset;
	};
}