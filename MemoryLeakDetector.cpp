#define LEAKGUARD_NO_MACRO
#include "MemoryLeakDetector.h"
#include <cstdlib>
#include <new>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits.h>
#include <mach-o/dyld.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static thread_local bool insideHook = false;

static MemoryTracker* trackerInstance(bool create) noexcept {
    alignas(MemoryTracker) static unsigned char storage[sizeof(MemoryTracker)];
    static MemoryTracker* tracker = nullptr;
    if (!tracker && create) tracker = ::new (storage) MemoryTracker();
    return tracker;
}
MemoryTracker& leakguardTracker() noexcept { return *trackerInstance(true); }

static void track(void* p, std::size_t size, const char* file = "Unknown", int line = 0) noexcept {
    if (!insideHook) { insideHook = true; leakguardTracker().recordAllocation(p, size, file, line); insideHook = false; }
}

static bool buildDashboard(const char* executable, const char* source) noexcept {
    const char* prefix = nullptr;
    if (access("/opt/homebrew/include/SDL2/SDL.h", R_OK) == 0) prefix = "/opt/homebrew";
    else if (access("/usr/local/include/SDL2/SDL.h", R_OK) == 0) prefix = "/usr/local";
    if (!prefix) {
        std::fprintf(stderr, "LeakGuard: SDL2 headers not found; install SDL2 to build the GUI.\n");
        return false;
    }

    char includeArg[PATH_MAX], libraryArg[PATH_MAX];
    std::snprintf(includeArg, sizeof(includeArg), "-I%s/include/SDL2", prefix);
    std::snprintf(libraryArg, sizeof(libraryArg), "-L%s/lib", prefix);
    char* const args[] = {
        const_cast<char*>("clang"), const_cast<char*>("-std=c11"),
        const_cast<char*>("-Wall"), const_cast<char*>("-Wextra"),
        includeArg, const_cast<char*>(source), libraryArg,
        const_cast<char*>("-lSDL2"), const_cast<char*>("-lm"),
        const_cast<char*>("-o"), const_cast<char*>(executable), nullptr
    };
    pid_t child = 0;
    const int spawnError = posix_spawnp(&child, "clang", nullptr, nullptr, args, environ);
    if (spawnError != 0) {
        std::fprintf(stderr, "LeakGuard: could not start clang to build the GUI (error %d).\n", spawnError);
        return false;
    }
    int childStatus = 0;
    if (waitpid(child, &childStatus, 0) < 0 || !WIFEXITED(childStatus) || WEXITSTATUS(childStatus) != 0) {
        std::fprintf(stderr, "LeakGuard: GUI build failed; the CSV report is still saved.\n");
        return false;
    }
    return access(executable, X_OK) == 0;
}

static void finishSession() noexcept {
    MemoryTracker* tracker = trackerInstance(false);
    if (!tracker) return;
    insideHook = true;
    tracker->generateReport();
    std::fflush(stdout);

    char executable[PATH_MAX];
    std::uint32_t length = sizeof(executable);
    if (_NSGetExecutablePath(executable, &length) != 0) {
        insideHook = false;
        return;
    }
    char resolved[PATH_MAX];
    if (!realpath(executable, resolved)) std::snprintf(resolved, sizeof(resolved), "%s", executable);
    char* slash = std::strrchr(resolved, '/');
    if (!slash) { insideHook = false; return; }
    *slash = '\0';
    char reportPath[PATH_MAX], guiPath[PATH_MAX], guiSource[PATH_MAX];
    std::snprintf(reportPath, sizeof(reportPath), "%s/leakguard_report.csv", resolved);
    std::snprintf(guiPath, sizeof(guiPath), "%s/leakguard_gui", resolved);
    std::snprintf(guiSource, sizeof(guiSource), "%s/LeakGuardGUI.c", resolved);
    if (!tracker->exportCSV(reportPath)) {
        std::fprintf(stderr, "LeakGuard: could not write report: %s\n", reportPath);
        insideHook = false;
        return;
    }
    insideHook = false;

    if (access(guiSource, R_OK) != 0) {
        std::fprintf(stderr, "LeakGuard: report saved, but LeakGuardGUI.c is missing beside the test executable.\n");
        return;
    }
    if (!buildDashboard(guiPath, guiSource)) return;
    pid_t child = 0;
    char* const args[] = {guiPath, nullptr};
    if (posix_spawn(&child, guiPath, nullptr, nullptr, args, environ) != 0)
        std::fprintf(stderr, "LeakGuard: report saved, but the GUI could not be opened.\n");
}

struct LeakGuardShutdown {
    ~LeakGuardShutdown() { finishSession(); }
};
static LeakGuardShutdown shutdownHook;

void* operator new(std::size_t size) {
    if (void* p = std::malloc(size ? size : 1)) { track(p, size); return p; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    if (void* p = std::malloc(size ? size : 1)) { track(p, size); return p; }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {
    if (!p) return;
    if (!insideHook) {
        insideHook = true;
        auto* record = leakguardTracker().searchAllocation(p);
        const bool release = !record || (!record->freed && leakguardTracker().recordDeallocation(p));
        insideHook = false;
        if (!release) return;
    }
    std::free(p);
}
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
void* operator new(std::size_t size, const char* file, int line) {
    if (void* p = std::malloc(size ? size : 1)) { track(p, size, file, line); return p; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size, const char* file, int line) {
    if (void* p = std::malloc(size ? size : 1)) { track(p, size, file, line); return p; }
    throw std::bad_alloc();
}
void operator delete(void* p, const char*, int) noexcept { std::free(p); }
void operator delete[](void* p, const char*, int) noexcept { std::free(p); }
