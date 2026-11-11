#include <iostream>
#include <cstring>

// Important: Standard headers must be included before LeakGuard.h
// so STL internal memory allocations are not rewritten by the macro.
#include "LeakGuard.h"

// Allow standalone single-file compilation with VS Code's Run / Play button
// (which executes: g++ test_dynamic_allocation.cpp -o test_dynamic_allocation)
#undef new
#include "MemoryLeakDetector.cpp"
#define new new(__FILE__, __LINE__)

// Example custom struct
struct SensorData {
    int sensorId;
    double reading;
    char label[32];
};

static int failures = 0;
static void check(bool condition, const char* description) {
    if (condition) {
        std::cout << "  [PASS] " << description << '\n';
    } else {
        std::cerr << "  [FAIL] " << description << '\n';
        ++failures;
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "      LEAKGUARD WORKING TEST CASE: DYNAMIC ALLOCATIONS  \n";
    std::cout << "========================================================\n\n";

    MemoryTracker& tracker = leakguardTracker();

    // ---------------------------------------------------------------
    // Test Case 1: Single primitive allocation & deallocation
    // ---------------------------------------------------------------
    std::cout << "Test 1: Single integer allocation & deallocation\n";
    int* singleInt = new int(42);
    check(singleInt != nullptr, "new int(42) succeeded");
    check(*singleInt == 42, "Stored value is 42");

    MemoryRecord* recInt = tracker.searchAllocation(singleInt);
    check(recInt != nullptr, "Tracker recorded singleInt allocation");
    check(recInt && recInt->size == sizeof(int), "Recorded size is sizeof(int)");
    check(recInt && !recInt->freed, "Record status is initially NOT freed");

    delete singleInt;
    check(recInt && recInt->freed, "delete singleInt marked record as FREED");
    std::cout << '\n';

    // ---------------------------------------------------------------
    // Test Case 2: Dynamic array allocation & array deallocation
    // ---------------------------------------------------------------
    std::cout << "Test 2: Dynamic double array allocation & deallocation\n";
    const std::size_t arrayLen = 10;
    double* dArray = new double[arrayLen];
    for (std::size_t i = 0; i < arrayLen; ++i) {
        dArray[i] = (i + 1) * 1.25;
    }
    check(dArray != nullptr, "new double[10] succeeded");

    MemoryRecord* recArr = tracker.searchAllocation(dArray);
    check(recArr != nullptr, "Tracker recorded array allocation");
    check(recArr && recArr->size == arrayLen * sizeof(double), "Recorded size matches 10 * sizeof(double)");

    delete[] dArray;
    check(recArr && recArr->freed, "delete[] dArray marked record as FREED");
    std::cout << '\n';

    // ---------------------------------------------------------------
    // Test Case 3: Custom struct allocation & deallocation
    // ---------------------------------------------------------------
    std::cout << "Test 3: Custom struct allocation & deallocation\n";
    SensorData* sensor = new SensorData{101, 23.85, "Temperature-Probe-A"};
    check(sensor != nullptr, "new SensorData succeeded");
    check(sensor->sensorId == 101, "Sensor ID is correct");

    MemoryRecord* recSensor = tracker.searchAllocation(sensor);
    check(recSensor != nullptr, "Tracker recorded SensorData allocation");
    check(recSensor && recSensor->size == sizeof(SensorData), "Recorded size matches sizeof(SensorData)");

    delete sensor;
    check(recSensor && recSensor->freed, "delete sensor marked record as FREED");
    std::cout << '\n';

    // ---------------------------------------------------------------
    // Test Case 4: Intentional memory leak verification (Buffer)
    // ---------------------------------------------------------------
    std::cout << "Test 4: Intentional leak verification (256-byte buffer)\n";
    char* leakedBuffer = new char[256];
    std::strcpy(leakedBuffer, "This buffer is intentionally leaked for testing.");
    MemoryRecord* recLeakBuf = tracker.searchAllocation(leakedBuffer);
    check(recLeakBuf != nullptr, "Tracker recorded leakedBuffer allocation");
    check(recLeakBuf && recLeakBuf->size == 256, "Recorded buffer size is 256 bytes");
    check(recLeakBuf && !recLeakBuf->freed, "Status remains NOT freed (leak detected)");
    std::cout << '\n';

    // ---------------------------------------------------------------
    // Test Case 5: Intentional memory leak verification (Integer)
    // ---------------------------------------------------------------
    std::cout << "Test 5: Intentional leak verification (integer counter)\n";
    int* leakedNumber = new int(999);
    MemoryRecord* recLeakNum = tracker.searchAllocation(leakedNumber);
    check(recLeakNum != nullptr, "Tracker recorded leakedNumber allocation");
    check(recLeakNum && recLeakNum->size == sizeof(int), "Recorded number size is sizeof(int)");
    check(recLeakNum && !recLeakNum->freed, "Status remains NOT freed (leak detected)");
    std::cout << '\n';

    // ---------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------
    if (failures == 0) {
        std::cout << ">>> ALL TEST CASES PASSED SUCCESSFULLY! <<<\n";
    } else {
        std::cerr << ">>> TEST FAILURES: " << failures << " <<<\n";
    }

    std::cout << "\nExiting main(). LeakGuard shutdown hook will now generate the audit report...\n";
    return 0;
}
