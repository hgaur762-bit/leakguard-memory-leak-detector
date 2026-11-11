#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H
#include "MemoryRecord.h"
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <cstdio>

class MemoryTracker {
    static constexpr std::size_t BucketCount = 1021;
    MemoryRecord* head_ = nullptr;
    MemoryRecord* buckets_[BucketCount]{};
    std::size_t allocated_ = 0, freed_ = 0, allocations_ = 0, deallocations_ = 0;
    std::size_t invalidFrees_ = 0, doubleFrees_ = 0;

    static std::size_t hash(void* p) noexcept {
        auto x = reinterpret_cast<std::uintptr_t>(p);
        x ^= x >> 17; x *= static_cast<std::uintptr_t>(0xed5ad4bbU);
        x ^= x >> 11;
        return static_cast<std::size_t>(x % BucketCount);
    }
    MemoryRecord* find(void* p) const noexcept {
        for (auto* r = buckets_[hash(p)]; r; r = r->hashNext)
            if (r->address == p) return r;
        return nullptr;
    }
public:
    MemoryTracker() = default;
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

    void recordAllocation(void* p, std::size_t n, const char* file = "Unknown", int line = 0) noexcept {
        if (!p) return;
        const auto slot = hash(p);
        // malloc avoids re-entering the overloaded global new operator.
        auto* r = static_cast<MemoryRecord*>(std::malloc(sizeof(MemoryRecord)));
        if (!r) return; // tracking must never make a successful allocation fail
        new (r) MemoryRecord(p, n, file, line);
        r->next = head_; head_ = r;
        r->hashNext = buckets_[slot]; buckets_[slot] = r;
        allocated_ += n; ++allocations_;
    }

    // Returns true only when the address is an active tracked allocation.
    bool recordDeallocation(void* p) noexcept {
        if (!p) return true;
        auto* r = find(p);
        if (!r) { ++invalidFrees_; return false; }
        if (r->freed) { ++doubleFrees_; return false; }
        r->freed = true; freed_ += r->size; ++deallocations_;
        return true;
    }
    MemoryRecord* searchAllocation(void* p) const noexcept { return find(p); }
    std::size_t invalidFreeCount() const noexcept { return invalidFrees_; }
    std::size_t doubleFreeCount() const noexcept { return doubleFrees_; }

    void generateReport(std::ostream& out = std::cout) const {
        out << "\n================ LEAKGUARD MEMORY AUDIT ================\n";
        out << std::left << std::setw(18) << "Address" << std::setw(12) << "Bytes"
            << std::setw(28) << "Source" << std::setw(8) << "Line" << "Status\n";
        out << "---------------------------------------------------------\n";
        std::size_t leaked = 0, leakedBytes = 0;
        for (auto* r = head_; r; r = r->next) {
            out << std::left << std::setw(18) << r->address << std::setw(12) << r->size
                << std::setw(28) << (r->file ? r->file : "Unknown") << std::setw(8) << r->line
                << (r->freed ? "FREED" : "LEAKED") << '\n';
            if (!r->freed) { ++leaked; leakedBytes += r->size; }
        }
        out << "---------------------------------------------------------\n"
            << "Allocations: " << allocations_ << " (" << allocated_ << " bytes)\n"
            << "Deallocations: " << deallocations_ << " (" << freed_ << " bytes)\n"
            << "Active leaks: " << leaked << " (" << leakedBytes << " bytes)\n"
            << "Invalid free attempts: " << invalidFrees_ << "\n"
            << "Repeated free attempts: " << doubleFrees_ << "\n"
            << "=========================================================\n";
    }
    bool exportCSV(const char* path) const noexcept {
        if (!path) return false;
        std::FILE* f = std::fopen(path, "w");
        if (!f) return false;
        std::fputs("address,bytes,source,line,status\n", f);
        for (auto* r = head_; r; r = r->next)
            std::fprintf(f, "%p,%zu,%s,%d,%s\n", r->address, r->size,
                         r->file ? r->file : "Unknown", r->line, r->freed ? "FREED" : "LEAKED");
        return std::fclose(f) == 0;
    }
    ~MemoryTracker() {
        auto* r = head_;
        while (r) { auto* next = r->next; r->~MemoryRecord(); std::free(r); r = next; }
    }
};
#endif
