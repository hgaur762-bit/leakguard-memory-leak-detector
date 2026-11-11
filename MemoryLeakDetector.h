#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H
#include "MemoryTracker.h"
#include <cstddef>

MemoryTracker& leakguardTracker() noexcept;
void* operator new(std::size_t size);
void* operator new[](std::size_t size);
void operator delete(void* p) noexcept;
void operator delete[](void* p) noexcept;
void operator delete(void* p, std::size_t size) noexcept;
void operator delete[](void* p, std::size_t size) noexcept;
void* operator new(std::size_t size, const char* file, int line);
void* operator new[](std::size_t size, const char* file, int line);
void operator delete(void* p, const char* file, int line) noexcept;
void operator delete[](void* p, const char* file, int line) noexcept;

#ifndef LEAKGUARD_NO_MACRO
#define new new(__FILE__, __LINE__)
#endif
#endif
