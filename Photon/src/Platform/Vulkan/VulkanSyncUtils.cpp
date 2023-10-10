#include "ptpch.h"
#include "VulkanSyncUtils.h"

namespace Photon
{
	vk::Semaphore VulkanSyncUtils::MakeSemaphore(vk::Device device)
	{
		vk::SemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.flags = vk::SemaphoreCreateFlags();

		try
		{
			return device.createSemaphore(semaphoreInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create semaphore ({0})", e.what());
		}
	}

	vk::Fence VulkanSyncUtils::MakeFence(vk::Device device)
	{
		vk::FenceCreateInfo fenceInfo = {};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		try
		{
			return device.createFence(fenceInfo);
		}
		catch (vk::SystemError e)
		{
			PT_CORE_ASSERT(false, "Could not create fence({0})", e.what());
		}
	}
}
