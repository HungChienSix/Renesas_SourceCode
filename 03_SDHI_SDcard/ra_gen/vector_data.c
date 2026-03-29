/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sdhimmc_accs_isr, /* SDHIMMC0 ACCS (Card access) */
            [1] = dmac_int_isr, /* DMAC0 INT (DMAC0 transfer end) */
            [2] = sci_uart_rxi_isr, /* SCI2 RXI (Receive data full) */
            [3] = sci_uart_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [4] = sci_uart_tei_isr, /* SCI2 TEI (Transmit end) */
            [5] = sci_uart_eri_isr, /* SCI2 ERI (Receive error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_ACCS,GROUP0), /* SDHIMMC0 ACCS (Card access) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_DMAC0_INT,GROUP1), /* DMAC0 INT (DMAC0 transfer end) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI2_RXI,GROUP2), /* SCI2 RXI (Receive data full) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP3), /* SCI2 TXI (Transmit data empty) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP4), /* SCI2 TEI (Transmit end) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SCI2_ERI,GROUP5), /* SCI2 ERI (Receive error) */
        };
        #endif
        #endif
