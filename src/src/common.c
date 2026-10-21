#include "common.h"

void handle_error(const char *msg) {
    DWORD err = GetLastError();
    LPSTR buf = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, 0, NULL);
    fprintf(stderr, "%s failed. GetLastError=%lu (%s)\n",
            msg, (unsigned long)err, buf ? buf : "?");
    if (buf) LocalFree(buf);
    ExitProcess(EXIT_FAILURE);
}

double now_sec(void) {
    static LARGE_INTEGER freq = {0};
    static LARGE_INTEGER start = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
    }
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}