import os
import subprocess
import platform

from SetupPython import PythonConfiguration

PythonConfiguration.Validate()

from SetupVulkan import VulkanConfiguration

VulkanConfiguration.validate()

# Updating git submodules
print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

# Creating the new project
subprocess.call([os.path.abspath("./scripts/Win-GenProjects.bat"), "nopause"])
