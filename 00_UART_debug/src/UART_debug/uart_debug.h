/* V1.0 UART_Debug */

#ifndef UART_DEBUG_H
#define UART_DEBUG_H

// TXD --> P302 (SCI2)
// RXD --> P301 (SCI2)

#include "hal_data.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG_UART_ENABLE     1    
#define UART_DEBUG_CTRL       g_uart0_ctrl
#define UART_DEBUG_CFG        g_uart0_cfg

#define CMD_BUFFER_SIZE 64

/* 解析命令结构体 */
typedef struct {
    char cmd[CMD_BUFFER_SIZE/2];      /* 命令名称 */
    char arg[CMD_BUFFER_SIZE/2];      /* 命令参数 */
    bool valid;        /* 解析是否成功 */
} UART_CmdInfo_t;

/* 函数声明 */
fsp_err_t UART_debug_Init();
fsp_err_t UART_debug_DeInit();
const char* UART_debug_GetCmdBuffer(void);
void UART_debug_ClearCmdBuffer(void);
bool UART_debug_HasCommand(void);

/* 解析命令 - 格式: :cmd arg\r\n, 通过指针返回结果避免栈复制 */
void UART_debug_ParseCommand(const char* cmd_str, UART_CmdInfo_t* cmd_info);

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
