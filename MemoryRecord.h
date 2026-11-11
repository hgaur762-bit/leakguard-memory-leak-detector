#ifndef MEMORY_RECORD_H
#define MEMORY_RECORD_H
#include <cstddef>

struct MemoryRecord {
    void* address;
    std::size_t size;
    bool freed;
    const char* file;
    int line;
    MemoryRecord* next;       // report traversal
    MemoryRecord* hashNext;   // collision chain

    MemoryRecord(void* p, std::size_t n, const char* f, int l) noexcept
        : address(p), size(n), freed(false), file(f), line(l), next(nullptr), hashNext(nullptr) {}
};
#endif
