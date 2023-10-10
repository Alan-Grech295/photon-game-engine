#include "ptpch.h"
#include "VulkanCommandUtils.h"
#include "VulkanContext.h"

namespace Photon
{
    vk::CommandPool VulkanCommandUtils::MakeCommandPool(vk::Device& device, VulkanContext::QueueFamilyIndices& queueFamilyIndices)
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

    vk::CommandBuffer VulkanCommandUtils::MakeCommandBuffer(vk::Device& device, vk::CommandPool& commandPool, std::vector<SwapchainFrame>& frames)
    {
        vk::CommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = vk::CommandBufferLevel::ePrimary;
        allocateInfo.commandBufferCount = 1;

        for (auto& frame : frames)
        {
            try
            {
                frame.CommandBuffer = device.allocateCommandBuffers(allocateInfo)[0];
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
