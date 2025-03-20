
#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

class Device;

class Image
{
public:
    Image(Device* device, uint32_t width, uint32_t height);
    ~Image();

private:
    void createImage();
    void createImageView();
    void createSampler();

private:
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    uint32_t width = 1;
    uint32_t height = 1;

    Device* device = nullptr;
};
