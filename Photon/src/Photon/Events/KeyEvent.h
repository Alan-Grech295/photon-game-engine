#pragma once
#include "Event.h"

namespace Photon
{
	class PHOTON_API KeyEvent : public Event
	{
	public:
		inline int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(int keyCode)
			: m_KeyCode(keyCode)
		{}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << GetName() << ": " << m_KeyCode;
			return ss.str();
		}

	protected:
		int m_KeyCode;
	};

	class PHOTON_API KeyPressedEvent : public KeyEvent
	{
	public: 
		KeyPressedEvent(int keyCode, int repeatCount)
			: KeyEvent(keyCode), m_RepeatCount(repeatCount)
		{}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << ToString() << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};

	class PHOTON_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keyCode)
			: KeyEvent(keyCode)
		{}

		EVENT_CLASS_TYPE(KeyReleased)
	};
}