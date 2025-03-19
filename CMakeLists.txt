
# setup the project
cmake_minimum_required(VERSION 3.20)
project(VmaSharedTexBug VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_FILE_LIST
	src/main.cpp
    src/third_party_setup.h

    src/device.h
    src/device.cpp

    src/string_utils.h
    src/string_utils.cpp
    src/vulkan_utils.h
    src/vulkan_utils.cpp
)

include_directories(
	${PROJECT_NAME}	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(${PROJECT_NAME}
	${SOURCE_FILE_LIST}
)

### THIRD PARTY ###
# setup Vulkan
set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
message("build type: ${BUILD_TYPE}")
IF(WIN32)
	IF("${BUILD_TYPE}" STREQUAL "")
		set(BUILD_TYPE "Debug")
	ENDIF()
ENDIF()
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

message(${Vulkan_shaderc_combined_LIBRARY})
string(REPLACE ".lib" "d.lib" SHADERC_DEBUG ${Vulkan_shaderc_combined_LIBRARY})

IF(WIN32)
	# the debug version of shaderc doesn't ship with .pdb files in the VulkanSDK
	# therefore supress the warning about missing pdb files
    target_link_options(${PROJECT_NAME} PUBLIC "/ignore:4099")
ENDIF()

target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC 
	debug ${SHADERC_DEBUG}
	optimized ${Vulkan_shaderc_combined_LIBRARY})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})

# add Vulkan Memory Allocator (for Vulkan memory management)
add_subdirectory(3rdparty/VulkanMemoryAllocator)
target_include_directories(
	${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/VulkanMemoryAllocator/include
)

# add Volk instead of wrapping every vulkan extension function manually
# in order to use external memory handles from vulkan (for OptiX), volk needs a define telling it to use those external handles
IF(WIN32)
	# VK_USE_PLATFORM_WIN32_KHR
	add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
	# Due to the use of OptiX in some cases and VK_USE_PLATFORM_WIN32_KHR in others,
	# the windows.h gets pulled into the project. This leads to macro overwrites of project contents,
	# such as the loggers ERROR severity. To avoid this, use the NOGDI define
	add_definitions(-DNOGDI)
	# The same problem arises with std::numeric_limits<>::max, since windows also overwrites these.
	# Prevent this with the NOMINMAX define
	add_definitions(-DNOMINMAX)
ENDIF()
add_subdirectory(3rdparty/volk)
target_link_libraries(${PROJECT_NAME} PRIVATE volk_headers)
