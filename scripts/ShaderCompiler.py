import argparse
import glob
import os
import subprocess

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument("--shaders", type=str, default="", help="The relative path of the shader folder")

    args = parser.parse_args()

    file_pattern = ["*.vert", "*.frag"]

    matching_files = []
    for files in [glob.glob(os.path.join(os.path.abspath(args.shaders), e)) for e in file_pattern]:
        matching_files.extend(files)

    vulkanSDK = os.environ.get("VULKAN_SDK")

    compiler_path = os.path.join(vulkanSDK, "Bin", "glslc.exe")

    for file_path in matching_files:
        result = subprocess.run([compiler_path, file_path, "-o", os.path.splitext(file_path)[0] + ".spv"],
                                capture_output=True, text=True)
        if result.returncode == 0:
            if result.stdout:
                print(result.stdout)
            print("\033[92mCompiled ", file_path, " successfully", sep="")
        else:
            print("\033[91mError when compiling ", file_path, ":\n", result.stderr, sep="")
