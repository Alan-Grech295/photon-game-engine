import os
import sys
import subprocess
from pathlib import Path

from io import BytesIO
from urllib.request import urlopen

import Utils

# Taken From https://github.com/TheCherno/Hazel/blob/master/scripts/SetupVulkan.py
class VulkanConfiguration:
    requiredVulkanVersion = "1.3."
    installVulkanVersion = "1.3.261.1"
    vulkanDirectory = "./Photon/vendor/Vulkan"

    @classmethod
    def validate(cls):
        if not cls.check_vulkan_sdk():
            print("Vulkan SDK not installed correctly.")
            return

        if not cls.check_vulkan_sdk_debug_libs():
            print("\nNo Vulkan SDK debug libs found. Install Vulkan SDK with debug libs.")

    @classmethod
    def check_vulkan_sdk(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if (vulkanSDK is None):
            print("\nYou don't have the Vulkan SDK installed!")
            cls.__install_vulkan_sdk()
            return False
        else:
            print(f"\nLocated Vulkan SDK at {vulkanSDK}")

        if (cls.requiredVulkanVersion not in vulkanSDK):
            print(f"You don't have the correct Vulkan SDK version! (Engine requires {cls.requiredVulkanVersion})")
            cls.__install_vulkan_sdk()
            return False

        print(f"Correct Vulkan SDK located at {vulkanSDK}")
        return True

    @classmethod
    def __install_vulkan_sdk(cls):
        permission_granted = False
        while not permission_granted:
            reply = str(input(
                "Would you like to install VulkanSDK {0:s}? [Y/N]: ".format(cls.installVulkanVersion))).lower().strip()[
                    :1]
            if reply == 'n':
                return
            permission_granted = (reply == 'y')

        vulkan_install_url = f"https://sdk.lunarg.com/sdk/download/{cls.installVulkanVersion}/windows/VulkanSDK-{cls.installVulkanVersion}-Installer.exe"
        vulkan_path = f"{cls.vulkanDirectory}/VulkanSDK-{cls.installVulkanVersion}-Installer.exe"
        print("Downloading {0:s} to {1:s}".format(vulkan_install_url, vulkan_path))
        Utils.DownloadFile(vulkan_install_url, vulkan_path)
        print("Running Vulkan SDK installer...")
        os.startfile(os.path.abspath(vulkan_path))
        print("Re-run this script after installation!")
        quit()

    @classmethod
    def check_vulkan_sdk_debug_libs(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        shadercdLib = Path(f"{vulkanSDK}/Lib/shaderc_sharedd.lib")

        return shadercdLib.exists()


if __name__ == "__main__":
    VulkanConfiguration.validate()
