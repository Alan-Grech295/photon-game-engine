workspace "Photon"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

VULKAN_SDK = os.getenv("VULKAN_SDK")

-- Include directories relative to root folder
IncludeDir = {}
IncludeDir["GLFW"] = "Photon/vendor/GLFW/include"
IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"
IncludeDir["ImGui"] = "Photon/vendor/imgui"
IncludeDir["NVProCore"] = "Photon/vendor/NVProCore"

LibraryDirs = {}
LibraryDirs["Vulkan"] = "%{VULKAN_SDK}/Lib"

include "Photon/vendor/GLFW"
include "Photon/vendor/ImGui"

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

        -- Vertex and fragment shaders
        "%{prj.name}/src/**.vert",
        "%{prj.name}/src/**.frag",
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.NVProCore}",
    }

    libdirs 
    {
        "%{LibraryDirs.Vulkan}",
    }

    links
    {
        "GLFW",
        "ImGui",
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

        prebuildcommands
        {
            "CD " .. os.getcwd(),
            "CALL scripts/ShaderCompiler.bat",
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }

    filter "configurations:Debug"
        defines { "PT_DEBUG", "PT_ENABLE_ASSERTS" }
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