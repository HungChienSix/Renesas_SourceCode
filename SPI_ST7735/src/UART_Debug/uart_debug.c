/* V1.0 UART_Debug */

#include <UART_Debug/uart_debug.h>

/* 调试串口初始化函数 */
void UART_Debug_Init(void)
{
   fsp_err_t err = FSP_SUCCESS;

   err = R_SCI_UART_Open (&g_uart0_ctrl, &g_uart0_cfg);
   assert(FSP_SUCCESS == err);
}

/* 发送完成标志 */
volatile bool uart_receive_complete_flag = false;
volatile bool uart_send_complete_flag = false;

/* 串口中断回调 */
void UART_Debug_Callback (uart_callback_args_t * p_args)
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
            /* 回显 */
            R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)&(p_args->data), 1);
            break;
        }
        default:
            break;
    }
}


#if defined __GNUC__ && !defined __clang__
int _write(int fd, char *pBuffer, int size); //防止编译警告
int _read(int fd, char *pBuffer, int size);

/* 重定向 printf 输出 */
int _write(int fd, char *pBuffer, int size)
{
   (void) fd;
   R_SCI_UART_Write (&g_uart0_ctrl, (uint8_t*) pBuffer, (uint32_t) size);
   while (uart_send_complete_flag == false);
   uart_send_complete_flag = false;

   return size;
}

/* 重定向scanf函数 */
int _read(int fd, char *pBuffer, int size)
{
    (void) fd;

    R_SCI_UART_Read (&g_uart0_ctrl, (uint8_t*) pBuffer, (uint32_t) size);
    while (uart_receive_complete_flag == false);
    uart_receive_complete_flag = false;

//    /* 回显 */
//    R_SCI_UART_Write (&g_uart0_ctrl, (uint8_t*) pBuffer, (uint32_t) size);

    return size;
}

#else
int fputc(int ch, FILE *f)
{
    (void)f;
   R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)&ch, 1);
   while(uart_send_complete_flag == false);
   uart_send_complete_flag = false;

   return ch;
}
#endif
