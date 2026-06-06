/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_uart_rxi_isr, /* SCI2 RXI (Receive data full) */
            [1] = sci_uart_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [2] = sci_uart_tei_isr, /* SCI2 TEI (Transmit end) */
            [3] = sci_uart_eri_isr, /* SCI2 ERI (Receive error) */
            [4] = spi_rxi_isr, /* SPI0 RXI (Receive buffer full) */
            [5] = spi_tei_isr, /* SPI0 TEI (Transmission complete event) */
            [6] = spi_eri_isr, /* SPI0 ERI (Error) */
            [7] = dmac_int_isr, /* DMAC0 INT (DMAC0 transfer end) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI2_RXI,GROUP0), /* SCI2 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP1), /* SCI2 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP2), /* SCI2 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI2_ERI,GROUP3), /* SCI2 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SPI0_RXI,GROUP4), /* SPI0 RXI (Receive buffer full) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TEI,GROUP5), /* SPI0 TEI (Transmission complete event) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SPI0_ERI,GROUP6), /* SPI0 ERI (Error) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_DMAC0_INT,GROUP7), /* DMAC0 INT (DMAC0 transfer end) */
        };
        #endif
        #endif
