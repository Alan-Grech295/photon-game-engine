workspace "Photon"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder
IncludeDir = {}
IncludeDir["GLFW"] = "Photon/vendor/GLFW/include"
IncludeDir["Vulkan"] = "Photon/vendor/Vulkan/include"

include "Photon/vendor/GLFW"

project "Photon"
    location "Photon"
    kind "SharedLib"
    language "C++"
    staticruntime "off"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "ptpch.h"
    pchsource "Photon/src/ptpch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Vulkan}",
    }

    libdirs 
    {
        "Photon/vendor/Vulkan/lib",
    }

    links
    {
        "GLFW",
        "vulkan-1.lib",
        -- "opengl32.lib",
        -- "dwmapi.lib",
    }

    filter "system:windows"
        cppdialect "C++20"
        systemversion "latest"

        defines 
        {
            "PT_PLATFORM_WINDOWS",
            "PT_BUILD_DLL",
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }

    filter "configurations:Debug"
        defines { "PT_DEBUG", "PT_ENABLE_ASSERT" }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines "PT_RELEASE"
        optimize "On"
        runtime "Release"

    filter "configurations:Dist"
        defines "PT_DIST"
        optimize "On"
        runtime "Release"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    staticruntime "off"

    language "C++"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "Photon/vendor/spdlog/include",
        "Photon/src",
    }

    links
    {
        "Photon"
    }

    filter "system:windows"
        cppdialect "C++20"
        systemversion "latest"

        defines 
        {
            "PT_PLATFORM_WINDOWS",
        }

    filter "configurations:Debug"
        defines "PT_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "PT_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "PT_DIST"
        runtime "Release"
        optimize "On"