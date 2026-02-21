/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_uart_rxi_isr, /* SCI3 RXI (Receive data full) */
            [1] = sci_uart_txi_isr, /* SCI3 TXI (Transmit data empty) */
            [2] = sci_uart_tei_isr, /* SCI3 TEI (Transmit end) */
            [3] = sci_uart_eri_isr, /* SCI3 ERI (Receive error) */
            [4] = sdhimmc_accs_isr, /* SDHIMMC0 ACCS (Card access) */
            [5] = sdhimmc_card_isr, /* SDHIMMC0 CARD (Card detect) */
            [6] = dmac_int_isr, /* DMAC0 INT (DMAC0 transfer end) */
            [7] = ssi_txi_isr, /* SSI0 TXI (Transmit data empty) */
            [8] = ssi_rxi_isr, /* SSI0 RXI (Receive data full) */
            [9] = ssi_int_isr, /* SSI0 INT (Error interrupt) */
            [10] = gpt_counter_overflow_isr, /* GPT0 COUNTER OVERFLOW (Overflow) */
            [11] = sci_spi_rxi_isr, /* SCI2 RXI (Receive data full) */
            [12] = sci_spi_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [13] = sci_spi_tei_isr, /* SCI2 TEI (Transmit end) */
            [14] = sci_spi_eri_isr, /* SCI2 ERI (Receive error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI3_RXI,GROUP0), /* SCI3 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TXI,GROUP1), /* SCI3 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TEI,GROUP2), /* SCI3 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI3_ERI,GROUP3), /* SCI3 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_ACCS,GROUP4), /* SDHIMMC0 ACCS (Card access) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_CARD,GROUP5), /* SDHIMMC0 CARD (Card detect) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_DMAC0_INT,GROUP6), /* DMAC0 INT (DMAC0 transfer end) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SSI0_TXI,GROUP7), /* SSI0 TXI (Transmit data empty) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_SSI0_RXI,GROUP0), /* SSI0 RXI (Receive data full) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_SSI0_INT,GROUP1), /* SSI0 INT (Error interrupt) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_GPT0_COUNTER_OVERFLOW,GROUP2), /* GPT0 COUNTER OVERFLOW (Overflow) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SCI2_RXI,GROUP3), /* SCI2 RXI (Receive data full) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP4), /* SCI2 TXI (Transmit data empty) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP5), /* SCI2 TEI (Transmit end) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_SCI2_ERI,GROUP6), /* SCI2 ERI (Receive error) */
        };
        #endif
        #endif
