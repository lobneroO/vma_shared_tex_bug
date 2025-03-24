
#include "third_party_setup.h" // IWYU pragma: export

#include <cstdint>

#include "device.h"
#include "image.h"

int main()
{
    Device device;

    uint32_t largeLength = 512;
    // don't know the exact side length that will lead to a crash,
    // I could reproduce it with ~300 and lower
    uint32_t smallLength = 32;
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    Image img1(&device, largeLength, largeLength, usageFlags);
    Image img2(&device, largeLength, largeLength, usageFlags);
    Image imgSmall1(&device, smallLength, smallLength, usageFlags);
    Image imgSmall2(&device, smallLength, smallLength, usageFlags);

    // these asserts pass
    // large images have different handles, as expected
    assert(img1.getExternalHandle() != img2.getExternalHandle());
    assert(img1.getExternalHandle() != imgSmall1.getExternalHandle());
    assert(img1.getExternalHandle() != imgSmall2.getExternalHandle());

    // these asserts also pass
    // still, large image
    assert(img2.getExternalHandle() != imgSmall1.getExternalHandle());
    assert(img2.getExternalHandle() != imgSmall2.getExternalHandle());

    // this assert fails
    // but the small images all have the exact same handle
    assert(imgSmall1.getExternalHandle() != imgSmall2.getExternalHandle());

    return 0;
}
