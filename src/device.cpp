
#include "device.h"

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

namespace //debugging
{
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	std::string severityType = "";
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severityType = "WARNING"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severityType = "ERROR"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severityType = "INFO"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severityType = "VERBOSE"; break;
	default: throw std::runtime_error("Unknown severity type in debug callback!");
	}

	std::string lMsg = callbackData->pMessage;
	// if (strcmp(callbackData->pMessageIdName, "UNASSIGNED-DEBUG-PRINTF") == 0)
	// {
	// 	// the message contains a lot of vulkan information which is of no real use to the user, 
	// 	// therefore strip it before printing the message
	// 	std::vector<std::string> split = StringUtils::split(callbackData->pMessage, "|");
	//
	// 	// TODO: user can't use '|' char in printf this way
	// 	lMsg = split[split.size() - 1];
	// }

    std::cout << lMsg;

	return VK_FALSE;
}
}

Device::Device()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not initialize volk!");
	}
    setupInstance();
    volkLoadInstance(instance);
    choosePhysicalDevice();
    createLogicalDevice();
    setupVma();
}

Device::~Device()
{
    if(device)
    {
        vkDestroyDevice(device, nullptr);
    }
    if(instance)
    {
        vkDestroyInstance(instance, nullptr);
    }
}

VkDevice Device::getDevice() const
{
    return device;
}

VmaAllocator Device::getAllocator() const
{
    return memoryAllocator;
}

VmaPool Device::getSharedPool() const
{
    return imageInteropPool;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

void Device::setupInstance()
{
	if (enableValidationLayers && !VulkanUtils::areInstanceLayersSupported(validationLayers))
	{
		throw std::runtime_error("Validation layers requested but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Quick Preview Visualize";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	// at least Vulkan 1.2 is required for ray tracing extensions
	// the version is needed for the Vulkan Memory Allocator as well, so store it
	vulkanApiVersion = VK_API_VERSION_1_3;
	appInfo.apiVersion = vulkanApiVersion;

	// get required extensions (needed at instance creation time)
	std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
	if (!VulkanUtils::areInstanceExtensionsAvailable(requiredExtensions))
	{
		throw std::runtime_error("Not all required Vulkan extensions are available!");
	}

	// setup an extra debug messenger to additionally catch any debug layer output
	// from the vulkan instance creation and destruction
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	// for printing from the shader, this keeps the 
	//		"WARNING - Debug Printf message was truncated, 
	//		likely due to a buffer size that was too small for the message"
	// from being printed instead of your print output
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	// actually create the instance
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	// vulkan best practice validation (needs to be in this scope)
	VkValidationFeatureEnableEXT enables[] = {
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
	VkValidationFeaturesEXT features{};

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);

		// enable Vulkan best practices validation
		// ( see https://vulkan.lunarg.com/doc/view/1.2.189.0/linux/best_practices.html )
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 2;
		features.pEnabledValidationFeatures = enables;

		features.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		createInfo.pNext = &features;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Vulkan instance!");
	}
}

void Device::choosePhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	// use a multimap to have a map sorted by score
	std::multimap<uint32_t, VkPhysicalDevice> candidates;
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		uint32_t score = ratePhysicalDevice(device);
		candidates.insert({ score, device });
	}

	if (candidates.rbegin()->first > 0)
	{
		// best candidate is a suitable one (since the score is >0)
		physicalDevice = candidates.rbegin()->second;
        // TODO: probably unnecessary
		// setOptionalExtensionAvailability();
	}
	else
	{
		throw std::runtime_error("failed to find a GPU which supports all required features and properties!");
	}

	// tell the user which device was chosen
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
    std::cout << "Using " << deviceProps.deviceName << std::endl;

	// tell the user which devices are available, but are rejected
	for (const auto& pair_scoreToDevice : candidates)
	{
		uint32_t score = pair_scoreToDevice.first;
		VkPhysicalDevice candidate = pair_scoreToDevice.second;

		VkPhysicalDeviceProperties2 candidateProps{};
		candidateProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps{};
		rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		candidateProps.pNext = &rtPipelineProps;
		VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps{};
		asProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		rtPipelineProps.pNext = &asProps;
		vkGetPhysicalDeviceProperties2(candidate, &candidateProps);

		if (candidate == physicalDevice)
		{
			// store the physical device properties such that 
			// they don't have to be queried every time they are needed
			physicalDeviceProperties = candidateProps;

            VkPhysicalDeviceMemoryProperties2 memoryProps{};
            // check that interop is actually possible
            memoryProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProps);
            memoryProperties = memoryProps.memoryProperties;
			continue;
		}

		std::string logStr = "Rejected ";
		logStr += candidateProps.properties.deviceName;
		if (score > 0)
		{
			logStr += " due to lower score.";
		}
		else
		{
			logStr += " due to being unsuitable for QuickPreviewVisualize.";
		}
        std::cout << logStr << std::endl;
	}

}

uint32_t Device::ratePhysicalDevice(VkPhysicalDevice phyDevice) const
{
	uint32_t score = 0;

	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(phyDevice, &deviceProps);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(phyDevice, &deviceFeatures);

	if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}
	else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		score += 500;
	}
	// if the device is CPU / virtual GPU, don't improve the score

	// check if the device address feature is supported, which we need for acceleration structure creation
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures{};
	deviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	deviceFeatures2.pNext = &deviceAddressFeatures;

	// check if the ray tracing pipeline properties are supported
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
	rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	deviceAddressFeatures.pNext = &rtPipelineFeatures;

	// check if the acceleration structure features are supported
	VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
	asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	rtPipelineFeatures.pNext = &asFeatures;

	vkGetPhysicalDeviceFeatures2(phyDevice, &deviceFeatures2);

	bool areExtensionsSupported = areRequiredDeviceExtensionsSupported(phyDevice);
	bool isSwapchainAdequate = false;
	if (areExtensionsSupported && !renderOffscreenOnly)
	{
		SwapchainSupportDetails swapChainSupport = VulkanUtils::getSwapchainSupportDetails(phyDevice, surface);
		isSwapchainAdequate = !swapChainSupport.Formats.empty()
			&& !swapChainSupport.PresentModes.empty();
	}

	QueueFamilyIndices indices = VulkanUtils::findQueueFamilies(phyDevice, surface);

	// if something important is missing, set score to 0
	if (!indices.GraphicsFamily.has_value()
		|| (!renderOffscreenOnly && !indices.PresentFamily.has_value())
		|| !areExtensionsSupported
		|| (!renderOffscreenOnly && !isSwapchainAdequate)
		|| !deviceFeatures.samplerAnisotropy
		|| !deviceAddressFeatures.bufferDeviceAddress)
	{
		const std::string logStr = std::string(deviceProps.deviceName) + " does not support all required features!";
        std::cout << logStr << std::endl;

		return 0;
	}

	return score;
}

void Device::createLogicalDevice()
{
	qfIndices = VulkanUtils::findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		qfIndices.GraphicsFamily.value(),
	};
	if (!renderOffscreenOnly)
	{
		uniqueQueueFamilies.insert(qfIndices.PresentFamily.value());
	}
	float queuePriority = 1.0f; // required, even if there is only one queue. 1.0f is highest priority

	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	std::vector<const char*> deviceExtensions = requiredDeviceExtensions;
	if (!renderOffscreenOnly)
	{
		deviceExtensions.insert(deviceExtensions.end(),
			requiredOnScreenRenderingDeviceExtensions.begin(), requiredOnScreenRenderingDeviceExtensions.end());
	}
    deviceExtensions.insert(deviceExtensions.end(),
        interopDeviceExtensions.begin(), interopDeviceExtensions.end());

	enableAvailableOptionalDeviceExtensions(deviceExtensions, optionalDeviceExtensions);

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures{};
	physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	// enable all required features:
	// 1. device address feature
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

	physicalDeviceFeatures.pNext = &bufferDeviceAddressFeatures;
	physicalDeviceFeatures.features = deviceFeatures;

		// const std::vector<const char*> renderdocIncomaptibleExtensions = {
		// 	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,	// renderdoc does not support ray tracing
		// 	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		// 	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
		// };
		//
		// // renderdoc is attached, remove the ray tracing extensions
		// for (size_t i = 0; i < deviceExtensions.size(); i++)
		// {
		// 	for (size_t j = 0; j < renderdocIncomaptibleExtensions.size(); j++)
		// 	{
		// 		if (strcmp(deviceExtensions[i], renderdocIncomaptibleExtensions[j]) == 0)
		// 		{
		// 			deviceExtensions.erase(deviceExtensions.begin() + i);
		// 			i--;
		// 			break;
		// 		}
		// 	}
		// }

	vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures);

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pNext = &physicalDeviceFeatures;
	// only backward compatibility for older devices, in newer vulkan
	// these validation layers are not used anymore and went to the instance validation layers
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Logical Device!");
	}

	std::string devName = "Logical Device for ";
	devName += physicalDeviceProperties.properties.deviceName;
	VulkanUtils::setDebugName(device, (uint64_t)device, VK_OBJECT_TYPE_DEVICE, devName);
	VulkanUtils::setDebugName(device, (uint64_t)instance, VK_OBJECT_TYPE_INSTANCE,
		"Visualize Vk Instance");


	fetchQueues();
}

void Device::setupVma()
{
	// set up dynamic vulkan functions via volk for VMA
	VmaVulkanFunctions vmaVkFunctions{};
	{
		vmaVkFunctions.vkAllocateMemory = vkAllocateMemory;
		vmaVkFunctions.vkBindBufferMemory = vkBindBufferMemory;
		vmaVkFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
		vmaVkFunctions.vkBindImageMemory = vkBindImageMemory;
		vmaVkFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
		vmaVkFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
		vmaVkFunctions.vkCreateBuffer = vkCreateBuffer;
		vmaVkFunctions.vkCreateImage = vkCreateImage;
		vmaVkFunctions.vkDestroyBuffer = vkDestroyBuffer;
		vmaVkFunctions.vkDestroyImage = vkDestroyImage;
		vmaVkFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
		vmaVkFunctions.vkFreeMemory = vkFreeMemory;
		vmaVkFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		vmaVkFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
		vmaVkFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
		vmaVkFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
		vmaVkFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		vmaVkFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		vmaVkFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
		vmaVkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vmaVkFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		vmaVkFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
		vmaVkFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		vmaVkFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
		vmaVkFunctions.vkMapMemory = vkMapMemory;
		vmaVkFunctions.vkUnmapMemory = vkUnmapMemory;
	}


	VmaAllocatorCreateInfo createInfo{};
	createInfo.instance = instance;
	createInfo.physicalDevice = physicalDevice;
	createInfo.device = device;
	createInfo.vulkanApiVersion = vulkanApiVersion;
	createInfo.pVulkanFunctions = &vmaVkFunctions;
	createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	VkResult result = vmaCreateAllocator(&createInfo, &memoryAllocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Vulkan Memory Allocator!");
	}

    // create an example image creation struct to get the correct memory type for the pool
    VkImageCreateInfo exampleImgInfo{};
    exampleImgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    exampleImgInfo.imageType = VK_IMAGE_TYPE_2D;
    exampleImgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    exampleImgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    exampleImgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    exampleImgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    exampleImgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    exampleImgInfo.flags = 0;
    exampleImgInfo.mipLevels = 1;
    exampleImgInfo.arrayLayers = 1;
    exampleImgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    exampleImgInfo.queueFamilyIndexCount = 1;
    exampleImgInfo.pQueueFamilyIndices = &qfIndices.GraphicsFamily.value();
    exampleImgInfo.extent = VkExtent3D(1, 1, 1);

    VkExternalMemoryImageCreateInfo extMemInfo{ VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
#ifdef WIN32
    extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
    exampleImgInfo.pNext = &extMemInfo;


    // allocation for the example image 
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

    uint32_t memTypeIndex;
    vmaFindMemoryTypeIndexForImageInfo(memoryAllocator, &exampleImgInfo, &allocCreateInfo,
                                       &memTypeIndex);

    // create an extra pool which will handle memory and allocations for shared resources
    poolCreateInfo.memoryTypeIndex = memTypeIndex;

    std::memset(&exportMemAllocInfo, 0, sizeof(exportMemAllocInfo));
    exportMemAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN32
    exportMemAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportMemAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
    poolCreateInfo.pMemoryAllocateNext = &exportMemAllocInfo;

    result = vmaCreatePool(memoryAllocator, &poolCreateInfo, 
                                    &imageInteropPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create allocation pool for interop resources!");
    }
    else
{
        vmaSetPoolName(memoryAllocator, imageInteropPool, "Image Interop Pool");
    }

}

std::vector<const char*> Device::getRequiredInstanceExtensions() const
{
	// uint32_t glfwExtensionCount = 0;
	// const char** glfwExtensions;
	//
	// glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions; //(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool Device::areRequiredDeviceExtensionsSupported(VkPhysicalDevice device) const
{
	if (!VulkanUtils::areDeviceExtensionsAvailable(device, requiredDeviceExtensions))
	{
		return false;
	}
	// disable ray tracing extension queries, if renderdoc is attached
	// renderdoc disables them, so they can't be used
	// if (!VisVulkanUtils::isRenderDocAttached())
	// {
		if (!renderOffscreenOnly 
            && !VulkanUtils::areDeviceExtensionsAvailable(device, requiredOnScreenRenderingDeviceExtensions))
		{
			return false;
		}
	// }
	
	if (!VulkanUtils::areDeviceExtensionsAvailable(device, interopDeviceExtensions))
	{
		return false;
	}
	return true;
}

void Device::enableAvailableOptionalDeviceExtensions(std::vector<const char*>& deviceExtensions, const std::map<const char*, bool>& optionalDeviceExtensions) const
{
	for (std::pair<const char*, bool> optionalExtension : optionalDeviceExtensions)
	{
		const char* extension = optionalExtension.first;
		bool available = optionalExtension.second;

		if (available)
		{
			deviceExtensions.push_back(extension);
		}
	}
}

void Device::fetchQueues()
{
	// queue family indices have been fetched for logical device creation,
	// therefore they are populated here
	// TODO: maybe IsComplete is not the best query, but right now we only need the graphics family anyway
	assert(qfIndices.GraphicsFamily.has_value() && "Queue Family Index of Graphics Pipeline fetch"
		" was either unsuccessful or skipped!");

	// only a single queue is created, so index 0 is sufficient
	vkGetDeviceQueue(device, qfIndices.GraphicsFamily.value(), 0, &graphicsQueue);

	VulkanUtils::setDebugName(device, (uint64_t)graphicsQueue, VK_OBJECT_TYPE_QUEUE, "Graphics Queue");

	if (!renderOffscreenOnly)
	{
		assert(qfIndices.PresentFamily.has_value() && "Queue Family Index of Present Pipeline fetch"
			" was either unsuccessful or skipped!");
		vkGetDeviceQueue(device, qfIndices.PresentFamily.value(), 0, &presentQueue);
		VulkanUtils::setDebugName(device, (uint64_t)presentQueue, VK_OBJECT_TYPE_QUEUE, "Present Queue");
	}
}

