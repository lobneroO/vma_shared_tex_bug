
#pragma once

#include <vector>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "vulkan_utils.h"

class Device
{
public:
    Device();
    ~Device();

private:
    void setupInstance();
    void choosePhysicalDevice();
    uint32_t ratePhysicalDevice(VkPhysicalDevice device) const;
    void createLogicalDevice();
    void setupVma();

    bool areValidationLayersSupported() const;
    std::vector<const char*> getRequiredInstanceExtensions() const;
    bool areRequiredInstanceExtensionsAvailable(const std::vector<const char*>& requiredExtensions) const;
    bool areRequiredDeviceExtensionsSupported(VkPhysicalDevice device) const;

private:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties2 physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties memoryProperties;
    VkDevice device = VK_NULL_HANDLE;
	VmaAllocator memoryAllocator = VK_NULL_HANDLE;

	/** pool for creating interop resources*/
	VmaPool imageInteropPool = VK_NULL_HANDLE;
	/** The Pool Create Info needs to stay alive while the pool is alive! */
	VmaPoolCreateInfo poolCreateInfo{};
	VkExportMemoryAllocateInfo exportMemAllocInfo{};

    QueueFamilyIndices qfIndices;

	uint32_t vulkanApiVersion = 0;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
    /**
    * Doing offscreen means we don't need GLFW or anything else
    */
    bool renderOffscreenOnly = true;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
    bool enableValidationLayers = true;
};
