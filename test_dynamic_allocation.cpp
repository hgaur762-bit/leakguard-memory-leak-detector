// #include <iostream>
// #include <cstring>

// // Important: Standard headers must be included before LeakGuard.h
// // so STL internal memory allocations are not rewritten by the macro.
// #include "LeakGuard.h"

// // Allow standalone single-file compilation with VS Code's Run / Play button
// // (which executes: g++ test_dynamic_allocation.cpp -o test_dynamic_allocation)
// #undef new
// #include "MemoryLeakDetector.cpp"
// #define new new(__FILE__, __LINE__)

// // Example custom struct
// struct SensorData {
//     int sensorId;
//     double reading;
//     char label[32];
// };

// static int failures = 0;
// static void check(bool condition, const char* description) {
//     if (condition) {
//         std::cout << "  [PASS] " << description << '\n';
//     } else {
//         std::cerr << "  [FAIL] " << description << '\n';
//         ++failures;
//     }
// }

// int main() {
//     std::cout << "========================================================\n";
//     std::cout << "      LEAKGUARD WORKING TEST CASE: DYNAMIC ALLOCATIONS  \n";
//     std::cout << "========================================================\n\n";

//     MemoryTracker& tracker = leakguardTracker();

//     // ---------------------------------------------------------------
//     // Test Case 1: Single primitive allocation & deallocation
//     // ---------------------------------------------------------------
//     std::cout << "Test 1: Single integer allocation & deallocation\n";
//     int* singleInt = new int(42);
//     check(singleInt != nullptr, "new int(42) succeeded");
//     check(*singleInt == 42, "Stored value is 42");

//     MemoryRecord* recInt = tracker.searchAllocation(singleInt);
//     check(recInt != nullptr, "Tracker recorded singleInt allocation");
//     check(recInt && recInt->size == sizeof(int), "Recorded size is sizeof(int)");
//     check(recInt && !recInt->freed, "Record status is initially NOT freed");

//     delete singleInt;
//     check(recInt && recInt->freed, "delete singleInt marked record as FREED");
//     std::cout << '\n';

//     // ---------------------------------------------------------------
//     // Test Case 2: Dynamic array allocation & array deallocation
//     // ---------------------------------------------------------------
//     std::cout << "Test 2: Dynamic double array allocation & deallocation\n";
//     const std::size_t arrayLen = 10;
//     double* dArray = new double[arrayLen];
//     for (std::size_t i = 0; i < arrayLen; ++i) {
//         dArray[i] = (i + 1) * 1.25;
//     }
//     check(dArray != nullptr, "new double[10] succeeded");

//     MemoryRecord* recArr = tracker.searchAllocation(dArray);
//     check(recArr != nullptr, "Tracker recorded array allocation");
//     check(recArr && recArr->size == arrayLen * sizeof(double), "Recorded size matches 10 * sizeof(double)");

//     delete[] dArray;
//     check(recArr && recArr->freed, "delete[] dArray marked record as FREED");
//     std::cout << '\n';

//     // ---------------------------------------------------------------
//     // Test Case 3: Custom struct allocation & deallocation
//     // ---------------------------------------------------------------
//     std::cout << "Test 3: Custom struct allocation & deallocation\n";
//     SensorData* sensor = new SensorData{101, 23.85, "Temperature-Probe-A"};
//     check(sensor != nullptr, "new SensorData succeeded");
//     check(sensor->sensorId == 101, "Sensor ID is correct");

//     MemoryRecord* recSensor = tracker.searchAllocation(sensor);
//     check(recSensor != nullptr, "Tracker recorded SensorData allocation");
//     check(recSensor && recSensor->size == sizeof(SensorData), "Recorded size matches sizeof(SensorData)");

//     delete sensor;
//     check(recSensor && recSensor->freed, "delete sensor marked record as FREED");
//     std::cout << '\n';

//     // ---------------------------------------------------------------
//     // Test Case 4: Intentional memory leak verification (Buffer)
//     // ---------------------------------------------------------------
//     std::cout << "Test 4: Intentional leak verification (256-byte buffer)\n";
//     char* leakedBuffer = new char[256];
//     std::strcpy(leakedBuffer, "This buffer is intentionally leaked for testing.");
//     MemoryRecord* recLeakBuf = tracker.searchAllocation(leakedBuffer);
//     check(recLeakBuf != nullptr, "Tracker recorded leakedBuffer allocation");
//     check(recLeakBuf && recLeakBuf->size == 256, "Recorded buffer size is 256 bytes");
//     check(recLeakBuf && !recLeakBuf->freed, "Status remains NOT freed (leak detected)");
//     std::cout << '\n';

//     // ---------------------------------------------------------------
//     // Test Case 5: Intentional memory leak verification (Integer)
//     // ---------------------------------------------------------------
//     std::cout << "Test 5: Intentional leak verification (integer counter)\n";
//     int* leakedNumber = new int(999);
//     MemoryRecord* recLeakNum = tracker.searchAllocation(leakedNumber);
//     check(recLeakNum != nullptr, "Tracker recorded leakedNumber allocation");
//     check(recLeakNum && recLeakNum->size == sizeof(int), "Recorded number size is sizeof(int)");
//     check(recLeakNum && !recLeakNum->freed, "Status remains NOT freed (leak detected)");
//     std::cout << '\n';

//     // ---------------------------------------------------------------
//     // Summary
//     // ---------------------------------------------------------------
//     if (failures == 0) {
//         std::cout << ">>> ALL TEST CASES PASSED SUCCESSFULLY! <<<\n";
//     } else {
//         std::cerr << ">>> TEST FAILURES: " << failures << " <<<\n";
//     }

//     std::cout << "\nExiting main(). LeakGuard shutdown hook will now generate the audit report...\n";
//     return 0;
// }

#include <iostream>
#include <cstring>
#include <cstdio>
#include "LeakGuard.h"
#ifndef LEAKGUARD_LINKED_SEPARATELY
#undef new
#include "MemoryLeakDetector.cpp"
#define new new(__FILE__, __LINE__)
#endif
struct Student {
    int rollNumber;
    float marks;
};

int main() {

    std::printf("\n===== LEAKGUARD TEST PROGRAM =====\n");

    // CASE 1: Dynamic allocation of a single integer

    std::printf("\n1. Allocating a single integer\n");

    int* number = new int(100);

    std::printf("Value: %d\n", *number);

    delete number;


    // CASE 2: Dynamic allocation of an array

    std::printf("\n2. Allocating an integer array\n");

    int* marks = new int[5];

    for (int index = 0; index < 5; index++) {
        marks[index] = (index + 1) * 10;
        std::printf("%d ", marks[index]);
    }

    std::printf("\n");

    delete[] marks;


    // CASE 3: Dynamic allocation of an object

    std::printf("\n3. Allocating a Student object\n");

    Student* student = new Student;

    student->rollNumber = 101;
    student->marks = 92.5f;

    std::printf("Roll Number: %d\n", student->rollNumber);
    std::printf("Marks: %.2f\n", student->marks);

    delete student;


    // CASE 4: Dynamic allocation of multiple objects

    std::printf("\n4. Allocating multiple Student objects\n");

    Student* students = new Student[3];

    for (int index = 0; index < 3; index++) {
        students[index].rollNumber = index + 1;
        students[index].marks = 80 + index * 5;
    }

    for (int index = 0; index < 3; index++) {
        std::printf("Student %d: %.2f marks\n",
                    students[index].rollNumber,
                    students[index].marks);
    }

    delete[] students;


    // CASE 5: Intentional memory leak

    std::printf("\n5. Creating an intentional memory leak\n");

    int* leakedNumber = new int(500);

    std::printf("Allocated value: %d\n", *leakedNumber);

    // Intentionally do not delete leakedNumber; LeakGuard should report this 4-byte leak.


    std::printf("\n===== TEST PROGRAM FINISHED =====\n");

    return 0;
}
