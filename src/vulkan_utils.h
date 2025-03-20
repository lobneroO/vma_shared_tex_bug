
#pragma once

#include "volk.h"

#include <optional>
#include <string>
#include <vector>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	bool IsComplete() const
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};

namespace VulkanUtils
{

SwapchainSupportDetails getSwapchainSupportDetails(VkPhysicalDevice device, 
	VkSurfaceKHR surface);

/**
* Finds all queue families. If surface is null, then offscreen rendering is assumed
*/
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

bool areInstanceLayersSupported(std::vector<const char*> layers);

bool areInstanceExtensionsAvailable(const std::vector<const char*>& requestedExtensions);

bool areDeviceExtensionsAvailable(VkPhysicalDevice device, const std::vector<const char*> requestedExtensions);

QueueFamilyIndices fetchQueues(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
* Sets the debug name to the given vulkan object. Call it like this:
* VisDebugNameUtils::setDebugName(Device, (uint64_t)VertexBuffer.Buffer,
    VK_OBJECT_TYPE_BUFFER, vertBufName)
*/
void setDebugName(VkDevice device, uint64_t objectHandle,
    VkObjectType objectType, const std::string& name);

}
