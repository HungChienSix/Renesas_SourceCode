// syscalls_stub.c
// Minimal stubs for newlib-nano system calls required by bare-metal targets.

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
  #define WEAK __attribute__((weak))
#else
  #define WEAK
#endif

WEAK void _exit(int status) {
    (void)status;
    while (1)
        ;
}

WEAK int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    return -1;
}

WEAK int _getpid(void) {
    return 1;
}
