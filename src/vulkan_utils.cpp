
#include "vulkan_utils.h"

#include <iostream>
#include <set>

namespace VulkanUtils
{

SwapchainSupportDetails getSwapchainSupportDetails(VkPhysicalDevice device, 
	VkSurfaceKHR surface)
{
	SwapchainSupportDetails details;

	// fetch device surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

	// fetch formats
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
			details.Formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount > 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
			details.PresentModes.data());
	}

	return details;
}


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (int32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}

		if (surface != VK_NULL_HANDLE)
		{
			// for querying presentation support, a surface is needed
			// but the surface is only created, if we're not only rendering off screen
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.PresentFamily = i;
			}
		}

		if (indices.IsComplete())
		{
			// no need to look for more
			break;
		}
	}
	return indices;
}

bool areInstanceLayersSupported(std::vector<const char*> layers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : layers)
	{
		bool layerFound = false;

		for (const VkLayerProperties& layerProps : availableLayers)
		{
			if (strcmp(layerName, layerProps.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

bool areInstanceExtensionsAvailable(const std::vector<const char*>& requestedExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::vector<const char*> missingExtensions;

	for (const char* required : requestedExtensions)
	{
		bool found = false;
		for (const VkExtensionProperties& extensionProp : extensions)
		{
			if (strcmp(required, extensionProp.extensionName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			missingExtensions.push_back(required);
		}
	}

	if (missingExtensions.size() > 0)
	{
		std::cerr << "Missing extensions:" << std::endl;
		for (const char* extension : missingExtensions)
		{
			std::cerr << "\t" << extension << std::endl;
		}
	}
	return true;
}

bool areDeviceExtensionsAvailable(VkPhysicalDevice device, const std::vector<const char*> requestedExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> extensions(requestedExtensions.begin(), requestedExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		extensions.erase(extension.extensionName);
	}

	return extensions.empty();
}

QueueFamilyIndices fetchQueues(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (int32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}

		if (surface != VK_NULL_HANDLE)
		{
			// for querying presentation support, a surface is needed
			// but the surface is only created, if we're not only rendering off screen
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.PresentFamily = i;
			}
		}

		if (indices.IsComplete())
		{
			// no need to look for more
			break;
		}
	}
	return indices;
}

void setDebugName(VkDevice device, uint64_t objectHandle,
	VkObjectType objectType, const std::string& name)
{
	// the extension may not be enabled if not in debug
	// TODO: allow for NDEBUG builds with this extension in use
#ifndef NDEBUG
	if (objectHandle != 0)
	{
		VkDebugUtilsObjectNameInfoEXT objNameInfo{};
		objNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		objNameInfo.objectType = objectType;
		objNameInfo.pObjectName = name.c_str();
		objNameInfo.objectHandle = objectHandle;

		vkSetDebugUtilsObjectNameEXT(device, &objNameInfo);
	}
	else
	{
        std::cerr << "Cannot give a name to a vulkan object that is VK_NULL_HANDLE!" << std::endl;
	}
#endif
}

}
