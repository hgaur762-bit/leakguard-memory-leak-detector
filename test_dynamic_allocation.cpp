
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
