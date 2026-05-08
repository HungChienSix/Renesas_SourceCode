// atexit_stub.c
// Lightweight stubs to satisfy aeabi_atexit dependencies when the full C++ runtime is not linked.
//
// WARNING: These are compatibility stubs only. They suppress running C++ static destructors.
// If your application needs static-destructor behavior, use the full C/C++ runtime library instead.

#include <stddef.h>
#include <stdint.h>

/* Many toolchains expect certain symbols used by aeabi_atexit.o.
   Provide weak or simple definitions so the linker can resolve them. */

#if defined(__GNUC__) || defined(__clang__)
  #define WEAK __attribute__((weak))
#else
  #define WEAK
#endif

/* Some runtimes expect a mutex/lock symbol; provide a simple integer as placeholder. */
WEAK int _atexit_mutex = 0;

/* _atexit_init is sometimes expected as an initializer function; provide a no-op. */
WEAK void _atexit_init(void) {
    /* no-op stub */
}

/* __aeabi_atexit is often used to register destructors for C++ static objects.
   Return non-zero/zero depending on your runtime's expectations; returning 0 usually OK.
   If you want to avoid registration entirely, simply return 0. */
WEAK int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) {
    (void)object; (void)destructor; (void)dso_handle;
    /* do not register destructor - stub */
    return 0;
}

/* Some toolchains call plain 'atexit' wrappers; provide minimal stub */
WEAK int atexit(void (*f)(void)) {
    (void)f;
    return 0;
}