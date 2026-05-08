/* wchar_compat.c
   Compatibility shim for some wchar functions.
   Implementations are marked WEAK so that if the C runtime provides them,
   the runtime's (strong) symbols win and these definitions are ignored.
   This file assumes input wchar strings are ASCII/low-byte; it's meant as
   an embedded-friendly compatibility layer, not a full Unicode library.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

/* weak macro for ARMClang / GCC */
#if defined(__GNUC__) || defined(__clang__)
  #define WEAK __attribute__((weak))
#else
  #define WEAK /* unknown compiler; weak unsupported */
#endif

/* Basic implementations (WEAK) */
WEAK size_t wcslen(const wchar_t *s) {
    const wchar_t *p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

WEAK wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == c) return (wchar_t *)&s[i];
    }
    return NULL;
}

WEAK int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] < s2[i]) return -1;
        if (s1[i] > s2[i]) return 1;
    }
    return 0;
}

/* helper: convert a wchar_t string (assumed ASCII/low-byte) to narrow buffer */
static int wchar_to_narrow(const wchar_t *w, char *out, size_t out_len) {
    size_t i = 0;
    while (*w && i + 1 < out_len) {
        wchar_t wc = *w++;
        // map to single byte (assume ASCII / low byte)
        out[i++] = (char)(wc & 0xFF);
    }
    out[i] = '\0';
    return (int)i;
}

/* Numeric conversion wrappers (WEAK).
   We use moderate-sized stack buffers; if your input may be longer,
   increase BUF_SZ accordingly.
*/
#define BUF_SZ 512

WEAK unsigned long wcstoul(const wchar_t *nptr, wchar_t **endptr, int base) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    unsigned long v = strtoul(buf, &local_end, base);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

WEAK long wcstol(const wchar_t *nptr, wchar_t **endptr, int base) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    long v = strtol(buf, &local_end, base);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

WEAK long long wcstoll(const wchar_t *nptr, wchar_t **endptr, int base) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    long long v = strtoll(buf, &local_end, base);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

WEAK unsigned long long wcstoull(const wchar_t *nptr, wchar_t **endptr, int base) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    unsigned long long v = strtoull(buf, &local_end, base);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

/* Floating conversions (WEAK).
   Use strtod/strtof underneath.
*/
WEAK double wcstod(const wchar_t *nptr, wchar_t **endptr) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    double v = strtod(buf, &local_end);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

WEAK float wcstof(const wchar_t *nptr, wchar_t **endptr) {
    char buf[BUF_SZ];
    wchar_to_narrow(nptr, buf, sizeof(buf));
    char *local_end;
    float v = strtof(buf, &local_end);
    if (endptr) {
        size_t pos = (size_t)(local_end - buf);
        *endptr = (wchar_t*)(nptr + pos);
    }
    return v;
}

/* Some runtimes call internal wrappers; provide weak aliases/delegates so there's no multiple-definition */
WEAK double __wcstod_int(const wchar_t *nptr, wchar_t **endptr) {
    return wcstod(nptr, endptr);
}
WEAK float __wcstof_int(const wchar_t *nptr, wchar_t **endptr) {
    return wcstof(nptr, endptr);
}

/* Minimal swprintf-like implementation (WEAK).
   This is ASCII-only and small; it uses narrow vsnprintf and copies to wchar output.
*/
WEAK int __2swprintf(wchar_t *s, const wchar_t *format, ...) {
    char fmtbuf[BUF_SZ];
    wchar_to_narrow(format, fmtbuf, sizeof(fmtbuf));
    char outbuf[BUF_SZ * 2];
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(outbuf, sizeof(outbuf), fmtbuf, ap);
    va_end(ap);
    if (ret < 0) return ret;
    // copy to wchar buffer (low-byte)
    int maxcpy = (int)(sizeof(outbuf) / sizeof(char));
    for (int i = 0; i <= ret && i < maxcpy; ++i) {
        s[i] = (wchar_t)outbuf[i];
    }
    return ret;
}