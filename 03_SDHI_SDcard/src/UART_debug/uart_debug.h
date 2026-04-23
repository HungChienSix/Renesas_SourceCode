/* V1.0 UART_Debug */

#ifndef UART_DEBUG_H
#define UART_DEBUG_H

// TXD --> P302 (SCI2)
// RXD --> P301 (SCI2)

#include "hal_data.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* 串口初始化 */
void UART_debug_Init(void);

/* 函数声明 防止编译器警告 */
int _isatty(int fd);
int _close(int fd);
int _lseek(int fd, int ptr, int dir);
int _fstat(int fd, struct stat *st);

 __attribute__((weak)) int _isatty(int fd)
 {
     if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
         return 1;

     errno = EBADF;
     return 0;
 }

 __attribute__((weak)) int _close(int fd)
 {
     if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
         return 0;

     errno = EBADF;
     return -1;
 }

 __attribute__((weak)) int _lseek(int fd, int ptr, int dir)
 {
     (void) fd;
     (void) ptr;
     (void) dir;

     errno = EBADF;
     return -1;
 }

 __attribute__((weak)) int _fstat(int fd, struct stat *st)
 {
     if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
     {
         st->st_mode = S_IFCHR;
         return 0;
     }

     errno = EBADF;
     return 0;
 }

#endif /* UART_DEBUG_H */
