
#pragma once

#include <map>
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
    void enableAvailableOptionalDeviceExtensions(
        std::vector<const char*>& deviceExtensions,
        const std::map<const char*, bool>& optionalDeviceExtensions) const;

    void fetchQueues();

private:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
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
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE; 

	uint32_t vulkanApiVersion = 0;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
    /**
    * Doing offscreen means we don't need GLFW or anything else
    */
    bool renderOffscreenOnly = true;


	const std::vector<const char*> requiredDeviceExtensions = {
		//VK_EXT_DEBUG_MARKER_EXTENSION_NAME
	};
	const std::vector<const char*> requiredOnScreenRenderingDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const std::vector<const char*> interopDeviceExtensions = {
#if _WIN32
		VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
#else
		VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
#endif
	};
	std::map<const char*, bool> optionalDeviceExtensions = {
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
    bool enableValidationLayers = true;
};
