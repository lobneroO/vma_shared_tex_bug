
#include "third_party_setup.h" // IWYU pragma: export

#include <cstdint>
#include <iostream>

#include "device.h"
#include "image.h"

int main(int argc, char** argv)
{
    uint32_t id = UINT32_MAX;
    if (argc > 2 && 
        (strcmp(argv[1], "-d") == 0
        || strcmp(argv[1], "--device") == 0))
    {
        id = std::atoi(argv[2]);
    }
    else if(argc > 1)
    {
        if(strcmp(argv[1], "-h") == 0
            || strcmp(argv[1], "--help") == 0)
        {
            std::cout << "Bug reproduction for creating small images with a shared handle" << std::endl;
            std::cout << "Observe that for \"small\" images, the handle is always the same" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage:" << std::endl;
            std::cout << "VmaSharedTexBug.exe" << std::endl;
            std::cout << "\texecute without parameters, the program will choose the best graphics card it can find" << std::endl;
            std::cout << "\t-l || --list-devices" << std::endl;
            std::cout << "\t\t Will print a list of devices and their Ids, if you want to choose one" << std::endl;
            std::cout << "\t-d <id> || --device <id>" << std::endl;
            std::cout << "\t\t Choose a device by id" << std::endl;
            std::cout << "\t\t (There is no input sanitation for this bug repro...)" << std::endl;
            std::cout << "\t-h || --help Print this help" << std::endl;

            return 0;
        }
        else if (strcmp(argv[1], "-l") == 0
            || strcmp(argv[1], "--list-devices") == 0)
        {
            std::cout << "available devices:" << std::endl;
            auto devices = Device::getDevices();
            for(auto device : devices)
            {
                std::cout << std::to_string(device.first) + " : " + device.second << std::endl;
            }

            return 0;
        }
    }
    else 
    {
        std::cout << "There are unknown parameters. Use -h or --help for more information." << std::endl;
        return -1;
    }
    Device device(id);

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

    // these asserts pass on several NVidia GPUs, but not on the integrated Intel iGPU
    // still, large image
    assert(img2.getExternalHandle() != imgSmall1.getExternalHandle());
    assert(img2.getExternalHandle() != imgSmall2.getExternalHandle());

    // this assert fails on both NVidia and Intel iGPU
    // but the small images all have the exact same handle
    assert(imgSmall1.getExternalHandle() != imgSmall2.getExternalHandle());

    return 0;
}
