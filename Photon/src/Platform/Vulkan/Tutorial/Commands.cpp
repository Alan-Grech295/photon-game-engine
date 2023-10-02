#include "ptpch.h"
#include "Commands.h"

namespace Photon
{
    vk::CommandPool MakeCommandPool(vk::Device& device, WindowsWindow::QueueFamilyIndices& queueFamilyIndices)
    {
        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try
        {
            return device.createCommandPool(poolInfo);
        }
        catch (vk::SystemError e)
        {
            PT_CORE_ASSERT(false, "Could not create command pool ({0})", e.what());
        }
    }

    vk::CommandBuffer MakeCommandBuffer(vk::Device& device, vk::CommandPool& commandPool, std::vector<SwapchainFrame>& frames)
    {
        vk::CommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = vk::CommandBufferLevel::ePrimary;
        allocateInfo.commandBufferCount = 1;

        for (auto& frame : frames)
        {
            try
            {
                frame.commandBuffer = device.allocateCommandBuffers(allocateInfo)[0];
            }
            catch (vk::SystemError e)
            {
                PT_CORE_ASSERT(false, "Could not allocate command buffer ({0})", e.what());
            }
        }

        try
        {
            return device.allocateCommandBuffers(allocateInfo)[0];
        }
        catch (vk::SystemError e)
        {
            PT_CORE_ASSERT(false, "Could not allocate main command buffer ({0})", e.what());
        }
    }
}
