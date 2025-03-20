
#pragma once

// ======================================
// === initialize single header files ===
// ======================================

#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#include "volk.h"

// on clang, VMA throws a lot of nullable warnings
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// reenable the nullability warnings
#ifdef __clang__
#pragma clang pop
#endif
