#define LEAKGUARD_NO_MACRO
#include "LeakGuard.h"
#include <cstdlib>
#include <iostream>

static int failures = 0;
static void check(bool passed, const char* message) {
    if (!passed) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

int main() {
    MemoryTracker& tracker = leakguardTracker();
    void* a = std::malloc(32);
    void* b = std::malloc(64);
    check(a && b, "test allocations succeeded");
    if (!a || !b) { std::free(a); std::free(b); return 1; }
    tracker.recordAllocation(a, 32, "test.cpp", 14);
    tracker.recordAllocation(b, 64, "test.cpp", 15);
    check(tracker.searchAllocation(a) != nullptr, "hash table finds an allocation");
    check(tracker.recordDeallocation(a), "first free is accepted");
    check(!tracker.recordDeallocation(a), "repeated free is detected");
    int invalidToken = 0;
    check(!tracker.recordDeallocation(&invalidToken), "unknown address is rejected");
    check(tracker.invalidFreeCount() == 1, "invalid-free count is one");
    check(tracker.doubleFreeCount() == 1, "repeated-free count is one");
    std::free(a); // tracked as freed above; b remains the intentional leak example

    int* value = new int(42);
    check(*value == 42, "overloaded new returns the requested value");
    MemoryRecord* valueRecord = tracker.searchAllocation(value);
    check(valueRecord != nullptr, "global new hook records the allocation");
    delete value;
    check(valueRecord && valueRecord->freed, "global delete hook marks the allocation freed");

    if (failures == 0)
        std::cout << "\nLeakGuard tests passed: hash lookup, allocation hooks, leak report, repeated-free and invalid-free checks.\n";
    else
        std::cerr << "\nLeakGuard tests failed: " << failures << " check(s). The report and dashboard will still open.\n";
    return failures == 0 ? 0 : 1;
}
