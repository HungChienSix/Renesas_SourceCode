/* V1.0 UART_Debug */

#include "UART_Debug/uart_debug.h"
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

/* 命令缓冲区 */
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint16_t cmd_buffer_index = 0;
static bool cmd_started = false;

/* 获取命令缓冲区 */
const char* UART_debug_GetCmdBuffer(void)
{
    return cmd_buffer;
}

/* 清除命令缓冲区 */
void UART_debug_ClearCmdBuffer(void)
{
    cmd_buffer_index = 0;
    cmd_started = false;
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
}

/* 检查是否有完整命令*/
bool UART_debug_HasCommand(void)
{
    if (cmd_buffer_index > 1 && cmd_started == false)
    {
        return true;
    }
    return false;
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
            if (ch == ':')  // 命令开始，清空缓冲区
            {
                UART_debug_ClearCmdBuffer();
                cmd_buffer_index = 0;
                cmd_started = true;
            }
            else if (ch == '\\')  // 命令结束，不存放 '\'，改为存放 '\0'
            {
                cmd_started = false;
                cmd_buffer[cmd_buffer_index] = '\0';
            }
            else if (cmd_started && cmd_buffer_index < CMD_BUFFER_SIZE - 1)
            {
                cmd_buffer[cmd_buffer_index++] = ch;
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
