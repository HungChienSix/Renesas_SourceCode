/* generated HAL source file - do not edit */
#include "hal_data.h"
qspi_instance_ctrl_t g_qspi0_ctrl;

static const spi_flash_erase_command_t g_qspi0_erase_command_list[] =
{
#if 4096 > 0
  { .command = 0x20, .size = 4096 },
#endif
#if 32768 > 0
  { .command = 0x52, .size = 32768 },
#endif
#if 65536 > 0
  { .command = 0xD8, .size = 65536 },
#endif
#if 0xC7 > 0
  { .command = 0xC7, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE },
#endif
        };
static const qspi_extended_cfg_t g_qspi0_extended_cfg =
{ .min_qssl_deselect_cycles = QSPI_QSSL_MIN_HIGH_LEVEL_4_QSPCLK, .qspclk_div = QSPI_QSPCLK_DIV_2, };
const spi_flash_cfg_t g_qspi0_cfg =
{ .spi_protocol = SPI_FLASH_PROTOCOL_EXTENDED_SPI,
  .read_mode = SPI_FLASH_READ_MODE_FAST_READ_QUAD_IO,
  .address_bytes = SPI_FLASH_ADDRESS_BYTES_3,
  .dummy_clocks = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,
  .page_program_address_lines = SPI_FLASH_DATA_LINES_1,
  .page_size_bytes = 256,
  .page_program_command = 0x02,
  .write_enable_command = 0x06,
  .status_command = 0x05,
  .write_status_bit = 0,
  .xip_enter_command = 0x20,
  .xip_exit_command = 0xFF,
  .p_erase_command_list = &g_qspi0_erase_command_list[0],
  .erase_command_list_length = sizeof(g_qspi0_erase_command_list) / sizeof(g_qspi0_erase_command_list[0]),
  .p_extend = &g_qspi0_extended_cfg, };
/** This structure encompasses everything that is needed to use an instance of this interface. */
const spi_flash_instance_t g_qspi0 =
{ .p_ctrl = &g_qspi0_ctrl, .p_cfg = &g_qspi0_cfg, .p_api = &g_qspi_on_spi_flash, };
sci_uart_instance_ctrl_t g_uart0_ctrl;

baud_setting_t g_uart0_baud_setting =
        {
        /* Baud rate calculated with 0.469% error. */.semr_baudrate_bits_b.abcse = 0,
          .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 53, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme =
                  false };

/** UART extended configuration for UARTonSCI HAL driver */
const sci_uart_extended_cfg_t g_uart0_cfg_extend =
{ .clock = SCI_UART_CLOCK_INT, .rx_edge_start = SCI_UART_START_BIT_FALLING_EDGE, .noise_cancel =
          SCI_UART_NOISE_CANCELLATION_DISABLE,
  .rx_fifo_trigger = SCI_UART_RX_FIFO_TRIGGER_MAX, .p_baud_setting = &g_uart0_baud_setting, .flow_control =
          SCI_UART_FLOW_CONTROL_RTS,
#if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
  .flow_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
  .rs485_setting =
  { .enable = SCI_UART_RS485_DISABLE, .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
#if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
    .de_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
          },
  .irda_setting =
  { .ircr_bits_b.ire = 0, .ircr_bits_b.irrxinv = 0, .ircr_bits_b.irtxinv = 0, }, };

/** UART interface configuration */
const uart_cfg_t g_uart0_cfg =
{ .channel = 2, .data_bits = UART_DATA_BITS_8, .parity = UART_PARITY_OFF, .stop_bits = UART_STOP_BITS_1, .p_callback =
          UART_debug_callback,
  .p_context = NULL, .p_extend = &g_uart0_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
  .rxi_ipl = (12),
  .txi_ipl = (12), .tei_ipl = (12), .eri_ipl = (12),
#if defined(VECTOR_NUMBER_SCI2_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI2_RXI,
#else
  .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI2_TXI,
#else
  .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI2_TEI,
#else
  .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI2_ERI,
#else
  .eri_irq = FSP_INVALID_VECTOR,
#endif
        };

/* Instance structure to use this module. */
const uart_instance_t g_uart0 =
{ .p_ctrl = &g_uart0_ctrl, .p_cfg = &g_uart0_cfg, .p_api = &g_uart_on_sci };
void g_hal_init(void)
{
    g_common_init ();
}
