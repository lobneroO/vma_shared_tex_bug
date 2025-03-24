
#pragma once

#include <volk.h>

#if defined(WIN32)
using Handle = HANDLE;
#else
using Handle                          = int;
constexpr Handle INVALID_HANDLE_VALUE = static_cast<Handle>(-1);
#endif
