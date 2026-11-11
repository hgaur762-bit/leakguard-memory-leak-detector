#include <iostream>
#include <cstring>

// Important: Standard headers should be included before LeakGuard.h
// so that STL internal allocations are not rewritten by the macro.
#include "LeakGuard.h"

// Allow standalone single-file compilation with VS Code's Run / Play button
// (which executes: g++ demo.cpp -o demo)
#undef new
#include "MemoryLeakDetector.cpp"
#define new new(__FILE__, __LINE__)

// Example custom struct
struct SensorData {
    int sensorId;
    double reading;
    char label[32];
};

void runDynamicAllocationDemo() {
    std::cout << "====================================================\n";
    std::cout << "      LEAKGUARD DYNAMIC ALLOCATION DEMO             \n";
    std::cout << "====================================================\n\n";

    // ---------------------------------------------------------------
    // 1. Single primitive dynamic allocation (Properly freed)
    // ---------------------------------------------------------------
    std::cout << "[1] Allocating single integer...\n";
    int* singleInt = new int(42);
    std::cout << "    Address: " << singleInt << " | Value: " << *singleInt << "\n";
    delete singleInt; // Expected: FREED in audit
    std::cout << "    -> Freed single integer successfully.\n\n";

    // ---------------------------------------------------------------
    // 2. Dynamic array allocation (Properly freed)
    // ---------------------------------------------------------------
    std::cout << "[2] Allocating dynamic array of 10 doubles (80 bytes)...\n";
    double* dArray = new double[10];
    for (int i = 0; i < 10; ++i) {
        dArray[i] = (i + 1) * 1.25;
    }
    std::cout << "    Array[0] = " << dArray[0] << ", Array[9] = " << dArray[9] << "\n";
    delete[] dArray; // Expected: FREED in audit
    std::cout << "    -> Freed array successfully.\n\n";

    // ---------------------------------------------------------------
    // 3. Custom struct dynamic allocation (Properly freed)
    // ---------------------------------------------------------------
    std::cout << "[3] Allocating custom SensorData struct...\n";
    SensorData* sensor = new SensorData{101, 23.85, "Temperature-Probe-A"};
    std::cout << "    Sensor ID: " << sensor->sensorId
              << " | Reading: " << sensor->reading
              << " | Label: " << sensor->label << "\n";
    delete sensor; // Expected: FREED in audit
    std::cout << "    -> Freed SensorData struct successfully.\n\n";

    // ---------------------------------------------------------------
    // 4. Intentional Memory Leak #1: Character buffer (NOT freed)
    // ---------------------------------------------------------------
    std::cout << "[4] Allocating 256-byte character buffer (INTENTIONALLY LEAKED)...\n";
    char* leakedBuffer = new char[256];
    std::strcpy(leakedBuffer, "This buffer was never deleted!");
    std::cout << "    Buffer content: \"" << leakedBuffer << "\"\n";
    std::cout << "    -> Intentionally omitting delete[] to test leak detection!\n\n";

    // ---------------------------------------------------------------
    // 5. Intentional Memory Leak #2: Integer counter (NOT freed)
    // ---------------------------------------------------------------
    std::cout << "[5] Allocating an integer (INTENTIONALLY LEAKED)...\n";
    int* leakedCounter = new int(999);
    std::cout << "    Counter Address: " << leakedCounter << " | Value: " << *leakedCounter << "\n";
    std::cout << "    -> Intentionally omitting delete to test leak detection!\n\n";

    std::cout << "Demo execution completed.\n";
    std::cout << "Returning from main(). The LeakGuard shutdown hook will now:\n";
    std::cout << "  1. Print the detailed Memory Audit report.\n";
    std::cout << "  2. Save results to 'leakguard_report.csv'.\n";
    std::cout << "  3. Launch the graphical dashboard ('leakguard_gui').\n";
}

int main() {
    runDynamicAllocationDemo();
    return 0;
}
