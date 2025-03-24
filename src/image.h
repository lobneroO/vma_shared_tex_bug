
#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

#include "handle.h"

class Device;

class Image
{
public:
    Image(Device* device, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags);
    ~Image();

    Handle getExternalHandle() const;

private:
    void createImage(const VkImageCreateInfo& createInfo);
    void createImageView();
    void createSampler();

    VkImageCreateInfo getImageCreateInfo();
    void setupExternalInfo();
    /**
    * Sets up the HANDLE for external access
    */
    void setupExternalAccess();

private:
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    uint32_t width = 1;
    uint32_t height = 1;

    VkImageUsageFlags usageFlags;

    Device* device = nullptr;

    /**
    * External memory access handle (can be used by OpenGL, CUDA, ...)
    */
    Handle externalHandle = 0;
    /** 
    * Export handle setup.
    */
    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo;
};
