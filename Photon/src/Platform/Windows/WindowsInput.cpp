#include "ptpch.h"
#include "WindowsInput.h"
#include "Photon/Application.h"
#include "WindowsWindow.h"

#include <GLFW/glfw3.h>

namespace Photon
{
    Input* Input::s_Instance = new WindowsInput();


    bool WindowsInput::IsKeyPressedImpl(int keycode)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();

        auto state = glfwGetKey(window, keycode);

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool WindowsInput::IsKeyReleasedImpl(int keycode)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();

        auto state = glfwGetKey(window, keycode);

        return state == GLFW_RELEASE;
    }

    bool WindowsInput::IsMouseButtonPressedImpl(int button)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();

        auto state = glfwGetMouseButton(window, button);

        return state == GLFW_RELEASE;
    }

    bool WindowsInput::IsMouseButtonReleasedImpl(int button)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();

        auto state = glfwGetMouseButton(window, button);

        return state == GLFW_PRESS;
    }

    std::pair<float, float> WindowsInput::GetMousePositionImpl()
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();

        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return { (float)xPos, (float)yPos };
    }

    float WindowsInput::GetMouseXImpl()
    {
        auto [x, y] = GetMousePositionImpl();
        return x;
    }

    float WindowsInput::GetMouseYImpl()
    {
        auto [x, y] = GetMousePositionImpl();
        return y;
    }
}
