
#include "image.h"

#include <cassert>
#include <stdexcept>

#include "device.h"

Image::Image(Device* device, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags)
    : device(device), width(width), height(height), usageFlags(usageFlags)
{
    setupExternalInfo();

    VkImageCreateInfo createInfo = getImageCreateInfo();
    createImage(createInfo);
    createImageView();
    createSampler();
    setupExternalAccess();
}

Image::~Image()
{
    if(sampler)
    {
        vkDestroySampler(device->getDevice(), sampler, nullptr);
    }
    if(imageView)
    {
        vkDestroyImageView(device->getDevice(), imageView, nullptr);
    }
    if(image)
    {
        vmaDestroyImage(device->getAllocator(), image, allocation);
    }
}

Handle Image::getExternalHandle() const
{
    return externalHandle;
}

void Image::createImage(const VkImageCreateInfo& createInfo)
{
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.pool = device->getSharedPool();

    // vmaCreate also does the allocation and image binding
    VkResult result = vmaCreateImage(device->getAllocator(), &createInfo, &allocInfo,
    	&image, &allocation, nullptr);
    if (result != VK_SUCCESS)
    {
    	throw std::runtime_error("VMA could not create image!");
    }
   
    // fetch the size of the image for the import of others
    VmaAllocationInfo alloc;
    vmaGetAllocationInfo(device->getAllocator(), allocation, &alloc);
    // size_bytes = alloc.size;
}

void Image::createImageView()
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

    // swizzle setup
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device->getDevice(), &createInfo, nullptr, &imageView);

    if (result != VK_SUCCESS)
    {
	    throw std::runtime_error("Could not create image view!");
    }

}

void Image::createSampler()
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // TODO: make this switchable for a full screen texture that is rendered full screen
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_FALSE; // TODO: enable? VK_TRUE;
    //PhysicalDeviceProperties.properties.limits.maxSamplerAnisotropy;
    // std::cout << "untested max sampler anisotropy!" << std::endl;
    createInfo.maxAnisotropy = 1.0; //maxSamplerAnisotropy;

    createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.f;
    createInfo.minLod = 0.f;
    createInfo.maxLod = 0.f;

    VkResult result = vkCreateSampler(device->getDevice(), &createInfo, nullptr, &sampler);
    if (result != VK_SUCCESS)
    {
	    throw std::runtime_error("Could not create Sampler!");
    }
}

VkImageCreateInfo Image::getImageCreateInfo()
{
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.extent = VkExtent3D{width, height, 1};
    createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usageFlags;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.flags = 0;
    // external memory setup should have been done at this point
    assert(externalMemoryImageCreateInfo.sType == VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO);
    createInfo.pNext = &externalMemoryImageCreateInfo;
    return createInfo;

}

void Image::setupExternalInfo()
{
	externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	externalMemoryImageCreateInfo.pNext = nullptr;
	externalMemoryImageCreateInfo.handleTypes =
#ifdef WIN32
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;     // Windows handleTypes
#else
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;        // Linux handleTypes
#endif
}

void Image::setupExternalAccess()
{
	// VkImage is a setup of a buffer associated with data on how to interpret the buffer data
	// VkImageView is the interpretation and VkDeviceMemory is the underlying data
	// Therefore get the VkDeviceMemory here and import it into CUDA
	VmaAllocationInfo alloc;
	vmaGetAllocationInfo(device->getAllocator(), allocation, &alloc);
	const VkDeviceMemory& sharedDeviceMem = alloc.deviceMemory;

#if _WIN32
	VkMemoryGetWin32HandleInfoKHR winHandleInfo{};
	winHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	winHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	winHandleInfo.memory = sharedDeviceMem;

	VkResult result = vkGetMemoryWin32HandleKHR(device->getDevice(), &winHandleInfo, &externalHandle);
#else
	VkMemoryGetFdInfoKHR memoryFdInfo{};
	memoryFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	memoryFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
	memoryFdInfo.memory = sharedDeviceMem;

	VkResult result = vkGetMemoryFdKHR(device->getDevice(), &memoryFdInfo, &externalHandle);
#endif
}
