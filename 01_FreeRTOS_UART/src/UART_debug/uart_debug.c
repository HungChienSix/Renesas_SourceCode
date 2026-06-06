/* V1.0 UART_Debug */

#include "UART_debug/uart_debug.h"
#include <string.h>
#include <stdbool.h>

fsp_err_t UART_debug_Init()
{
    fsp_err_t err = R_SCI_UART_Open (&UART_DEBUG_CTRL, &UART_DEBUG_CFG);
    return err;
}

fsp_err_t UART_debug_DeInit()
{
    fsp_err_t err = R_SCI_UART_Close(&UART_DEBUG_CTRL);
    return err;
}

/* 发送接受完成标志 */
volatile bool uart_receive_complete_flag = false;
volatile bool uart_send_complete_flag = false;

/* 命令接收缓冲区 */
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_buffer_index = 0;
static bool cmd_start_flag = false;
static bool cmd_ready_flag = false;  /* 命令已收完标志 */

/* 获取命令缓冲区指针 */
char* UART_debug_GetCmdBuffer(void)
{
    return cmd_buffer;
}

/* 清空命令缓冲区 */
void UART_debug_ClearCmdBuffer(void)
{
    cmd_buffer_index = 0;
    cmd_start_flag = false;
    cmd_ready_flag = false;
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
}

/* 获取命令，成功返回1，失败返回0 */
/* 命令格式: (para value) 空格分隔 */
int UART_debug_GetCmd(char* para, char* value)
{
    if (para == NULL || value == NULL) return 0;

    *para = '\0';
    *value = '\0';

    if (!cmd_ready_flag) {
        /* 没有新命令 */
        return 0;
    }

    /* 解析命令缓冲区，格式: (para value) */
    char *p = cmd_buffer;

    /* 跳过 '(' */
    while (*p == '(' || *p == ' ') p++;

    /* 提取 para */
    char *q = para;
    while (*p != '\0' && *p != ' ' && *p != ')' && (q - para) < 31) {
        *q++ = *p++;
    }
    *q = '\0';

    /* 跳过空格 */
    while (*p == ' ') p++;

    /* 提取 value */
    q = value;
    while (*p != '\0' && *p != ')' && (q - value) < 31) {
        *q++ = *p++;
    }
    *q = '\0';

    /* 清空缓冲区，准备接收下一条 */
    UART_debug_ClearCmdBuffer();

    return 1;
}

/* 检查是否有命令待取走 */
bool UART_debug_HasCommand(void)
{
    return cmd_ready_flag;
}

/* 串口中断回调 */
void UART_debug_callback (uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_RX_COMPLETE:
        {
            uart_receive_complete_flag  = true;
            break;
        }
        case UART_EVENT_TX_COMPLETE:
        {
            uart_send_complete_flag     = true;
            break;
        }
        case UART_EVENT_RX_CHAR:
        {
            uint8_t ch = (uint8_t)(p_args->data & 0xFF);
            if (ch == '(') {
                cmd_buffer_index = 0;
                cmd_start_flag = true;
                cmd_ready_flag = false;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));
            }
            else if (ch == ')') {
                if (cmd_start_flag && cmd_buffer_index > 0) {
                    cmd_start_flag = false;
                    cmd_buffer[cmd_buffer_index] = '\0';
                    cmd_ready_flag = true;  /* 命令接收完成 */
                }
            }
            else if (cmd_start_flag && cmd_buffer_index < CMD_BUFFER_SIZE - 1) {
                cmd_buffer[cmd_buffer_index++] = (char)ch;
            }
            break;
        }
        default:
            break;
    }
}

#if defined __GNUC__ && !defined __clang__
int _write(int fd, char *pBuffer, int size);
int _read(int fd, char *pBuffer, int size);

/* 重定向 printf 输出 */
int _write(int fd, char *pBuffer, int size)
{
   (void) fd;

   R_SCI_UART_Write (&UART_DEBUG_CTRL, (uint8_t*) pBuffer, (uint32_t) size);
   while (uart_send_complete_flag == false);
   uart_send_complete_flag = false;

   return size;
}

/* 重定向scanf函数 */
int _read(int fd, char *pBuffer, int size)
{
    (void) fd;

    R_SCI_UART_Read (&UART_DEBUG_CTRL, (uint8_t*) pBuffer, (uint32_t) size);
    while (uart_receive_complete_flag == false);
    uart_receive_complete_flag = false;

    return size;
}

#else
int fputc(int ch, FILE *f)
{
    (void)f;
   R_SCI_UART_Write(&UART_DEBUG_CTRL, (uint8_t *)&ch, 1);
   while(uart_send_complete_flag == false);
   uart_send_complete_flag = false;

   return ch;
}
#endif