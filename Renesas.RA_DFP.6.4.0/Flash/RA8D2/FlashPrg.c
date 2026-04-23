/**************************************************************************//**
 * @file     FlashDev.c
 * @brief    Flash Device Description for RA8D2
 * @version  V1.0.0
 * @date     31 Dec 2025
 ******************************************************************************/
/*
 * Copyright (c) 2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
 /**********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO
 * THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2025 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 31.12.2025 1.0.0    First Release
 *********************************************************************************************************************/
/** FlashOS Structures */
#include "../FlashOS.h"
#include <stdbool.h>

/* 
   Mandatory Flash Programming Functions (Called by FlashOS):
                int Init        (unsigned long adr,   // Initialize Flash
                                 unsigned long clk,
                                 unsigned long fnc);
                int UnInit      (unsigned long fnc);  // De-initialize Flash
                int EraseSector (unsigned long adr);  // Erase Sector Function
                int ProgramPage (unsigned long adr,   // Program Page Function
                                 unsigned long sz,
                                 unsigned char *buf);

   Optional  Flash Programming Functions (Called by FlashOS):
                int BlankCheck  (unsigned long adr,   // Blank Check
                                 unsigned long sz,
                                 unsigned char pat);
                int EraseChip   (void);               // Erase complete Device
      unsigned long Verify      (unsigned long adr,   // Verify Function
                                 unsigned long sz,
                                 unsigned char *buf);

       - BlanckCheck  is necessary if Flash space is not mapped into CPU memory space
       - Verify       is necessary if Flash space is not mapped into CPU memory space
       - if EraseChip is not provided than EraseSector for all sectors is called
*/

/*
   Definition to access a specific address with any size
*/
#define M8(adr)  (*((volatile unsigned char  *) (adr)))
#define M16(adr) (*((volatile unsigned short *) (adr)))
#define M32(adr) (*((volatile unsigned long  *) (adr)))
/*
   Clock related registers adrress and bit definition
*/
#define MCU_SYSTEM_BASE                 (0x4001E000)
#define MCU_SCKDIVCR_ADDR               (MCU_SYSTEM_BASE+0x20)
    #define SCKDIVCR_MRPCK_1DEV_MASK    (0x0FFFFFFF)
    #define SCKDIVCR_ICLK_1DEV_MASK     (0xF0FFFFFF)
    #define SCKDIVCR_MRPCK_MASK         (0xF0000000)
    #define SCKDIVCR_ICK_MASK           (0x0F000000)
#define MCU_SCKDIVCR2_ADDR              (MCU_SYSTEM_BASE+0x20)
    #define SCKDIVCR2_MRICK_1DEV_MASK   (0x0FFF)
    #define SCKDIVCR2_MRICK_MASK        (0xF000)
#define MCU_SCKSCR_ADDR                 (MCU_SYSTEM_BASE + 0x026)
    #define SCKSCR_CKSEL_HOCO           (0x00)
    #define SCKSCR_CKSEL_MOCO           (0x01)
    #define SCKSCR_CKSEL_MOSC           (0x03)
    #define SCKSCR_CKSEL_SOSC           (0x04)
    #define SCKSCR_CKSEL_PLL1P          (0x05)
    #define SCKSCR_CKSEL_MASK           (0x07)
#define MCU_PLLCCR_ADDR                 (MCU_SYSTEM_BASE+0xAC)
    #define PLLCCR_MUL                  (0x0001FF00)
    #define PLLCCR_PLSRCSEL_HOCO        (0x00000010)
    #define PLLCCR_PLSRCSEL_MAIN        (0x00000000)
    #define PLLCCR_PLSRCSEL_MASK        (0x00000010)
    #define PLLCCR_PLIDIV_MASK          (0x00000003)
    #define PLLCCR_PLIDIV_1DEV          (0x00000000)
    #define PLLCCR_PLIDIV_2DEV          (0x00000001)
    #define PLLCCR_PLIDIV_3DEV          (0x00000002)
    
    #define PLLCCR_PLLMULNF_MASK        (0x000000C0)
    #define PLLCCR_PLLMULNF_0ADD_MUL    (0x00000000)
    #define PLLCCR_PLLMULNF_1_3ADD_MUL  (0x00000040) 
    #define PLLCCR_PLLMULNF_2_3ADD_MUL  (0x00000080) 
    #define PLLCCR_PLLMULNF_1_2ADD_MUL  (0x000000C0)
    
    #define PLLCCR_PLLMULNF_0VALUE      (0)
    #define PLLCCR_PLLMULNF_1_3VALUE    (1.0f/3.0f)
    #define PLLCCR_PLLMULNF_2_3VALUE    (2.0f/3.0f)
    #define PLLCCR_PLLMULNF_1_2VALUE    (1.0f/2.0f)
#define MCU_PLLCCR2_ADDR                (MCU_SYSTEM_BASE+0x4C)
    #define PLLCCR2_PLODIVP_MASK        (0x000F)
    #define PLLCCR2_PLODIVP_2DEV        (0x0001)
    #define PLLCCR2_PLODIVP_3DEV        (0x0002)
    #define PLLCCR2_PLODIVP_4DEV        (0x0003)
    #define PLLCCR2_PLODIVP_6DEV        (0x0005)
    #define PLLCCR2_PLODIVP_8DEV        (0x0007)
    #define PLLCCR2_PLODIVP_16DEV       (0x000F)

    #define PLLCCR2_PLODIVQ_MASK        (0x00F0)
    #define PLLCCR2_PLODIVQ_2DEV        (0x0010)
    #define PLLCCR2_PLODIVQ_3DEV        (0x0020)
    #define PLLCCR2_PLODIVQ_4DEV        (0x0030)
    #define PLLCCR2_PLODIVQ_5DEV        (0x0040)
    #define PLLCCR2_PLODIVQ_6DEV        (0x0050)
    #define PLLCCR2_PLODIVQ_8DEV        (0x0070)
    #define PLLCCR2_PLODIVQ_9DEV        (0x0080)
    #define PLLCCR2_PLODIVQ_1_5DEV      (0x0090)

    #define PLLCCR2_PLODIVR_MASK        (0x0F00)
    #define PLLCCR2_PLODIVR_2DEV        (0x0100)
    #define PLLCCR2_PLODIVR_3DEV        (0x0200)
    #define PLLCCR2_PLODIVR_4DEV        (0x0300)
    #define PLLCCR2_PLODIVR_5DEV        (0x0400)
    #define PLLCCR2_PLODIVR_6DEV        (0x0500)
    #define PLLCCR2_PLODIVR_8DEV        (0x0700)
    #define PLLCCR2_PLODIVR_9DEV        (0x0800)
    #define PLLCCR2_PLODIVR_1_5DEV      (0x0900)

#define MCU_OCTACKCR_ADDR               (MCU_SYSTEM_BASE + 0x075)
    #define OCTACKCR_OCTACKSEL_HOCO     (0x00)
    #define OCTACKCR_OCTACKSEL_MOCO     (0x01)
    #define OCTACKCR_OCTACKSEL_LOCO     (0x02)
    #define OCTACKCR_OCTACKSEL_MOSC     (0x03)
    #define OCTACKCR_OCTACKSEL_SOSC     (0x04)
    #define OCTACKCR_OCTACKSEL_PLL1P    (0x05)
    #define OCTACKCR_OCTACKSEL_PLL2P    (0x06)
    #define OCTACKCR_OCTACKSEL_PLL1Q    (0x07)
    #define OCTACKCR_OCTACKSEL_PLL1R    (0x08)
    #define OCTACKCR_OCTACKSEL_PLL2Q    (0x09)
    #define OCTACKCR_OCTACKSEL_PLL2R    (0x0A)
    #define OCTACKCR_OCTACKSEL_MASK     (0x0F)

    #define OCTACKCR_OCTACKSREQ_REQ     (0x40)
    #define OCTACKCR_OCTACKSREQ_NO_REQ  (0xBF)
    #define OCTACKCR_OCTACKSREQ_MASK    (0x40)

    #define OCTACKCR_OCTACKSRDY_POSSIBLE  (0x80)
    #define OCTACKCR_OCTACKSRDY_IMPOSIBLE (0x00)
    #define OCTACKCR_OCTACKSRDY_MASK      (0x80)

#define MCU_PLL2CCR_ADDR                (MCU_SYSTEM_BASE+0x0C8)
    #define PLL2CCR_MUL                 (0x0001FF00)
    #define PLL2CCR_PL2SRCSEL_HOCO      (0x00000010)
    #define PLL2CCR_PL2SRCSEL_MAIN      (0x00000000)
    #define PLL2CCR_PL2SRCSEL_MASK      (0x00000010)
    #define PLL2CCR_PL2IDIV_MASK        (0x00000003)
    #define PLL2CCR_PL2IDIV_1DEV        (0x00000000)
    #define PLL2CCR_PL2IDIV_2DEV        (0x00000001)
    #define PLL2CCR_PL2IDIV_3DEV        (0x00000002)

    #define PLL2CCR_PLL2MULNF_MASK        (0x000000C0)
    #define PLL2CCR_PLL2MULNF_0ADD_MUL    (0x00000000)
    #define PLL2CCR_PLL2MULNF_1_3ADD_MUL  (0x00000040)
    #define PLL2CCR_PLL2MULNF_2_3ADD_MUL  (0x00000080)
    #define PLL2CCR_PLL2MULNF_1_2ADD_MUL  (0x000000C0)

    #define PLL2CCR_PLL2MULNF_0VALUE    (0)
    #define PLL2CCR_PLL2MULNF_1_3VALUE  (1.0f/3.0f)
    #define PLL2CCR_PLL2MULNF_2_3VALUE  (2.0f/3.0f)
    #define PLL2CCR_PLL2MULNF_1_2VALUE  (1.0f/2.0f)

#define MCU_PLL2CCR2_ADDR               (MCU_SYSTEM_BASE+0x4E)
    #define PLL2CCR2_PL2ODIVP_MASK      (0x000F)
    #define PLL2CCR2_PL2ODIVP_2DEV      (0x0001)
    #define PLL2CCR2_PL2ODIVP_3DEV      (0x0002)
    #define PLL2CCR2_PL2ODIVP_4DEV      (0x0003)
    #define PLL2CCR2_PL2ODIVP_6DEV      (0x0005)
    #define PLL2CCR2_PL2ODIVP_8DEV      (0x0007)
    #define PLL2CCR2_PL2ODIVP_16DEV     (0x000F)

    #define PLL2CCR2_PL2ODIVQ_MASK      (0x00F0)
    #define PLL2CCR2_PL2ODIVQ_2DEV      (0x0010)
    #define PLL2CCR2_PL2ODIVQ_3DEV      (0x0020)
    #define PLL2CCR2_PL2ODIVQ_4DEV      (0x0030)
    #define PLL2CCR2_PL2ODIVQ_5DEV      (0x0040)
    #define PLL2CCR2_PL2ODIVQ_6DEV      (0x0050)
    #define PLL2CCR2_PL2ODIVQ_8DEV      (0x0070)
    #define PLL2CCR2_PL2ODIVQ_9DEV      (0x0080)
    #define PLL2CCR2_PL2ODIVQ_1_5DEV    (0x0090)

    #define PLL2CCR2_PL2ODIVR_MASK      (0x0F00)
    #define PLL2CCR2_PL2ODIVR_2DEV      (0x0100)
    #define PLL2CCR2_PL2ODIVR_3DEV      (0x0200)
    #define PLL2CCR2_PL2ODIVR_4DEV      (0x0300)
    #define PLL2CCR2_PL2ODIVR_5DEV      (0x0400)
    #define PLL2CCR2_PL2ODIVR_6DEV      (0x0500)
    #define PLL2CCR2_PL2ODIVR_8DEV      (0x0700)
    #define PLL2CCR2_PL2ODIVR_9DEV      (0x0800)
    #define PLL2CCR2_PL2ODIVR_1_5DEV    (0x0900)

#define MCU_OCTACKDIVCR_ADDR            (MCU_SYSTEM_BASE + 0x06D)
    #define OCTACKDIVCR_OCTACKDIV_MASK  (0x0F)
    #define OCTACKDIVCR_OCTACKDIV_1DEV  (0x00)
    #define OCTACKDIVCR_OCTACKDIV_2DEV  (0x01)
    #define OCTACKDIVCR_OCTACKDIV_4DEV  (0x02)
    #define OCTACKDIVCR_OCTACKDIV_6DEV  (0x03)
    #define OCTACKDIVCR_OCTACKDIV_8DEV  (0x04)
    #define OCTACKDIVCR_OCTACKDIV_3DEV  (0x05)
    #define OCTACKDIVCR_OCTACKDIV_5DEV  (0x06)
    #define OCTACKDIVCR_OCTACKDIV_10DEV (0x07)
    #define OCTACKDIVCR_OCTACKDIV_16DEV (0x08)
    #define OCTACKDIVCR_OCTACKDIV_32DEV (0x09)

#define MCU_PLLCR_ADDR                  (MCU_SYSTEM_BASE + 0x02A)
    #define PLLCR_ON                    (0x00)
    #define PLLCR_STOP                  (0x01)

#define MCU_PRCR_ADDR                   (0x4001E3FA)
    #define KEYCODE_PRCR                (0xA500)
    #define PRCR_PRC1                   (0x0002)
    #define PRCR_PRC0                   (0x0001)
    #define PRCR_PRC3                   (0x0008)
    #define PRCR_PRC4                   (0x0010)
    #define PRCR_PRC5                   (0x0020)
#define MCU_OPCCR_ADDR                  (MCU_SYSTEM_BASE+0x0A0)
    #define OPCCR_OPCCR                 (0x03)
    #define OPCCR_OPCCR_HIGH            (0x00)
    #define OPCCR_OPCMTSF               (0x10)
    #define OPCCR_OPCMTSF_COMPLETE      (0x00)
    #define OPCCR_OPCMTSF_TRANS         (0x10)
#define MCU_OFS1_ADDR                   (0x02C9F0C0)
    #define OFS1_HOCOFRQ0_16MHZ         (0x00000000)
    #define OFS1_HOCOFRQ0_18MHZ         (0x00000200)
    #define OFS1_HOCOFRQ0_20MHZ         (0x00000400)
    #define OFS1_HOCOFRQ0_32MHZ         (0x00000800)
    #define OFS1_HOCOFRQ0_48MHZ         (0x00000E00)
    #define OFS1_HOCOFRQ0_MASK          (0x00000E00)
    #define OFS1_HOCOEN                 (0x00000100)
#define MCU_MOCOCR_ADDR                 (MCU_SYSTEM_BASE + 0x038)
    #define MOCOCR_STOP                 (0x01)
    #define MOCOCR_ON                   (0x00)
#define MCU_HOCOCR_ADDR                 (MCU_SYSTEM_BASE + 0x036)
    #define HOCOCR_STOP                 (0x01)
    #define HOCOCR_ON                   (0x00)
#define MCU_OSCSF_ADDR                  (MCU_SYSTEM_BASE + 0x03C)
    #define OSCSF_HOCOSF                (0x01)
    #define OSCSF_MOSCSF                (0x08)
    #define OSCSF_PLLSF                 (0x20)
    #define OSCSF_PLL2SF                (0x40)
/*
   Flash related registers adrress and bit definition
*/

#define MCU_MRAM_BASE                   (0x40130000)
#define MCU_MACI_CMD_AREA_ADDR          (0x40120000)
#define MCU_MASTAT_ADDR                 (MCU_MRAM_BASE + 0xE010)
    #define MASTAT_MREAE                (0x08UL)
    #define MASTAT_CMDLK                (0x10UL)
#define MCU_MPAEINT_ADDR                (MCU_MRAM_BASE + 0xE014)
#define MCU_MRDYIE_ADDR                 (MCU_MRAM_BASE + 0xE018)
#define MCU_MSADDR_ADDR                 (MCU_MRAM_BASE + 0xE030)
#define MCU_MSTATR_ADDR                 (MCU_MRAM_BASE + 0xE080)
    #define MSTATR_MRDY                 (0x00008000UL)
    #define MSTATR_ILGLERR              (0x00004000UL)
    #define MSTATR_PRGERR               (0x00001000UL)
#define MCU_MENTRYR_ADDR                (MCU_MRAM_BASE + 0xE084)
    #define MENTRYR_READ_MODE           (0xAA00UL)
    #define MENTRYR_EMRAM_PROGRAM       (0xAA80UL)
    #define MENTRYR_CHK_EMRAM           (0x0080UL)
    #define MENTRYR_CHK_READ            (0x0000UL)
#define MCU_MSUINITR_ADDR               (MCU_MRAM_BASE + 0xE08C)
#define MCU_MCMDR_ADDR                  (MCU_MRAM_BASE + 0xE0A0)

#define MCU_MRCPFB_ADDR                 (MCU_MRAM_BASE + 0xC000)
    #define MRCPFB_MPFBEN_EN            (0x01UL)
    #define MRCPFB_MPFBEN_DIS           (0x00UL)
#define MCU_MRCFREQ_ADDR                (MCU_MRAM_BASE + 0xC004)
    #define MRCFREQ_KEY                 (0x1E000000UL)
#define MCU_MREFREQ_ADDR                (MCU_MRAM_BASE + 0xC008)
    #define MREFREQ_KEY                 (0xE1000000UL)

#define MCU_MRCPS_ADDR                  (MCU_MRAM_BASE + 0xF010)
    #define MRCPS_PRGERRC               (0x01UL)
    #define MRCPS_ECCERRC               (0x02UL)
    #define MRCPS_ABUFEMP               (0x20UL)
    #define MRCPS_ABUFFULL              (0x40UL)
    #define MRCPS_PRGBSYC               (0x80UL)
#define MCU_MRCFLR_ADDR                 (MCU_MRAM_BASE + 0xF030)
    #define MRCFLR_KEY                  (0xC300UL)
    #define MRCFLR_MRCFL_EXECUTE        (0x01UL)
    
#define MCU_MRCPC0_ADDR                 (0x4013F000)
    #define MRCPC0_KEY                  (0x8600UL)
    #define MRCPC0_MRCPSEN_ENABLE       (0x0001UL)
#define MCU_MRCPC1_ADDR                 (0x4013F004) 
    #define MRCPC1_KEY                  (0x6800UL)
    #define MRCPC1_MRCPSEN_ENABLE       (0x0001UL)
#define MCU_MRCBPROT0_ADDR              (0x4013F008)
    #define MRCBPROT0_KEY               (0x07800UL)
    #define MRCBPROT0_BPCN0_ENABLE      (0x0000UL)
    #define MRCBPROT0_BPCN0_DISABLE     (0x0001UL)
#define MCU_MRCBPROT1_ADDR              (0x4013F00C)
    #define MRCBPROT1_KEY               (0xB100UL)
    #define MRCBPROT1_BPCN1_ENABLE      (0x0000UL)
    #define MRCBPROT1_BPCN1_DISABLE     (0x0001UL)
#define MCU_MSTPCRB_ADDR                (0x40203004)
    #define MCU_MSTPCRB_MSTPB17_MSK     (0x00020000UL)
#define MCU_PWPR_S_ADDR                 (0x40400D14)
    #define MCU_PWPR_S_B0WI_MSK         (0x80)
    #define MCU_PWPR_S_PFSWE_MSK        (0x40)
#define MCU_PmnPFS_ADDR(port, pin)      (0x40400800 + ((port) * 0x40) + ((pin) * 4))

/*
   OSPI related registers adrress and bit definition
*/
#define OSPI_BASE(n)                    (0x40268000 + 0x0400 * (n))
#define OSPI_LIOCFGCS_ADDR(n, cs)       (OSPI_BASE(n) + 0x050 + (0x004 * cs))
    #define OSPI_LIOCFGCS(n, cs)        M32(OSPI_LIOCFGCS_ADDR(n, cs))
    #define OSPI_LIOCFGCS_PRTMD_888     (0x000003FFUL)
    #define OSPI_LIOCFGCS_PRTMD_POS     (0UL)
    #define OSPI_LIOCFGCS_PRTMD_MSK     (0x3FFUL)
    #define OSPI_LIOCFGCS_LATEMD_POS    (10UL)
    #define OSPI_LIOCFGCS_LATEMD_MSK    (0x400UL)
    #define OSPI_LIOCFGCS_CSMIN_8       (0x00070000UL)
    #define OSPI_LIOCFGCS_CSMIN_POS     (16UL)
    #define OSPI_LIOCFGCS_CSMIN_MSK     (0xF0000UL)
    #define OSPI_LIOCFGCS_CSASTEX_1     (0x00100000UL)
    #define OSPI_LIOCFGCS_CSASTEX_POS   (20UL)
    #define OSPI_LIOCFGCS_CSASTEX_MSK   (0x100000UL)
    #define OSPI_LIOCFGCS_CSNEGEX_1     (0x00200000UL)
    #define OSPI_LIOCFGCS_CSNEGEX_POS   (21UL)
    #define OSPI_LIOCFGCS_CSNEGEX_MSK   (0x200000UL)
    #define OSPI_LIOCFGCS_SDRDRV_POS    (22UL)
    #define OSPI_LIOCFGCS_SDRDRV_MSK    (0x400000UL)
    #define OSPI_LIOCFGCS_SDRSMPMD_POS  (23UL)
    #define OSPI_LIOCFGCS_SDRSMPMD_MSK  (0x800000UL)
    #define OSPI_LIOCFGCS_SDRSMPSFT_POS (24UL)
    #define OSPI_LIOCFGCS_SDRSMPSFT_MSK (0xf000000UL)
    #define OSPI_LIOCFGCS_DDRSMPEX_1    (0x10000000UL)
    #define OSPI_LIOCFGCS_DDRSMPEX_POS  (28UL)
    #define OSPI_LIOCFGCS_DDRSMPEX_MSK  (0xF0000000UL)
#define OSPI_WRAPCFG_ADDR(n)            (OSPI_BASE(n) + 0x000)
    #define OSPI_WRAPCFG(n)             M32(OSPI_WRAPCFG_ADDR(n))
    #define OSPI_WRAPCFG_DSSFTCS0(n)    (n << 8)
    #define OSPI_WRAPCFG_DSSFTCS1(n)    (n << 24)
    #define OSPI_WRAPCFG_DSSFTCS0_POS   (8UL)
    #define OSPI_WRAPCFG_DSSFTCS0_MSK   (0x1F00UL)
    #define OSPI_WRAPCFG_DSSFTCS1_POS   (24UL)
    #define OSPI_WRAPCFG_DSSFTCS1_MSK   (0x1F000000UL)
#define OSPI_CDCTL0_ADDR(n)             (OSPI_BASE(n) + 0x070)
    #define OSPI_CDCTL0(n)              M32(OSPI_CDCTL0_ADDR(n))
    #define OSPI_CDCTL0_TRREQ           (0x00000001UL)
    #define OSPI_CDCTL0_TRREQ_MSK       (1UL)
    #define OSPI_CDCTL0_PERMD           (0x00000002UL)
    #define OSPI_CDCTL0_PERMD_DIR       (0x00000000UL)
    #define OSPI_CDCTL0_CSSEL_CS0       (0x00000000UL)
    #define OSPI_CDCTL0_CSSEL_CS1       (0x00000008UL)
    #define OSPI_CDCTL0_CSSEL_POS       (3UL)
    #define OSPI_CDCTL0_CSSEL_MSK       (0x8UL)
    #define OSPI_CDCTL0_TRNUM_1CMD      (0x00000000UL)
    #define OSPI_CDCTL0_TRNUM_2CMD      (0x00000010UL)
    #define OSPI_CDCTL0_TRNUM_3CMD      (0x00000020UL)
    #define OSPI_CDCTL0_TRNUM_4CMD      (0x00000030UL)
    #define OSPI_CDCTL0_PERITV(cycles)  ((log2(cycles) - 1) << 16)
    #define OSPI_CDCTL0_PERREP(times)   (log2(times) << 24)
#define OSPI_CDTBUF_ADDR(n, buf)        (OSPI_BASE(n) + 0x080 + 0x010 * (buf))
    #define OSPI_CDTBUF(n, buf)         M32(OSPI_CDTBUF_ADDR(n, buf))
    #define OSPI_CDTBUF_CMDSIZE_POS     (0UL)
    #define OSPI_CDTBUF_CMDSIZE_MSK     (0x3UL)
    #define OSPI_CDTBUF_CMD(cmd)        (cmd << 16)
    #define OSPI_CDTBUF_TRTYPE_READ     (0x00000000UL)
    #define OSPI_CDTBUF_TRTYPE_WRITE    (0x00008000UL)
    #define OSPI_CDTBUF_TRTYPE_POS      (15UL)
    #define OSPI_CDTBUF_TRTYPE_MSK      (0x8000UL)
    #define OSPI_CDTBUF_CMDSIZE(n)      (n << 0)
    #define OSPI_CDTBUF_ADDSIZE(n)      (n << 2)
    #define OSPI_CDTBUF_ADDSIZE_POS     (2UL)
    #define OSPI_CDTBUF_ADDSIZE_MSK     (0x1CUL)
    #define OSPI_CDTBUF_DATASIZE(n)     (n << 5)
    #define OSPI_CDTBUF_DATASIZE_POS    (5UL)
    #define OSPI_CDTBUF_DATASIZE_MSK    (0x1E0UL)
    #define OSPI_CDTBUF_LATENCY(n)      (n << 9)
    #define OSPI_CDTBUF_LATE_POS        (9UL)
    #define OSPI_CDTBUF_LATE_MSK        (0x3e00UL)
#define OSPI_CDABUF_ADDR(n, buf)        (OSPI_BASE(n) + 0x084 + 0x010 * (buf))
    #define OSPI_CDABUF(n, buf)         M32(OSPI_CDABUF_ADDR(n, buf))
    #define OSPI_CDABUF_ADD(addr)       (addr)
#define OSPI_CDD0BUF_ADDR(n, buf)       (OSPI_BASE(n) + 0x088 + 0x010 * (buf))
    #define OSPI_CDD0BUF(n, buf)        M32(OSPI_CDD0BUF_ADDR(n, buf))
    #define OSPI_CDD0BUF_DATA(data)     (data)
#define OSPI_CDD1BUF_ADDR(n, buf)       (OSPI_BASE(n) + 0x08C + 0x010 * (buf))
    #define OSPI_CDD1BUF(n, buf)        M32(OSPI_CDD1BUF_ADDR(n, buf))
    #define OSPI_CDD1BUF_DATA(data)     (data)
    
#define OSPI_CCCTL0CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x130 + (0x020 * cs))
    #define OSPI_CCCTL0CS(n, cs)        M32(OSPI_CCCTL0CS_ADDR(n, cs))
    #define OSPI_CCCTL0CS_CAEN          (0x00000001UL)
    #define OSPI_CCCTL0CS_CAEN_DIS      (0x00000000UL)
    #define OSPI_CCCTL0CS_CAEN_MSK      (0x01UL)
    #define OSPI_CCCTL0CS_CANOWR        (0x00000002UL)
    #define OSPI_CCCTL0CS_CANOWR_POS    (1UL)
    #define OSPI_CCCTL0CS_CANOWR_WR     (0x00000000UL)
    #define OSPI_CCCTL0CS_CAITV(val)    ((udword_t)(log2(val) - 1) << 8)
    #define OSPI_CCCTL0CS_CAITV_POS     (8UL)
    #define OSPI_CCCTL0CS_CASFTSTA(val) (val << 16)
    #define OSPI_CCCTL0CS_CASFTEND(val) (val << 24)
    #define OSPI_CCCTL0CS_CASFTEND_POS  (24UL)
#define OSPI_CCCTL1CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x134 + (0x020 * cs))
    #define OSPI_CCCTL1CS(n, cs)        M32(OSPI_CCCTL1CS_ADDR(n, cs))
    #define OSPI_CCCTL1CS_CACMDSIZE(n)  (n << 0)
    #define OSPI_CCCTL1CS_CACMDSIZE_POS (0UL)
    #define OSPI_CCCTL1CS_CACMDSIZE_MSK (3UL)
    #define OSPI_CCCTL1CS_CAADDSIZE(n)  (n << 2)
    #define OSPI_CCCTL1CS_CAADDSIZE_POS (2UL)
    #define OSPI_CCCTL1CS_CAADDSIZE_MSK (0x1CUL)
    #define OSPI_CCCTL1CS_CADATASIZE(n) ((n - 1) << 5)
    #define OSPI_CCCTL1CS_CADATASIZE_POS  (5UL)
    #define OSPI_CCCTL1CS_CADATASIZE_MSK  (0x1E0UL)
    #define OSPI_CCCTL1CS_CAWRLATE(n)   (n << 16)
    #define OSPI_CCCTL1CS_CAWRLATE_POS  (16UL)
    #define OSPI_CCCTL1CS_CAWRLATE_MSK  (0x1F0000UL)
    #define OSPI_CCCTL1CS_CARDLATE(n)   (n << 24)
    #define OSPI_CCCTL1CS_CARDLATE_POS  (24UL)
    #define OSPI_CCCTL1CS_CARDLATE_MSK  (0x1F000000UL)
#define OSPI_CCCTL2CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x138 + (0x020 * cs))
    #define OSPI_CCCTL2CS(n, cs)        M32(OSPI_CCCTL2CS_ADDR(n, cs))
    #define OSPI_CCCTL2CS_CAWRCMD(cmd)  (cmd << 0)
    #define OSPI_CCCTL2CS_CARDCMD(cmd)  (cmd << 16)
    #define OSPI_CCCTL2CS_CARDCMD_POS   (16UL)
    #define OSPI_CCCTL2CS_CARDCMD_MSK   (0xFFFF0000UL)
#define OSPI_CCCTL3CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x13C + (0x020 * cs))
    #define OSPI_CCCTL3CS(n, cs)        M32(OSPI_CCCTL3CS_ADDR(n, cs))
    #define OSPI_CCCTL3CS_CAADD(addr)   (addr)
#define OSPI_CCCTL4CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x140 + (0x020 * cs))
    #define OSPI_CCCTL4CS(n, cs)        M32(OSPI_CCCTL4CS_ADDR(n, cs))
    #define OSPI_CCCTL4CS_CADATA(data)  (data)
#define OSPI_CCCTL5CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x144 + (0x020 * cs))
    #define OSPI_CCCTL5CS(n, cs)        M32(OSPI_CCCTL5CS_ADDR(n, cs))
    #define OSPI_CCCTL5CS_CADATA(data)  (data)
#define OSPI_CCCTL6CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x148 + (0x020 * cs))
    #define OSPI_CCCTL6CS(n, cs)        M32(OSPI_CCCTL6CS_ADDR(n, cs))
    #define OSPI_CCCTL6CS_CADATA(data)  (data)
#define OSPI_CCCTL7CS_ADDR(n, cs)       (OSPI_BASE(n) + 0x14C + (0x020 * cs))
    #define OSPI_CCCTL7CS(n, cs)        M32(OSPI_CCCTL7CS_ADDR(n, cs))
    #define OSPI_CCCTL7CS_CADATA(data)  (data)

#define OSPI_INTS_ADDR(n)               (OSPI_BASE(n) + 0x190)
    #define OSPI_INTS(n)                M32(OSPI_INTS_ADDR(n))
    #define OSPI_INTS_CMDCMP            (0x00000001UL)
    #define OSPI_INTS_CAFAILCS0         (0x10000000UL)
    #define OSPI_INTS_CASUCCS0          (0x40000000UL)
    #define OSPI_INTS_CAFAILCS1         (0x20000000UL)
    #define OSPI_INTS_CAFAILCS_POS      (28UL)
    #define OSPI_INTS_CASUCCS1          (0x80000000UL)
    #define OSPI_INTS_CASUCCS_POS       (30UL)
#define OSPI_INTC_ADDR(n)               (OSPI_BASE(n) + 0x194)
    #define OSPI_INTC(n)                M32(OSPI_INTC_ADDR(n))
    #define OSPI_INTC_CMDCMPC           (0x00000001UL)
    #define OSPI_INTC_CAFAILCS0C        (0x10000000UL)
    #define OSPI_INTC_CASUCCS0C         (0x40000000UL)
    #define OSPI_INTC_CAFAILCS1C        (0x20000000UL)
    #define OSPI_INTC_CASUCCS1C         (0x80000000UL)
    
#define OSPI_BMCTL0_ADDR(n)             (OSPI_BASE(n) + 0x060)
    #define OSPI_BMCTL0_CH0CS0ACC_MSK   (0x03UL)
    #define OSPI_BMCTL0_CH1CS0ACC_MSK   (0x30UL)
    #define OSPI_BMCTL0_CH0CS1ACC_MSK   (0x0CUL)
    #define OSPI_BMCTL0_CH1CS1ACC_MSK   (0xC0UL)
#define OSPI_BMCTL1_ADDR(n)             (OSPI_BASE(n) + 0x064)
    #define OSPI_BMCTL1_PBUFCLRCH_POS   (10UL)
    #define OSPI_BMCTL1_MWRPUSHCH_POS   (8UL)
#define OSPI_CMCFG0CS_ADD(n, cs)        (OSPI_BASE(n) + 0x010 + (0x010 * cs))
    #define OSPI_CMCFG0CS_ADDRPEN_POS   (16UL)
    #define OSPI_CMCFG0CS_FFMT_POS      (0UL)
    #define OSPI_CMCFG0CS_ADDSIZE_POS   (2UL)
    #define OSPI_CMCFG0CS_ADDSIZE_MSK   (0xCUL)
#define OSPI_CMCFG1CS_ADD(n, cs)        (OSPI_BASE(n) + 0x014 + (0x010 * cs))
    #define OSPI_CMCFG1CS_RDCMD_POS     (0UL)
    #define OSPI_CMCFG1CS_RDLATE_POS    (16UL)
    #define OSPI_CMCFG1CS_RDLATE_MSK    (0x1F0000UL)
#define OSPI_CMCFG2CS_ADD(n, cs)        (OSPI_BASE(n) + 0x018 + (0x010 * cs))
    #define OSPI_CMCFG2CS_WRCMD_POS     (0UL)
    #define OSPI_CMCFG2CS_WRLATE_POS    (16UL)
    #define OSPI_CMCFG2CS_WRLATE_MSK    (0x1f0000UL)
#define OSPI_BMCFGCH_ADD(n, cs)         (OSPI_BASE(n) + 0x008 + (0x004 * cs))
    #define OSPI_BMCFGCH_WRMD_POS       (0UL)
    #define OSPI_BMCFGCH_MWRCOMB_POS    (7UL)
    #define OSPI_BMCFGCH_MWRCOMB_MSK    (0x80UL)
    #define OSPI_BMCFGCH_MWRSIZE_MSK    (0xFF00UL)
    #define OSPI_BMCFGCH_PREEN_POS      (16UL)
    #define OSPI_BMCFGCH_PREEN_MSK      (0x10000UL)
#define OSPI_LIOCTL_ADDR(n)             (OSPI_BASE(n) + 0x108)
    #define OSPI_LIOCTL_RSTCS0_MSK      (0x10000UL)
#define OSPI_COMSTT_ADDR(n)             (OSPI_BASE(n) + 0x184)
    #define OSPI_COMSTT_MEMACCCH_POS    (0UL)
    
#define OSPI_FLAG_STATUS_ERASE_FAILURE        (1 << 5)
#define OSPI_FLAG_STATUS_PROGRAM_FAILURE      (1 << 4)

/*
   Other parameters
*/
#define MACI_FORCED_QUIT_CMD            (0xB3)
#define MACI_STATUS_CLR_CMD             (0x50)
#define MACI_INCREAMENT_COUNTER         (0x35)
#define MACI_READ_COUNTER               (0x39)
#define MACI_PROG_CMD                   (0xE8)
#define MACI_PROG_NUM                   (0x08)
#define MACI_CONF_EXTRA_CMD             (0x40)
#define MACI_CONF_EXTRA_NUM             (0x08)
#define MACI_OTP_EXTRA_CMD              (0xE8)
#define MACI_OTP_EXTRA_NUM              (0x08)
#define MACI_END_CMD                    (0xD0)
#define EMRAM_MASK_ADDR                 (0x0FFFFFFF)

#define MCU_CODEMRAM_ADDR               (0x02000000)
#define MCU_CONF_EXTRAMRAM_ADDR         (0x02C9F040)
#define MCU_OTP_EXTRAMRAM_ADDR          (0x02E07600)

#define MCU_CODEMRAM_BLOCK_SIZE         (0x8000)
#define CODEMRAM_32K_BLOCK_NUM          (0x00)

#define OSPI_PE_BUSY                    (0x00)
#define OSPI_PE_BUSY_MASK               (0x80)
#define OSPI_ERASE_FAILURE              (0x20)
#define OSPI_PROGRAM_FAILURE            (0x10)

// OSPI Specific Configuration setting
#define OSPI_PROTOCOL_MODE              OSPI_PROTOCOL_MODE_1S1S1S
#define OSPI_READ_MODE                  OSPI_READ_MODE_STANDARD
#define OSPI_ADDRESS_BYTES              OSPI_ADDRESS_BYTES_3
#define OSPI_DUMMY_CLOCKS               OSPI_DUMMY_CLOCKS_DEFAULT
#define OSPI_CHANNEL                    OSPI_B_DEVICE_NUMBER_1


// Timing setting
#define OSPI_TS_CMD_TO_CMD_INTERVAL     OSPI_CMD_INTERVAL_CLOCKS_2
#define OSPI_TS_CS_PULLUP_LAG           OSPI_CMD_CS_PULLUP_CLOCKS_NO_ETX
#define OSPI_TS_CS_PULLDOWN_LEAD        OSPI_CMD_CS_PULLDOWN_CLOCKS_NO_ETX
#define OSPI_TS_SDR_DRIVE_TIMING        OSPI_SDR_DRIVE_TIMING_BEFORE_CK
#define OSPI_TS_SDR_SAMPLING_EDGE       OSPI_CK_EDGE_FALLING
#define OSPI_TS_SDR_SAMPLING_DELAY      OSPI_SDR_SAMPLING_DELAY_NONE
#define OSPI_TS_SDR_SAMPLING_EXTENSION  OSPI_DDR_SAMPLING_EXTENSION_NONE

#define OSPI_CMD_INTERVAL_CLOCKS_2      (1)
#define OSPI_CMD_CS_PULLUP_CLOCKS_NO_ETX    (0)
#define OSPI_CMD_CS_PULLDOWN_CLOCKS_NO_ETX  (0)
#define OSPI_SDR_DRIVE_TIMING_BEFORE_CK (0)
#define OSPI_CK_EDGE_FALLING            (0)
#define OSPI_SDR_SAMPLING_DELAY_NONE    (0)
#define OSPI_DDR_SAMPLING_EXTENSION_NONE    (0)


// OSPI General Configuration setting
#define OSPI_PROTOCOL_MODE_1S1S1S       (0x000)
#define OSPI_PROTOCOL_MODE_8D8D8D       (0x3FF)
#define OSPI_READ_MODE_STANDARD         (0)
#define OSPI_ADDRESS_BYTES_3            (2)
#define OSPI_ADDRESS_BYTES_4            (3)
#define OSPI_DUMMY_CLOCKS_DEFAULT       (0xFF)
#define OSPI_PAGE_PROGRAM_ADDRESS_LINES (0)
#define OSPI_PAGE_SIZE_BYTES            (64)
#define OSPI_WRITE_STATUS_BIT           (0)
#define OSPI_WRITE_ENABLE_BIT           (1)
#define OSPI_PAGE_PROGRAM_COMMAND       (0)
#define OSPI_WRITE_ENABLE_COMMAND       (0)
#define OSPI_STATUS_COMMAND             (0)
#define OSPI_READ_COMMAND               (0)
#define OSPI_B_DEVICE_NUMBER_0          (0U)
#define OSPI_B_DEVICE_NUMBER_1          (1U)

#define OSPI_DATA_LATCH_DELAY_CLOCKS    (0)

// OSPI Specific Command set setting
#define OSPI_CMDSET_PROTOCOL            OSPI_PROTOCOL_MODE_1S1S1S
#define OSPI_CMDSET_FRAME_FORMAT        OSPI_FRAME_FORMAT_STANDARD
#define OSPI_CMDSET_LATENCY_MODE        OSPI_LATENCY_MODE_FIXED
#define OSPI_CMDSET_COMMAND_BYTES       OSPI_COMMAND_BYTES_1
#define OSPI_CMDSET_ADDRESS_BYTES       OSPI_ADDRESS_BYTES_3
#define OSPI_CMDSET_ADDRESS_BYTES_MM    OSPI_ADDRESS_BYTES_4
#define OSPI_CMDSET_ERASE_COMMAND       OSPI_ERASE_COMMAND_4KB

#define OSPI_CMDSET_ADDRESS_MSB_MSK         (0x80)
#define OSPI_CMDSET_ADDRESS_MSB_MSK_MM      (0xF0)
#define OSPI_CMDSET_STATUS_ADDRESS          (0)
#define OSPI_CMDSET_STATUS_ADDRESS_BYTES    (0)
#define OSPI_CMDSET_READ_COMMAND            (0x0B)
#define OSPI_CMDSET_READ_COMMAND_MM         (0x13)
#define OSPI_CMDSET_READ_DUMMY_CYCLES       (1)
#define OSPI_CMDSET_READ_DUMMY_CYCLES_MM    (0)
#define OSPI_CMDSET_PROGRAM_COMMAND         (0x02)
#define OSPI_CMDSET_PROGRAM_COMMAND_MM      (0x12)
#define OSPI_CMDSET_PROGRAM_DUMMY_CYCLES    (0)
#define OSPI_CMDSET_PROGRAM_DUMMY_CYCLES_MM (0)
#define OSPI_CMDSET_WRITE_ENABLE_COMMAND    (0x06)
#define OSPI_CMDSET_STATUS_COMMAND          (0x05)
#define OSPI_CMDSET_STATUS_DUMMY_CYCLES     (0)

#define OSPI_B_COMMAND_WRITE_ENABLE_SPI     (0x06)
#define OSPI_B_COMMAND_WRITE_ENABLE_OPI     (0x06F9)
#define OSPI_B_COMMAND_READ_STATUS_SPI      (0x05)
#define OSPI_B_COMMAND_READ_STATUS_OPI      (0x05FA)
#define OSPI_B_COMMAND_READ_DEVICE_ID_SPI   (0x9F)
#define OSPI_B_COMMAND_READ_DEVICE_ID_OPI   (0x9F60)

#define OSPI_B_ADDRESS_DUMMY            (0U)
#define OSPI_B_ADDRESS_LENGTH_ZERO      (0U)
#define OSPI_B_ADDRESS_LENGTH_THREE     (3U)
#define OSPI_B_ADDRESS_LENGTH_FOUR      (4U)

#define OSPI_B_DATA_DUMMY               (0U)
#define OSPI_B_DATA_LENGTH_ZERO         (0U)
#define OSPI_B_DATA_LENGTH_ONE          (1U)
#define OSPI_B_DATA_LENGTH_TWO          (2U)
#define OSPI_B_DATA_LENGTH_THREE        (3U)
#define OSPI_B_DATA_LENGTH_FOUR         (4U)

#define OSPI_B_COMMAND_LENGTH_SPI       (1U)
#define OSPI_B_COMMAND_LENGTH_OPI       (2U)

#define OSPI_B_DUMMY_CYCLE_WRITE_SPI    (0U)
#define OSPI_B_DUMMY_CYCLE_WRITE_OPI    (0U)
#define OSPI_B_DUMMY_CYCLE_READ_STATUS_SPI  (0U)
#define OSPI_B_DUMMY_CYCLE_READ_STATUS_OPI  (4U)

#define OSPI_FRAME_FORMAT_STANDARD      (0)
#define OSPI_LATENCY_MODE_FIXED         (0)
#define OSPI_COMMAND_BYTES_1            (1)
#define OSPI_ERASE_COMMAND_4KB          (0x20)

#define OSPI_B_PRV_ADDRESS_REPLACE_VALUE                (0xF0U)
#define OSPI_B_PRV_ADDRESS_REPLACE_ENABLE_BITS          (OSPI_B_PRV_ADDRESS_REPLACE_VALUE << (OSPI_CMCFG0CS_ADDRPEN_POS))
#define OSPI_B_PRV_ADDRESS_REPLACE_MASK                 (~(OSPI_B_PRV_ADDRESS_REPLACE_VALUE << 24))

#define OSPI_B_PRV_AUTOCALIBRATION_FRAME_INTERVAL       (0x1FU)
#define OSPI_B_PRV_AUTOCALIBRATION_NO_WRITE_CMD         (0x1U)
#define OSPI_B_PRV_AUTOCALIBRATION_SHIFT_DS_END_VALUE   (0x1FU)

#define OSPI_B_PRV_CDTBUF_CMD_OFFSET                    (16U)
#define OSPI_B_PRV_CDTBUF_CMD_UPPER_OFFSET              (24U)
#define OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_MASK             (0xFFU)
#define OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_SHIFT            (8U)
#define OSPI_B_PRV_CDTBUF_CMD_2B_VALUE_MASK             (0xFFFFU)

#define OSPI_B_PRV_COMSTT_MEMACCCH_MASK                 (0x03 << OSPI_COMSTT_MEMACCCH_POS)

#define OSPI_B_PRV_BMCTL1_CLEAR_PREFETCH_MASK           (0x03 << OSPI_BMCTL1_PBUFCLRCH_POS)
#define OSPI_B_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK   (0x03 << OSPI_BMCTL1_MWRPUSHCH_POS)

#define OSPI_B_PRV_AUTOCALIBRATION_PREAMBLE_PATTERN_0   (0xFFFF0000U)
#define OSPI_B_PRV_AUTOCALIBRATION_PREAMBLE_PATTERN_1   (0x000800FFU)
#define OSPI_B_PRV_AUTOCALIBRATION_PREAMBLE_PATTERN_2   (0x00FFF700U)
#define OSPI_B_PRV_AUTOCALIBRATION_PREAMBLE_PATTERN_3   (0xF700F708U)

#define OSPI_B_PRV_UINT32_BITS                          (32)

/** Indicates the provided protocol mode requires the Data-Strobe signal. */
#define OSPI_B_PRV_PROTOCOL_USES_DS_SIGNAL(protocol)    ((bool) (((udword_t) (protocol)) & 0x200))

/** Converts @ref spi_flash_address_bytes_t to a register compatible length value. */
#define OSPI_B_PRV_ADDR_BYTES_TO_LENGTH(spi_flash_bytes)((ubyte_t) ((spi_flash_bytes) + 1))
#define OSPI_B_PRV_AUTOCALIBRATION_DATA_SIZE            (0xFU)
#define OSPI_B_PRV_AUTOCALIBRATION_LATENCY_CYCLES       (0U)

/* These are used as modulus checking, make sure they are powers of 2. */
#define OSPI_B_PRV_CPU_ACCESS_LENGTH                    (8U)
#define OSPI_B_PRV_CPU_ACCESS_ALIGNMENT                 (8U)

#define OSPI_B_COMBINATION_FUNCTION_64BYTE              (0x1F)
#define OSPI_B_CFG_COMBINATION_FUNCTION                 (OSPI_B_COMBINATION_FUNCTION_64BYTE)
#define OSPI_B_CFG_PREFETCH_FUNCTION                    (1)

#define OSPI_B_APP_DATA_SIZE                            (64)

/* Flash device sector size */
#define OSPI_B_SECTOR_SIZE_4K                           (0x1000)
#define OSPI_B_SECTOR_SIZE_256K                          (0x40000)

/* Flash device timing */
#define OSPI_B_TIME_UNIT                                (BSP_DELAY_UNITS_MICROSECONDS)
#define OSPI_B_TIME_RESET_SETUP                         (2)           /* Type 50ns */
#define OSPI_B_TIME_RESET_PULSE                         (1000)        /* Type 500us */
#define OSPI_B_TIME_ERASE_CHIP                          (20000000)
#define OSPI_B_TIME_ERASE_256K                          (1500000)     /* Type 256KB sector is 331 KBps -> Type 0.773s */
#define OSPI_B_TIME_ERASE_4K                            (100000)      /* Type 4KB sector is 95 KBps -> Type 0.042s */
#define OSPI_B_TIME_WRITE                               (10000)       /* Type 256B page (4KB/256KB) is 595/533 KBps -> Type */

/* Flash device status bit */
#define OSPI_B_WEN_BIT_MASK                             (0x00000002)
#define OSPI_B_BUSY_BIT_MASK                            (0x00000001)

#define RESET_VALUE                                     (0x00)
#define UINT8_MAX                                       (0xFF)
#define UINT32_MAX                                      (0xFFFFFFFF)

#define PFS_PMR_BIT                                     (1UL << 16)   // Peripheral Mode
#define PFS_PCR_BIT                                     (1UL << 4)    // Pull-up Control
#define PFS_NCODR_BIT                                   (1UL << 6)    // N-ch Open-drain
#define PFS_DSCR_MIDDLE                                 (0x1UL << 10) // Drive: 01b = Middle
#define PFS_DSCR(n)                                     ((n) << 10)   // Port Drive Capability
#define PFS_PSEL(n)                                     ((n) << 24)   // Peripheral Select
#define PFS_PDR_OUT                                     (1UL << 2)    // Port Direction: Output
#define PFS_PDR_IN                                      (~PFS_PDR_OUT)// Port Direction: Input
#define PFS_PIDR_BIT                                    (1UL << 1)    // High-level state
#define PFS_PODR_BIT                                    (1UL << 0)    // Output high
#define PFS_CLEAR_ALL                                   (0x00000000UL)

// PSEL value for OSPI function
#define OSPI_PSEL                                       0x1C          // 11100b

#define HIGH_SPEED_HIGH_DRIVER                          0x2           // Drive: 10b = High-speed high-drive

#define SPI_FLASH_ERASE_SIZE_CHIP_ERASE                 (UINT32_MAX)

#define OSPI_AUTOCALIBRATION_PREAMBLE_PATTERN_ADDR      ((ubyte_t*) 0x08001000)

#define READ_DATA_SIZE                                  (128)

#ifdef RA8D2_1M
	#undef  CODEMRAM_32K_BLOCK_NUM
	#define CODEMRAM_32K_BLOCK_NUM      (0x20)
	#undef READ_DATA_SIZE               // Undef macro first
	#define READ_DATA_SIZE              (128)
#endif

#ifdef RA8D2_512K
	#undef  CODEMRAM_32K_BLOCK_NUM
	#define CODEMRAM_32K_BLOCK_NUM      (0x10)
	#undef READ_DATA_SIZE               // Undef macro first
	#define READ_DATA_SIZE              (128)
#endif

#ifdef RA8D2_CONF //The Configuration area does not support Erase, the default number for blocks 32KB is 0.
	#define CODEMRAM_32K_BLOCK_NUM      (0x00)
	#undef READ_DATA_SIZE               // Undef macro first
	#define READ_DATA_SIZE              (64)
#endif

#ifdef RA8D2_OTP //The OTP area does not support Erase, the default number for blocks 32KB is 0.
	#define CODEMRAM_32K_BLOCK_NUM      (0x00)
	#undef READ_DATA_SIZE               // Undef macro first
	#define READ_DATA_SIZE              (64)
#endif

/** Type declaration */
typedef unsigned char  ubyte_t;
typedef unsigned short uword_t;
typedef unsigned long  udword_t;
typedef unsigned long long  uqword_t;

typedef struct st_spi_flash_status
{
	/** Whether or not a write is in progress.  This is determined by reading the @ref spi_flash_cfg_t::write_status_bit
	 * from the @ref spi_flash_cfg_t::status_command. */
	bool write_in_progress;
} spi_flash_status_t;

/** Structure to define an erase command and associated erase size. */
typedef struct st_spi_flash_erase_command
{
	uword_t command;                    ///< Erase command
	udword_t size;                      ///< Size of erase for associated command, set to SPI_FLASH_ERASE_SIZE_CHIP_ERASE for chip erase
} spi_flash_erase_command_t;

/** Simple array length table structure. */
typedef struct st_ospi_b_table
{
	void  * p_table;                    ///< Pointer to the table array.
	ubyte_t length;                     ///< Number of entries in the table.
} ospi_b_table_t;

static const spi_flash_erase_command_t g_ospi_b_command_set_initial_erase_commands[] =
{
{ .command = 0xD8, .size = 131072 },
	{ .command = 0x20, .size = 4096 },
	{ .command = 0x60, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE }, 
};

static const ospi_b_table_t g_ospi_b_command_set_initial_erase_table =
		{ .p_table = (void*) g_ospi_b_command_set_initial_erase_commands, .length =
				  sizeof(g_ospi_b_command_set_initial_erase_commands)
						  / sizeof(g_ospi_b_command_set_initial_erase_commands[0]), };

/** Structure to define a direct transfer. */
typedef struct st_spi_flash_direct_transfer
{
	union
	{
		uqword_t data_u64;              ///< Data (64-bit)
		udword_t data;                  ///< Data
	};
	udword_t address;                   ///< Starting address
	uword_t  command;                   ///< Transfer command
	ubyte_t  dummy_cycles;              ///< Number of dummy cycles
	ubyte_t  command_length;            ///< Command length
	ubyte_t  address_length;            ///< Address length
	ubyte_t  data_length;               ///< Data length
} spi_flash_direct_transfer_t;

/** Direct Read and Write direction */
typedef enum e_spi_flash_direct_transfer_dir_option
{
	SPI_FLASH_DIRECT_TRANSFER_DIR_READ  = 0x0,
	SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE = 0x1
} spi_flash_direct_transfer_dir_t;

typedef enum e_ospi_b_transfer
{
	OSPI_B_TRANSFER_WRITE_ENABLE_SPI = 0,
	OSPI_B_TRANSFER_READ_STATUS_SPI,
	OSPI_B_TRANSFER_READ_DEVICE_ID_SPI,
	OSPI_B_TRANSFER_WRITE_ENABLE_OPI,
	OSPI_B_TRANSFER_READ_STATUS_OPI,
	OSPI_B_TRANSFER_READ_DEVICE_ID_OPI,
	OSPI_B_TRANSFER_MAX
} ospi_b_transfer_t;

spi_flash_direct_transfer_t g_ospi_b_direct_transfer [OSPI_B_TRANSFER_MAX] =
{
/* Transfer structure for SPI mode */
	[OSPI_B_TRANSFER_WRITE_ENABLE_SPI] =
	{
	.command        = OSPI_B_COMMAND_WRITE_ENABLE_SPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_SPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_ZERO,
	.data_length    = OSPI_B_DATA_LENGTH_ZERO,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_WRITE_SPI
	},

	[OSPI_B_TRANSFER_READ_STATUS_SPI] =
	{
	.command        = OSPI_B_COMMAND_READ_STATUS_SPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_SPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_ZERO,
	.data_length    = OSPI_B_DATA_LENGTH_ONE,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_READ_STATUS_SPI
	},

	[OSPI_B_TRANSFER_READ_DEVICE_ID_SPI] =
	{
	.command        = OSPI_B_COMMAND_READ_DEVICE_ID_SPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_SPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_ZERO,
	.data_length    = OSPI_B_DATA_LENGTH_THREE,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_READ_STATUS_SPI
	},

	/* Transfer structure for OPI mode */
	[OSPI_B_TRANSFER_WRITE_ENABLE_OPI] =
	{
	.command        = OSPI_B_COMMAND_WRITE_ENABLE_OPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_OPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_ZERO,
	.data_length    = OSPI_B_DATA_LENGTH_ZERO,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_WRITE_OPI
	},


	[OSPI_B_TRANSFER_READ_STATUS_OPI] =
	{
	.command        = OSPI_B_COMMAND_READ_STATUS_OPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_OPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_FOUR,
	.data_length    = OSPI_B_DATA_LENGTH_TWO,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_READ_STATUS_OPI
	},

	[OSPI_B_TRANSFER_READ_DEVICE_ID_OPI] =
	{
	.command        = OSPI_B_COMMAND_READ_DEVICE_ID_OPI,
	.address        = OSPI_B_ADDRESS_DUMMY,
	.data           = OSPI_B_DATA_DUMMY,
	.command_length = OSPI_B_COMMAND_LENGTH_OPI,
	.address_length = OSPI_B_ADDRESS_LENGTH_FOUR,
	.data_length    = OSPI_B_DATA_LENGTH_THREE,
	.dummy_cycles   = OSPI_B_DUMMY_CYCLE_READ_STATUS_OPI
	},
};

typedef enum ospi_status
{
	SUCCESS = 0,
	ERROR = 1,
} ospi_status_t;

/** Index of internal data buffer */
typedef enum reg_data{
	PRCR = 0,
	SCKSCR,
	SCKDIVCR,
	SCKDIVCR2,
	PLLCCR,
	PLLCCR2,
	PLL2CCR,
	PLL2CCR2,
	HOCOCR,
	MOCOCR,
	OPCCR,
	OFS1,
	MRCFREQ,
	MREFREQ,
	MRCPFB,
	MRCPC1,
	MRCBPROT1,
	OCTACKCR,
	OCTACKDIVCR,
	REG_MAX,
}reg_data;

/** Index of internal data buffer for OSPI Port */
typedef enum reg_data_ospi_port{
	PmnPFS_6_2 = 0,
	PmnPFS_6_3,
	PmnPFS_6_4,
	PmnPFS_6_5,
	PmnPFS_6_6,
	PmnPFS_6_7,
	PmnPFS_12_0,
	PmnPFS_12_1,
	PmnPFS_12_2,
	PmnPFS_12_3,
	PmnPFS_12_4,
	PmnPFS_12_5,
	PmnPFS_12_6,
	PmnPFS_12_7,
	PmnPFS_12_9,
	PmnPFS_12_10,
	REG_OSPI_PORT_MAX,
}reg_data_ospi_port;

/** Index of internal data buffer for OSPI Configuration */
typedef enum reg_data_ospi_configuration{
	BMCTL0 = 0,
	LIOCFGCS,
	WRAPCFG,
	BMCFGCH0,
	BMCFGCH1,
	CMCFG0,
	CMCFG1,
	CMCFG2,
	REG_OSPI_CONFIGURATION_MAX,
}reg_data_ospi_configuration;

/** Register save structure */
static udword_t s_udwClockUserVal[REG_MAX];
static udword_t s_udwOSPIPortUserVal[REG_OSPI_PORT_MAX];
static udword_t s_udwOSPIConfigurationUserVal[REG_OSPI_CONFIGURATION_MAX];

/*
 Current ICLK frequency
*/
static udword_t dwICLKValue;
/*
 MRCMHZ/MREMHZ Change Flag
 0: low to high change
 1: high to low change
*/
static udword_t dwMRCFREQChgFlg;
static udword_t dwMREFREQChgFlg;

/** Internal function declaration */
static udword_t change_clock_and_mode(udword_t *udwp_clk, udword_t *udwp_cmram_clk, udword_t *udwp_emram_clk);
static udword_t check_clock_and_mode(udword_t *udwp_clk, udword_t *udwp_cmram_clk, udword_t *udwp_emram_clk, udword_t *udw_octa_clk);
static udword_t restore_clock_and_mode(void);
static ubyte_t saveModeAndClock(void);
static udword_t pe_mode_entry(udword_t dwAddr, udword_t *adrRet);
static udword_t ilgl_chk(void);
static void set_mrcfreq(udword_t *udwp_clk);
static void set_mrefreq(udword_t *udwp_clk);
static void Wait_us(udword_t us);
static void read_page(udword_t adr, udword_t sz, ubyte_t *buf);
static ospi_status_t r_ospi_b_protocol_specific_settings(void);
static ospi_status_t r_ospi_b_open(void);
static void r_ospi_b_direct_transfer(spi_flash_direct_transfer_t * const p_transfer, spi_flash_direct_transfer_dir_t direction);
static ospi_status_t r_ospi_b_erase(ubyte_t * const p_device_address, udword_t byte_count);
static bool r_ospi_b_status_sub (ubyte_t bit_pos);
static ospi_status_t ospi_b_wait_operation(udword_t timeout);
static ospi_status_t r_ospi_b_statusget (spi_flash_status_t * const p_status);
static ospi_status_t r_ospi_b_write (ubyte_t const * const p_src, ubyte_t * const p_dest, udword_t byte_count);
static ospi_status_t r_ospi_b_program(ubyte_t * const p_device_address, ubyte_t byte_count, ubyte_t const * const p_src);
static ospi_status_t r_ospi_b_write_enable();
static void ospi_port_configuration();

/**************************************************************************//**
* @details Initialize Flash Programming Functions
* @param[in] adr:  Device Base Address
* @param[in] clk:  Clock Frequency (Hz)
* @param[in] fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int Init (unsigned long adr, unsigned long clk, unsigned long fnc)
{
	ubyte_t ub_ret_val;
	udword_t udwMRPCKValue;
	udword_t udwMRICKValue;
	udword_t udwOCTACLKValue;
	
	// Initialize the maximum value of ICLK
	dwICLKValue = 250000000;
	
	// Initialize the maximum value of MRPCK
	udwMRPCKValue = 125000000;
	
	// Initialize the maximum value of MRICK
	udwMRICKValue = 250000000;
	
	// Initialize the maximum value of OCTACLK
	udwOCTACLKValue = 333330000;
	
	// Initialize MRCMHZ Change Flag
	dwMRCFREQChgFlg = 0;
	
	// Initialize MREMHZ Change Flag
	dwMREFREQChgFlg = 0;
	
	// Save current settings
	saveModeAndClock();
	ub_ret_val = check_clock_and_mode(&clk, &udwMRICKValue, &udwMRPCKValue, &udwOCTACLKValue);
	/*
	Check if flash rewritable clock and power control mode
	and save current frequency for dwICLKValue
	*/
	
#ifdef CHG_CLK_AND_MOD_ENA
	/** If the clock and mode cannot rewrite the flash, switch to a possible state */
	if (ub_ret_val)
	{
		ub_ret_val = change_clock_and_mode(&clk, &udwMRICKValue,  &udwMRPCKValue);
		
		// Save ICLK frequency for wait processing
		dwICLKValue = clk;
	}
#endif
	
	if (ub_ret_val)
	{
		/** Failure if the flash cannot be rewritten */
		return (1); // Failed
	}
	
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	ospi_status_t stat = SUCCESS;
	
	/* Configure OSPI ports */ 
	ospi_port_configuration();
	
	/* Open OSPI module */
	stat |= r_ospi_b_open();
	
	 /* Reset flash device */
	M32(OSPI_LIOCTL_ADDR(0)) = M32(OSPI_LIOCTL_ADDR(0)) & (~OSPI_LIOCTL_RSTCS0_MSK);
	Wait_us(10);
	M32(OSPI_LIOCTL_ADDR(0)) = M32(OSPI_LIOCTL_ADDR(0)) | OSPI_LIOCTL_RSTCS0_MSK;
	Wait_us(10);
	
	return stat;
#endif
	
	return ilgl_chk(); // Check for command locked and illegal status 
}

/**************************************************************************//**
* @details   De-Initialize Flash Programming Functions
* @param[in] fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int UnInit (unsigned long fnc)
{
	M16(MCU_MENTRYR_ADDR) = MENTRYR_READ_MODE; // Transition to read mode
	while (M16(MCU_MENTRYR_ADDR) != MENTRYR_CHK_READ) // Wait until switching to read mode
	{
		;
	}
	
	/** Restore clock and power control mode to the state before flash rewriting */
	if (restore_clock_and_mode())
	{
		return (1); // Failed
	}
	return (0); // OK
}

/**************************************************************************//**
* @details   Blank Check in Flash Memory
* @param[in] adr:  Block start Address
* @param[in] sz:   Block size in byte
* @param[in] pat:  Pattern to compare
* @retval    0 - Memory is blank,  1 - Memory is not blank
******************************************************************************/
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat)
{
#if (defined RA8D2_CONF) || (defined RA8D2_OTP) 
	/** Configuration area and OTP area not support BlankCheck */
	return (0); // OK
#endif

	ubyte_t ub_rd_buf[128];
	unsigned long adr_tmp;
	uword_t i, j ,k, m;
	adr_tmp = adr;
	m = 0;
	
	M16(MCU_MENTRYR_ADDR) = MENTRYR_READ_MODE; // Transition to read mode
	while (M16(MCU_MENTRYR_ADDR) != MENTRYR_CHK_READ) // Wait until switching to read mode
	{
		;
	}
	
	for (i = 0; i < sz ; i += 128)
	{
		read_page(adr_tmp, 128, &ub_rd_buf[0]);
		
		/** Determine size to compare */
		if ((sz - i) >= 128)
		{
			k = 128;
		} else
		{
			k = (sz - i);
		}
		
		/** Check up to 128 bytes if equal to pattern "pat" */
		for (j = 0; j < k; j++)
		{
			if (ub_rd_buf[j] != pat)
			{
				return (1); // Memory is not blank
			}
			m++;
		}
		adr_tmp = adr + m;
	}
	
	return (0); // Memory is blank
}

/**************************************************************************//**
* @details   Erase complete Flash Memory
* @param     None
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int EraseChip (void)
{
#if (defined RA8D2_CONF) || (defined RA8D2_OTP) 
	/** Configuration area and OTP area not support EraseChip */
	return (0); // OK
#endif

#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	ospi_status_t stat = SUCCESS;
	udword_t sector_size = RESET_VALUE;
	udword_t erase_timeout = RESET_VALUE;
	
	sector_size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE;
	erase_timeout = OSPI_B_TIME_ERASE_CHIP;
	
	stat |= r_ospi_b_erase((ubyte_t * const)0x08000000, sector_size);
	
	stat |= ospi_b_wait_operation(erase_timeout);
	
	return stat;
#else
	udword_t adr;
	
	for (adr = MCU_CODEMRAM_ADDR; adr < (MCU_CODEMRAM_ADDR + CODEMRAM_32K_BLOCK_NUM * MCU_CODEMRAM_BLOCK_SIZE); adr += MCU_CODEMRAM_BLOCK_SIZE)
	{
		if (EraseSector(adr))
		{
			return 1;
		}
	}
	
	return (0); // OK
#endif
}

/**************************************************************************//**
* @details   Erase Sector in Flash Memory
* @param[in] adr:  Sector Address
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int EraseSector (unsigned long adr)
{
#if (defined RA8D2_CONF) || (defined RA8D2_OTP) 
	/** Configuration area and OTP area not support EraseSector */
	return (0); // OK
#endif

#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	ospi_status_t stat = SUCCESS;
	udword_t sector_size = RESET_VALUE;
	udword_t erase_timeout = RESET_VALUE;
	
	sector_size = OSPI_B_SECTOR_SIZE_4K;
	erase_timeout = OSPI_B_TIME_ERASE_4K;
	
	stat |= r_ospi_b_erase((ubyte_t * const)adr, sector_size);
	
	stat |= ospi_b_wait_operation(erase_timeout);
	
	return stat;
#else
	ubyte_t uby_ilgl_chk;
	uword_t uw_wrt_cnt;
	uword_t uw_wrt_byte;
	uword_t i;
	udword_t udw_adr_tmp;
	
	unsigned long sz = MCU_CODEMRAM_BLOCK_SIZE;
	udword_t pat = 0xFFFFFFFF;
	
	pe_mode_entry(adr,&udw_adr_tmp); // Check for command locked and illegal status
	
	M16(MCU_MRCPC1_ADDR) = MRCPC1_KEY | MRCPC1_MRCPSEN_ENABLE;
	M16(MCU_MRCBPROT1_ADDR) = MRCBPROT1_KEY | MRCBPROT1_BPCN1_DISABLE;
	
	while (sz)
	{
		while ((M8(MCU_MRCPS_ADDR) & MRCPS_PRGBSYC) == MRCPS_PRGBSYC) // Code MRAM is not in program status.
		{
			;
		}
		
		while ((M8(MCU_MRCPS_ADDR) & MRCPS_ABUFFULL) == MRCPS_ABUFFULL) // Address buffer is not full, and code MRAM write transaction is acceptable
		{
			;
		}
		
		uw_wrt_cnt = 8;
		uw_wrt_byte = uw_wrt_cnt * 4;
		
		for(i = 0; i < uw_wrt_cnt; i++)
		{
			/** Store 4 bytes each for programming data */
			M32(adr + (i * 4)) = pat;
			
			__asm volatile ("dmb");
			M16(MCU_MRCFLR_ADDR) = MRCFLR_KEY | MRCFLR_MRCFL_EXECUTE;
			
			while ((M8(MCU_MRCPS_ADDR) & MRCPS_ABUFEMP) == 0) // Address buffer is empty, and code MRAM write data cannot flush
			{
				;
			}
			
			while ((M8(MCU_MRCPS_ADDR) & MRCPS_PRGBSYC) == MRCPS_PRGBSYC) // Code MRAM is not in program status.
			{
				;
			}
			
			if (M8(MCU_MRCPS_ADDR) & (MRCPS_PRGERRC | MRCPS_ECCERRC)) 
			{
				return (1);
			}
		}
				
		adr += uw_wrt_byte;
		sz -= uw_wrt_byte; // Update remaining size 
	}
	
	/** Check for command locked and illegal status */
	uby_ilgl_chk = ilgl_chk();
	if (uby_ilgl_chk)
	{
		return (1); // Failed
	}
	
	return (0); // OK
#endif
}

/**************************************************************************//**
* @details   Program Page in Flash Memory
* @param[in] adr:  Page Start Address
* @param[in] sz:   Page Size
* @param[in] buf:  Page Data
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	ospi_status_t stat = SUCCESS;
	
	while (sz)
	{
		udword_t current_size = (sz < OSPI_B_APP_DATA_SIZE) ? sz : OSPI_B_APP_DATA_SIZE;
		stat |= r_ospi_b_write((ubyte_t const * const) buf, (ubyte_t * const) adr, current_size);
		
		/* Wait until write operation completes */
		stat |= ospi_b_wait_operation(OSPI_B_TIME_WRITE);
		
		buf += current_size;
		adr += current_size;
		sz -= current_size;
	}
	
	return stat;
#else
	ubyte_t uby_ilgl_chk;
	uword_t uw_start_data = 0;
	uword_t uw_wrt_cnt;
	uword_t uw_wrt_byte;
	uword_t i;
	udword_t udw_adr_tmp;
	ubyte_t uby_area_flg = 0;
	
	pe_mode_entry(adr,&udw_adr_tmp); // Check for command locked and illegal status
	M16(MCU_MRCPC1_ADDR) = MRCPC1_KEY | MRCPC1_MRCPSEN_ENABLE;
	M16(MCU_MRCBPROT1_ADDR) = MRCBPROT1_KEY | MRCBPROT1_BPCN1_DISABLE;
	
	while (sz)
	{
	
		while ((M8(MCU_MRCPS_ADDR) & MRCPS_PRGBSYC) == MRCPS_PRGBSYC) // Code MRAM is not in program status.
		{
			;
		}
		
		while ((M8(MCU_MRCPS_ADDR) & MRCPS_ABUFFULL) == MRCPS_ABUFFULL) // Address buffer is not full, and code MRAM write transaction is acceptable
		{
			;
		}
		
		if (adr < MCU_CONF_EXTRAMRAM_ADDR)
		{
			uby_area_flg = 0; // Code MRAM area
			
			if (sz < 32)
			{
				for (i = sz; i < 32; i++)
				{
					buf[uw_start_data + i] = 0xFF;
				}
				sz = 32;
			}
		} else if (adr < MCU_OTP_EXTRAMRAM_ADDR)
		{
			uby_area_flg = 1; // Configuration setting area
			
			if (sz < 16)
			{
				for (i = sz; i < 16; i++)
				{
					buf[uw_start_data + i] = 0xFF;
				}
				sz = 16;
			}
		} else
		{
			uby_area_flg = 2; // OTP area
			
			if (sz < 16)
			{
				for (i = sz; i < 16; i++)
				{
					buf[uw_start_data + i] = 0xFF;
				}
				sz = 16;
			}
		}
		
		
		if (uby_area_flg == 0) 
		{
			uw_wrt_cnt = 8;
			uw_wrt_byte = uw_wrt_cnt * 4;
			
			for(i = 0; i < uw_wrt_cnt; i++)
			{
				/** Store 4 bytes each for programming data */
				M32(adr + (i * 4)) = (udword_t)(buf[uw_start_data] | (buf[uw_start_data+1] << 8) | (buf[uw_start_data+2] << 16) | (buf[uw_start_data+3] << 24));
				uw_start_data +=4;
				
				__asm volatile ("dmb");
				
				M16(MCU_MRCFLR_ADDR) = MRCFLR_KEY | MRCFLR_MRCFL_EXECUTE;
				
				while ((M8(MCU_MRCPS_ADDR) & MRCPS_ABUFEMP) == 0) // Address buffer is empty, and code MRAM write data cannot flush
				{
					;
				}
				
				while ((M8(MCU_MRCPS_ADDR) & MRCPS_PRGBSYC) == MRCPS_PRGBSYC) // Code MRAM is not in program status.
				{
					;
				}
				
				if (M8(MCU_MRCPS_ADDR) & (MRCPS_PRGERRC | MRCPS_ECCERRC)) 
				{
					return 1;
				}
				
			}
			
		} else
		{
			M32(MCU_MSADDR_ADDR) = adr; // Set the program start address
			if (uby_area_flg == 2)      // OTP area
			{
				M8(MCU_MACI_CMD_AREA_ADDR) = MACI_OTP_EXTRA_CMD; // Execution of program command
				M8(MCU_MACI_CMD_AREA_ADDR) = MACI_OTP_EXTRA_NUM; // Set the number of write data access
				uw_wrt_cnt = MACI_OTP_EXTRA_NUM;
				uw_wrt_byte = MACI_OTP_EXTRA_NUM * 2; // Data size to write with one command
			} else                      // Configuration setting area
			{
				M8(MCU_MACI_CMD_AREA_ADDR) = MACI_CONF_EXTRA_CMD; // Execution of program command
				M8(MCU_MACI_CMD_AREA_ADDR) = MACI_CONF_EXTRA_NUM; // Set the number of write data access
				uw_wrt_cnt = MACI_CONF_EXTRA_NUM;
				uw_wrt_byte = MACI_CONF_EXTRA_NUM * 2; // Data size to write with one command
			}
			
			for (i = 0; i < uw_wrt_cnt; i++)
			{
				/** Store 2 bytes each for programming data */
				M16(MCU_MACI_CMD_AREA_ADDR) = (uword_t)(buf[uw_start_data] | (buf[uw_start_data+1] << 8));
				uw_start_data +=2;
			}
			
			M8(MCU_MACI_CMD_AREA_ADDR) = MACI_END_CMD; // Execution of termination command
			
			while (!(M32(MCU_MSTATR_ADDR) & MSTATR_MRDY))
			{
				; // Waiting for command completion
			}
			
			if (M32(MCU_MSTATR_ADDR) & MSTATR_PRGERR)
			{
				/** Programming error */
				return (1); // Failed
			}
		}
		
		adr += uw_wrt_byte;
		sz -= uw_wrt_byte; // Update remaining size 
	}
	
	/** Check for command locked and illegal status */
	uby_ilgl_chk = ilgl_chk();
	if (uby_ilgl_chk)
	{
		return (1); // Failed
	}
	
	return (0); // OK
#endif
}

/**************************************************************************//**
* @details   Verify the size from the specified address
* @param[in] adr:  Start Address
* @param[in] sz:   Page Size
* @param[in] buf:  Page Data
* @retval    (adr + sz) - OK,  (adr + m) - Failed (Represent the failing address)
******************************************************************************/
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf)
{
	ubyte_t ub_rd_buf[READ_DATA_SIZE];
	unsigned long adr_tmp;
	uword_t i, j ,k, m;
	adr_tmp = adr;
	m = 0;
	
	M16(MCU_MENTRYR_ADDR) = MENTRYR_READ_MODE; // Transition to read mode
	while (M16(MCU_MENTRYR_ADDR) != MENTRYR_CHK_READ) // Wait until switching to read mode
	{
		;
	}
	
	for (i = 0; i < sz ; i += READ_DATA_SIZE)
	{
		read_page(adr_tmp, READ_DATA_SIZE, &ub_rd_buf[0]);
	
		/** Determine size to compare */
		if ((sz - i) >= READ_DATA_SIZE)
		{
			k = READ_DATA_SIZE;
		} else
		{
			k = (sz - i);
		}
		
		/** Check up to 128 bytes if equal to read data */
		for (j = 0; j < k; j++)
		{
			if (ub_rd_buf[j] != buf[m])
			{
				return (adr + m); // Address of Memory is not blank
			}
			m++;
		}
		adr_tmp = adr + m;
	}
	
	return (adr + sz); // Verify is success, in this line: sz = m
}


/**************************************************************************//**
* @details   Check if flash rewritable clock and power control mode
* @param[in] udwp_clk:       System Clock Frequency (Hz)
* @param[in] udwp_cmram_clk: Code MRAM Clock Frequency (Hz)
* @param[in] udwp_emram_clk: Extra MRAM Clock Frequency (Hz)
* @param[in] udw_octa_clk:   Octal-SPI Clock Frequency (Hz)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t check_clock_and_mode(udword_t *udwp_clk, udword_t *udwp_cmram_clk, udword_t *udwp_emram_clk, udword_t *udw_octa_clk)
{
	udword_t udw_sckdivcr;
	udword_t udw_sckdivcr_val;
	udword_t udw_clk_tmp, udw_clk_tmp_PLLMUL, udw_clk_tmp_PLL2MUL, udw_hoco_clk, udw_plli_clk, udw_pll2i_clk, udw_pll_vco_clk, udw_pll2_vco_clk;
	uword_t uw_bai, uw_bai2;
	uword_t uw_plli_div, uw_pll2i_div;
	uword_t uw_sckdivcr2;
	ubyte_t ub_octackdivcr;
	ubyte_t ub_octackdivcr_val;
	ubyte_t octa_clk_div[10] = {1, 2, 4, 6, 8, 3, 5, 10, 16, 32};
	
	udw_sckdivcr = M32(MCU_SCKDIVCR_ADDR);
	uw_sckdivcr2 = M16(MCU_SCKDIVCR2_ADDR);
	ub_octackdivcr = M8(MCU_OCTACKDIVCR_ADDR);
	
	// Check HOCO clock
	if (OFS1_HOCOFRQ0_16MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
	{
		udw_hoco_clk = 16000000; // HOCO frequency is 16MHz
	} else if (OFS1_HOCOFRQ0_18MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
	{
		udw_hoco_clk = 18000000; // HOCO frequency is 18MHz
	} else if (OFS1_HOCOFRQ0_20MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
	{
		udw_hoco_clk = 20000000; // HOCO frequency is 20MHz
	} else if (OFS1_HOCOFRQ0_32MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
	{
		udw_hoco_clk = 32000000; // HOCO frequency is 32MHz
	} else if (OFS1_HOCOFRQ0_48MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
	{
		udw_hoco_clk = 48000000; // HOCO frequency is 48MHz
	} else
	{
		//
	}
	
	// Check PLLCCR
	if (s_udwClockUserVal[PLLCCR] & PLLCCR_PLSRCSEL_HOCO)
	{ // Input clock source is HOCO
		udw_plli_clk = udw_hoco_clk;
	} else
	{
		udw_plli_clk = *udwp_clk; // Input clock source is External clock
	}
	
	uw_plli_div = (s_udwClockUserVal[PLLCCR] & PLLCCR_PLIDIV_MASK) + 1; // PLL1 input frequency division
	uw_bai = (((s_udwClockUserVal[PLLCCR] & PLLCCR_MUL) >> 8) + 1) / 2; // PLL1 frequency multiplication factor (PLLMUL)
	udw_clk_tmp_PLLMUL = (udw_plli_clk / (udword_t)uw_plli_div) * (udword_t)uw_bai; // Part PLLMUL of Frequency output PLL1
	
	/* Frequency output as PLL1 = Part PLLMUL of Frequency output PLL1 + Part PLLMULNF of Frequency output PLL1 */
	if (PLLCCR_PLLMULNF_0ADD_MUL == (s_udwClockUserVal[PLLCCR] & PLLCCR_PLLMULNF_MASK))
	{
		udw_pll_vco_clk = udw_clk_tmp_PLLMUL;
	}
	else if (PLLCCR_PLLMULNF_1_3ADD_MUL == (s_udwClockUserVal[PLLCCR] & PLLCCR_PLLMULNF_MASK))
	{
		udw_pll_vco_clk = udw_clk_tmp_PLLMUL + ((udw_plli_clk / (udword_t)uw_plli_div) * PLLCCR_PLLMULNF_1_3VALUE);
	}
	else if (PLLCCR_PLLMULNF_2_3ADD_MUL == (s_udwClockUserVal[PLLCCR] & PLLCCR_PLLMULNF_MASK))
	{
		udw_pll_vco_clk = udw_clk_tmp_PLLMUL + ((udw_plli_clk / (udword_t)uw_plli_div) * PLLCCR_PLLMULNF_2_3VALUE);
	}
	else // (PLLCCR_PLLMULNF_1_2ADD_MUL == (s_udwClockUserVal[PLLCCR] & PLLCCR_PLLMULNF_MASK))
	{
		udw_pll_vco_clk = udw_clk_tmp_PLLMUL + ((udw_plli_clk / (udword_t)uw_plli_div) * PLLCCR_PLLMULNF_1_2VALUE);
	}
	
	// Check PLL2CCR
	if (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PL2SRCSEL_HOCO)
	{ // Input clock source is HOCO
		udw_pll2i_clk = udw_hoco_clk;
	} else
	{
		udw_pll2i_clk = *udwp_clk; // Input clock source is External clock
	}
	
	uw_pll2i_div = (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PL2IDIV_MASK) + 1; // PLL2 input frequency division
	uw_bai2 = (((s_udwClockUserVal[PLL2CCR] & PLL2CCR_MUL) >> 8) + 1) / 2; // PLL2 frequency multiplication factor (PLL2MUL)
	udw_clk_tmp_PLL2MUL = (udw_pll2i_clk / (udword_t)uw_pll2i_div) * (udword_t)uw_bai2; // Part PLL2MUL of Frequency output PLL2
	
	/* Frequency output as PLL2 = Part PLL2MUL of Frequency output PLL2 + Part PLL2MULNF of Frequency output PLL2 */
	if (PLL2CCR_PLL2MULNF_0ADD_MUL == (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PLL2MULNF_MASK))
	{
		udw_pll2_vco_clk = udw_clk_tmp_PLL2MUL;
	}
	else if (PLL2CCR_PLL2MULNF_1_3ADD_MUL == (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PLL2MULNF_MASK))
	{
		udw_pll2_vco_clk = udw_clk_tmp_PLL2MUL + ((udw_pll2i_clk / (udword_t)uw_pll2i_div) * PLL2CCR_PLL2MULNF_1_3VALUE);
	}
	else if (PLL2CCR_PLL2MULNF_2_3ADD_MUL == (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PLL2MULNF_MASK))
	{
		udw_pll2_vco_clk = udw_clk_tmp_PLL2MUL + ((udw_pll2i_clk / (udword_t)uw_pll2i_div) * PLL2CCR_PLL2MULNF_2_3VALUE);
	}
	else // (PLL2CCR_PLL2MULNF_1_2ADD_MUL == (s_udwClockUserVal[PLL2CCR] & PLL2CCR_PLL2MULNF_MASK))
	{
		udw_pll2_vco_clk = udw_clk_tmp_PLL2MUL + ((udw_pll2i_clk / (udword_t)uw_pll2i_div) * PLL2CCR_PLL2MULNF_1_2VALUE);
	}
	
	
	/** Calculate FCLK */
	switch (s_udwClockUserVal[SCKSCR])
	{
		case SCKSCR_CKSEL_PLL1P:
			/* Frequency output as PLL1P */
			uw_plli_div = (s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVP_MASK) + 1; // PLL1 Output Frequency Division Ratio Select for output clock P (PLL1P)
			udw_clk_tmp = udw_pll_vco_clk / (udword_t)uw_plli_div; // Frequency output as PLL1P
		break;
		case SCKSCR_CKSEL_MOSC:
			udw_clk_tmp = *udwp_clk; // Input clock source is External clock
		break;
		case SCKSCR_CKSEL_MOCO:
			udw_clk_tmp = 8000000; // MOCO frequency is 8MHz
		break;
		case SCKSCR_CKSEL_HOCO:
			udw_clk_tmp = udw_hoco_clk;
		break;
		case SCKSCR_CKSEL_SOSC: // Failed because SOSC frequency is below 4MHz
			udw_clk_tmp = 32768;
		break;
		default:
			return (1); // Failed - This case cannot happen
	}

	udw_sckdivcr = (udw_sckdivcr & SCKDIVCR_MRPCK_MASK) >> 28;
	if (udw_sckdivcr & 0x8) //case udw_sckdivcr = 8 (1/3), 9 (1/6) ,or 10 (1/12)
	{ 
		udw_sckdivcr_val = 3 * (1 << (udw_sckdivcr - 8)); // MRPCK division value
		*udwp_emram_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate MRPCK frequency
	}
	else
	{
		udw_sckdivcr_val = 1 << udw_sckdivcr; // MRPCK division value
		*udwp_emram_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate MRPCK frequency
	}
	
	udw_sckdivcr = M32(MCU_SCKDIVCR_ADDR);
	udw_sckdivcr = (udw_sckdivcr & SCKDIVCR_ICK_MASK) >> 24;
	if (udw_sckdivcr & 0x8) //case udw_sckdivcr = 8 (1/3), 9 (1/6) ,or 10 (1/12)
	{
		udw_sckdivcr_val = 3 * (1 << (udw_sckdivcr - 8)); // ICLK division value
		dwICLKValue = udw_clk_tmp / udw_sckdivcr_val; // Calculate ICLK frequency
	}
	else
	{	
		udw_sckdivcr_val = 1 << udw_sckdivcr; // ICLK division value
		dwICLKValue = udw_clk_tmp / udw_sckdivcr_val; // Calculate ICLK frequency
	}
	
	uw_sckdivcr2 = (uw_sckdivcr2 & SCKDIVCR2_MRICK_MASK) >> 12; 
	if (uw_sckdivcr2 & 0x8) //case udw_sckdivcr = 8 (1/3), 9 (1/6) ,or 10 (1/12)
	{ 
		udw_sckdivcr_val = 3 * (1 << (uw_sckdivcr2 - 8)); // MRICK division value
		*udwp_cmram_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate MRICK frequency
	}
	else
	{
		udw_sckdivcr_val = 1 << uw_sckdivcr2; // MRICK division value
		*udwp_cmram_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate MRICK frequency
	}
	
	/** Failed if power control mode is Low-speed mode */
	if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCCR) != OPCCR_OPCCR_HIGH)
	{
		return (1); // Failed
	}
	
	/** Failed because outside the allowable frequency(0-250MHz) of ICLK */
	if (((dwICLKValue) <= 0) || (250000000 < (dwICLKValue)))
	{
		return (1); // Failed
	}
	
	/** Failed because outside the allowable frequency(0-250MHz) of MRICK */
	if (((*udwp_cmram_clk) <= 0) || (250000000 < (*udwp_cmram_clk)))
	{
		return (1); // Failed
	}
	
	/** Failed because outside the allowable frequency(0-125MHz) of MRPCLK */
	if (((*udwp_emram_clk) <= 0) || (125000000 < (*udwp_emram_clk)))
	{
		return (1); // Failed
	}
	
	set_mrcfreq(udwp_cmram_clk);
	
	set_mrefreq(udwp_emram_clk);
	
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	/** Calculate OCTACLK */
	switch (s_udwClockUserVal[OCTACKCR] & OCTACKCR_OCTACKSEL_MASK)
	{
		case OCTACKCR_OCTACKSEL_PLL1P:
			/* Frequency output as PLL1P */
			uw_plli_div = (s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVP_MASK) + 1; // PLL1 Output Frequency Division Ratio Select for output clock P (PLL1P)
			udw_clk_tmp = udw_pll_vco_clk / (udword_t)uw_plli_div; // Frequency output as PLL1P
		break;
		case OCTACKCR_OCTACKSEL_PLL1Q:
			/* Frequency output as PLL1Q */
			if (((s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVQ_MASK) >> 4) == 9)
			{
				udw_clk_tmp = (udw_pll_vco_clk * 2) / 3; // Frequency output as PLL1Q
			} else
			{
				uw_plli_div = ((s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVQ_MASK) >> 4) + 1; // PLL1 Output Frequency Division Ratio Select for output clock Q (PLL1Q)
				udw_clk_tmp = udw_pll_vco_clk / (udword_t)uw_plli_div; // Frequency output as PLL1Q
			}
		break;
		case OCTACKCR_OCTACKSEL_PLL1R:
			/* Frequency output as PLL1R */
			if (((s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVR_MASK) >> 8) == 9){
				udw_clk_tmp = (udw_pll_vco_clk * 2) / 3; // Frequency output as PLL1R
			} else
			{
				uw_plli_div = ((s_udwClockUserVal[PLLCCR2] & PLLCCR2_PLODIVR_MASK) >> 8) + 1; // PLL1 Output Frequency Division Ratio Select for output clock R (PLL1R)
				udw_clk_tmp = udw_pll_vco_clk / (udword_t)uw_plli_div; // Frequency output as PLL1R
			}
		break;
		case OCTACKCR_OCTACKSEL_PLL2P:
			/* Frequency output as PLL2P */
			uw_pll2i_div = (s_udwClockUserVal[PLL2CCR2] & PLL2CCR2_PL2ODIVP_MASK) + 1; // PLL2 Output Frequency Division Ratio Select for output clock P (PLL2P)
			udw_clk_tmp = udw_pll2_vco_clk / (udword_t)uw_pll2i_div; // Frequency output as PLL2P
		break;
		case OCTACKCR_OCTACKSEL_PLL2Q:
			/* Frequency output as PLL2Q */
			if (((s_udwClockUserVal[PLL2CCR2] & PLL2CCR2_PL2ODIVQ_MASK) >> 4) == 9)
			{
				udw_clk_tmp = (udw_pll2_vco_clk * 2) / 3; // Frequency output as PLL2Q
			} else
			{
				uw_pll2i_div = ((s_udwClockUserVal[PLL2CCR2] & PLL2CCR2_PL2ODIVQ_MASK) >> 4) + 1; // PLL2 Output Frequency Division Ratio Select for output clock Q (PLL2Q)
				udw_clk_tmp = udw_pll2_vco_clk / (udword_t)uw_pll2i_div; // Frequency output as PLL2Q
			}
		break;
		case OCTACKCR_OCTACKSEL_PLL2R:
			/* Frequency output as PLL2R */
			if (((s_udwClockUserVal[PLL2CCR2] & PLL2CCR2_PL2ODIVR_MASK) >> 8) == 9)
			{
				udw_clk_tmp = (udw_pll2_vco_clk * 2) / 3; // Frequency output as PLL2R
			} else
			{
				uw_pll2i_div = ((s_udwClockUserVal[PLL2CCR2] & PLL2CCR2_PL2ODIVR_MASK) >> 8) + 1; // PLL2 Output Frequency Division Ratio Select for output clock R (PLL2R)
				udw_clk_tmp = udw_pll2_vco_clk / (udword_t)uw_pll2i_div; // Frequency output as PLL2R
			}
		break;
		case OCTACKCR_OCTACKSEL_MOSC:
			udw_clk_tmp = *udwp_clk; // Input clock source is External clock
		break;
		case OCTACKCR_OCTACKSEL_MOCO:
			udw_clk_tmp = 8000000; // MOCO frequency is 8MHz
		break;
		case OCTACKCR_OCTACKSEL_HOCO:
			udw_clk_tmp = udw_hoco_clk;
		break;
		case OCTACKCR_OCTACKSEL_SOSC:
			udw_clk_tmp = 32768;
		break;
		case OCTACKCR_OCTACKSEL_LOCO:
			udw_clk_tmp = 32768;
		break;
		default:
			return (1); // Failed - This case cannot happen
	}
	
	ub_octackdivcr = ub_octackdivcr & OCTACKDIVCR_OCTACKDIV_MASK;
	ub_octackdivcr_val = octa_clk_div[ub_octackdivcr]; // OCTACLK division value
	*udw_octa_clk = udw_clk_tmp / ub_octackdivcr_val; // Calculate OCTACLK frequency
	
	/** Failed because outside the allowable frequency(0-333.33MHz) of OCTACLK */
	if (((*udw_octa_clk) <= 0) || (333330000 < (*udw_octa_clk)))
	{
		return (1); // Failed
	}
#endif
	
	return (0); // OK
}

/**************************************************************************//**
* @details   Change to flash rewritable clock and power control mode
* @param[in] udwp_clk:       System Clock Frequency (Hz)
* @param[in] udwp_cmram_clk: Code MRAM Clock Frequency (Hz)
* @param[in] udwp_emram_clk: Extra MRAM Clock Frequency (Hz)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t change_clock_and_mode(udword_t *udwp_clk, udword_t *udwp_cmram_clk, udword_t *udwp_emram_clk)
{
	M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | PRCR_PRC1 | PRCR_PRC0; // Unprotect some registers
	
	if ((*udwp_cmram_clk) <= 0) // MRICK low to high change
	{
		dwMRCFREQChgFlg = 0;
	}
	else if (250000000 < (*udwp_cmram_clk)) // MRICK high to low change
	{
		M8(MCU_MRCPFB_ADDR) = MRCPFB_MPFBEN_DIS;
		dwMRCFREQChgFlg = 1;
	}
	else
	{
		; // This process does not pass because there is no change in MRICK
	}
	
	if ((*udwp_emram_clk) <= 0) // MRPCLK low to high change
	{
		dwMREFREQChgFlg = 0;
	}
	else if (125000000 < (*udwp_emram_clk)) // MRPCLK high to low change
	{
		dwMREFREQChgFlg = 1;
	}
	else
	{
		; // This process does not pass because there is no change in MRPCLK
	}
	
	/** If it is Low-speed mode, set it to High-speed mode */
	if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCCR) != OPCCR_OPCCR_HIGH)
	{
		M8(MCU_OPCCR_ADDR) = OPCCR_OPCCR_HIGH;
		while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
		{
			; // Wait till transition completed
		}
	}

	/** If OFS1 is in the initial state, set the clock source to MOCO */
	if ((s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) != OFS1_HOCOFRQ0_16MHZ && (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) != OFS1_HOCOFRQ0_18MHZ && (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) != OFS1_HOCOFRQ0_20MHZ && (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) != OFS1_HOCOFRQ0_32MHZ && (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) != OFS1_HOCOFRQ0_48MHZ) 
	{
		*udwp_emram_clk = 8000000; // MRPCK frequency is 8MHz
		*udwp_cmram_clk = 8000000; // MRICK frequency is 8MHz
		*udwp_clk = 8000000;       // ICLK frequency is 8MHz
		
		if (dwMRCFREQChgFlg == 0)
		{
			set_mrcfreq(udwp_cmram_clk);
		}
		
		if (dwMREFREQChgFlg == 0)
		{
			set_mrefreq(udwp_emram_clk);
		}
		
		/** If the clock source is MOCO, set the clock source to MOCO */
		if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
		{
			/** If MOCO oscillation is stopped, oscillate it */
			if (s_udwClockUserVal[MOCOCR] == MOCOCR_STOP)
			{
				M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
				Wait_us(15);// Wait for oscillation stabilization of MOCO 
			}
			
			M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;// Set the clock source to MOCO
		}
		/** Change MRPCK and ICLK to divide by 1 */
		M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_MRPCK_1DEV_MASK & SCKDIVCR_ICLK_1DEV_MASK);
		/** Change MRICK to divide by 1 */
		M32(MCU_SCKDIVCR2_ADDR) &= SCKDIVCR2_MRICK_1DEV_MASK;
		
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
		// Change OCTASPI Clock
		/** Request to change the OCTASPI Clock */
		M8(MCU_OCTACKCR_ADDR) |= OCTACKCR_OCTACKSREQ_REQ;
		/** Wait for the clock to be stopped */
		while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_POSSIBLE)
		{
			;
		}
		/** Write the settings */
		M8(MCU_OCTACKCR_ADDR) = OCTACKCR_OCTACKSEL_MOCO | OCTACKCR_OCTACKSREQ_REQ;
		M8(MCU_OCTACKDIVCR_ADDR) = OCTACKDIVCR_OCTACKDIV_1DEV;
		/** Start the OCTASPI Clock by setting OCTACKSREQ to zero */
		M8(MCU_OCTACKCR_ADDR) &= OCTACKCR_OCTACKSREQ_NO_REQ;
		/** Wait for the OCTASPI Clock to be started */
		while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_IMPOSIBLE)
		{
			;
		}
#endif
	} else
	{/** If OFS1 is not in the initial state, set the clock source to HOCO */
		// Check HOCO freqency
		switch (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK)
		{
			case OFS1_HOCOFRQ0_16MHZ:
				*udwp_cmram_clk = 16000000; // MRICK frequency is 16MHz
				*udwp_emram_clk = 16000000; // MRPCK frequency is 16MHz
				*udwp_clk = 16000000;       // ICLK frequency is 16MHz
			break;
			case OFS1_HOCOFRQ0_18MHZ:
				*udwp_cmram_clk = 18000000; // MRICK frequency is 18MHz
				*udwp_emram_clk = 18000000; // MRPCK frequency is 18MHz
				*udwp_clk = 18000000;       // ICLK frequency is 18MHz
			break;
			case OFS1_HOCOFRQ0_20MHZ:
				*udwp_cmram_clk = 20000000; // MRICK frequency is 20MHz
				*udwp_emram_clk = 20000000; // MRPCK frequency is 20MHz
				*udwp_clk = 20000000;       // ICLK frequency is 20MHz
			break;
			case OFS1_HOCOFRQ0_32MHZ:
				*udwp_cmram_clk = 32000000; // MRICK frequency is 32MHz
				*udwp_emram_clk = 32000000; // MRPCK frequency is 32MHz
				*udwp_clk = 32000000;       // ICLK frequency is 32MHz
			break;
			case OFS1_HOCOFRQ0_48MHZ:
				*udwp_cmram_clk = 48000000; // MRICK frequency is 48MHz
				*udwp_emram_clk = 48000000; // MRPCK frequency is 48MHz
				*udwp_clk = 48000000;       // ICLK frequency is 48MHz
			break;
			default:
				return (1); // Failed
		}
		
		if (dwMRCFREQChgFlg == 0)
		{
			set_mrcfreq(udwp_cmram_clk);
		}
		
		if (dwMREFREQChgFlg == 0)
		{
			set_mrefreq(udwp_emram_clk);
		}
		
		if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_HOCO)
		{
			/** If HOCO oscillation is stopped, oscillate it */
			if (s_udwClockUserVal[HOCOCR] == HOCOCR_STOP)
			{
				M8(MCU_HOCOCR_ADDR) = HOCOCR_ON;
				while (!(M8(MCU_OSCSF_ADDR) & OSCSF_HOCOSF))
				{
					Wait_us(65);
				}
			}
			
			M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_HOCO;// Set the clock source to HOCO
		}
		
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
		/** Change MRPCK and ICLK to divide by 1 */
		M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_MRPCK_1DEV_MASK & SCKDIVCR_ICLK_1DEV_MASK);
		/** Change MRICK to divide by 1 */
		M32(MCU_SCKDIVCR2_ADDR) &= SCKDIVCR2_MRICK_1DEV_MASK;
		
		// Change OCTASPI Clock
		/** Request to change the OCTASPI Clock */
		M8(MCU_OCTACKCR_ADDR) |= OCTACKCR_OCTACKSREQ_REQ;
		/** Wait for the clock to be stopped */
		while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_POSSIBLE)
		{
			;
		}
		/** Write the settings */
		M8(MCU_OCTACKCR_ADDR) = OCTACKCR_OCTACKSEL_PLL1P | OCTACKCR_OCTACKSREQ_REQ;
		M8(MCU_OCTACKDIVCR_ADDR) = OCTACKDIVCR_OCTACKDIV_2DEV;
		/** Start the OCTASPI Clock by setting OCTACKSREQ to zero */
		M8(MCU_OCTACKCR_ADDR) &= OCTACKCR_OCTACKSREQ_NO_REQ;
		/** Wait for the OCTASPI Clock to be started */
		while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_IMPOSIBLE)
		{
			;
		}
#endif
	}
	
	if (dwMRCFREQChgFlg == 1)
	{
		set_mrcfreq(udwp_cmram_clk);
	}
	
	if (dwMREFREQChgFlg == 1)
	{
		set_mrefreq(udwp_emram_clk);
	}
	
	if ((*udwp_cmram_clk / 1000000) > 101) 
	{
		M8(MCU_MRCPFB_ADDR) = MRCPFB_MPFBEN_EN;
	}
	
	return (0); // OK
}

/*
 *  Save Mode & Clock before rewriting flash
 *  Parameter:    None
 *  Return Value: 0 - OK,  1 - Failed
 */
static ubyte_t saveModeAndClock(void)
{
	s_udwClockUserVal[PRCR] = (udword_t)M16(MCU_PRCR_ADDR);
	s_udwClockUserVal[SCKSCR] = (udword_t)M8(MCU_SCKSCR_ADDR);
	s_udwClockUserVal[SCKDIVCR] = M32(MCU_SCKDIVCR_ADDR);
	s_udwClockUserVal[SCKDIVCR2] = M32(MCU_SCKDIVCR2_ADDR);
	s_udwClockUserVal[PLLCCR] = (udword_t)M16(MCU_PLLCCR_ADDR);
	s_udwClockUserVal[PLLCCR2] = (udword_t)M16(MCU_PLLCCR2_ADDR);
	s_udwClockUserVal[PLL2CCR] = (udword_t)M16(MCU_PLL2CCR_ADDR);
	s_udwClockUserVal[PLL2CCR2] = (udword_t)M16(MCU_PLL2CCR2_ADDR);
	s_udwClockUserVal[HOCOCR] = (udword_t)M8(MCU_HOCOCR_ADDR);
	s_udwClockUserVal[MOCOCR] = (udword_t)M8(MCU_MOCOCR_ADDR);
	s_udwClockUserVal[OPCCR] = (udword_t)M8(MCU_OPCCR_ADDR);
	s_udwClockUserVal[OFS1] = M32(MCU_OFS1_ADDR);
	s_udwClockUserVal[MRCFREQ] = M32(MCU_MRCFREQ_ADDR);
	s_udwClockUserVal[MREFREQ] = M32(MCU_MREFREQ_ADDR);
	s_udwClockUserVal[MRCPFB] = (udword_t)M8(MCU_MRCPFB_ADDR);
	s_udwClockUserVal[MRCPC1] = (udword_t)M16(MCU_MRCPC1_ADDR);
	s_udwClockUserVal[MRCBPROT1] = (udword_t)M16(MCU_MRCBPROT1_ADDR);
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	// Cancel the module-stop state to access to the OSPI related registers
	M32(MCU_MSTPCRB_ADDR) = M32(MCU_MSTPCRB_ADDR) & (~MCU_MSTPCRB_MSTPB17_MSK);
	
	// Save OSPI Clock
	s_udwClockUserVal[OCTACKCR] = (udword_t)M8(MCU_OCTACKCR_ADDR);
	s_udwClockUserVal[OCTACKDIVCR] = (udword_t)M8(MCU_OCTACKDIVCR_ADDR);
	
	// Save OSPI Configuration
	s_udwOSPIConfigurationUserVal[BMCTL0] = M32(OSPI_BMCTL0_ADDR(1));
	s_udwOSPIConfigurationUserVal[LIOCFGCS] = M32(OSPI_LIOCFGCS_ADDR(1, 1));
	s_udwOSPIConfigurationUserVal[WRAPCFG] = M32(OSPI_WRAPCFG_ADDR(1));
	s_udwOSPIConfigurationUserVal[BMCFGCH0] = M32(OSPI_BMCFGCH_ADD(1, 0));
	s_udwOSPIConfigurationUserVal[BMCFGCH1] = M32(OSPI_BMCFGCH_ADD(1, 1));
	s_udwOSPIConfigurationUserVal[CMCFG0] = M32(OSPI_CMCFG0CS_ADD(1, 1));
	s_udwOSPIConfigurationUserVal[CMCFG1] = M32(OSPI_CMCFG1CS_ADD(1, 1));
	s_udwOSPIConfigurationUserVal[CMCFG2] = M32(OSPI_CMCFG2CS_ADD(1, 1));
	
	// Save OSPI Port
	s_udwOSPIPortUserVal[PmnPFS_6_2] = M32(MCU_PmnPFS_ADDR(6, 2));
	s_udwOSPIPortUserVal[PmnPFS_6_3] = M32(MCU_PmnPFS_ADDR(6, 3));
	s_udwOSPIPortUserVal[PmnPFS_6_4] = M32(MCU_PmnPFS_ADDR(6, 4));
	s_udwOSPIPortUserVal[PmnPFS_6_5] = M32(MCU_PmnPFS_ADDR(6, 5));
	s_udwOSPIPortUserVal[PmnPFS_6_6] = M32(MCU_PmnPFS_ADDR(6, 6));
	s_udwOSPIPortUserVal[PmnPFS_6_7] = M32(MCU_PmnPFS_ADDR(6, 7));
	s_udwOSPIPortUserVal[PmnPFS_12_0] = M32(MCU_PmnPFS_ADDR(12, 0));
	s_udwOSPIPortUserVal[PmnPFS_12_1] = M32(MCU_PmnPFS_ADDR(12, 1));
	s_udwOSPIPortUserVal[PmnPFS_12_2] = M32(MCU_PmnPFS_ADDR(12, 2));
	s_udwOSPIPortUserVal[PmnPFS_12_3] = M32(MCU_PmnPFS_ADDR(12, 3));
	s_udwOSPIPortUserVal[PmnPFS_12_4] = M32(MCU_PmnPFS_ADDR(12, 4));
	s_udwOSPIPortUserVal[PmnPFS_12_5] = M32(MCU_PmnPFS_ADDR(12, 5));
	s_udwOSPIPortUserVal[PmnPFS_12_6] = M32(MCU_PmnPFS_ADDR(12, 6));
	s_udwOSPIPortUserVal[PmnPFS_12_7] = M32(MCU_PmnPFS_ADDR(12, 7));
	s_udwOSPIPortUserVal[PmnPFS_12_9] = M32(MCU_PmnPFS_ADDR(12, 9));
	s_udwOSPIPortUserVal[PmnPFS_12_10] = M32(MCU_PmnPFS_ADDR(12, 10));
#endif
	
	return (0); // Finished without Errors
}

/**************************************************************************//**
* @details   Restore clock and power control mode to the state before flash rewriting
* @param     None
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t restore_clock_and_mode(void)
{
	/** There is no problem with the following cast conversions
	 * because it is a type conversion from unsigned to unsigned */
	
	if (dwMRCFREQChgFlg == 1)
	{
		M32(MCU_MRCFREQ_ADDR) = MRCFREQ_KEY | s_udwClockUserVal[MRCFREQ];
	}
	
	if (dwMREFREQChgFlg == 1)
	{
		M32(MCU_MREFREQ_ADDR) = MREFREQ_KEY | s_udwClockUserVal[MREFREQ];
	}
	
	
	M16(MCU_PRCR_ADDR) = KEYCODE_PRCR + PRCR_PRC1 + PRCR_PRC0;
	M32(MCU_SCKDIVCR_ADDR) = s_udwClockUserVal[SCKDIVCR]; // Restore system clock division
	M32(MCU_SCKDIVCR2_ADDR) = s_udwClockUserVal[SCKDIVCR2]; // Restore system clock division
	M16(MCU_SCKSCR_ADDR) = (uword_t)s_udwClockUserVal[SCKSCR]; // Restore system clock source
	M8(MCU_HOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[HOCOCR]; // Restore the oscillation state of HOCO
	M8(MCU_MOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[MOCOCR]; // Restore the oscillation state of MOCO
	
#if (defined RA8D2_SiP_8M) || (defined RA8D2_SiP_4M)
	/** Restore OCTASPI Clock */
	/** Request to change the OCTASPI Clock */
	M8(MCU_OCTACKCR_ADDR) |= OCTACKCR_OCTACKSREQ_REQ;
	/** Wait for the clock to be stopped */
	while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_POSSIBLE)
	{
		;
	}
	/** Write the settings */
	M8(MCU_OCTACKCR_ADDR) = s_udwClockUserVal[OCTACKCR] | OCTACKCR_OCTACKSREQ_REQ;
	M8(MCU_OCTACKDIVCR_ADDR) = s_udwClockUserVal[OCTACKDIVCR];
	/** Start the OCTASPI Clock by setting OCTACKSREQ to zero */
	M8(MCU_OCTACKCR_ADDR) &= OCTACKCR_OCTACKSREQ_NO_REQ;
	/** Wait for the OCTASPI Clock to be started */
	while ((M8(MCU_OCTACKCR_ADDR) & OCTACKCR_OCTACKSRDY_MASK) != OCTACKCR_OCTACKSRDY_IMPOSIBLE)
	{
		;
	}
	
	// Restore OSPI Configuration
	// Cancel the module-stop state to access to the OSPI related registers
	M32(MCU_MSTPCRB_ADDR) = M32(MCU_MSTPCRB_ADDR) & (~MCU_MSTPCRB_MSTPB17_MSK);
	/* Disable memory-mapping for this slave */
	if (OSPI_B_DEVICE_NUMBER_0 == OSPI_CHANNEL)
	{
		M32(OSPI_BMCTL0_ADDR(1)) &= ~(OSPI_BMCTL0_CH0CS0ACC_MSK | OSPI_BMCTL0_CH1CS0ACC_MSK);
	}
	else
	{
		M32(OSPI_BMCTL0_ADDR(1)) &= ~(OSPI_BMCTL0_CH0CS1ACC_MSK | OSPI_BMCTL0_CH1CS1ACC_MSK);
	}
	M32(OSPI_LIOCFGCS_ADDR(1, 1)) = s_udwOSPIConfigurationUserVal[LIOCFGCS];
	M32(OSPI_WRAPCFG_ADDR(1)) = s_udwOSPIConfigurationUserVal[WRAPCFG];
	M32(OSPI_BMCFGCH_ADD(1, 0)) = s_udwOSPIConfigurationUserVal[BMCFGCH0];
	M32(OSPI_BMCFGCH_ADD(1, 1)) = s_udwOSPIConfigurationUserVal[BMCFGCH1];
	M32(OSPI_CMCFG0CS_ADD(1, 1)) = s_udwOSPIConfigurationUserVal[CMCFG0];
	M32(OSPI_CMCFG1CS_ADD(1, 1)) = s_udwOSPIConfigurationUserVal[CMCFG1];
	M32(OSPI_CMCFG2CS_ADD(1, 1)) = s_udwOSPIConfigurationUserVal[CMCFG2];
	M32(OSPI_BMCTL0_ADDR(1)) = s_udwOSPIConfigurationUserVal[BMCTL0];
	
	// Restore OSPI Port
	// Write 0x00: Clear both B0WI and PFSWE
	M8(MCU_PWPR_S_ADDR) = 0x00;
	// Write 0x40: B0WI=0, PFSWE=1 (allow PFS write)
	M8(MCU_PWPR_S_ADDR) = 0x40;
	// Write 0xC0: B0WI=1, PFSWE=1
	M8(MCU_PWPR_S_ADDR) = 0xC0;
	
	M32(MCU_PmnPFS_ADDR(6, 2)) = s_udwOSPIPortUserVal[PmnPFS_6_2];
	M32(MCU_PmnPFS_ADDR(6, 3)) = s_udwOSPIPortUserVal[PmnPFS_6_3];
	M32(MCU_PmnPFS_ADDR(6, 4)) = s_udwOSPIPortUserVal[PmnPFS_6_4];
	M32(MCU_PmnPFS_ADDR(6, 5)) = s_udwOSPIPortUserVal[PmnPFS_6_5];
	M32(MCU_PmnPFS_ADDR(6, 6)) = s_udwOSPIPortUserVal[PmnPFS_6_6];
	M32(MCU_PmnPFS_ADDR(6, 7)) = s_udwOSPIPortUserVal[PmnPFS_6_7];
	M32(MCU_PmnPFS_ADDR(12, 0)) = s_udwOSPIPortUserVal[PmnPFS_12_0];
	M32(MCU_PmnPFS_ADDR(12, 1)) = s_udwOSPIPortUserVal[PmnPFS_12_1];
	M32(MCU_PmnPFS_ADDR(12, 2)) = s_udwOSPIPortUserVal[PmnPFS_12_2];
	M32(MCU_PmnPFS_ADDR(12, 3)) = s_udwOSPIPortUserVal[PmnPFS_12_3];
	M32(MCU_PmnPFS_ADDR(12, 4)) = s_udwOSPIPortUserVal[PmnPFS_12_4];
	M32(MCU_PmnPFS_ADDR(12, 5)) = s_udwOSPIPortUserVal[PmnPFS_12_5];
	M32(MCU_PmnPFS_ADDR(12, 6)) = s_udwOSPIPortUserVal[PmnPFS_12_6];
	M32(MCU_PmnPFS_ADDR(12, 7)) = s_udwOSPIPortUserVal[PmnPFS_12_7];
	M32(MCU_PmnPFS_ADDR(12, 9)) = s_udwOSPIPortUserVal[PmnPFS_12_9];
	M32(MCU_PmnPFS_ADDR(12, 10)) = s_udwOSPIPortUserVal[PmnPFS_12_10];
	
	// Write 0x80: B0WI=1, PFSWE=0 (protect PFS)
	M8(MCU_PWPR_S_ADDR) = 0x80;
#endif
	
	/** Restore power control mode */
	M8(MCU_OPCCR_ADDR) = (ubyte_t)s_udwClockUserVal[OPCCR];
	while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
	{
		; // Wait till transition completed
	}
	
	if (dwMRCFREQChgFlg == 0)
	{
		M32(MCU_MRCFREQ_ADDR) = MRCFREQ_KEY | s_udwClockUserVal[MRCFREQ];
	}
	
	if (dwMREFREQChgFlg == 0)
	{
		M32(MCU_MREFREQ_ADDR) = MREFREQ_KEY | s_udwClockUserVal[MREFREQ];
	}
	
	M8(MCU_MRCPFB_ADDR) = s_udwClockUserVal[MRCPFB];
	M16(MCU_MRCPC1_ADDR) = s_udwClockUserVal[MRCPC1];
	M16(MCU_MRCBPROT1_ADDR) = s_udwClockUserVal[MRCBPROT1];
	M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | (uword_t)s_udwClockUserVal[PRCR]; // Restore the protection status
	
	return (0); // OK
}

/**************************************************************************//**
* @details    Transition to P/E mode
* @param[in]  udw_addr: Write start address
* @param[out] udwp_adr_ret: Write start address after mask
* @retval     0 - OK,  1 - Failed
******************************************************************************/
static udword_t pe_mode_entry(udword_t udw_addr, udword_t *udwp_adr_ret)
{
	if (udw_addr < MCU_CONF_EXTRAMRAM_ADDR) // Code MRAM area
	{
		return (0); // OK
	
	} else // Extra MRAM area
	{
		M16(MCU_MENTRYR_ADDR) = MENTRYR_READ_MODE; // Transition to read mode
		while (M16(MCU_MENTRYR_ADDR) != MENTRYR_CHK_READ) // Wait until switching to read mode
		{
			;
		}
		
		M16(MCU_MENTRYR_ADDR) = MENTRYR_EMRAM_PROGRAM; // Transition to Extra MRAM Program mode
		*udwp_adr_ret = udw_addr & EMRAM_MASK_ADDR; // Set the write start address after mask
		while (M16(MCU_MENTRYR_ADDR) != MENTRYR_CHK_EMRAM) // Wait until switching to Extra MRAM Program mode
		{
			;
		}
	}
	
	return (0); // OK
}


/**************************************************************************//**
* @details   Check for command locked and illegal status
* @param     None
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t ilgl_chk(void)
{
	ubyte_t ubydata;
	udword_t udwdata;
	
	/** Check for command locked state */
	ubydata = M8(MCU_MASTAT_ADDR);
	if (ubydata & MASTAT_CMDLK)
	{
		return(1); // Failed
	}
	
	/** Check for illegal state */
	udwdata = M32(MCU_MSTATR_ADDR);
	if (udwdata & MSTATR_ILGLERR)
	{
		return(1); // Failed
	}
	
	return (0); // OK
}

/**************************************************************************//**
* @details   MRCFREQ register setting
* @param[in] udwp_clk: Clock Frequency (Hz)
* @retval    None
******************************************************************************/
static void set_mrcfreq(udword_t *udwp_clk)
{
	ubyte_t ub_mhz_mrcfreq;
	
	ub_mhz_mrcfreq = (*udwp_clk) / 1000000; // Conver to MHz unit
	
	if ((*udwp_clk) % 1000000)
	{
		ub_mhz_mrcfreq += 1; // Round up the first decimal place in MHz
	}
	M32(MCU_MRCFREQ_ADDR) = MRCFREQ_KEY | ub_mhz_mrcfreq;
	
	while (M32(MCU_MRCFREQ_ADDR) != ub_mhz_mrcfreq);
}

/**************************************************************************//**
* @details   MREFREQ register setting
* @param[in] udwp_clk: Clock Frequency (Hz)
* @retval    None
******************************************************************************/
static void set_mrefreq(udword_t *udwp_clk)
{
	ubyte_t ub_mhz_mrefreq;
	
	ub_mhz_mrefreq = (*udwp_clk) / 1000000; // Conver to MHz unit
	
	if ((*udwp_clk) % 1000000)
	{
		ub_mhz_mrefreq += 1; // Round up the first decimal place in MHz
	}
	M32(MCU_MREFREQ_ADDR) = MREFREQ_KEY | ub_mhz_mrefreq;
	
	while (M32(MCU_MREFREQ_ADDR) != ub_mhz_mrefreq);
}

/*
 * Wait processing in us units
 * Parameter: us: wait time
 * Return Value: none
 * Note: Please note that the weight processing is less accurate.
 */
#pragma GCC push_options
#pragma GCC optimize ("O0")

static void Wait_us(udword_t us)
{
	udword_t dwCount;
	udword_t dwTotalNsPerCycle;
	
	dwTotalNsPerCycle = 1000000000 / dwICLKValue; // Calculate ns per cycle
	if (dwTotalNsPerCycle < 4)
	{
		// Fail-safe when exceeding 250Mhz
		dwTotalNsPerCycle = 4;	
	}
	// Calculates the number of cycles required to wait for the specified us unit time
	dwCount = (us * 1000) / dwTotalNsPerCycle;
	// Since it takes at least about 14 cycles to execute one for statement, the number of executions of it is calculated based on that.
	dwCount = dwCount / 14;
	
	if (dwCount == 0)
	{
		// Fail-safe if dwCount is below 14
		dwCount = 1;  
	}
	
	for ( ; dwCount > 0; dwCount--)
	{
		__asm("NOP");
	}
}
#pragma GCC pop_options

/**************************************************************************//**
* @details   Read the data written in the flash
* @param[in] udw_addr:  Read start address
* @param[in] udw_sz:    Read size
* @param[in] ubp_buf:   Buffer to store read data
* @retval    None
******************************************************************************/
static void read_page(udword_t udw_addr, udword_t udw_sz, ubyte_t *ubp_buf)
{
	uword_t i;
	
	for (i = 0; i < udw_sz ; i++)
	{
		ubp_buf[i] = M8(udw_addr + i); // Store read data byte by byte
	}
}

static ospi_status_t r_ospi_b_protocol_specific_settings(void)
{
	ospi_status_t stat = SUCCESS;
	
	/* Update the SPI protocol and latency mode. */
	udword_t liocfg = M32(OSPI_LIOCFGCS_ADDR(1, 1)) & ~(OSPI_LIOCFGCS_LATEMD_MSK | OSPI_LIOCFGCS_PRTMD_MSK);
	liocfg |= (((udword_t) OSPI_PROTOCOL_MODE << OSPI_LIOCFGCS_PRTMD_POS) & OSPI_LIOCFGCS_PRTMD_MSK);
	liocfg |= (((udword_t) OSPI_CMDSET_LATENCY_MODE << OSPI_LIOCFGCS_LATEMD_POS) & OSPI_LIOCFGCS_LATEMD_MSK);
	M32(OSPI_LIOCFGCS(1, 1)) = liocfg;
	
	/* Specifies the read/write commands and Read dummy clocks for Device
	 * (see "Flow of Memory-mapping" in the OSPI section of the relevant hardware manual). */
	udword_t cmcfg0 = ((udword_t) (OSPI_CMDSET_ADDRESS_MSB_MSK_MM << OSPI_CMCFG0CS_ADDRPEN_POS)) |
					  ((udword_t) (OSPI_CMDSET_FRAME_FORMAT << OSPI_CMCFG0CS_FFMT_POS)) |
					  (((udword_t) OSPI_CMDSET_ADDRESS_BYTES_MM << OSPI_CMCFG0CS_ADDSIZE_POS) &
					  OSPI_CMCFG0CS_ADDSIZE_MSK);
	
	/* When using 4-byte addressing, always mask off the most-significant nybble to remove the system bus offset from
	 * the transmitted addresses. Ex. CS1 starts at 0x9000_0000 so it needs to mask off bits [31:28]. */
	if (OSPI_CMDSET_ADDRESS_BYTES == OSPI_ADDRESS_BYTES_4)
	{
		cmcfg0 |= OSPI_B_PRV_ADDRESS_REPLACE_ENABLE_BITS;
	}
	
	/* Apply the frame format setting and update the register. */
	cmcfg0 |= (udword_t) (OSPI_CMDSET_FRAME_FORMAT << OSPI_CMCFG0CS_FFMT_POS);
	M32(OSPI_CMCFG0CS_ADD(1, 1)) = cmcfg0;
	
	/* Cache the appropriate command values for later use. */
	uword_t read_command  = OSPI_CMDSET_READ_COMMAND_MM;
	uword_t write_command = OSPI_CMDSET_PROGRAM_COMMAND_MM;
	
	/* If no length is specified or if the command byte length is 1, move the command to the upper byte. */
	if (OSPI_COMMAND_BYTES_1 == OSPI_CMDSET_COMMAND_BYTES)
	{
		read_command = (uword_t) ((read_command & OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_MASK) << OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
		write_command = (uword_t) ((write_command & OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_MASK) << OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
	}
	
	const ubyte_t read_dummy_cycles	 = OSPI_CMDSET_READ_DUMMY_CYCLES_MM;
	const ubyte_t write_dummy_cycles = OSPI_CMDSET_PROGRAM_DUMMY_CYCLES_MM;
	
	M32(OSPI_CMCFG1CS_ADD(1, 1)) =
		(udword_t) (((udword_t) (read_command) << OSPI_CMCFG1CS_RDCMD_POS) |
					((udword_t) (read_dummy_cycles << OSPI_CMCFG1CS_RDLATE_POS) &
					 OSPI_CMCFG1CS_RDLATE_MSK));
	
	M32(OSPI_CMCFG2CS_ADD(1, 1)) =
		(udword_t) (((udword_t) (write_command) << OSPI_CMCFG2CS_WRCMD_POS) |
					((udword_t) (write_dummy_cycles << OSPI_CMCFG2CS_WRLATE_POS) &
					 OSPI_CMCFG2CS_WRLATE_MSK));
	
	return stat;
}

/**************************************************************************//**
* @details   Open OSPI Open OSPI to begin connecting to External Flash Memory.
* @param[in] None
* @retval    SUCCESS - OK, ERROR - Failed
******************************************************************************/
static ospi_status_t r_ospi_b_open(void)
{
	ospi_status_t stat = SUCCESS;
	
	/* Disable memory-mapping for this slave. It will be enabled later on after initialization. */
	if (OSPI_B_DEVICE_NUMBER_0 == OSPI_CHANNEL)
	{
		M32(OSPI_BMCTL0_ADDR(1)) &= ~(OSPI_BMCTL0_CH0CS0ACC_MSK | OSPI_BMCTL0_CH1CS0ACC_MSK);
	}
	else
	{
		M32(OSPI_BMCTL0_ADDR(1)) &= ~(OSPI_BMCTL0_CH0CS1ACC_MSK | OSPI_BMCTL0_CH1CS1ACC_MSK);
	}
	
	/* Perform xSPI Initial configuration as described in hardware manual (see
	 * "Flows of Operations" in the OSPI section of the relevant hardware manual). */
	
	/* Set xSPI protocol mode. */
	udword_t liocfg = ((udword_t) OSPI_PROTOCOL_MODE) << OSPI_LIOCFGCS_PRTMD_POS;
	M32(OSPI_LIOCFGCS_ADDR(1, 1)) = liocfg;
	
	/* Set xSPI drive/sampling timing. */
	if (OSPI_B_DEVICE_NUMBER_0 == OSPI_CHANNEL)
	{
		M32(OSPI_WRAPCFG_ADDR(1)) = ((udword_t) OSPI_DATA_LATCH_DELAY_CLOCKS << OSPI_WRAPCFG_DSSFTCS0_POS) & OSPI_WRAPCFG_DSSFTCS0_MSK;
	}
	else
	{
		M32(OSPI_WRAPCFG_ADDR(1)) = ((udword_t) OSPI_DATA_LATCH_DELAY_CLOCKS << OSPI_WRAPCFG_DSSFTCS1_POS) & OSPI_WRAPCFG_DSSFTCS1_MSK;
	}
	
	
	/* Set minimum cycles between xSPI frames. */
	liocfg |= ((udword_t) OSPI_TS_CMD_TO_CMD_INTERVAL << OSPI_LIOCFGCS_CSMIN_POS) &
			  OSPI_LIOCFGCS_CSMIN_MSK;
			
	/* Set CS asserting extension in cycles */
	liocfg |= ((udword_t) OSPI_TS_CS_PULLDOWN_LEAD << OSPI_LIOCFGCS_CSASTEX_POS) &
			  OSPI_LIOCFGCS_CSASTEX_MSK;
			
	/* Set CS releasing extension in cycles */
	liocfg |= ((udword_t) OSPI_TS_CS_PULLUP_LAG << OSPI_LIOCFGCS_CSNEGEX_POS) &
			  OSPI_LIOCFGCS_CSNEGEX_MSK;
			
	/* Set SDR and DDR timing. */
	liocfg |= ((udword_t) OSPI_TS_SDR_DRIVE_TIMING << OSPI_LIOCFGCS_SDRDRV_POS) &
			  OSPI_LIOCFGCS_SDRDRV_MSK;
	liocfg |= ((udword_t) OSPI_TS_SDR_SAMPLING_EDGE << OSPI_LIOCFGCS_SDRSMPMD_POS) &
			  OSPI_LIOCFGCS_SDRSMPMD_MSK;
	liocfg |= ((udword_t) OSPI_TS_SDR_SAMPLING_DELAY << OSPI_LIOCFGCS_SDRSMPSFT_POS) &
			  OSPI_LIOCFGCS_SDRSMPSFT_MSK;
	liocfg |= ((udword_t) OSPI_TS_SDR_SAMPLING_EXTENSION << OSPI_LIOCFGCS_DDRSMPEX_POS) &
			  OSPI_LIOCFGCS_DDRSMPEX_MSK;
	
	/* Set xSPI CSn signal timings. */
	M32(OSPI_LIOCFGCS_ADDR(1, 1)) = liocfg; 
	
	/* Set xSPI memory-mapping operation. */
	stat |= r_ospi_b_protocol_specific_settings();
	
	/* Return response after issuing write transaction to xSPI bus, Enable prefetch function and combination if desired. */
	const udword_t bmcfgch = (0 << OSPI_BMCFGCH_WRMD_POS) |
							 ((OSPI_B_CFG_COMBINATION_FUNCTION << OSPI_BMCFGCH_MWRCOMB_POS) &
							  (OSPI_BMCFGCH_MWRCOMB_MSK | OSPI_BMCFGCH_MWRSIZE_MSK)) |
							 ((OSPI_B_CFG_PREFETCH_FUNCTION << OSPI_BMCFGCH_PREEN_POS) &
							  OSPI_BMCFGCH_PREEN_MSK);

	/* Both of these should have the same configuration and it affects all OSPI slave channels. */
	M32(OSPI_BMCFGCH_ADD(1, 0)) = bmcfgch;
	M32(OSPI_BMCFGCH_ADD(1, 1)) = bmcfgch;
	
	/* Re-activate memory-mapped mode in Read/Write. */
	if (OSPI_B_DEVICE_NUMBER_0 == OSPI_CHANNEL)
	{
		M32(OSPI_BMCTL0_ADDR(1)) |= OSPI_BMCTL0_CH0CS0ACC_MSK | OSPI_BMCTL0_CH1CS0ACC_MSK;
	}
	else
	{
		M32(OSPI_BMCTL0_ADDR(1)) |= OSPI_BMCTL0_CH0CS1ACC_MSK | OSPI_BMCTL0_CH1CS1ACC_MSK;
	}
	
	return stat; 
}

/**************************************************************************//**
* @details   Read/Write raw data directly with the OctaFlash.
* @param[in] p_transfer: The data packet needs to be transmitted
* @param[in] direction:  The transfer direction
* @retval    None
******************************************************************************/
static void r_ospi_b_direct_transfer(spi_flash_direct_transfer_t * const p_transfer, spi_flash_direct_transfer_dir_t direction)
{
	udword_t cdtbuf0 =
	(((udword_t) p_transfer->command_length << OSPI_CDTBUF_CMDSIZE_POS) & OSPI_CDTBUF_CMDSIZE_MSK) |
	(((udword_t) p_transfer->address_length << OSPI_CDTBUF_ADDSIZE_POS) & OSPI_CDTBUF_ADDSIZE_MSK) |
	(((udword_t) p_transfer->data_length << OSPI_CDTBUF_DATASIZE_POS) & OSPI_CDTBUF_DATASIZE_MSK) |
	(((udword_t) p_transfer->dummy_cycles << OSPI_CDTBUF_LATE_POS) & OSPI_CDTBUF_LATE_MSK) |
	(((udword_t) direction << OSPI_CDTBUF_TRTYPE_POS) & OSPI_CDTBUF_TRTYPE_MSK);
	
	cdtbuf0 |= (1 == p_transfer->command_length) ?
			   ((p_transfer->command & OSPI_B_PRV_CDTBUF_CMD_1B_VALUE_MASK) << OSPI_B_PRV_CDTBUF_CMD_UPPER_OFFSET) :
			   ((p_transfer->command & OSPI_B_PRV_CDTBUF_CMD_2B_VALUE_MASK) << OSPI_B_PRV_CDTBUF_CMD_OFFSET);
	
	/* Setup the manual command control. Cancel any ongoing transactions, direct mode, set channel, 1 transaction. */
	M32(OSPI_CDCTL0_ADDR(1)) = ((((udword_t) OSPI_CHANNEL) << OSPI_CDCTL0_CSSEL_POS) & OSPI_CDCTL0_CSSEL_MSK);
	
	/* Direct Read/Write settings
	 * (see "Flow of Manual-command Procedure" in the OSPI section of the relevant hardware manual). */
	while (0 != (M32(OSPI_CDCTL0_ADDR(1)) & OSPI_CDCTL0_TRREQ_MSK))
	{
		// Do nothing
	}
	
	M32(OSPI_CDTBUF_ADDR(1, 0)) = cdtbuf0;
	M32(OSPI_CDABUF_ADDR(1, 0)) = p_transfer->address;
	
	if (SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE == direction)
	{
		M32(OSPI_CDD0BUF_ADDR(1, 0)) = (udword_t) (p_transfer->data_u64 & UINT32_MAX);
		if (p_transfer->data_length > sizeof(udword_t))
		{
			M32(OSPI_CDD1BUF_ADDR(1, 0)) = (udword_t) (p_transfer->data_u64 >> OSPI_B_PRV_UINT32_BITS);
		}
	}
	
	/* Start the transaction and wait for completion. */
	M32(OSPI_CDCTL0_ADDR(1)) |= OSPI_CDCTL0_TRREQ_MSK;
	while (0 != (M32(OSPI_CDCTL0_ADDR(1)) & OSPI_CDCTL0_TRREQ_MSK))
	{
		// Do nothing
	}
	
	if (SPI_FLASH_DIRECT_TRANSFER_DIR_READ == direction)
	{
		p_transfer->data_u64 = M32(OSPI_CDD0BUF_ADDR(1, 0));
		if (p_transfer->data_length > sizeof(udword_t))
		{
			p_transfer->data_u64 |= (((uqword_t) M32(OSPI_CDD1BUF_ADDR(1, 0))) << OSPI_B_PRV_UINT32_BITS);
		}
	}
	
	/* Clear interrupt flags. */
	M32(OSPI_INTC_ADDR(1)) = M32(OSPI_INTS_ADDR(1));
}

static ospi_status_t r_ospi_b_write (ubyte_t const * const p_src, ubyte_t * const p_dest, udword_t byte_count)
{
	ospi_status_t stat = SUCCESS;
	
	if (true == r_ospi_b_status_sub(OSPI_WRITE_STATUS_BIT))
	{
		stat |= ERROR;
		return stat;
	}
	
	uqword_t * p_dest64 = (uqword_t *) ((udword_t) p_dest & ~(OSPI_B_PRV_CPU_ACCESS_ALIGNMENT - 1));
	uqword_t * p_src64	= (uqword_t *) p_src;
	
	while (sizeof(uqword_t) <= byte_count)
	{
		stat |= r_ospi_b_write_enable();
		
		if (stat != SUCCESS)
		{
			return stat;
		}
		/* When combination function is enabled, xSPI master transmits a xSPI
		 * frame with the selected size while the sequential address is incremental. Please read
		 * "Combination Function" in the OSPI Operation section of the relevant hardware manual.
		 * So Basically Enable command should be
		 * sent only once for a single burst(incremented addresses up to set combination size.). */
		*p_dest64 = *p_src64;
		p_dest64++;
		p_src64++;
		byte_count -= sizeof(uqword_t);
	}
	
	__asm volatile ("dmb");
	
	M32(OSPI_BMCTL1_ADDR(1)) = OSPI_B_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK;
	
	return stat;
}

/**************************************************************************//**
* @details   Erase raw data directly with the OctaFlash.
* @param[in] p_device_address: The address to begin erase raw data
* @param[in] byte_count:       The amount of raw data that needs to be erase
* @retval    None
******************************************************************************/
static ospi_status_t r_ospi_b_erase(ubyte_t * const p_device_address, udword_t byte_count)
{
	ospi_status_t stat = SUCCESS;
	bool send_address = true;
	uword_t erase_command = 0;
	
	/* Use the address bytes and MSB mask to convert from system address to chip addresses.
	 * This is better than using pointer subtraction because some devices (i.e., SiP) have different start addresses.
	 * This method better mimics how memory-mapped addresses are converted in the hardware. */
	udword_t addr_mask = (udword_t) (~UINT8_MAX | OSPI_CMDSET_ADDRESS_MSB_MSK);         // Start with a full 32-bit mask with the address MSB mask in the lower byte.
	addr_mask <<= 8 * (OSPI_B_PRV_ADDR_BYTES_TO_LENGTH(OSPI_CMDSET_ADDRESS_BYTES) - 1); // Shift the MSB to the correct position.
	udword_t chip_address = (udword_t) ((udword_t) p_device_address & ~addr_mask);      // Mask off the address bits that aren't needed.
	
	if (true == r_ospi_b_status_sub(OSPI_WRITE_STATUS_BIT))
	{
		stat |= ERROR;
		return stat;
	}
	
	/* Select the appropriate erase command from the command set. */
	spi_flash_erase_command_t const * p_erase_list = g_ospi_b_command_set_initial_erase_commands;
	const ubyte_t erase_list_length = g_ospi_b_command_set_initial_erase_table.length;
	
	for (udword_t index = 0; index < erase_list_length; index++)
	{
		/* If requested byte_count is supported by underlying flash, store the command. */
		if (byte_count == p_erase_list[index].size)
		{
			if (SPI_FLASH_ERASE_SIZE_CHIP_ERASE == byte_count)
			{
				/* Don't send address for chip erase. */
				send_address = false;
			}
			
			erase_command = p_erase_list[index].command;
			break;
		}
	}
	
	stat |= r_ospi_b_write_enable();
	
	spi_flash_direct_transfer_t direct_command =
	{
		.command        = erase_command,
		.command_length = (ubyte_t) OSPI_CMDSET_COMMAND_BYTES,
		.address        = chip_address,
		.address_length = (send_address) ? OSPI_B_PRV_ADDR_BYTES_TO_LENGTH(OSPI_CMDSET_ADDRESS_BYTES) : 0,
		.data_length    = 0,
	};
	
	r_ospi_b_direct_transfer(&direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	
	while ((M32(OSPI_COMSTT_ADDR(1)) & OSPI_B_PRV_COMSTT_MEMACCCH_MASK) != 0)
	{
		// Do nothing
	}
	
	
	M32(OSPI_BMCTL1_ADDR(1)) = OSPI_B_PRV_BMCTL1_CLEAR_PREFETCH_MASK;
	
	return stat;
}

/**************************************************************************//**
* @details   Check the readiness status of the External Flash Memory.
* @param[in] bit_pos: The position of the bits to be checked
* @retval    True, False
******************************************************************************/
static bool r_ospi_b_status_sub (ubyte_t bit_pos)
{
	spi_flash_direct_transfer_t direct_command =
	{
		.command        = OSPI_CMDSET_STATUS_COMMAND,
		.command_length = (ubyte_t) OSPI_CMDSET_COMMAND_BYTES,
		.address_length = 0,
		.address        = 0,
		.data_length    = 1,
		.dummy_cycles   = OSPI_CMDSET_STATUS_DUMMY_CYCLES,
	};
	
	/* 8D-8D-8D mode requires an address for any kind of read. If the address wasn't set by the configuration
	 * set it to the general address length. */
	if ((direct_command.address_length != 0) && (OSPI_PROTOCOL_MODE_8D8D8D == OSPI_PROTOCOL_MODE))
	{
		direct_command.address_length = OSPI_B_PRV_ADDR_BYTES_TO_LENGTH(OSPI_CMDSET_ADDRESS_BYTES);
	}
	
	r_ospi_b_direct_transfer(&direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	
	return (direct_command.data >> bit_pos) & 1;
}

static ospi_status_t r_ospi_b_statusget (spi_flash_status_t * const p_status)
{
	/* Read device status. */
	p_status->write_in_progress = r_ospi_b_status_sub(OSPI_WRITE_STATUS_BIT);
	
	return SUCCESS;
}

static ospi_status_t ospi_b_wait_operation(udword_t timeout)
{
	ospi_status_t stat = SUCCESS;
	spi_flash_status_t status = {RESET_VALUE};
	
	status.write_in_progress = true;
	while (status.write_in_progress)
	{
		/* Get device status */
		r_ospi_b_statusget(&status);
		if(RESET_VALUE == timeout)
		{
			stat |= ERROR;
			break;
		}
		Wait_us(1);
		timeout --;
	}
	return stat;
}

/**************************************************************************//**
* @details   Switch External Flash Memory to Write Enable mode.
* @param[in] None
* @retval    SUCESS - OK, ERROR - Failed
******************************************************************************/
static ospi_status_t r_ospi_b_write_enable(void)
{
	ospi_status_t stat = SUCCESS;
	
	spi_flash_direct_transfer_t transfer =
	{
		.command        = RESET_VALUE,
		.address        = RESET_VALUE,
		.data           = RESET_VALUE,
		.command_length = RESET_VALUE,
		.address_length = RESET_VALUE,
		.data_length    = RESET_VALUE,
		.dummy_cycles   = RESET_VALUE
	};
	
	/* Transfer write enable command */
	transfer = (OSPI_PROTOCOL_MODE_1S1S1S == OSPI_PROTOCOL_MODE)
				? g_ospi_b_direct_transfer[OSPI_B_TRANSFER_WRITE_ENABLE_SPI]
				: g_ospi_b_direct_transfer[OSPI_B_TRANSFER_WRITE_ENABLE_OPI];
	
	r_ospi_b_direct_transfer(&transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	
	/* Read Status Register */
	transfer = (OSPI_PROTOCOL_MODE_1S1S1S == OSPI_PROTOCOL_MODE)
				? g_ospi_b_direct_transfer[OSPI_B_TRANSFER_READ_STATUS_SPI]
				: g_ospi_b_direct_transfer[OSPI_B_TRANSFER_READ_STATUS_OPI];
	r_ospi_b_direct_transfer(&transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	
	/* Check Write Enable bit in Status Register */
	if(OSPI_B_WEN_BIT_MASK != (transfer.data & OSPI_B_WEN_BIT_MASK))
	{
		stat |= ERROR;
	}
	return stat;
}

/**************************************************************************//**
* @details   Configure pins/ports for OSPI.
* @param[in] None
* @retval    None
******************************************************************************/
void ospi_port_configuration(void)
{
	// ========================================================================
	// STEP 1: Unlock PWPR - Direct write (safer and clearer)
	// ========================================================================
	// Write 0x00: Clear both B0WI and PFSWE
	M8(MCU_PWPR_S_ADDR) = 0x00;
	
	// Write 0x40: B0WI=0, PFSWE=1 (allow PFS write)
	M8(MCU_PWPR_S_ADDR) = 0x40;
	
	// Write 0xC0: B0WI=1, PFSWE=1
	M8(MCU_PWPR_S_ADDR) = 0xC0;
	
	// ========================================================================
	// STEP 2: Configure OSPI pins
	// ========================================================================
	// Clock signals
	M32(MCU_PmnPFS_ADDR(6, 2)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(6, 3)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(6, 4)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(6, 5)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(6, 6)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(6, 7)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 0)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 1)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 2)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 3)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 4)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 5)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 6)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 7)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 9)) = PFS_CLEAR_ALL;
	M32(MCU_PmnPFS_ADDR(12, 10)) = PFS_CLEAR_ALL;
	
	
	M32(MCU_PmnPFS_ADDR(6, 2)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT);                                // P602: OM_1_SCLKN
	M32(MCU_PmnPFS_ADDR(6, 3)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT);                                // P603: OM_1_SCLK
	M32(MCU_PmnPFS_ADDR(6, 4)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                  // P604: IO[7]
	M32(MCU_PmnPFS_ADDR(6, 5)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                  // P605: IO[1]
	M32(MCU_PmnPFS_ADDR(6, 6)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                  // P606: IO[6]
	M32(MCU_PmnPFS_ADDR(6, 7)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT);
	M32(MCU_PmnPFS_ADDR(12, 0)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                 // PC00: IO[5]
	M32(MCU_PmnPFS_ADDR(12, 1)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                 // PC01: IO[0]
	M32(MCU_PmnPFS_ADDR(12, 2)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                 // PC02: IO[3]
	M32(MCU_PmnPFS_ADDR(12, 3)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                 // PC03: IO[4]
	M32(MCU_PmnPFS_ADDR(12, 4)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT);                 // PC04: IO[2]
	M32(MCU_PmnPFS_ADDR(12, 5)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PIDR_BIT | PFS_PODR_BIT); // PC05: OM_1_CS1
	M32(MCU_PmnPFS_ADDR(12, 6)) = (PFS_PSEL(OSPI_PSEL) | PFS_PCR_BIT | PFS_PDR_OUT);                                                    // PC06: ECS#/INT#
	M32(MCU_PmnPFS_ADDR(12, 7)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT);                               // PC07: RESET#
	M32(MCU_PmnPFS_ADDR(12, 9)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT);                                                                  // PC09: RSTO#
	M32(MCU_PmnPFS_ADDR(12, 10)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT);                                                                 // PC10: WP#
	
	
	M32(MCU_PmnPFS_ADDR(6, 2)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(6, 3)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(6, 4)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PCR_BIT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(6, 5)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PCR_BIT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(6, 6)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PCR_BIT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(6, 7)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 0)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 1)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 2)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 3)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 4)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 5)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PIDR_BIT | PFS_PODR_BIT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 6)) = (PFS_PSEL(OSPI_PSEL) | PFS_PCR_BIT | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 7)) = (PFS_PSEL(OSPI_PSEL) | PFS_DSCR(HIGH_SPEED_HIGH_DRIVER) | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 9)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT | PFS_PMR_BIT);
	M32(MCU_PmnPFS_ADDR(12, 10)) = (PFS_PSEL(OSPI_PSEL) | PFS_PDR_OUT | PFS_PMR_BIT);
	
	// ========================================================================
	// STEP 3: Lock PWPR
	// ========================================================================
	// Write 0x80: B0WI=1, PFSWE=0 (protect PFS)
	M8(MCU_PWPR_S_ADDR) = 0x80;
}