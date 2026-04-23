/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for RA4W1
 * @version  V1.0.3
 * @date     27 Mar 2024
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
 * Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 31.08.2020 1.0.0    First Release
 *         : 28.03.2022 1.0.1    Implemented source code improvements
 *     		 : 16.02.2024	1.0.2		 Increase Programming Page Size
 *         : 27.03.2024 1.0.3    Modify Programming Page Size (Code/Data 2KB/1KB)
 *********************************************************************************************************************/

#include "../FlashOS.h"        // FlashOS Structures

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
#define MCU_SCKDIVCR_ADDR               (MCU_SYSTEM_BASE+0x020)
    #define SCKDIVCR_FCLK_1DEV		   		(0x00000000)
    #define SCKDIVCR_FCLK_2DEV     			(0x11000101)
#define MCU_SCKSCR_ADDR                 (MCU_SYSTEM_BASE+0x026)
    #define SCKSCR_CKSEL_HOCO           (0x00)
    #define SCKSCR_CKSEL_MOCO           (0x01)
    #define SCKSCR_CKSEL_LOCO           (0x02)
    #define SCKSCR_CKSEL_MOSC           (0x03)
    #define SCKSCR_CKSEL_SOSC           (0x04)
    #define SCKSCR_CKSEL_PLL            (0x05)
#define MCU_PLLCCR2_ADDR                (MCU_SYSTEM_BASE+0x02B)
    #define PLLCCR2_MUL                 (0x1F)
		#define PLLCCR2_PLODIV_MASK         (0xC0)
    #define PLLCCR2_PLODIV_1DEV         (0x00)
    #define PLLCCR2_PLODIV_2DEV         (0x40)
    #define PLLCCR2_PLODIV_3DEV         (0x80)
#define MCU_HOCOCR_ADDR                 (MCU_SYSTEM_BASE+0x036)
    #define HOCOCR_STOP                 (0x01)
    #define HOCOCR_ON                   (0x00)
#define MCU_MOCOCR_ADDR                 (MCU_SYSTEM_BASE+0x038)
    #define MOCOCR_STOP                 (0x01)
    #define MOCOCR_ON                   (0x00)
#define MCU_OSCSF_ADDR                  (MCU_SYSTEM_BASE+0x03C)
    #define OSCSF_HOCOSF                (0x01)
    #define OSCSF_MOSCSF                (0x08)
    #define OSCSF_PLLSF                 (0x20)
#define MCU_PRCR_ADDR                   (MCU_SYSTEM_BASE+0x3FE)
    #define KEYCODE_PRCR                (0xA500)
    #define PRCR_PRC1                   (0x0002)
    #define PRCR_PRC0                   (0x0001)
    #define PRCR_PRC3                   (0x0008)
#define MCU_OPCCR_ADDR                  (MCU_SYSTEM_BASE+0x0A0)
    #define OPCCR_OPCM									(0x03)
    #define OPCCR_OPCM_HIGH							(0x00)
    #define OPCCR_OPCM_MID							(0x01)
		#define OPCCR_OPCM_LOW_VOL					(0x02)
		#define OPCCR_OPCM_LOW							(0x03)
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
    #define OFS1_HOCOFRQ1_MASK          (0x00007000)
    #define OFS1_HOCOFRQ1_24MHZ         (0x00000000)
    #define OFS1_HOCOFRQ1_32MHZ         (0x00002000)
    #define OFS1_HOCOFRQ1_48MHZ         (0x00004000)
		#define OFS1_HOCOFRQ1_64MHZ         (0x00005000)

/*
   Flash related registers adrress and bit definition
*/
#define MCU_FLASH_BASE									(0x407EC000)
#define MCU_FISR_ADDR										(MCU_FLASH_BASE + 0x1D8)
#define MCU_FRESETR_ADDR								(MCU_FLASH_BASE + 0x124)
		#define FRESETR_FRESET_ASSRT				(0x01)
		#define FRESETR_FRESET_NEGATE				(0x00)
#define MCU_FENTRYR_ADDR								(MCU_FLASH_BASE + 0x3FB2)
    #define FENTRYR_READ_MODE						(0xAA00)
    #define FENTRYR_DATAF_PE						(0xAA80)
    #define FENTRYR_CODEF_PE						(0xAA01)
    #define FENTRYR_CHK_FENTRYD					(0x0080)
    #define FENTRYR_CHK_FENTRYC					(0x0001)
    #define FENTRYR_CHK_READ						(0x0000)
#define MCU_DFLCTL_ADDR									(MCU_FLASH_BASE + 0x090)
		#define DFLCTL_DFLEN_ENA						(0x01)
		#define DFLCTL_DFLEN_DIS						(0x00)
#define MCU_FPR_ADDR		                (MCU_FLASH_BASE + 0x180)
		#define FPR_UNLOCK		              (0xA5)
#define MCU_FPMCR_ADDR									(MCU_FLASH_BASE + 0x100)
		#define FPMCR_FMS_FMS0							(0x02)
		#define FPMCR_FMS_FMS1							(0x10)
		#define FPMCR_FMS_FMS2							(0x80)
		#define FPMCR_FMS_FMS								(0x92)
		#define FPMCR_FMS_READMODE					(0x00)
		#define FPMCR_FMS_DISCHARGE1				(0x12)
		#define FPMCR_FMS_DISCHARGE2				(0x92)
		#define FPMCR_FMS_CODEPE						(0x82)
		#define FPMCR_FMS_DATAPE						(0x10)
		#define FPMCR_FMS_RPDIS							(0x08)
		#define FPMCR_FMS_RPDIS_DIS					(0x08)
		#define FPMCR_FMS_RPDIS_ENA					(0x00)
		#define FPMCR_FMS_LVPE							(0x40)
#define MCU_FASR_ADDR										(MCU_FLASH_BASE + 0x104)
		#define FASR_EXS_USR								(0x00)
		#define FASR_EXS_EXTRA							(0x01)
#define MCU_FSTATR2_ADDR								(MCU_FLASH_BASE + 0x1F0)
		#define FSTATR2_ERERR_MASK         	(0x1)
		#define FSTATR2_PRGERR_MASK        	(0x2)
		#define FSTATR2_PRGERR01_MASK     	(0x4)
		#define FSTATR2_BCERR_MASK 	      	(0x8)
		#define FSTATR2_ILGLERR_MASK     	  (0x10)
		#define FSTATR2_EILGLERR_MASK   	  (0x20)
#define MCU_FSTATR01_ADDR								(MCU_FLASH_BASE + 0x13C)
		#define FSTATR01_ERERR1							(0x0001)
		#define FSTATR01_PRGERR1						(0x0002)
		#define FSTATR01_BCERR1							(0x0008)
#define MCU_FSARH_ADDR									(MCU_FLASH_BASE + 0x110)
#define MCU_FSARL_ADDR									(MCU_FLASH_BASE + 0x108)
#define MCU_FEARH_ADDR									(MCU_FLASH_BASE + 0x120)
#define MCU_FEARL_ADDR									(MCU_FLASH_BASE + 0x118)
#define MCU_FWBL0_ADDR									(MCU_FLASH_BASE + 0x130)
#define MCU_FWBH0_ADDR									(MCU_FLASH_BASE + 0x138)
#define MCU_FWBL1_ADDR									(MCU_FLASH_BASE + 0x140)
#define MCU_FWBH1_ADDR									(MCU_FLASH_BASE + 0x144)
#define MCU_FRBL1_ADDR	               	(MCU_FLASH_BASE + 0x148)
#define MCU_FRBH1_ADDR	                (MCU_FLASH_BASE + 0x14C)
#define MCU_FRBL0_ADDR	               	(MCU_FLASH_BASE + 0x188)
#define MCU_FRBH0_ADDR	               	(MCU_FLASH_BASE + 0x190)
#define MCU_FCR_ADDR										(MCU_FLASH_BASE + 0x114)
		#define FCR_CMD_MASK								(0x0F)
		#define FCR_DRC_MASK								(0x10)
		#define FCR_DCLR_MASK								(0x20)
		#define FCR_STOP_MASK								(0x40)
		#define FCR_OPST_MASK								(0x80)
#define MCU_FSTATR1_ADDR								(MCU_FLASH_BASE + 0x12C)
		#define FSTATR1_DRRDY_MASK					(0x02)
		#define FSTATR1_FRDY_MASK						(0x40)
		#define FSTATR1_EXRDY_MASK					(0x80)
#define MCU_FEXCR_ADDR									(MCU_FLASH_BASE + 0x1DC)
		#define FEXCR_CMD										(0x07)
		#define FEXCR_CMD_STARTUP_AREA_SELECT		(0x01)
		#define FEXCR_CMD_ACCESS_WINDOW			(0x02)
		#define FEXCR_CMD_OCDID1						(0x03)
		#define FEXCR_CMD_OCDID2						(0x04)
		#define FEXCR_CMD_OCDID3						(0x05)
		#define FEXCR_CMD_OCDID4						(0x06)
		#define FEXCR_CMD_CLEAR							(0x07)
		#define FEXCR_OPST_START						(0x80)
		#define FEXCR_OPST_STOP							(0x00)


/*
   Other parameters
*/
#define RES_ERROR												(1)
#define RES_OK													(0)
#define FLASH_WRITEABLE_MIN_CLOCK_HZ		(1000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH		(32000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_MID		(8000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_LOWV		(4000000)
#define FENTRYR_CODEF_START_ADDR				(0x00000000)
#define FENTRYR_DATAF_START_ADDR				(0x40100000)
#define DATAF_MASK_ADDR									(0x000FFFFF)
#define CODEF_PE_MASK_ADDR							(0x000FFFFF)
#define MCU_CODEF_BASEADDR							(0x00000000)
#define MCU_DATAF_BASEADDR							(0x40100000)
#define CONFIG_START										(0x01010000)
#define MCU_CODEF_SIZE									(0x00040000)
#define MCU_DATAF_SIZE									(0x00002000)
#define SECTOR_SIZE											(0x000800)
#define PROGRAMMING_PAGE_SIZE						(128)
#define BLANK_VALUE											(0xFF)
#define TARGET_CONFIG										(0)
#define TARGET_DATAF										(1)
#define TARGET_CODEF										(2)

#define MCU_AWSC_ADDR										(CONFIG_START + 0x08)
#define MCU_AWS_ADDR										(CONFIG_START + 0x10)
#define MCU_OSIS1_ADDR									(CONFIG_START + 0x18)
#define MCU_OSIS2_ADDR									(CONFIG_START + 0x20)
#define MCU_OSIS3_ADDR									(CONFIG_START + 0x28)
#define MCU_OSIS4_ADDR									(CONFIG_START + 0x30)

#ifdef RA4W1_512K
    #undef MCU_CODEF_SIZE								// Undef macro first
		#undef PROGRAMMING_PAGE_SIZE				// Undef macro first
		#undef SECTOR_SIZE									// Undef macro first
    #define MCU_CODEF_SIZE							(0x00080000)
		#define SECTOR_SIZE									(0x000800)
		#define PROGRAMMING_PAGE_SIZE				(2048)
#endif
#ifdef RA4W1_DATA
    #undef DATAF_SIZE										// Undef macro first
    #undef SECTOR_SIZE									// Undef macro first
		#undef PROGRAMMING_PAGE_SIZE				// Undef macro first
		#define DATAF_SIZE									(0x00002000)
    #define SECTOR_SIZE									(0x000400)
		#define PROGRAMMING_PAGE_SIZE				(1024)
#endif
#ifdef RA4W1_CONF
	#undef PROGRAMMING_PAGE_SIZE					// Undef macro first
	#undef SECTOR_SIZE										// Undef macro first
	#define PROGRAMMING_PAGE_SIZE					(4)
	#define SECTOR_SIZE										(0x000004)
#endif

/** Type define */
typedef unsigned char										BYTE;
typedef unsigned short									WORD;
typedef unsigned long										DWORD;

/*
   Index of internal data buffer
*/
typedef enum REG_DATA {
    PRCR = 0,
    SCKSCR,
    SCKDIVCR,
    PLLCCR2,
    HOCOCR,
    MOCOCR,
    SOPCCR,
    OPCCR,
    OFS1,
	  DFLCTL,
	  FASR,
	  FISR,
    REG_MAX,
} REG_DATA;
	
/*
   Register value storage
*/
static DWORD    wClockUserVal[REG_MAX];
/*
   Current ICLK frequency
*/
static DWORD dwICLKValue;

/*
   Internal function prototype
*/
static DWORD checkClockAndMode(DWORD *clk);
static DWORD changeClockAndMode(DWORD *clk);
static BYTE saveModeAndClock(void);
static BYTE restoreModeAndClock(void);
static BYTE enterPEMode(DWORD adr, DWORD *adrRet);
static BYTE checkInternalError(void);
static void Wait_us(DWORD us);
static DWORD extractBits(DWORD dwValue, unsigned int startBit, unsigned int bitCount);
static int EraseChipArea(unsigned long adr, unsigned long sz);
static void SpecificSequence(unsigned char ucFpmcr);
static void SetReadMode(void);
static void readpage(unsigned long adr, unsigned long sz, unsigned char *buf);

 /*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify) - Ignored
 *    Return Value:   0 - OK,  1 - Failed
 */
int Init (unsigned long adr, unsigned long clk, unsigned long fnc)
{
	DWORD byteVal;
  BYTE byteClk;
  DWORD dwDummyAdr;
	
	dwICLKValue = 0;
	// Save current settings
	saveModeAndClock();
	
  byteVal = checkClockAndMode(&clk); // Check if flash rewritable clock and power control mode
#ifdef CHG_CLK_AND_MOD_ENA
  /** If the clock and mode cannot rewrite the flash, switch to a possible state */
  if(byteVal)
  {
      byteVal = changeClockAndMode(&clk);
  }
#endif

  if(byteVal)
  {
    /** Failure if the flash cannot be rewritten */
    return (RES_ERROR); // Failed
  }
	// Save ICLK frequency for wait processing
	dwICLKValue = clk;
	// Convert clock from Hz to MHz unit
	byteClk = clk / 1000000;
	// Round up the first decimal place in MHz
	if (clk % 1000000)
	{
		byteClk += 1;
	}
	// Enter PE Mode
	enterPEMode(adr, &dwDummyAdr);
	M8(MCU_FISR_ADDR) = (M8(MCU_FISR_ADDR) | (byteClk - 1));
	// Return to READ Mode
	M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;
	
	return checkInternalError(); // Check internal errors
}

/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */
int UnInit (unsigned long fnc)
{
	
	// Restore clock and power control mode to the state before flash rewriting
#ifdef CHG_CLK_AND_MOD_ENA
  if (restoreModeAndClock()) {
    return (RES_ERROR); // Finished with Errors
  }
	M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | wClockUserVal[PRCR];
#endif

  // Return to READ Mode
	SetReadMode();
		
  return (RES_OK); // Finished without Errors
}

/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */
int EraseChip (void) 
{
	// Temporary variables
	BYTE byteIsFailed = RES_OK;
	
	// Just delete Code flash area or Data flash area base on user's macro
#ifdef RA4W1_512K
	// Clear Code flash
	byteIsFailed |= EraseChipArea(MCU_CODEF_BASEADDR, MCU_CODEF_SIZE);
#endif
#ifdef RA4W1_DATA
	// Clear Data flash
	byteIsFailed |= EraseChipArea(MCU_DATAF_BASEADDR, MCU_DATAF_SIZE);
#endif
	
  return (byteIsFailed); // Return status
}

/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr)
{
	// Temporary variables
	DWORD dwTargetAddressPE;
	// Detect Area
	if (adr < CONFIG_START)
	{
		// This is Code flash - Support Erase
	} else if (adr < MCU_DATAF_BASEADDR)
	{
		// This is Configuration Area - Not support Erase
		return (RES_OK);
	} else
	{
		// This is Data flash - Support Erase
	}

	// Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);

	// Set Start address
	M16(MCU_FSARL_ADDR) = extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits

	// Set End address
	M16(MCU_FEARL_ADDR) = extractBits(dwTargetAddressPE + SECTOR_SIZE - 1, 0, 16); // 16 lower bits
	M16(MCU_FEARH_ADDR) = extractBits(dwTargetAddressPE + SECTOR_SIZE - 1, 16, 16); // 16 higher bits

	// Write FCR with 84h
	M8(MCU_FCR_ADDR) = 0x84;

	// Wait for Ready flag (FSTATR1.FRDY = 1)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}

	// Write FCR with 00h
	M8(MCU_FCR_ADDR) = 0x00;

	// Wait for Ready flag (FSTATR1.FRDY = 0)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) == FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}

	// Check error - If error, reset then exit
	// No error means: FSTATR2.ILGLERR = 0  & FSTATR2.ERERR = 0
	if (((M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK) == FSTATR2_ILGLERR_MASK) ||
				((M16(MCU_FSTATR2_ADDR) & FSTATR2_ERERR_MASK) == FSTATR2_ERERR_MASK))
	{
		// Sequencer initialization
		M8(MCU_FRESETR_ADDR) = 1;
		M8(MCU_FRESETR_ADDR) = 0;
		// Exit
		return (RES_ERROR); // Finished with Errors
	}

  return (RES_OK); // Finished without Errors
}

/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
	// Temporary variables
	DWORD dwTargetAddressPE;
	BYTE byteTargetArea;
	BYTE byteDataInBlock;
	BYTE byteExCmd;
	int dataIndex;

	// Standardized data
	if (sz != PROGRAMMING_PAGE_SIZE)
	{
		for (dataIndex = sz; dataIndex < PROGRAMMING_PAGE_SIZE; dataIndex++)
		{
			buf[dataIndex] = BLANK_VALUE;
		}
		sz = PROGRAMMING_PAGE_SIZE;
	}
	dataIndex = 0; // Reset - To re-use in below processing

	// Detect area to define the target data in 1 block
	// (32 bits for Code flash - 4 data, 8 bits for Data flash - 1 data)
	// Data flash (as default)
	byteTargetArea = TARGET_DATAF;
	byteDataInBlock = 1;
	if (adr < CONFIG_START)
	{
		// This is Code flash
		// Update parameter for Code flash
		byteTargetArea = TARGET_CODEF;
		byteDataInBlock = 8;
	} else if (adr < MCU_DATAF_BASEADDR)
	{
		// Code flash
		// Update parameter for Code flash
		byteTargetArea = TARGET_CONFIG;
		byteDataInBlock = 4;
	} else
	{
		// This is Data flash
		// Use defult definition
	}

	// Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);

	// Set programming address
	M16(MCU_FSARL_ADDR) = extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits

	// Loop until end of data
	do {
		// Set programming data (Code flash: 4 data, Data flash: 1 data, Config Area: 4 data)
		if (byteTargetArea == TARGET_CODEF)
		{
			M16(MCU_FWBL0_ADDR) = (buf[dataIndex] | (buf[dataIndex + 1] << 8)); // 16 lower bits
			M16(MCU_FWBH0_ADDR) = (buf[dataIndex + 2] | (buf[dataIndex + 3] << 8)); // 16 higher bits
			M16(MCU_FWBL1_ADDR) = (buf[dataIndex + 4] | (buf[dataIndex + 5] << 8)); // 16 lower bits
			M16(MCU_FWBH1_ADDR) = (buf[dataIndex + 6] | (buf[dataIndex + 7] << 8)); // 16 higher bits
		} else if (byteTargetArea == TARGET_DATAF)
		{
			M16(MCU_FWBL0_ADDR) = buf[dataIndex]; // 8 bits
		} else
		{
			// Configuration Area
			M16(MCU_FWBL0_ADDR) = (buf[dataIndex] | (buf[dataIndex + 1] << 8)); // 16 lower bits
			M16(MCU_FWBH0_ADDR) = (buf[dataIndex + 2] | (buf[dataIndex + 3] << 8)); // 16 higher bits
		}

		if (byteTargetArea != TARGET_CONFIG)
		{
		
		// Write FCR with 81h
		M8(MCU_FCR_ADDR) = 0x81;

		// Wait for ready flag (FSTATR1.FRDY = 1)
		while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
		{
			// Do nothing
			;
		}

		// Write FCR with 00h
		M8(MCU_FCR_ADDR) = 0x00;

		// Wait for ready flag (FSTATR1.FRDY = 0)
		while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) == FSTATR1_FRDY_MASK)
		{
			// Do nothing
			;
		}
	} else
	{
		switch(adr + dataIndex)
		{
		case MCU_AWSC_ADDR:
		case MCU_AWS_ADDR:
			byteExCmd = FEXCR_CMD_ACCESS_WINDOW;
			break;
		case MCU_OSIS1_ADDR:
			byteExCmd = FEXCR_CMD_OCDID1;
			break;
		case MCU_OSIS2_ADDR:
			byteExCmd = FEXCR_CMD_OCDID2;
			break;
		case MCU_OSIS3_ADDR:
			byteExCmd = FEXCR_CMD_OCDID3;
			break;
		case MCU_OSIS4_ADDR:
			byteExCmd = FEXCR_CMD_OCDID4;
			break;
		default:
			byteExCmd = 0;
			break;
		}
		if(byteExCmd != 0)
		{
			M8(MCU_FEXCR_ADDR) = byteExCmd | FEXCR_OPST_START;
		
			// Wait for ready flag (FSTATR1.FRDY = 1)
			while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_EXRDY_MASK) != FSTATR1_EXRDY_MASK)
			{
				// Do nothing
				;
			}
			// Write FEXCR with 00h
			M8(MCU_FEXCR_ADDR) = 0x00;

			// Wait for ready flag (FSTATR1.FRDY = 0)
			while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_EXRDY_MASK) == FSTATR1_EXRDY_MASK)
			{
				// Do nothing
				;
			}
		}
	}
		
	// Check error - If error, reset then exit
	// No error means: FSTATR2.ILGLERR = 0  & FSTATR2.PRGERR = 0
		if (((M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK) == FSTATR2_ILGLERR_MASK) ||
				((M16(MCU_FSTATR2_ADDR) & FSTATR2_PRGERR_MASK) == FSTATR2_PRGERR_MASK))
	{
		// Sequencer initialization
		M8(MCU_FRESETR_ADDR) = 1;
		M8(MCU_FRESETR_ADDR) = 0;
		// Break
		return (RES_ERROR); // Finished with Errors
	}
		
	// Update index - Next loop will get correct data in buffer
	dataIndex += byteDataInBlock;
	} while (dataIndex < sz);
	
  return (RES_OK); // Finished without Errors
}

/*
 *  Verify flash data inside specified range
 *    Parameter:      adr:  Start Address
 *                    sz:   Size
 *                    buf:  Data
 *    Return Value:   the sum of (adr+sz) on success. Any other number on failure, and it represents the failed address.
 */
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf)
{
	BYTE byte_rd_buf[128];
	unsigned long adr_tmp;
	WORD i, j ,k, m;
	adr_tmp = adr;
	m = 0;
	
	// Return to READ Mode
	SetReadMode();
	
	for(i = 0; i < sz ; i += 128)
	{
		readpage(adr_tmp, 128, &byte_rd_buf[0]);

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
				if (byte_rd_buf[j] != buf[m])
				{
					return (1); // The written data and the actually written flash contents do not match	
				}
					m++;
			}
			adr_tmp = adr + m;
	}
	return (adr + sz); // Verify is success
}

/*
 *  Blank Check
 *    Parameter:      adr: Address
 *                    sz:  Block Size
 *                    pat: Pattern (skip)
 *    Return Value:   0 - OK,  1 - Failed
 */
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat) {
	// Temporary variables
	DWORD dwTargetAddressPE;
	
	// Detect Area
	if (adr < CONFIG_START)
	{
		// This is Code flash - Support BlankCheck
	} else if (adr < MCU_DATAF_BASEADDR)
	{
		// This is Configuration Area - Not support BlankCheck
		return (RES_OK);
	} else
	{
		// This is Data flash - Support BlankCheck
	}

  // Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);
	
	// Set Start address
	M16(MCU_FSARL_ADDR) = extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits
	
	// Set End address
	M16(MCU_FEARL_ADDR) = extractBits(dwTargetAddressPE + sz - 1, 0, 16); // 16 lower bits
	M16(MCU_FEARH_ADDR) = extractBits(dwTargetAddressPE + sz - 1, 16, 16); // 16 higher bits
	
	// Write FCR with 83h
	M8(MCU_FCR_ADDR) = 0x83;
	
	// Internal BlankCheck exeucting
	// Wait for ready flag (FSTATR1.FRDY = 1)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}
	
	// Write FCR with 00h
	M8(MCU_FCR_ADDR) = 0x00;
	
	// Wait until FSTATR1.FRDY = 0
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) == FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}
	
	// Check error - If error, reset then exit
	// No error means: FSTATR2.ILGLERR = 0  & FSTATR2.BCERR = 0
	if (((M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK) == FSTATR2_ILGLERR_MASK) ||
					((M16(MCU_FSTATR2_ADDR) & FSTATR2_BCERR_MASK) == FSTATR2_BCERR_MASK))
	{
		// Sequencer initialization
		M8(MCU_FRESETR_ADDR) = 1;
		M8(MCU_FRESETR_ADDR) = 0;
		// Exit
		return (RES_ERROR); // Finished with Errors
	}

  return (RES_OK); // Finished without Errors
}


/*
 *  Check Mode & Clock for rewriting flash
 *    Parameter:      clk: Input clock value (Hz)
 *    Return Value:   0 - OK,  1 - Failed
 */
static DWORD checkClockAndMode(DWORD *clk)
{
  DWORD dwordSckdivcr;
  DWORD dwordSckdivcrVal;
  DWORD dwordClk;
  DWORD dwordBai;
  DWORD dwordPlloDiv;
  DWORD dwWriteableMaxClock;
	
	dwordSckdivcr = wClockUserVal[SCKDIVCR];
	
  /** Failed if power control mode is Subosc-speed mode */
  if ((wClockUserVal[SOPCCR] & SOPCCR_SOPCM) != SOPCCR_SOPCM_OTH)
  {
    return (RES_ERROR); // Failed
  }

  /** Failed if power control mode is Low-speed mode */
  if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_LOW)
  {
    return (RES_ERROR); // Failed
  }
		
	/** Calculate FCLK */
  switch (wClockUserVal[SCKSCR])
  {
	  case SCKSCR_CKSEL_PLL:

			dwordClk = *clk; // Input clock source is External clock

      dwordPlloDiv = 1 << ((wClockUserVal[PLLCCR2] & PLLCCR2_PLODIV_MASK) >> 6); // PLL input frequency division
      dwordBai = (wClockUserVal[PLLCCR2] & PLLCCR2_MUL) + 1; // PLL frequency multiplication factor
      dwordClk = (dwordClk / (DWORD)dwordPlloDiv) * (DWORD)dwordBai; // Frequency output as PLL
      break;
    case SCKSCR_CKSEL_MOSC:
      dwordClk = *clk; // Input clock source is External clock
      break;
    case SCKSCR_CKSEL_MOCO:
      dwordClk = 8000000; // MOCO frequency is 8MHz
      break;
    case SCKSCR_CKSEL_HOCO:
      if(OFS1_HOCOFRQ1_24MHZ == (wClockUserVal[OFS1] & OFS1_HOCOFRQ1_MASK))
      {
				dwordClk = 24000000; // HOCO frequency is 24MHz
      } else if(OFS1_HOCOFRQ1_32MHZ == (wClockUserVal[OFS1] & OFS1_HOCOFRQ1_MASK))
      {
        dwordClk = 32000000; // HOCO frequency is 32MHz
      } else if(OFS1_HOCOFRQ1_48MHZ == (wClockUserVal[OFS1] & OFS1_HOCOFRQ1_MASK))
      {
        dwordClk = 48000000; // HOCO frequency is 48MHz
      } else if(OFS1_HOCOFRQ1_64MHZ == (wClockUserVal[OFS1] & OFS1_HOCOFRQ1_MASK))
      {
        dwordClk = 64000000; // HOCO frequency is 64MHz
      } else
      {
        return (RES_ERROR); // Failed
      }
      break;
    case SCKSCR_CKSEL_LOCO: 
    case SCKSCR_CKSEL_SOSC: 
    default:
        return (RES_ERROR); // Failed
    }
		
  dwordSckdivcr = dwordSckdivcr >> 28;
  dwordSckdivcrVal = 1 << dwordSckdivcr; // FCLK division value
  *clk = dwordClk / dwordSckdivcrVal; // Calculate FCLK frequency
		
	// Update the flash rewrite upper limit frequency for each power control mode
	if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH)
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH;
  } else if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_MID;
  } else
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_LOWV;
  }

  /** Failed because outside the allowable frequency of FCLK */
  if(((*clk) < FLASH_WRITEABLE_MIN_CLOCK_HZ) || (dwWriteableMaxClock < (*clk)))
  {
    return (RES_ERROR); // Failed
  }

  return (RES_OK); // OK
}

/*
 *  Change Mode & Clock for rewriting flash
 *    Parameter:      clk: Input clock value (Hz)
 *    Return Value:   0 - OK,  1 - Failed
 */
static DWORD changeClockAndMode(DWORD *clk)
{
  M16(MCU_PRCR_ADDR) = KEYCODE_PRCR | PRCR_PRC1 | PRCR_PRC0; // Unprotect some registers

  /** If it is Subosc-speed mode, set it to other than Subosc-speed mode */
  if ((wClockUserVal[SOPCCR] & SOPCCR_SOPCM) != SOPCCR_SOPCM_OTH)
  {
      M8(MCU_SOPCCR_ADDR) = SOPCCR_SOPCM_OTH;
      while (M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF)
      {
          ; // Wait till transition completed
      }
  }

  /** If it is Low-speed mode, set it to Middle-speed mode */
  if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_LOW)
  {
      M8(MCU_OPCCR_ADDR) = OPCCR_OPCM_MID;
      while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
      {
          ; // Wait till transition completed
      }
  }

	if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH){
  /** If OFS1 is in the initial state, set the clock source to MOCO */
  if ((wClockUserVal[OFS1] & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_MASK)
  {
      /** If the clock source is MOCO, set the clock source to MOCO */
      if (wClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
      {
          /** If MOCO oscillation is stopped, oscillate it */
          if(wClockUserVal[MOCOCR] == MOCOCR_STOP)
          {
              M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
              Wait_us(15);// Wait for oscillation stabilization of MOCO
          }
          M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;// Set the clock source to MOCO
      }
      *clk = 8000000; // FCLK frequency is 8MHz
  } else
  {/** If OFS1 is not in the initial state, set the clock source to HOCO */
      if (wClockUserVal[SCKSCR] != SCKSCR_CKSEL_HOCO)
      {

          /** If HOCO oscillation is stopped, oscillate it */
          if(wClockUserVal[HOCOCR] == HOCOCR_STOP)
          {
              M8(MCU_HOCOCR_ADDR) = HOCOCR_ON;
              while (!(M8(MCU_OSCSF_ADDR) & OSCSF_HOCOSF))
              {
                  ;// Wait for oscillation stabilization of HOCO
              }
          }
          M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_HOCO;// Set the clock source to HOCO
      }

			// Frequency is 24/32/48/64 MHz
			switch (M32(MCU_OFS1_ADDR) & OFS1_HOCOFRQ1_MASK)
      {
        case OFS1_HOCOFRQ1_24MHZ:
					*clk = 24000000;
          break;
        case OFS1_HOCOFRQ1_32MHZ:
					*clk = 32000000;
          break;
        case OFS1_HOCOFRQ1_48MHZ:
					*clk = 48000000;
					break;
				case OFS1_HOCOFRQ1_64MHZ:
					*clk = 64000000;
					break;
        default:
					return (RES_ERROR); // Finished with Errors
      }
    }

		if (*clk != 64000000 && *clk != 48000000)
		{
			// Change clock division to 1
			M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_FCLK_1DEV;
		} else
		{
			// Change clock division to 2
			M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_FCLK_2DEV;
			if (*clk == 64000000)
			{	
				*clk = 32000000;
			} else
			{
				*clk = 24000000;
			}
		}
	} else
	{
		  /** If the clock source is MOCO, set the clock source to MOCO */
      if (wClockUserVal[SCKSCR] != SCKSCR_CKSEL_MOCO)
      {
          /** If MOCO oscillation is stopped, oscillate it */
          if(wClockUserVal[MOCOCR] == MOCOCR_STOP)
          {
              M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
              Wait_us(15);// Wait for oscillation stabilization of MOCO
      }
          M16(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;// Set the clock source to MOCO
      }
			
			if ((wClockUserVal[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
			{
				// Change clock division to 1
				M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_FCLK_1DEV;
				*clk = 8000000;
			} else
			{
				// Change clock division to 2
				M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_FCLK_2DEV;
				*clk = 4000000;
    }

			
	}
    return (RES_OK); // OK

}

/*
 *  Save Mode & Clock before rewriting flash
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE saveModeAndClock(void)
{
  wClockUserVal[PRCR] = (DWORD)M16(MCU_PRCR_ADDR);
	wClockUserVal[SCKSCR] = (DWORD)M8(MCU_SCKSCR_ADDR);
  wClockUserVal[SCKDIVCR] = M32(MCU_SCKDIVCR_ADDR);
	wClockUserVal[HOCOCR] = (DWORD)M8(MCU_HOCOCR_ADDR);
	wClockUserVal[MOCOCR] = (DWORD)M8(MCU_MOCOCR_ADDR);
	wClockUserVal[OFS1] = M32(MCU_OFS1_ADDR);
	wClockUserVal[SOPCCR] = (DWORD)M8(MCU_SOPCCR_ADDR);
	wClockUserVal[OPCCR] = (DWORD)M8(MCU_OPCCR_ADDR);
  wClockUserVal[PLLCCR2] = (DWORD)M8(MCU_PLLCCR2_ADDR);
	wClockUserVal[DFLCTL] = (DWORD)M8(MCU_DFLCTL_ADDR);  
	wClockUserVal[FASR] = (DWORD)M8(MCU_FASR_ADDR);  
  wClockUserVal[FISR] = (DWORD)M8(MCU_FISR_ADDR); 
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Restore Mode & Clock after rewriting flash
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE restoreModeAndClock(void)
{
	M8(MCU_SCKSCR_ADDR) = wClockUserVal[SCKSCR];
	M32(MCU_SCKDIVCR_ADDR) = wClockUserVal[SCKDIVCR];
	M8(MCU_MOCOCR_ADDR) = wClockUserVal[MOCOCR];
	M8(MCU_HOCOCR_ADDR) = wClockUserVal[HOCOCR];
	M8(MCU_OPCCR_ADDR) = wClockUserVal[OPCCR];
	while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
  {
    ; // Wait till transition completed
  }
	M8(MCU_SOPCCR_ADDR) = wClockUserVal[SOPCCR];
	while (M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF)
  {
    ; // Wait till transition completed
  }
	M8(MCU_PLLCCR2_ADDR) = wClockUserVal[PLLCCR2];
	M8(MCU_DFLCTL_ADDR) = wClockUserVal[DFLCTL];
	M8(MCU_FASR_ADDR) = wClockUserVal[FASR];
	M8(MCU_FISR_ADDR) = wClockUserVal[FISR];
	
	return (RES_OK); // Finished without Errors
}

/*
 *  PE Mode entry
 *    Parameter:      adr: Target address area (may be Code flash or Data flash start address)
 *                    adrRet: PE address (as returned value)
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE enterPEMode(DWORD adr, DWORD *adrRet)
{
	
	BYTE byFpmcr;
	
	// Enter READ Mode
	M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;
	// Wait until successfully entering READ Mode
	while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_READ)
	{
		// Do nothing
		;
	}
	
	// Detect target area is Code Flash
	if (adr < FENTRYR_DATAF_START_ADDR)
	{
		// Enter Code Flash PE Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_CODEF_PE;
		
		// Discharge1
		byFpmcr = FPMCR_FMS_DISCHARGE1;
		SpecificSequence(byFpmcr);
		Wait_us(2);
		// Discharge2
		byFpmcr = FPMCR_FMS_DISCHARGE2;

		if((M8(MCU_OPCCR_ADDR) & OPCCR_OPCM) == OPCCR_OPCM_LOW_VOL)
		{
			byFpmcr |= FPMCR_FMS_LVPE;
		}
		SpecificSequence(byFpmcr);
		Wait_us(2);
		// Code Flash
		byFpmcr = FPMCR_FMS_CODEPE;
		
		if((M8(MCU_OPCCR_ADDR) & OPCCR_OPCM) == OPCCR_OPCM_LOW_VOL)
		{
			byFpmcr |= FPMCR_FMS_LVPE;
		}
		SpecificSequence(byFpmcr);
		Wait_us(2);
		// Set the Start PE address of Code flash
		*adrRet = adr & CODEF_PE_MASK_ADDR;
		// Wait until successfully entering Code Flash PE Mode
		while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYC)
		{
			// Do nothing
			;
		}
	} else
	{
		//DataFlash
		M8(MCU_DFLCTL_ADDR) = DFLCTL_DFLEN_ENA;
		// Enter Data Flash PE Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_DATAF_PE;
		Wait_us(5);
		byFpmcr = FPMCR_FMS_DATAPE;
		
		if((M8(MCU_OPCCR_ADDR) & OPCCR_OPCM) == OPCCR_OPCM_LOW_VOL)
		{
			byFpmcr |= FPMCR_FMS_LVPE;
		}
		SpecificSequence(byFpmcr);
		
		// Set the Start PE address of Data flash
		*adrRet = (0xFE000000) | (adr & DATAF_MASK_ADDR);
		// Wait until successfully entering Data Flash PE Mode
		while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_FENTRYD)
		{
			// Do nothing
			;
		}
	}
	
	if((CONFIG_START <= adr) && (adr < FENTRYR_DATAF_START_ADDR))
	{
		M8(MCU_FASR_ADDR) = FASR_EXS_EXTRA;
	} else
	{
		M8(MCU_FASR_ADDR) = FASR_EXS_USR;
	}
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Check internal errors
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE checkInternalError(void)
	{
	// Check for ILGLERR
	if (M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK) {
		return (RES_ERROR);
	}
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Wait processing in us units
 *    Parameter:      us: wait time
 *    Return Value:   none
 *	  Note:			  Please note that the weight processing is less accurate.
 */
static void Wait_us(DWORD us)
{
	DWORD dwCount;
	DWORD dwTotalNsPerCycle;

	dwTotalNsPerCycle = 1000000000 / dwICLKValue; // Calculate ns per cycle
	if (dwTotalNsPerCycle == 0)
	{
		// Fail-safe when exceeding 100Mhz
		dwTotalNsPerCycle = 1;	
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

/*
 *  Extract some bits from input
 *    Parameter:      dwValue:	Input value
 *                    startBit:	Start bit
 *                    bitCount:	Number of bits to get
 *    Return Value:   extracted bits
 */
static DWORD extractBits(DWORD dwValue, unsigned int startBit, unsigned int bitCount) {
	return (((1 << bitCount) - 1) & (dwValue >> ((startBit + 1) - 1)));
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
	DWORD dwTargetAddressPE;

	// Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);

	// Set Start address
	M16(MCU_FSARL_ADDR) = extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits

	// Set End address
	M16(MCU_FEARL_ADDR) = extractBits(dwTargetAddressPE + sz - 1, 0, 16); // 16 lower bits
	M16(MCU_FEARH_ADDR) = extractBits(dwTargetAddressPE + sz - 1, 16, 16); // 16 higher bits

	// Write FCR with 86h
	M8(MCU_FCR_ADDR) = 0x86;

	// Wait for Ready flag (FSTATR1.FRDY = 1)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}

	// Write FCR with 00h
	M8(MCU_FCR_ADDR) = 0x00;

	// Wait for Ready flag (FSTATR1.FRDY = 0)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) == FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}

	// Check error - If error, reset then exit
	// No error means: FSTATR2.ILGLERR = 0  & FSTATR2.ERERR = 0
	if (((M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK) == FSTATR2_ILGLERR_MASK) ||
				((M16(MCU_FSTATR2_ADDR) & FSTATR2_ERERR_MASK) == FSTATR2_ERERR_MASK))
	{
		// Sequencer initialization
		M8(MCU_FRESETR_ADDR) = 1;
		M8(MCU_FRESETR_ADDR) = 0;
		// Exit
		return (RES_ERROR); // Finished with Errors
	}

	return (RES_OK); // Finished without Errors
}

/*
 *  Setting the FPMCR register
 *    Parameter:      ucFpmcr: Flash Operating Mode Select paramater
 *    Return Value:   None
 */
static void SpecificSequence(unsigned char ucFpmcr)
{

	//FPMCR UNLOCK
	M8(MCU_FPR_ADDR)= FPR_UNLOCK;
	//FPMCR 1st write
	M8(MCU_FPMCR_ADDR) = ucFpmcr;
	//FPMCR 2nd write
	M8(MCU_FPMCR_ADDR) = ~ucFpmcr;
	//FPMCR 3rd write
	M8(MCU_FPMCR_ADDR) = ucFpmcr;

	return;
}

/*
 *  READ Mode entry
 *    Parameter:      None
 *    Return Value:   None
 */
static void SetReadMode(void){

	if((M8(MCU_FPMCR_ADDR) & FPMCR_FMS_CODEPE) == FPMCR_FMS_CODEPE)
	{
		//ROM PE
		//DISCHARGE2
		SpecificSequence(FPMCR_FMS_DISCHARGE2);
		Wait_us(2);
		//DISCHARGE1
		SpecificSequence(FPMCR_FMS_DISCHARGE1);
	}
	//ReadMode
	SpecificSequence(FPMCR_FMS_RPDIS | FPMCR_FMS_READMODE);
	Wait_us(5);
	//FENTRYR
	M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;
	//FENTRYR=0
	while(M16(MCU_FENTRYR_ADDR));

	return;
}

/*
 *  Read the data written in the flash
 *    Parameter:      udw_addr:  Read start address
 *    Parameter:      udw_sz:   	Read size
 *    Parameter:      ubp_buf:   Buffer to store read data
 *    Return Value:   None
 */
static void readpage(unsigned long udw_addr, unsigned long udw_sz, unsigned char *ubp_buf)
{

  unsigned long i;

  for(i = 0; i < udw_sz ; i++)
  {
    ubp_buf[i] = M8(udw_addr + i); // Store read data byte by byte
  }

}
