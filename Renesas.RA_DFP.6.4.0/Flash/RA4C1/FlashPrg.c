/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for RA4C1
 * @version  V1.0.0
 * @date     30 Nov 2024
 ******************************************************************************/
/*
 * Copyright (c) 2010-2018 Arm Limited. All rights reserved.
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
 * Copyright (C) 2024 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 30.11.2024 1.0.0    First Release	
 *********************************************************************************************************************/
 
 /*
    FlashOS Structures
 */
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
      unsigned  long Verify     (unsigned long adr,   // Verify Function
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
#define MCU_SYSTEM_BASE									(0x4001E000)
#define MCU_SCKDIVCR_ADDR               (MCU_SYSTEM_BASE+0x20)
    #define SCKDIVCR_FCLK_DEV_MASK      (0x8FFFFFFF)
    #define SCKDIVCR_ICLK_DEV_MASK      (0xF8FFFFFF)
		#define SCKDIVCR_FCLK_2DEV_MASK     (0x10000000)
    #define SCKDIVCR_ICLK_2DEV_MASK     (0x01000000)
		#define SCKDIVCR_FCLK_8DEV_MASK     (0x30000000)
    #define SCKDIVCR_ICLK_8DEV_MASK     (0x03000000)
    #define SCKDIVCR_FCK_MASK           (0x70000000)
    #define SCKDIVCR_ICK_MASK           (0x07000000)
#define MCU_PRCR_ADDR                   (MCU_SYSTEM_BASE+0x3FE)
    #define KEYCODE_PRCR                (0xA500)
    #define PRCR_PRC1                   (0x0002)
    #define PRCR_PRC0                   (0x0001)
    #define PRCR_PRC4                   (0x0010)
    #define PRCR_PRC3                   (0x0008)
#define MCU_SCKSCR_ADDR									(MCU_SYSTEM_BASE + 0x026)
		#define SCKSCR_CKSEL_HOCO	          (0x00)
		#define SCKSCR_CKSEL_MOCO	          (0x01)
		#define SCKSCR_CKSEL_LOCO	          (0x02)
		#define SCKSCR_CKSEL_MOSC	          (0x03)
		#define SCKSCR_CKSEL_SOSC	          (0x04)
		#define SCKSCR_CKSEL_PLL            (0x05)
		#define SCKSCR_CKSEL_MASK	          (0x07)
#define MCU_PLLCCR_ADDR                 (MCU_SYSTEM_BASE+0x28)
    #define PLLCCR_MUL                  (0x1F00)
    #define PLLCCR_PLSRCSEL_HOCO        (0x0010)
    #define PLLCCR_PLSRCSEL_MAIN        (0x0000)
    #define PLLCCR_PLSRCSEL_MASK        (0x0010)
    #define PLLCCR_PLIDIV_MASK          (0x0003)
    #define PLLCCR_PLIDIV_1DEV          (0x0000)
    #define PLLCCR_PLIDIV_4DEV          (0x0001)
    #define PLLCCR_PLIDIV_6DEV          (0x0002)
#define MCU_SOPCCR_ADDR                 (MCU_SYSTEM_BASE+0x0AA)
    #define SOPCCR_SOPCM                (0x01)
    #define SOPCCR_SOPCM_SUBMODE        (0x01)
    #define SOPCCR_SOPCM_OTH            (0x00)
    #define SOPCCR_SOPCMTSF             (0x10)
    #define SOPCCR_SOPCMTSF_COMPLETE    (0x00)
    #define SOPCCR_SOPCMTSF_TRANS       (0x10)
#define MCU_OPCCR_ADDR                  (MCU_SYSTEM_BASE+0x0A0)
    #define OPCCR_OPCM                  (0x03)
    #define OPCCR_OPCM_HIGH             (0x00)
		#define OPCCR_OPCM_MID            	(0x01)
    #define OPCCR_OPCM_LOW              (0x03)
    #define OPCCR_OPCMTSF               (0x10)
    #define OPCCR_OPCMTSF_COMPLETE      (0x00)
    #define OPCCR_OPCMTSF_TRANS         (0x10)
#define MCU_OFS1_ADDR										(0x0100A200)
		#define OFS1_HOCOFRQ0_48MHZ					(0x00000000)
		#define OFS1_HOCOFRQ0_64MHZ					(0x00000200)
		#define OFS1_HOCOFRQ0_80MHZ					(0x00000400)
		#define OFS1_HOCOFRQ0_24MHZ					(0x00000800)
		#define OFS1_HOCOFRQ0_32MHZ					(0x00000A00)
		#define OFS1_HOCOFRQ0_40MHZ					(0x00000C00)
		#define OFS1_HOCOFRQ0_MASK					(0x00000E00)
		#define OFS1_HOCOEN                 (0x00000100)
#define MCU_MOCOCR_ADDR									(MCU_SYSTEM_BASE + 0x038)
		#define MOCOCR_STOP									(0x01)
		#define MOCOCR_ON										(0x00)
#define MCU_HOCOCR_ADDR									(MCU_SYSTEM_BASE + 0x036)
		#define HOCOCR_STOP									(0x01)
		#define HOCOCR_ON										(0x00)
#define MCU_OSCSF_ADDR									(MCU_SYSTEM_BASE + 0x03C)
		#define OSCSF_HOCOSF                (0x01)
    #define OSCSF_MOSCSF                (0x08)
    #define OSCSF_PLLSF                 (0x20)

/*
   Flash related registers adrress and bit definition
*/
#define MCU_FLASH_BASE                  (0x407F0000)
#define MCU_FASTAT_ADDR                 (MCU_FLASH_BASE + 0xE010)
		#define FASTAT_DFAE                 (0x08)
    #define FASTAT_CMDLK                (0x10)
		#define FASTAT_CFAE                 (0x80)
#define MCU_FAEINT_ADDR                 (MCU_FLASH_BASE + 0xE014)
#define MCU_FRDYIE_ADDR                 (MCU_FLASH_BASE + 0xE018)
#define MCU_FSADDR_ADDR                 (MCU_FLASH_BASE + 0xE030)
#define MCU_FEADDR_ADDR                 (MCU_FLASH_BASE + 0xE034)
#define MCU_FMEPROT_ADDR                (MCU_FLASH_BASE + 0xE044)
    #define FMEPROT_CEPROT              (0x0001)
    #define FMEPROT_KEY                 (0xD900)
    #define FMEPROT_NOTPROTECT_MODE     (0x0000)
    #define FMEPROT_PROTECT_MODE        (0x0001)
#define MCU_FSTATR_ADDR                 (MCU_FLASH_BASE + 0xE080)
    #define FSTATR_FRDY                 (0x00008000)
    #define FSTATR_ILGLERR              (0x00004000)
    #define FSTATR_ERSERR               (0x00002000)
    #define FSTATR_PRGERR               (0x00001000)
#define MCU_FENTRYR_ADDR                (MCU_FLASH_BASE + 0xE084)
    #define FENTRYR_READ_MODE           (0xAA00)
    #define FENTRYR_DATAF_PE            (0xAA80)
    #define FENTRYR_CODEF_PE            (0xAA01)
    #define FENTRYR_CHK_FENTRYD         (0x0080)
    #define FENTRYR_CHK_FENTRYC         (0x0001)
    #define FENTRYR_CHK_READ            (0x0000)
#define MCU_FBCSTAT_ADDR                (MCU_FLASH_BASE + 0xE0D4)
		#define FBCSTAT_FULL          	    (0x01)
#define MCU_FSUINITR_ADDR               (MCU_FLASH_BASE + 0xE08C)
#define MCU_FCMDR_ADDR                  (MCU_FLASH_BASE + 0xE0A0)
#define MCU_FBCCNT_ADDR                 (MCU_FLASH_BASE + 0xE0D0)
		#define FBCCNT_BCDIR_INCRE				  (0x00)
		#define FBCCNT_BCDIR_DECRE			    (0x01)
#define MCU_FPCKAR_ADDR                 (MCU_FLASH_BASE + 0xE0E4)
		#define FPCKAR_KEY                  (0x1E00)
#define MCU_FACI_CMD_AREA_ADDR          (0x407E0000)
#define MCU_FWEPROR_ADDR                (0x4001E416)
#define MCU_DFLCTL_ADDR								  (0x407EC000 + 0x090)
		#define DFLCTL_DFLEN_ENA					  (0x01)
	  #define DFLCTL_DFLEN_DIS					  (0x00)
/*
   Other parameters
*/
#define MAX_OPERATING_FREQUENCY					(80000000)
#define RES_ERROR												(1)
#define RES_OK													(0)
#define FLASH_WRITEABLE_MIN_CLOCK_HZ		(1000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH		(80000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_MID		(4000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_LOW		(1000000)
#define FWEPROR_PE_ENA            			(0x01)
#define MCU_FENTRYC_CMPADDR      	  	  (0x00000000)
#define MCU_FENTRYD_CMPADDR      				(0x08000000)
#define MCU_CONFAREA_CMPADDR      			(0x0100A100)
#define DATAF_MASK_ADDR         			  (0x0FFFFFFF)
#define CODEF_MASK_ADDR          			  (0x000FFFFF)

#define BLANK_VALUE											(0xFF)
#define CODEF_START_ADDR								(0x00000000)
#define DATAF_START_ADDR								(0x40100000)
#define CONFIG_START_ADDR								(0x01010000)
#define CODEF_LOWER_START_ADDR      		(0x00000000)
#define CODEF_UPPER_START_ADDR      		(0x00200000)
#define CODEF_SIZE											(0x00080000) // Should re-define this value base on user's marco
#define DATAF_SIZE											(0x00002000) 
#define FACI_FORCED_QUIT_CMD    			  (0xB3)
#define FACI_ERASE1_CMD          	  		(0x20)
#define FACI_STATUS_CLR_CMD   			    (0x50)
#define FACI_ERASE2_CMD        			    (0x21)
#define FACI_PROG_CMD          			    (0xE8)
#define FACI_READ_LB_CMD        			  (0x71)
#define FACI_CHIP_ERASE_CMD							(0x2F)

#define FACI_CONF_EXTA_CMD     			    (0x40)
#define FACI_END_CMD            			  (0xD0)
#define FACI_PROG_U_NUM         			  (0x04)
#define FACI_PROG_D_NUM         			  (0x01)
#define FACI_CONF_EXTA_NUM      			  (0x08)

#ifdef RA4C1_512K
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00080000)
#endif
#ifdef RA4C1_512K_dual_lower
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00040000)
#endif
#ifdef RA4C1_512K_dual_upper
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00040000)
#endif
#ifdef RA4C1_256K
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00040000)
#endif
#ifdef RA4C1_256K_dual_lower
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00020000)
#endif
#ifdef RA4C1_256K_dual_upper
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00020000)
#endif
#ifdef RA4C1_DATA
		#ifdef CODE_512K
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00080000)
		#endif
		
		#ifdef CODE_512K_dual
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00040000)
		#endif
		
		#ifdef CODE_256K
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00040000)
		#endif
		
		#ifdef CODE_256K_dual
		#undef CODEF_SIZE										// Undef macro first
		#define CODEF_SIZE									(0x00020000)
		#endif
		
		#undef DATAF_SIZE										// Undef macro first
		#define DATAF_SIZE									(0x00002000)
#endif
#ifdef RA4C1_CONF
#endif
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
		DFLCTL,
    FWEPROR,
	  FMEPROT,
    REG_MAX,
}reg_data;

/** Register save structure */
static udword_t    s_udwClockUserVal[REG_MAX];
/*
   Current ICLK frequency
*/
static udword_t dwICLKValue;

/** Internal function declaration */
static udword_t check_clock_and_mode(udword_t *clk);
static udword_t change_clock_and_mode(udword_t *clk);
static udword_t restore_clock_and_mode(void);
static ubyte_t saveModeAndClock(void);
static udword_t pe_mode_entry(udword_t dwAddr, udword_t *adrRet);
static udword_t ilgl_chk(void);
static void Wait_us(udword_t us);
static void read_page(udword_t adr, udword_t sz, ubyte_t *buf);
static int EraseChipArea(unsigned long adr, unsigned long sz);
static void SetReadMode(void);

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
		
    // Initialize the maximum value of ICLK
    dwICLKValue = 80000000;
		// Save current settings
		saveModeAndClock();
		ub_ret_val = check_clock_and_mode(&clk); 
		/* Check if flash rewritable clock and power control mode
				and save current frequency for dwICLKValue
		*/	
#ifdef CHG_CLK_AND_MOD_ENA
    /** If the clock and mode cannot rewrite the flash, switch to a possible state */
    if (ub_ret_val)
    {
        ub_ret_val = change_clock_and_mode(&clk);
			
				// Save ICLK frequency for wait processing
				dwICLKValue = clk; // Both FCLK and ICLK have the same Source Clock and Division
    }
#endif

    if (ub_ret_val)
    {
        /** Failure if the flash cannot be rewritten */
        return (RES_ERROR); // Failed
    }

    M8(MCU_FWEPROR_ADDR) = FWEPROR_PE_ENA; // Permits P/E
    ub_mhz_clk = clk / 1000000; // Conver to MHz unit

    if (clk % 1000000)
    {
        ub_mhz_clk += 1; // Round up the first decimal place in MHz
    }

    M16(MCU_FPCKAR_ADDR) = FPCKAR_KEY | ub_mhz_clk; // Flash Sequencer Operating Clock Notification
		
		SetReadMode();
		M8(MCU_DFLCTL_ADDR) = DFLCTL_DFLEN_ENA;
		// Wait for tDSTOP(Wait 250ns or more)
		Wait_us(1);
		
    return ilgl_chk(); // Check for command locked and illegal status	
}

/**************************************************************************//**
* @details   De-Initialize Flash Programming Functions
* @param[in] fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int UnInit (unsigned long fnc)
{
	
    M16(MCU_FPCKAR_ADDR) = FPCKAR_KEY | (uword_t)s_udwClockUserVal[FPCKAR];
		SetReadMode();
	
		/** Restore clock and power control mode to the state before flash rewriting */
    if (restore_clock_and_mode())
    {
        return (RES_ERROR); // Failed
    }

    return (RES_OK); // OK
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
    udword_t udw_addr, dataIndex = 0;
    ubyte_t uby_ilgl_chk, byteDataInBlock;
	
		if (adr < MCU_CONFAREA_CMPADDR)
		{
				/** Configuration area not support BlankCheck */
				byteDataInBlock = 8;
		} else if (adr < MCU_FENTRYD_CMPADDR)
		{
				// This is Configuration Area
				return (RES_OK); // Return without error
		} else
		{
				// This is Data flash
				byteDataInBlock = 1;
		}
		
		pe_mode_entry(adr, &udw_addr);
		do{
					// This is Code Flash and Data flash area
					// Perform blank check by FACI command
					M8(MCU_FBCCNT_ADDR) = FBCCNT_BCDIR_INCRE; 
					M32(MCU_FSADDR_ADDR) = udw_addr;
					M32(MCU_FEADDR_ADDR) = udw_addr + byteDataInBlock - 1;
					M8(MCU_FACI_CMD_AREA_ADDR) = FACI_READ_LB_CMD;
					M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD;
					while (!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
					{
							; // Waiting for command completion
					}

					if (M8(MCU_FBCSTAT_ADDR) & FBCSTAT_FULL)
					{
							/** The target area has been programmed */
							return (RES_ERROR); // Failed
					}

					/** Check for command locked and illegal status */
					uby_ilgl_chk = ilgl_chk();
					if (uby_ilgl_chk)
					{
							return (RES_ERROR); // Failed
					}
					
					dataIndex += byteDataInBlock;
		} while (dataIndex < sz);
		
    return (RES_OK); // Memory is blank
}

/**************************************************************************//**
* @details   Erase complete Flash Memory
* @param     None
* @retval    0 - OK,  1 - Failed
******************************************************************************/
int EraseChip (void)
{
		// Temporary variables
		ubyte_t byteResult = RES_OK;
		
		#ifdef RA4C1_CONF
	  /** Configuration area not support EraseChip */
    return (RES_OK); // OK
		#endif
	
		#if (defined CODE_512K_dual) || (defined CODE_256K_dual) || (defined RA4C1_512K_dual_lower) || (defined RA4C1_512K_dual_upper) || (defined RA4C1_256K_dual_lower) || (defined RA4C1_256K_dual_upper)
		// Clear Code flash
		byteResult |= EraseChipArea(CODEF_LOWER_START_ADDR, CODEF_SIZE);
		byteResult |= EraseChipArea(CODEF_UPPER_START_ADDR, CODEF_SIZE);
		#else // Linear Mode
		// Clear Code flash
		byteResult |= EraseChipArea(CODEF_START_ADDR, CODEF_SIZE);
		#endif
		// Clear Data flash
		byteResult |= EraseChipArea(DATAF_START_ADDR, DATAF_SIZE);
		
		return (byteResult); // Return status
}

/*
 *  Erase whole data of specified chip area
 *    Parameter:      adr: Start address of area
 *                    sz:  Size
 *    Return Value:   0 - OK,  1 - Failed
 */
static int EraseChipArea(unsigned long adr, unsigned long sz)
{
		// Temporary variables
		udword_t udw_addr;
    ubyte_t uby_ilgl_chk;
		
		// Enter PE mode
		pe_mode_entry(adr, &udw_addr);
		
		// Set Start Address 
		M32(MCU_FSADDR_ADDR) = udw_addr;
		
		// Set End Address
		M32(MCU_FEADDR_ADDR) = udw_addr + sz - 1;
		
		// Implement Chip Erase FACI command
		M8(MCU_FACI_CMD_AREA_ADDR) = FACI_CHIP_ERASE_CMD;
		M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD;
		
		while (!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
		{
				; // Waiting for command completion
		}
	
		if (M32(MCU_FSTATR_ADDR) & FSTATR_ERSERR)
    {
        /** Erase error */
        return (RES_ERROR); // Failed
    }
		
		 /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if (uby_ilgl_chk)
    {
        return (RES_ERROR); // Failed
    }

    return (RES_OK); // OK
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
    if (adr < MCU_CONFAREA_CMPADDR)
    {
        ; // Execute EraseSector command if Code Flash
    } else if (adr < MCU_FENTRYD_CMPADDR)
    {
        /** Configuration area not support EraseSector */
        return (RES_OK); // OK
    } else
    {
        ; // Execute EraseSector command if Data Flash
    }

    pe_mode_entry(adr, &udw_addr); // Transition to P/E mode
    M32(MCU_FSADDR_ADDR) = udw_addr; // Set Erase Sector address
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_ERASE1_CMD; // Execution of erase command
    M8(MCU_FACI_CMD_AREA_ADDR) = FACI_END_CMD; // Execution of termination command

    while (!(M32(MCU_FSTATR_ADDR) & FSTATR_FRDY))
    {
        ; // Waiting for command completion
    }

    if (M32(MCU_FSTATR_ADDR) & FSTATR_ERSERR)
    {
        /** Erase error */
        return (RES_ERROR); // Failed
    }

    /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if (uby_ilgl_chk)
    {
        return (RES_ERROR); // Failed
    }

    return (RES_OK); // OK
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

		pe_mode_entry(adr,&udw_adr_tmp); // Check for command locked and illegal status
		
		while (sz)
    {	
        /** Check Flash Write Mode */
        if (adr < MCU_CONFAREA_CMPADDR)
        {
            uby_area_flg = 1; // Code Flash
            if (sz < 8)
            {
                for (i = sz; i < 8; i++)
                {
                    buf[i] = 0xFF;
                }
                    sz = 8;
            }
        } else if (adr < MCU_FENTRYD_CMPADDR)
        {
            uby_area_flg = 2; // Configuration area
            if (sz < 16)
            {
                for (i = sz; i < 16; i++)
                {
                    buf[i] = 0xFF;
                }
                sz = 16;
            }
        } else
        {
            uby_area_flg = 0; // Data Flash
            if (sz < 1)
            {
                for (i = sz; i < 1; i++)
                {
                    buf[i] = 0xFF;
                }
                sz = 1;
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
                uw_wrt_byte = FACI_PROG_D_NUM; // Data size to write with one command
            }
        }
				
				for (i = 0; i < uw_wrt_cnt; i++)
				{
						if(uby_area_flg == 0)
						{
								M16(MCU_FACI_CMD_AREA_ADDR) = (uword_t) buf[uw_start_data];
								uw_start_data +=1;
						} else 
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
            return (RES_ERROR); // Failed
        }
    }

    /** Check for command locked and illegal status */
    uby_ilgl_chk = ilgl_chk();
    if (uby_ilgl_chk)
    {
        return (RES_ERROR); // Failed
    }

    return (RES_OK); // OK
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
    ubyte_t ub_rd_buf[128];
		unsigned long adr_tmp;
    uword_t i, j ,k, m;
		adr_tmp = adr;
    m = 0;
	
		SetReadMode();

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
                return (adr + m); // Address of Memory is not blank	
            }
            m++;
        }	
				adr_tmp = adr + m;
    }
    return (adr + sz); // Verify is success
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
		udword_t dwWriteableMaxClock;

    udw_sckdivcr = M32(MCU_SCKDIVCR_ADDR);

    /** Calculate FCLK */
    switch (s_udwClockUserVal[SCKSCR])
    {
        case SCKSCR_CKSEL_PLL:
            if (s_udwClockUserVal[PLLCCR] & PLLCCR_PLSRCSEL_HOCO)
            { // Input clock source is HOCO
                if (OFS1_HOCOFRQ0_24MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 24000000;
                } else if (OFS1_HOCOFRQ0_32MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 32000000;
                } else if (OFS1_HOCOFRQ0_40MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 40000000;
								} else if (OFS1_HOCOFRQ0_48MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 48000000;
                } else if (OFS1_HOCOFRQ0_64MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
										udw_clk_tmp = 64000000;
								} else if (OFS1_HOCOFRQ0_80MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 80000000;
                } else
                {
                    return (RES_ERROR); // Failed  - This case cannot happen
                }
            } else
            {
                udw_clk_tmp = *udwp_clk; // Input clock source is External clock
            }
						
						if ((s_udwClockUserVal[PLLCCR] & PLLCCR_PLIDIV_MASK) == 0)
						{	
								uw_plli_div = 1;
						} else if ((s_udwClockUserVal[PLLCCR] & PLLCCR_PLIDIV_MASK) == 1) 
						{
								uw_plli_div = 4;
						} else 
						{
								uw_plli_div = 6;
						}
						
            uw_bai = (((s_udwClockUserVal[PLLCCR] & PLLCCR_MUL) >> 8) + 1) / 2; // PLL frequency multiplication factor
            udw_clk_tmp = (udw_clk_tmp / (udword_t)uw_plli_div) * (udword_t)uw_bai; // Frequency output as PLL
        break;
        case SCKSCR_CKSEL_MOSC:
            udw_clk_tmp = *udwp_clk; // Input clock source is External clock
        break;
        case SCKSCR_CKSEL_MOCO:
            udw_clk_tmp = 8000000; // MOCO frequency is 8MHz
        break;
        case SCKSCR_CKSEL_HOCO:
            if (OFS1_HOCOFRQ0_24MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 24000000;
                } else if (OFS1_HOCOFRQ0_32MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 32000000;
                } else if (OFS1_HOCOFRQ0_40MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 40000000;
								} else if (OFS1_HOCOFRQ0_48MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 48000000;
                } else if (OFS1_HOCOFRQ0_64MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
										udw_clk_tmp = 64000000;
								} else if (OFS1_HOCOFRQ0_80MHZ == (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK))
                {
                    udw_clk_tmp = 80000000;
                } else
                {
                    return (RES_ERROR); // Failed  - This case cannot happen
                }
        break;
        case SCKSCR_CKSEL_LOCO: // Failed because LOCO frequency is below 4MHz
						udw_clk_tmp = 32768;
				break;
        case SCKSCR_CKSEL_SOSC: // Failed because SOSC frequency is below 4MHz
						udw_clk_tmp = 32768;
				break;
        default:
            return (RES_ERROR); // Failed - This case cannot happen 
    }

    udw_sckdivcr = (udw_sckdivcr & SCKDIVCR_FCK_MASK) >> 28;
    udw_sckdivcr_val = 1 << udw_sckdivcr; // FCLK division value
    *udwp_clk = udw_clk_tmp / udw_sckdivcr_val; // Calculate FCLK frequency
		
    udw_sckdivcr = M32(MCU_SCKDIVCR_ADDR);
    udw_sckdivcr = (udw_sckdivcr & SCKDIVCR_ICK_MASK) >> 24;
    udw_sckdivcr_val = 1 << udw_sckdivcr; // ICLK division value
    dwICLKValue = udw_clk_tmp / udw_sckdivcr_val; // Calculate ICLK frequency

    /** Failed if power control mode is Subosc-speed mode */
    if ((s_udwClockUserVal[SOPCCR] & SOPCCR_SOPCM) != SOPCCR_SOPCM_OTH)
    {
        return (RES_ERROR); // Failed
    }

    /** Failed if power control mode is Low-speed mode */
    if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCM) != OPCCR_OPCM_HIGH)
    {
        return (RES_ERROR); // Failed
    }
		
		// Update the flash rewrite upper limit frequency for each power control mode
		if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH)
		{
				dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH;
		} else if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
		{
				dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_MID;
		} else
		{
				dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_LOW;
		}
		
		// Return Failure if current ICLK clock speed out of accepted range
		if (((dwICLKValue) < FLASH_WRITEABLE_MIN_CLOCK_HZ) || ((dwICLKValue) > dwWriteableMaxClock))
		{
				return (RES_ERROR); // Finished with Errors
		}

    /** Failed because outside the allowable frequency(4-50MHz) of FCLK */
    if (((*udwp_clk) < 4000000) || (50000000 < (*udwp_clk)))
    {
        return (RES_ERROR); // Failed
    }

    if ((s_udwClockUserVal[FMEPROT] & FMEPROT_CEPROT) != FMEPROT_NOTPROTECT_MODE)
    {
         M16(MCU_FMEPROT_ADDR) = FMEPROT_KEY | FMEPROT_NOTPROTECT_MODE;
    }

    return (RES_OK); // OK
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
		
		if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH){
				/** If OFS1 is in the initial state, set the clock source to MOCO */
				if ((s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK) == OFS1_HOCOFRQ0_MASK)
				{
						/** If the clock source is MOCO, set the clock source to MOCO */
						if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
						{
								/** If MOCO oscillation is stopped, oscillate it */
								if (s_udwClockUserVal[MOCOCR] == MOCOCR_STOP)
								{
										M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
										Wait_us(1);// Wait for oscillation stabilization of MOCO
								}
								M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;// Set the clock source to MOCO
						}
						/** Change ICLK and FCLK to divide by 1 */
						//M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_1DEV_MASK & SCKDIVCR_ICLK_1DEV_MASK);
						*udwp_clk = 8000000; // FCLK frequency is 8MHz
				} else
				{/** If OFS1 is not in the initial state, set the clock source to HOCO */
						if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_HOCO)
						{
								/** If HOCO oscillation is stopped, oscillate it */
								if (s_udwClockUserVal[HOCOCR] == HOCOCR_STOP)
								{
										M8(MCU_HOCOCR_ADDR) = HOCOCR_ON;
										while (!(M8(MCU_OSCSF_ADDR) & OSCSF_HOCOSF))
										{
												Wait_us(65);// Wait for oscillation stabilization of HOCO
										}								
								}
								M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_HOCO;// Set the clock source to HOCO
						}

						switch (s_udwClockUserVal[OFS1] & OFS1_HOCOFRQ0_MASK)
						{
								case OFS1_HOCOFRQ0_24MHZ:
										*udwp_clk = 24000000; // FCLK frequency is 16MHz
								break;
								case OFS1_HOCOFRQ0_32MHZ:
										*udwp_clk = 32000000; // FCLK frequency is 18MHz
								break;
								case OFS1_HOCOFRQ0_40MHZ:
										*udwp_clk = 40000000; // FCLK frequency is 20MHz
								break;
								case OFS1_HOCOFRQ0_48MHZ:
										*udwp_clk = 48000000; // FCLK frequency is 20MHz
								break;
								case OFS1_HOCOFRQ0_64MHZ:
										*udwp_clk = 64000000; // FCLK frequency is 20MHz
								break;
								case OFS1_HOCOFRQ0_80MHZ:
										*udwp_clk = 80000000; // FCLK frequency is 20MHz
								break;
								default:
										return (RES_ERROR); // Failed
						}
				}
				
				if (*udwp_clk == 64000000)
				{
						// Change clock division to 2
						M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_DEV_MASK & SCKDIVCR_ICLK_DEV_MASK);
						M32(MCU_SCKDIVCR_ADDR) |= (SCKDIVCR_FCLK_2DEV_MASK | SCKDIVCR_ICLK_2DEV_MASK);
						*udwp_clk = 32000000;
				} else if (*udwp_clk == 80000000)
				{
						// Change clock division to 2
						M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_DEV_MASK & SCKDIVCR_ICLK_DEV_MASK);
						M32(MCU_SCKDIVCR_ADDR) |= (SCKDIVCR_FCLK_2DEV_MASK | SCKDIVCR_ICLK_2DEV_MASK);
						*udwp_clk = 340000000;
				} else
				{
						// Change clock division to 1
						M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_DEV_MASK & SCKDIVCR_ICLK_DEV_MASK);
				}
		} else {
				// Change clock source to MOCO
				if (s_udwClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
				{
						// If MOCO is stoped, start it
						if (s_udwClockUserVal[MOCOCR] == MOCOCR_STOP)
						{
								M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
								// Wait until MOCO is stable
								Wait_us(1);		
						}
						// Select clock as MOCO
						M8(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;
				}
				
				if ((s_udwClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
				{
						// Change clock division to 2
						M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_DEV_MASK & SCKDIVCR_ICLK_DEV_MASK);
						M32(MCU_SCKDIVCR_ADDR) |= (SCKDIVCR_FCLK_2DEV_MASK | SCKDIVCR_ICLK_2DEV_MASK);
						*udwp_clk = 4000000;
				} else // Low-speed mode
				{
						// Change clock division to 8
						M32(MCU_SCKDIVCR_ADDR) &= (SCKDIVCR_FCLK_DEV_MASK & SCKDIVCR_ICLK_DEV_MASK);
						M32(MCU_SCKDIVCR_ADDR) |= (SCKDIVCR_FCLK_8DEV_MASK | SCKDIVCR_ICLK_8DEV_MASK);
						*udwp_clk = 1000000;
				}	
		}

    if ((s_udwClockUserVal[FMEPROT] & FMEPROT_CEPROT) != FMEPROT_NOTPROTECT_MODE)
    {
         M16(MCU_FMEPROT_ADDR) = FMEPROT_KEY | FMEPROT_NOTPROTECT_MODE;
    }
		
    return (RES_OK); // OK
}

/*
 *  Save Mode & Clock before rewriting flash
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static ubyte_t saveModeAndClock(void)
{
    s_udwClockUserVal[PRCR] = (udword_t)M16(MCU_PRCR_ADDR);
    s_udwClockUserVal[SCKSCR] = (udword_t)M8(MCU_SCKSCR_ADDR);
    s_udwClockUserVal[SCKDIVCR] = M32(MCU_SCKDIVCR_ADDR);
    s_udwClockUserVal[PLLCCR] = (udword_t)M16(MCU_PLLCCR_ADDR);
    s_udwClockUserVal[HOCOCR] = (udword_t)M8(MCU_HOCOCR_ADDR);
    s_udwClockUserVal[MOCOCR] = (udword_t)M8(MCU_MOCOCR_ADDR);
    s_udwClockUserVal[SOPCCR] = (udword_t)M8(MCU_SOPCCR_ADDR);
    s_udwClockUserVal[OPCCR] = (udword_t)M8(MCU_OPCCR_ADDR);
    s_udwClockUserVal[FPCKAR] = (udword_t)M16(MCU_FPCKAR_ADDR);
    s_udwClockUserVal[OFS1] = M32(MCU_OFS1_ADDR);
		s_udwClockUserVal[DFLCTL] = (udword_t)M8(MCU_DFLCTL_ADDR); 
    s_udwClockUserVal[FWEPROR] = (udword_t)M8(MCU_FWEPROR_ADDR);
    s_udwClockUserVal[FMEPROT] = (udword_t)M16(MCU_FMEPROT_ADDR);

    return (RES_OK); // Finished without Errors
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
    M16(MCU_PRCR_ADDR) = KEYCODE_PRCR + PRCR_PRC1 + PRCR_PRC0;
    M32(MCU_SCKDIVCR_ADDR) = s_udwClockUserVal[SCKDIVCR]; // Restore system clock division
    M16(MCU_SCKSCR_ADDR) = (uword_t)s_udwClockUserVal[SCKSCR]; // Restore system clock source
    M8(MCU_HOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[HOCOCR]; // Restore the oscillation state of HOCO
    M8(MCU_MOCOCR_ADDR) = (ubyte_t)s_udwClockUserVal[MOCOCR]; // Restore the oscillation state of MOCO

    /** Restore power control mode */
    M8(MCU_OPCCR_ADDR) = (ubyte_t)s_udwClockUserVal[OPCCR];
    while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
    {
        ; // Wait till transition completed
    }
    M8(MCU_SOPCCR_ADDR) = (ubyte_t)s_udwClockUserVal[SOPCCR];
    while (M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF)
    {
        ; // Wait till transition completed
    }
		
		M8(MCU_DFLCTL_ADDR) = s_udwClockUserVal[DFLCTL];
    M8(MCU_FWEPROR_ADDR) = (ubyte_t)s_udwClockUserVal[FWEPROR]; // Restore flash P/E protect state
    M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | (uword_t)s_udwClockUserVal[PRCR]; // Restore the protection status
    M16(MCU_FMEPROT_ADDR) = FMEPROT_KEY | (uword_t)s_udwClockUserVal[FMEPROT];
	
    return (RES_OK); // OK
}

/**************************************************************************//**
* @details    Transition to P/E mode
* @param[in]  udw_addr: Write start address
* @param[out] udwp_adr_ret: Write start address after mask
* @retval    0 - OK,  1 - Failed
******************************************************************************/
static udword_t pe_mode_entry(udword_t udw_addr, udword_t *udwp_adr_ret)
{
		SetReadMode();

    if (udw_addr < MCU_FENTRYD_CMPADDR)
    {
        M16(MCU_FENTRYR_ADDR) = FENTRYR_CODEF_PE; // Transition to Code Flash P/E mode
        *udwp_adr_ret = udw_addr & CODEF_MASK_ADDR; // Set the write start address after mask
        while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYC) // Wait until switching to Code Flash P/E mode
        {
            ;
        }
    } else
    {
        M16(MCU_FENTRYR_ADDR) = FENTRYR_DATAF_PE; // Transition to Data Flash P/E mode
        *udwp_adr_ret = udw_addr & DATAF_MASK_ADDR; // Set the write start address after mask
        while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYD) // Wait until switching to Data Flash P/E mode
        {
            ;
        }
    }

    return (RES_OK); // OK
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
    if (ubydata & FASTAT_CMDLK)
    {
        return(RES_ERROR); // Failed
    }

    /** Check for illegal state */
    udwdata = M32(MCU_FSTATR_ADDR);
    if (udwdata & FSTATR_ILGLERR)
    {
        return(RES_ERROR); // Failed
    }

    return (RES_OK); // OK
}

/*
 *  Wait processing in us units
 *    Parameter:      us: wait time
 *    Return Value:   none
 *	  Note:			  Please note that the weight processing is less accurate.
 */
#pragma GCC push_options
#pragma GCC optimize ("O0")

static void Wait_us(udword_t us)
{
    udword_t dwCount;
    udword_t dwTotalNsPerCycle;

    dwTotalNsPerCycle = 1000000000 / dwICLKValue; // Calculate ns per cycle
    if (dwTotalNsPerCycle < (1000000000 / MAX_OPERATING_FREQUENCY)) // T = dwTotalNsPerCycle = 10^9 / f < 12.5
    {
        // Fail-safe when exceeding 80Mhz
        dwTotalNsPerCycle = 13;	
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

/*
 *  READ Mode entry
 *    Parameter:      None
 *    Return Value:   None
 */
static void SetReadMode(void)
{
		M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE; // Transition to read mode
    while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_READ) // Wait until switching to read mode
    {
        ;
    }

		return;
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

    for (i = 0; i < udw_sz ; i++)
    {
        ubp_buf[i] = M8(udw_addr + i); // Store read data byte by byte
    }
}