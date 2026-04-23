/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for RA6T1
 * @version  V1.0.1
 * @date     15 Feb 2024
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
 * Copyright (C) 2021 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 15.09.2021 1.0.0    First Release
 *				 : 15.02.2024	1.0.1		 Increase Programming Page Size
 *********************************************************************************************************************/
/** FlashOS Structures */
#include "../FlashOS.h"

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

/** Definition to access a specific address with any size */
#define M8(adr)  (*((volatile unsigned char  *) (adr)))
#define M16(adr) (*((volatile unsigned short *) (adr)))
#define M32(adr) (*((volatile unsigned long  *) (adr)))

/** Clock related registers adrress and bit definition */
#define MCU_SYSTEM_BASE                 (0x4001E000)
#define MCU_SCKDIVCR_ADDR               (MCU_SYSTEM_BASE+0x20)
    #define SCKDIVCR_FCLK_1DEV_MASK     (0x8FFFFFFF)
    #define SCKDIVCR_ICLK_1DEV_MASK     (0xF8FFFFFF)
#define MCU_SCKSCR_ADDR                 (MCU_SYSTEM_BASE+0x26)
    #define SCKSCR_CKSEL_HOCO           (0x00)
    #define SCKSCR_CKSEL_MOCO           (0x01)
    #define SCKSCR_CKSEL_LOCO           (0x02)
    #define SCKSCR_CKSEL_MOSC           (0x03)
    #define SCKSCR_CKSEL_SOSC           (0x04)
    #define SCKSCR_CKSEL_PLL            (0x05)
#define MCU_PLLCCR_ADDR                 (MCU_SYSTEM_BASE+0x28)
    #define PLLCCR_MUL                  (0x3F00)
    #define PLLCCR_PLSRCSEL_HOCO        (0x0010)
    #define PLLCCR_PLSRCSEL_MAIN        (0x0000)
    #define PLLCCR_PLSRCSEL_MASK        (0x0010)
    #define PLLCCR_PLIDIV_MASK          (0x0003)
    #define PLLCCR_PLIDIV_1DEV          (0x0000)
    #define PLLCCR_PLIDIV_2DEV          (0x0001)
    #define PLLCCR_PLIDIV_3DEV          (0x0002)
#define MCU_HOCOCR_ADDR                 (MCU_SYSTEM_BASE+0x36)
    #define HOCOCR_STOP                 (1)
    #define HOCOCR_ON                   (0)
#define MCU_MOCOCR_ADDR                 (MCU_SYSTEM_BASE+0x38)
    #define MOCOCR_STOP                 (1)
    #define MOCOCR_ON                   (0)
#define MCU_OSCSF_ADDR                  (MCU_SYSTEM_BASE+0x3C)
    #define OSCSF_HOCOSF                (0x01)
    #define OSCSF_MOSCSF                (0x08)
    #define OSCSF_PLLSF                 (0x20)
#define MCU_PRCR_ADDR                   (MCU_SYSTEM_BASE+0x3FE)
    #define KEYCODE_PRCR                (0xA500)
    #define PRCR_PRC1                   (0x0002)
    #define PRCR_PRC0                   (0x0001)
    #define PRCR_PRC2                   (0x0004)
    #define PRCR_PRC3                   (0x0008)
#define MCU_OPCCR_ADDR                  (MCU_SYSTEM_BASE+0x0A0)
    #define OPCCR_OPCCR                 (0x03)
    #define OPCCR_OPCCR_HIGH            (0x00)
    #define OPCCR_OPCCR_LOW             (0x03)
    #define OPCCR_OPCMTSF               (0x10)
    #define OPCCR_OPCMTSF_COMPLETE      (0x00)
    #define OPCCR_OPCMTSF_TRANS         (0x10)
#define MCU_SOPCCR_ADDR                 (MCU_SYSTEM_BASE+0x0AA)
    #define SOPCCR_SOPCM                (0x01)
    #define SOPCCR_SOPCM_SUBMODE        (0x01)
    #define SOPCCR_SOPCM_OTH            (0x00)
    #define SOPCCR_SOPCMTSF             (0x10)
    #define SOPCCR_SOPCMTSF_COMPLETE    (0x00)
    #define SOPCCR_SOPCMTSF_TRANS       (0x10)
#define MCU_OFS1_ADDR                   (0x00000404)
    #define OFS1_HOCOEN                 (0x00000100)
    #define OFS1_HOCOFRQ0_MASK          (0x00000600)
    #define OFS1_HOCOFRQ0_16MHZ         (0x00000000)
    #define OFS1_HOCOFRQ0_18MHZ         (0x00000200)
    #define OFS1_HOCOFRQ0_20MHZ         (0x00000400)

/** Flash related registers adrress and bit definition */
#define MCU_FLASH_BASE                 (0x407F0000)
#define MCU_FASTAT_ADDR                (MCU_FLASH_BASE + 0xE010)
    #define FASTAT_CMDLK               (0x10)
#define MCU_FAEINT_ADDR                (MCU_FLASH_BASE + 0xE014)
#define MCU_FRDYIE_ADDR                (MCU_FLASH_BASE + 0xE018)
#define MCU_FSADDR_ADDR                (MCU_FLASH_BASE + 0xE030)
#define MCU_FEADDR_ADDR                (MCU_FLASH_BASE + 0xE034)
#define MCU_FMEPROT_ADDR               (MCU_FLASH_BASE + 0xE044)
    #define FMEPROT_CEPROT             (0x0001)
    #define FMEPROT_KEY                (0xD900)
    #define FMEPROT_NOTPROTECT_MODE    (0x0000)
    #define FMEPROT_PROTECT_MODE       (0x0001)
#define MCU_FSTATR_ADDR                (MCU_FLASH_BASE + 0xE080)
    #define FSTATR_FRDY                (0x00008000)
    #define FSTATR_ILGLERR             (0x00004000)
    #define FSTATR_ERSERR              (0x00002000)
    #define FSTATR_PRGERR              (0x00001000)
    #define FSTATR_DBFULL              (0x00000400)
#define MCU_FENTRYR_ADDR               (MCU_FLASH_BASE + 0xE084)
    #define FENTRYR_READ_MODE          (0xAA00)
    #define FENTRYR_DATAF_PE           (0xAA80)
    #define FENTRYR_CODEF_PE           (0xAA01)
    #define FENTRYR_CHK_FENTRYD        (0x0080)
    #define FENTRYR_CHK_FENTRYC        (0x0001)
    #define FENTRYR_CHK_READ           (0x0000)
#define MCU_FSUINITR_ADDR              (MCU_FLASH_BASE + 0xE08C)
#define MCU_FCMDR_ADDR                 (MCU_FLASH_BASE + 0xE0A0)
#define MCU_FPESTAT_ADDR               (MCU_FLASH_BASE + 0xE0C0)
#define MCU_FCPSR_ADDR                 (MCU_FLASH_BASE + 0xE0E0)
#define MCU_FBCCNT_ADDR                (MCU_FLASH_BASE + 0xE0D0)
#define MCU_FBCSTAT_ADDR               (MCU_FLASH_BASE + 0xE0D4)
#define MCU_FPSADDR_ADDR               (MCU_FLASH_BASE + 0xE0D8)
#define MCU_FPCKAR_ADDR                (MCU_FLASH_BASE + 0xE0E4)
#define MCU_FMATSELC_ADDR              (MCU_FLASH_BASE + 0xE020)
    #define FMATSELC_EXTRA2            (0x3B02)
    #define FMATSELC_USER_AREA         (0x3B00)
#define MCU_FACI_CMD_AREA_ADDR         (0x407E0000)
#define MCU_FWEPROR_ADDR               (0x4001E416)

/** Various parameters */
#define FPCKAR_KEY              (0x1E00)
#define FWEPROR_PE_ENA          (0x01)
#define MCU_FENTRYC_CMPADDR     (0x00000000)
#define MCU_FENTRYD_CMPADDR     (0x40100000)
#define MCU_CONFAREA_CMPADDR    (0x0100A100)
#define DATAF_MASK_ADDR         (0x000FFFFF)
#define CODEF_MASK_ADDR         (0x00FFFFFF)
#define FACI_FORCED_QUIT_CMD    (0xB3)
#define FACI_ERASE1_CMD         (0x20)
#define FACI_STATUS_CLR_CMD     (0x50)
#define FACI_ERASE2_CMD         (0x21)
#define FACI_PROG_CMD           (0xE8)
#define FACI_READ_LB_CMD        (0x71)
#define FACI_PROG_LB_CMD        (0x77)
#define FACI_CONF_EXTA_CMD      (0x40)
#define FACI_END_CMD            (0xD0)
#define FACI_PROG_U_NUM         (0x40)
#define FACI_PROG_D_NUM         (0x02)
#define FACI_CONF_EXTA_NUM      (0x08)

#define CODEF_8K_BLOCK_NUM      (0x08)

#ifdef RA6T1_512K
#define CODEF_32K_BLOCK_NUM     (0x0E)
#define DATAF_64B_BLOCK_NUM     (0x80)
#endif
#ifdef RA6T1_256K
#define CODEF_32K_BLOCK_NUM     (0x06)
#define DATAF_64B_BLOCK_NUM     (0x80)
#endif
#ifdef RA6T1_DATA
#ifdef CODE_512K
#define CODEF_32K_BLOCK_NUM     (0x0E)
#endif
#ifdef CODE_256K
#define CODEF_32K_BLOCK_NUM     (0x06)
#endif
#define DATAF_64B_BLOCK_NUM     (0x80)
#endif
#ifdef RA6T1_CONF
#define CODEF_32K_BLOCK_NUM     (0x00)
#define DATAF_64B_BLOCK_NUM     (0x00)
#endif

#define __NOP          __builtin_arm_nop

/** Type declaration */
typedef unsigned char  ubyte_t;
typedef unsigned short uword_t;
typedef unsigned long  udword_t;

/** Index of internal data buffer */
typedef enum reg_data{
    PRCR = 0,
    SCKSCR,
    SCKDIVCR,
    PLLCCR,
    HOCOCR,
    MOCOCR,
    SOPCCR,
    OPCCR,
    FPCKAR,
    OFS1,
    FWEPROR,
		FMEPROT,
    REG_MAX,
}reg_data;

/** Register save structure */
static udword_t    s_udwClockUserVal[REG_MAX];

/** Internal function declaration */
static udword_t check_clock_and_mode(udword_t *clk);
static udword_t change_clock_and_mode(udword_t *clk);
static udword_t restore_clock_and_mode(void);
static udword_t pe_mode_entry(udword_t dwAddr, udword_t *adrRet);
static udword_t ilgl_chk(void);
static void moco_wait(udword_t *clk);
static void read_page(udword_t adr, udword_t sz, ubyte_t *buf);

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
    ubyte_t ub_mhz_clk;

    ub_ret_val = check_clock_and_mode(&clk); // Check if flash rewritable clock and power control mode
#ifdef CHG_CLK_AND_MOD_ENA
    /** If the clock and mode cannot rewrite the flash, switch to a possible state */
    if(ub_ret_val)
    {
        ub_ret_val = change_clock_and_mode(&clk);
    }
#endif

    if(ub_ret_val)
    {
        /** Failure if the flash cannot be rewritten */
        return (1); // Failed
    }


    M8(MCU_FWEPROR_ADDR) = FWEPROR_PE_ENA; // Permits P/E
    ub_mhz_clk = clk / 1000000; // Conver to MHz unit

    if (clk % 1000000)
    {
        ub_mhz_clk += 1; // Round up the first decimal place in MHz
    }

    M16(MCU_FPCKAR_ADDR) = FPCKAR_KEY | ub_mhz_clk; // Flash Sequencer Operating Clock Notification
    return ilgl_chk(); // Check for command locked and illegal status
		
}

/**************************************************************************//**
* @details   De-Initialize Flash Programming Functions
* @param[in] fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int UnInit (unsigned long fnc)
{
	
    M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE; // Transition to read mode

#ifdef CHG_CLK_AND_MOD_ENA
    /** Restore clock and power control mode to the state before flash rewriting */
    if (restore_clock_and_mode())
    {
        return (1); // Failed
    }
#endif

    return (0); // OK
		
}

/**************************************************************************//**
* @details   Program Page in Flash Memory
* @param[in] adr:  Block start Address
* @param[in] sz:   Block size in byte
* @param[in] pat:  Pattern to compare
* @retval    0 - Memory is blank,  1 - Memory is not blank
******************************************************************************/
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat)
{
	
    ubyte_t ub_rd_buf[128];
    uword_t i, j ,k;

    M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE; // Transition to read mode
	
    for(i = 0; i < sz ; i += 128)
    {
        read_page(adr, 128, &ub_rd_buf[0]);
		
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
        }	
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
	
    udword_t udw_addr;
    ubyte_t uby_ilgl_chk;
    uword_t uw_block_total_num;
    uword_t i;
	
#ifdef RA6T1_CONF
	  /** Configuration area not support EraseChip */
    return (0); // OK
#endif
    uw_block_total_num = CODEF_8K_BLOCK_NUM + CODEF_32K_BLOCK_NUM + DATAF_64B_BLOCK_NUM;

    /** Repeat the process for the number of blocks of Code Flash and Data Flash */
    for(i = 0; i < uw_block_total_num; i++)
    {
            /** Calculate the start address of a block from the size and number of blocks */
            if (i == 0)
    				{
                pe_mode_entry(MCU_FENTRYC_CMPADDR, &udw_addr); // Transition to P/E mode
	    	    } else if (0 < i && i <= CODEF_8K_BLOCK_NUM)
		        {
                udw_addr += 0x2000;
            } else if (CODEF_8K_BLOCK_NUM < i && i < (CODEF_8K_BLOCK_NUM + CODEF_32K_BLOCK_NUM))
            {
                udw_addr += 0x8000;
            } else if (i == CODEF_8K_BLOCK_NUM + CODEF_32K_BLOCK_NUM) // First block of Data Flash
            {
                pe_mode_entry(MCU_FENTRYD_CMPADDR, &udw_addr); // Transition to P/E mode
            } else 
            {
                udw_addr += 0x40;
            }
			  
    M32(MCU_FSADDR_ADDR) = udw_addr; // Set Erase Sector address
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_ERASE1_CMD; // Execution of erase command
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD; // Execution of termination command

    while(!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
    {
        ; // Waiting for command completion
    }

    if(M32(MCU_FSTATR_ADDR) & FSTATR_ERSERR)
    {
        /** Erase error */
        return (1); // Failed
    }

    /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if(uby_ilgl_chk)
    {
        return (1); // Failed
    }
		
	}
    return (0); // OK

}

/**************************************************************************//**
* @details   Erase Sector in Flash Memory
* @param[in] adr:  Sector Address
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int EraseSector (unsigned long adr)
{

    udword_t udw_addr;
    ubyte_t uby_ilgl_chk;

    /** Check Flash Write Mode */
    if(adr < MCU_CONFAREA_CMPADDR)
    {
        ; // Execute EraseSector command if Code Flash
    } else if(adr < MCU_FENTRYD_CMPADDR)
    {
        /** Configuration area not support EraseSector */
        return (0); // OK
    } else
    {
        ; // Execute EraseSector command if Data Flash
    }

    pe_mode_entry(adr, &udw_addr); // Transition to P/E mode
    M32(MCU_FSADDR_ADDR) = udw_addr; // Set Erase Sector address
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_ERASE1_CMD; // Execution of erase command
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD; // Execution of termination command

    while(!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
    {
        ; // Waiting for command completion
    }

    if(M32(MCU_FSTATR_ADDR) & FSTATR_ERSERR)
    {
        /** Erase error */
        return (1); // Failed
    }

    /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if(uby_ilgl_chk)
    {
        return (1); // Failed
    }

    return (0); // OK
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

    uword_t uw_start_data = 0;
    uword_t uw_wrt_cnt;
    uword_t uw_wrt_byte;
    uword_t i;
    udword_t udw_adr_tmp;
    ubyte_t uby_area_flg;
    ubyte_t uby_ilgl_chk;

		pe_mode_entry(adr,&udw_adr_tmp); // Transition to P/E mode
		
		while (sz)
    {	
				/** Check Flash Write Mode */
				if (adr < MCU_CONFAREA_CMPADDR)
				{
						uby_area_flg = 1; // Code Flash
						if (sz < 128)
						{
								for (i = sz; i < 128; i++){
										buf[i] = 0xFF;
								}
								sz = 128;
						}
				} else if (adr < MCU_FENTRYD_CMPADDR)
				{
						uby_area_flg = 2; // Configuration area
						if (sz < 16)
						{
								for(i = sz; i < 16; i++){
										buf[i] = 0xFF;
								}
								sz = 16;
						}
				} else
				{
						uby_area_flg = 0; // Data Flash
						if (sz < 4)
						{
								for(i = sz; i < 4; i++){
										buf[i] = 0xFF;
								}
								sz = 4;
						}
				}

				if (uby_area_flg == 2)
				{
						M32(MCU_FSADDR_ADDR) = adr; // Set the program start address
						M8(MCU_FACI_CMD_AREA_ADDR) = FACI_CONF_EXTA_CMD; // Execution of program command for configuration area
						M8(MCU_FACI_CMD_AREA_ADDR) = FACI_CONF_EXTA_NUM; // Set the number of write data access
						uw_wrt_cnt = FACI_CONF_EXTA_NUM;
						uw_wrt_byte = FACI_CONF_EXTA_NUM * 2; // Data size to write with one command
				} else
				{
						M32(MCU_FSADDR_ADDR) = udw_adr_tmp; // Set the program start address
						if (uby_area_flg == 1)
						{
								M8(MCU_FACI_CMD_AREA_ADDR) = FACI_PROG_CMD; // Execution of program command
								M8(MCU_FACI_CMD_AREA_ADDR) = FACI_PROG_U_NUM; // Set the number of write data access
								uw_wrt_cnt = FACI_PROG_U_NUM;
								uw_wrt_byte = FACI_PROG_U_NUM * 2; // Data size to write with one command
						} else
						{
								M8(MCU_FACI_CMD_AREA_ADDR) = FACI_PROG_CMD; // Execution of program command
								M8(MCU_FACI_CMD_AREA_ADDR) = FACI_PROG_D_NUM; // Set the number of write data access
								uw_wrt_cnt = FACI_PROG_D_NUM;
								uw_wrt_byte = FACI_PROG_D_NUM * 2; // Data size to write with one command
						}
				}

				if ((uby_area_flg == 1) || (uby_area_flg == 0))
				{
						for (i = 0; i < uw_wrt_cnt; i++)
						{
								/** Store 2 bytes each for programming data */
								M16(MCU_FACI_CMD_AREA_ADDR) = (uword_t)(buf[uw_start_data] | (buf[uw_start_data+1] << 8));
								uw_start_data +=2;
								while (M32(MCU_FSTATR_ADDR) & FSTATR_DBFULL)
								{
										; // Waiting for Data Buffer full to resolve
								}
						}
				} else
				{
						for (i = 0; i < uw_wrt_cnt; i++)
						{
								/** Store 2 bytes each for programming data */
								M16(MCU_FACI_CMD_AREA_ADDR) = (uword_t)(buf[uw_start_data] | (buf[uw_start_data+1] << 8));
								uw_start_data +=2;
						}
				}

				M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD; // Execution of termination command
				while (!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
				{
						; // Waiting for command completion
				}
				
				adr += uw_wrt_byte;
				udw_adr_tmp += uw_wrt_byte; // Update programming start address
				sz -= uw_wrt_byte; // Update remaining size
				if (M32(MCU_FSTATR_ADDR) & FSTATR_PRGERR)
				{
						/** Programming error */
						return (1); // Failed
				}

    }

    /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if (uby_ilgl_chk)
    {
        return (1); // Failed
    }

    return (0); // OK
}

/**************************************************************************//**
* @details   Verify the size from the specified address
* @param[in] adr:  Start Address
* @param[in] sz:   Page Size
* @param[in] buf:  Page Data
* @retval    0 - OK,  1 - Failed
******************************************************************************/
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf)
{

    ubyte_t ub_rd_buf[128];
		unsigned long adr_tmp;
    uword_t i, j ,k, m;
		adr_tmp = adr;
    m = 0;
	
    M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE; // Transition to read mode
	
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
				
        /** Check up to 128 bytes if equal to read data */
        for (j = 0; j < k; j++)
        {
            if (ub_rd_buf[j] != buf[m])
            {
                return (1); // Memory is not blank	
            }
            m++;
        }	
				adr_tmp = adr + m;
    }
	  return (adr + m); // Verify is success

}

/**************************************************************************//**
* @details   Check if flash rewritable clock and power control mode
* @param[in] udwp_clk:  Clock Frequency (Hz)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t check_clock_and_mode(udword_t *udwp_clk)
{

    udword_t udw_sckdivcr;
    udword_t udw_sckdivcr_val;
    udword_t udw_clk_tmp;
    uword_t uw_bai;
    uword_t uw_plli_div;

    /** There is no problem with the following cast conversions
     * because it is a type conversion from unsigned to unsigned */
    /** Saving registers */
    s_udwClockUserVal[PRCR] = (udword_t)M16(MCU_PRCR_ADDR);
    s_udwClockUserVal[SCKSCR] = (udword_t)M8(MCU_SCKSCR_ADDR);
    udw_sckdivcr = M32(MCU_SCKDIVCR_ADDR);
    s_udwClockUserVal[SCKDIVCR] = udw_sckdivcr;
    s_udwClockUserVal[PLLCCR] = (udword_t)M16(MCU_PLLCCR_ADDR);
    s_udwClockUserVal[HOCOCR] = (udword_t)M8(MCU_HOCOCR_ADDR);
    s_udwClockUserVal[MOCOCR] = (udword_t)M8(MCU_MOCOCR_ADDR);
    s_udwClockUserVal[SOPCCR] = (udword_t)M8(MCU_SOPCCR_ADDR);
    s_udwClockUserVal[OPCCR] = (udword_t)M8(MCU_OPCCR_ADDR);
    s_udwClockUserVal[FPCKAR] = (udword_t)M16(MCU_FPCKAR_ADDR);
    s_udwClockUserVal[OFS1] = M32(MCU_OFS1_ADDR);
    s_udwClockUserVal[FWEPROR] = (udword_t)M8(MCU_FWEPROR_ADDR);

    /** Failed if power control mode is Subosc-speed mode */
    if ((s_udwClockUserVal[SOPCCR] & SOPCCR_SOPCM) != SOPCCR_SOPCM_OTH)
    {
        return (1); // Failed
    }

    /** Failed if power control mode is Low-speed mode */
    if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCCR) != OPCCR_OPCCR_HIGH)
    {
        return (1); // Failed
    }

    /** Calculate FCLK */
    switch (s_udwClockUserVal[SCKSCR])
    {
        case SCKSCR_CKSEL_PLL:
            if(s_udwClockUserVal[PLLCCR] & PLLCCR_PLSRCSEL_HOCO)
            { // Input clock source is HOCO
                if(OFS1_HOCOFRQ0_16MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 16000000;
                } else if(OFS1_HOCOFRQ0_18MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 18000000;
                } else if(OFS1_HOCOFRQ0_20MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 20000000;
                } else
                {
                    return (1); // Failed
                }
            } else
            {
                udw_clk_tmp = *udwp_clk; // Input clock source is External clock
            }

            uw_plli_div = (s_udwClockUserVal[PLLCCR] & PLLCCR_PLIDIV_MASK) + 1; // PLL input frequency division
            uw_bai = ((s_udwClockUserVal[PLLCCR] & PLLCCR_MUL) >> 8) + 1; // PLL frequency multiplication factor
            udw_clk_tmp = (udw_clk_tmp / (udword_t)uw_plli_div) * (udword_t)uw_bai; // Frequency output as PLL
        break;
        case SCKSCR_CKSEL_MOSC:
            udw_clk_tmp = *udwp_clk; // Input clock source is External clock
        break;
        case SCKSCR_CKSEL_MOCO:
            udw_clk_tmp = 8000000; // MOCO frequency is 8MHz
        break;
        case SCKSCR_CKSEL_HOCO:
            if(OFS1_HOCOFRQ0_16MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
            {
                udw_clk_tmp = 16000000; // HOCO frequency is 16MHz
            } else if(OFS1_HOCOFRQ0_18MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
            {
                udw_clk_tmp = 18000000; // HOCO frequency is 18MHz
            } else if(OFS1_HOCOFRQ0_20MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
            {
                udw_clk_tmp = 20000000; // HOCO frequency is 20MHz
            } else
            {
                return (1); // Failed
            }
        break;
        case SCKSCR_CKSEL_LOCO: // Failed because LOCO frequency is below 4MHz
        case SCKSCR_CKSEL_SOSC: // Failed because SOSC frequency is below 4MHz
        default:
            return (1); // Failed
    }

    udw_sckdivcr = udw_sckdivcr >> 28;
    udw_sckdivcr_val = 1 << udw_sckdivcr; // FCLK division value
    *udwp_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate FCLK frequency

    /** Failed because outside the allowable frequency(4-60MHz) of FCLK */
    if(((*udwp_clk) < 4000000) || (60000000 < (*udwp_clk)))
    {
        return (1); // Failed
    }

    return (0); // OK

}

/**************************************************************************//**
* @details   Change to flash rewritable clock and power control mode
* @param[in] udwp_clk:  Clock Frequency (Hz)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t change_clock_and_mode(udword_t *udwp_clk)
{

    M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | PRCR_PRC1 | PRCR_PRC0; // Unprotect some registers

    /** If it is Subosc-speed mode, set it to other than Subosc-speed mode */
    if ((s_udwClockUserVal[SOPCCR] & SOPCCR_SOPCM) != SOPCCR_SOPCM_OTH)
    {
        M8(MCU_SOPCCR_ADDR) = SOPCCR_SOPCM_OTH;
        while (M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF)
        {
            ; // Wait till transition completed
        }
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
    if ((s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) == OFS1_HOCOFRQ0_MASK)
    {
        /** If the clock source is MOCO, set the clock source to MOCO */
        if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
        {
            /** If MOCO oscillation is stopped, oscillate it */
            if(s_udwClockUserVal[MOCOCR] == MOCOCR_STOP)
            {
                M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
                moco_wait(udwp_clk);// Wait for oscillation stabilization of MOCO
            }
            M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;// Set the clock source to MOCO
        }
        /** Change ICLK and FCLK to divide by 1 */
        M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_1DEV_MASK & SCKDIVCR_ICLK_1DEV_MASK);
        *udwp_clk = 8000000; // FCLK frequency is 8MHz
    } else
    {/** If OFS1 is not in the initial state, set the clock source to HOCO */
        if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_HOCO)
        {

            /** If HOCO oscillation is stopped, oscillate it */
            if(s_udwClockUserVal[HOCOCR] == HOCOCR_STOP)
            {
                M8(MCU_HOCOCR_ADDR) = HOCOCR_ON;
                while (!(M8(MCU_OSCSF_ADDR) & OSCSF_HOCOSF))
                {
                    ;// Wait for oscillation stabilization of HOCO
                }
            }
            M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_HOCO;// Set the clock source to HOCO
        }

        /** Change ICLK and FCLK to divide by 1 */
        M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_1DEV_MASK & SCKDIVCR_ICLK_1DEV_MASK);

        switch (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK)
        {
            case OFS1_HOCOFRQ0_16MHZ:
                *udwp_clk = 16000000; // FCLK frequency is 16MHz
            break;
            case OFS1_HOCOFRQ0_18MHZ:
                *udwp_clk = 18000000; // FCLK frequency is 18MHz
            break;
            case OFS1_HOCOFRQ0_20MHZ:
                *udwp_clk = 20000000; // FCLK frequency is 20MHz
            break;
            default:
                return (1); // Failed
        }
    }
		
    return (0); // OK

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
    M32(MCU_SCKDIVCR_ADDR) = s_udwClockUserVal[SCKDIVCR]; // Restore system clock division
    M16(MCU_SCKSCR_ADDR) = (uword_t)s_udwClockUserVal[SCKSCR]; // Restore system clock source
    M8(MCU_HOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[HOCOCR]; // Restore the oscillation state of HOCO
    M8(MCU_MOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[MOCOCR]; // Restore the oscillation state of MOCO

    /** Restore power control mode */
    M8(MCU_OPCCR_ADDR) = (ubyte_t)s_udwClockUserVal[OPCCR];
    M8(MCU_SOPCCR_ADDR) = (ubyte_t)s_udwClockUserVal[SOPCCR];

    M8(MCU_FWEPROR_ADDR) = (ubyte_t)s_udwClockUserVal[FWEPROR]; // Restore flash P/E protect state
    M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | (ubyte_t)s_udwClockUserVal[PRCR]; // Restore the protection status
	
    return (0); // OK

}

/**************************************************************************//**
* @details    Transition to P/E mode
* @param[in]  udw_addr: Write start address
* @param[out] udwp_adr_ret: Write start address after mask
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t pe_mode_entry(udword_t udw_addr, udword_t *udwp_adr_ret)
{

    M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE; // Transition to read mode
    while(M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_READ) // Wait until switching to read mode
    {
        ;
    }

    if(udw_addr < MCU_FENTRYD_CMPADDR)
    {
        M16(MCU_FENTRYR_ADDR) = FENTRYR_CODEF_PE; // Transition to Code Flash P/E mode
        *udwp_adr_ret = udw_addr & CODEF_MASK_ADDR; // Set the write start address after mask
        while(M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYC) // Wait until switching to Code Flash P/E mode
        {
            ;
        }
    } else
    {
        M16(MCU_FENTRYR_ADDR) = FENTRYR_DATAF_PE; // Transition to Data Flash P/E mode
        *udwp_adr_ret = udw_addr & DATAF_MASK_ADDR; // Set the write start address after mask
        while(M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYD) // Wait until switching to Data Flash P/E mode
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
    ubydata = M8(MCU_FASTAT_ADDR);
    if(ubydata & FASTAT_CMDLK)
    {
        return(1); // Failed
    }

    /** Check for illegal state */
    udwdata = M32(MCU_FSTATR_ADDR);
    if(udwdata & FSTATR_ILGLERR)
    {
        return(1); // Failed
    }

    return (0); // OK

}

/**************************************************************************//**
* @details   Wait for oscillation stabilization of MOCO (about 15us)
* @param[in] udwp_clk: Clock Frequency (Hz)
* @retval    None
******************************************************************************/
static void moco_wait(udword_t *udwp_clk)
{

    udword_t    udw_count;
    udword_t    udw_ns_per_cycle;

    udw_ns_per_cycle = 1000000000 / (*udwp_clk); // Calculate ns per cycle
    /** Calculate the number of cycles required to wait about 15us */
    udw_count = 15000 / udw_ns_per_cycle;
    /** Since it takes 10 cycles to execute one for statement,
     * the number of executions of it is calculated based on that. */
    udw_count = udw_count / 10;

    for ( ; udw_count > 0; udw_count--)
    {
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
    }

}

/**************************************************************************//**
* @details   Read the data written in the flash
* @param[in] udw_addr:  Read start address
* @param[in] udw_sz:   	Read size
* @param[in] ubp_buf:   Buffer to store read data
* @retval    None
******************************************************************************/
static void read_page(udword_t udw_addr, udword_t udw_sz, ubyte_t *ubp_buf)
{

    uword_t i;

    for(i = 0; i < udw_sz ; i++)
    {
        ubp_buf[i] = M8(udw_addr + i); // Store read data byte by byte
    }

}
