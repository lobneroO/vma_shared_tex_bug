
#include "image.h"

#include "device.h"

Image::Image(Device* device, uint32_t width, uint32_t height)
    : device(device), width(width), height(height)
{
    createImage();
    createImageView();
    createSampler();
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

void Image::createImage()
{

}

void Image::createImageView()
{

}

void Image::createSampler()
{

}

