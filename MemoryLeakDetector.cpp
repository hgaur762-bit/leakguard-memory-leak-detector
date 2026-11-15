
#define LEAKGUARD_NO_MACRO
#include "MemoryLeakDetector.h"
#include <cstdlib>
#include <new>
#include <cstdio>// for using gui report generation and fflush
#include <cstdint>
#include <cstring>
#include <limits.h>

//for windows as in windows it automatcally activates this macros 
#ifdef _WIN32
    #include <windows.h>

//for mac system
#else
    #include <mach-o/dyld.h>
    #include <spawn.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif


//to get the environment variables to be used for running GUI code 
#ifndef _WIN32
extern char **environ;
#endif

//used to check recursive tracking 
static thread_local bool insideHook = false;



//this function is used to create a memory tracker object in storage block with name tracker
static MemoryTracker* trackerInstance(bool create) noexcept {
    alignas(MemoryTracker) static unsigned char storage[sizeof(MemoryTracker)];
    static MemoryTracker* tracker = nullptr;
    if (!tracker && create) tracker = ::new (storage) MemoryTracker();
    return tracker;
}


//to get the tracker object 
MemoryTracker& leakguardTracker() noexcept {
     return *trackerInstance(true);
    }


//to check for active object of tracker and making allocation in it 
static void track(void* p, std::size_t size, const char* file = "Unknown", int line = 0) noexcept {
    if (!insideHook) {
        insideHook = true;
        leakguardTracker().recordAllocation(p, size, file, line);
        insideHook = false;
    }
}


static void finishSession() noexcept {
    MemoryTracker* tracker = trackerInstance(false);
    if (!tracker) return;

    insideHook = true;
    tracker->generateReport();
    std::fflush(stdout);

#ifdef _WIN32

    char executable[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, executable, MAX_PATH);

    if (length == 0 || length >= MAX_PATH){
        insideHook = false;
        return;
    }

   char* slash = std::strrchr(executable, '\\');

    if (!slash){
    slash = std::strrchr(executable, '/');
    }

    if (!slash){
        insideHook = false;
        return;
    }

    *slash = '\0';

    char reportPath[MAX_PATH];

    std::snprintf(reportPath,sizeof(reportPath),"%s\\leakguard_report.csv",executable);

    if(!tracker->exportCSV(reportPath)) {
        std::fprintf(stderr, "LeakGuard: could not write report.\n");
    } 
    else{
        std::printf("\nLeakGuard: Report saved at %s\n", reportPath);
        std::printf("Build the GUI separately to view the dashboard.\n");
    }

#else

    char executable[PATH_MAX];
    std::uint32_t length = sizeof(executable);

    if (_NSGetExecutablePath(executable, &length) != 0){
        insideHook = false;
        return;
    }

    char resolved[PATH_MAX];

    if (!realpath(executable, resolved))
        std::snprintf(resolved, sizeof(resolved), "%s", executable);

    char* slash = std::strrchr(resolved, '/');

    if (!slash){
        insideHook = false;
        return;
    }

    *slash = '\0';

    char reportPath[PATH_MAX], guiPath[PATH_MAX];

    std::snprintf(reportPath, sizeof(reportPath),
                  "%s/leakguard_report.csv", resolved);

    std::snprintf(guiPath, sizeof(guiPath),
                  "%s/leakguard_gui", resolved);

    if (!tracker->exportCSV(reportPath)) {
        std::fprintf(stderr, "LeakGuard: could not write report.\n");
        insideHook = false;
        return;
    }

    insideHook = false;

    if (access(guiPath, X_OK) == 0) {
        pid_t child = 0;
        char* const args[] = {guiPath, nullptr};

        if (posix_spawn(&child, guiPath, nullptr, nullptr,
                        args, environ) != 0)
            std::fprintf(stderr, "LeakGuard: GUI could not be opened.\n");
    } else {
        std::fprintf(stderr, "LeakGuard: Report saved.\n");
    }

#endif

    insideHook = false;
}



//this is made to automatically generate a report when th porgram terminates! as it is calling the finishSession() function
struct LeakGuardShutdown{
    ~LeakGuardShutdown(){
        finishSession();
    }
};
static LeakGuardShutdown shutdownHook;



//operator overloading of NEW AND DELETE
void* operator new(std::size_t size) {
    if (void* p = std::malloc(size ? size : 1)){
        track(p, size);
        return p;
    }

    throw std::bad_alloc();
}


void* operator new[](std::size_t size){
    if (void* p = std::malloc(size ? size : 1)){
        track(p, size);
        return p;
    }

    throw std::bad_alloc();
}


void operator delete(void* p)noexcept{
    if (!p) return;
    if (!insideHook){
        insideHook = true;
        auto* record = leakguardTracker().searchAllocation(p);
        const bool release = !record || (!record->freed && leakguardTracker().recordDeallocation(p));
        insideHook = false;
        if (!release){
            return;
        }
    }
    std::free(p);
}


void operator delete[](void* p) noexcept{
    ::operator delete(p);
}


void operator delete(void* p, std::size_t)noexcept{
    ::operator delete(p);
}

void operator delete[](void* p, std::size_t)noexcept{
    ::operator delete(p);
}

void* operator new(std::size_t size, const char* file, int line){
    if (void* p = std::malloc(size ? size : 1)){ 
        track(p, size, file, line);
         return p;
    }
    throw std::bad_alloc();
}


void* operator new[](std::size_t size, const char* file, int line) {
    if (void* p = std::malloc(size ? size : 1)){
        track(p, size, file, line);
        return p;
    }

    throw std::bad_alloc();
}


void operator delete(void* p, const char*, int)noexcept{
    std::free(p);
}


void operator delete[](void* p, const char*, int)noexcept{
    std::free(p);
}

