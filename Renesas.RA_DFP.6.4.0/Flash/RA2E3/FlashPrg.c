/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for RA2E3
 * @version  V1.0.1
 * @date     07 Mar 2024
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
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 31.07.2023 1.0.0    First Release
 *			   : 07.03.2024	1.0.1 	 Increase Programming Page Size
 *********************************************************************************************************************/
 
 /*
    FlashOS Structures
 */
#include "FlashOS.h"

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
#define MCU_SYSTEM_BASE									(0x4001E000)
#define MCU_PRCR_ADDR										(MCU_SYSTEM_BASE + 0x3FE)
		#define PRCR_PRC0_ENABLE						(0x1)
		#define PRCR_PRC1_ENABLE						(0x2)
		#define PRCR_KEY										(0xA500)
#define MCU_SCKSCR_ADDR									(MCU_SYSTEM_BASE + 0x026)
		#define SCKSCR_CKSEL_HOCO	          (0x00)
		#define SCKSCR_CKSEL_MOCO	          (0x01)
		#define SCKSCR_CKSEL_LOCO	          (0x02)
		#define SCKSCR_CKSEL_MOSC	          (0x03)
		#define SCKSCR_CKSEL_SOSC	          (0x04)
		#define SCKSCR_CKSEL_MASK	          (0x07)
#define MCU_SOPCCR_ADDR									(MCU_SYSTEM_BASE + 0x0AA)
		#define SOPCCR_SOPCM_SUBMODE				(0x01)
		#define SOPCCR_SOPCM_OTHMODE				(0x00)
		#define SOPCCR_SOPCM_MASK						(0x01)
		#define SOPCCR_SOPCMTSF_TRANS				(0x10) 
		#define SOPCCR_SOPCMTSF_COMPL				(0x00)
		#define SOPCCR_SOPCMTSF_MASK				(0x10)
#define MCU_OPCCR_ADDR									(MCU_SYSTEM_BASE + 0x0A0)
		#define OPCCR_OPCM									(0x03)
    #define OPCCR_OPCM_HIGH							(0x00)
    #define OPCCR_OPCM_MID							(0x01)
		#define OPCCR_OPCM_LOW							(0x03)
    #define OPCCR_OPCMTSF               (0x10)
    #define OPCCR_OPCMTSF_COMPLETE      (0x00)
    #define OPCCR_OPCMTSF_TRANS         (0x10)
#define MCU_OFS1_ADDR										(0x00000404)
		#define OFS1_HOCOFRQ1_24MHZ					(0x00000000)
		#define OFS1_HOCOFRQ1_32MHZ					(0x00002000)
		#define OFS1_HOCOFRQ1_48MHZ					(0x00004000)
		#define OFS1_HOCOFRQ1_64MHZ					(0x00005000)
		#define OFS1_HOCOFRQ1_MASK					(0x00007000)
#define MCU_SCKDIVCR_ADDR								(MCU_SYSTEM_BASE + 0x020)
		#define SCKDIVCR_ICK_DIV_1					(0x00)
		#define SCKDIVCR_ICK_DIV_2					(0x01)
		#define SCKDIVCR_ICK_DIV_4					(0x02)
		#define SCKDIVCR_ICK_DIV_8					(0x03)
		#define SCKDIVCR_ICK_DIV_16					(0x04)
		#define SCKDIVCR_ICK_DIV_32					(0x05)
		#define SCKDIVCR_ICK_DIV_64					(0x06)
		#define SCKDIVCR_ICK_MASK						(0x07000000)
		#define SCKDIVCR_ICK_DIV1_MASK			(0x00000000)
		#define SCKDIVCR_ICK_DIV2_MASK			(0x01000101)
		#define SCKDIVCR_ICK_DIV4_MASK			(0x02000202)
#define MCU_MOCOCR_ADDR									(MCU_SYSTEM_BASE + 0x038)
		#define MOCOCR_STOP									(0x01)
		#define MOCOCR_ON										(0x00)
#define MCU_HOCOCR_ADDR									(MCU_SYSTEM_BASE + 0x036)
		#define HOCOCR_STOP									(0x01)
		#define HOCOCR_ON										(0x00)
#define MCU_OSCSF_ADDR									(MCU_SYSTEM_BASE + 0x03C)
		#define OSCSF_HOCOSF_MASK						(0x01)
		#define OSCSF_MOSCSF_MASK						(0x08)
		#define OSCSF_HOCOSF_STABLE					(0x01)
		#define OSCSF_MOSCSF_STABLE					(0x08)

/*
   Flash related registers adrress and bit definition
*/
#define MCU_FLASH_BASE                 (0x407EC000)
#define MCU_FENTRYR_ADDR               (MCU_FLASH_BASE + 0x3FB0)
		#define FENTRYR_READ_MODE          (0xAA00)
		#define FENTRYR_DATA_PE_MODE       (0xAA80)
		#define FENTRYR_CODE_PE_MODE       (0xAA01)
		#define FENTRYR_CHK_READ_MODE      (0x0000)
		#define FENTRYR_CHK_DATA_PE_MODE   (0x0080)
		#define FENTRYR_CHK_CODE_PE_MODE   (0x0001)
#define MCU_FISR_ADDR		               (MCU_FLASH_BASE + 0x1D8)
#define MCU_FRESETR_ADDR               (MCU_FLASH_BASE + 0x124)
    #define FRESETR_FRESET_MASK        (0x1)
#define MCU_FCR_ADDR		               (MCU_FLASH_BASE + 0x114)
		#define FCR_CMD_MASK	             (0xF)
		#define FCR_DRC_MASK	             (0x10)
		#define FCR_STOP_MASK	             (0x40)
		#define FCR_OPST_MASK	             (0x80)
#define MCU_FSARH_ADDR	               (MCU_FLASH_BASE + 0x110)
#define MCU_FSARL_ADDR	               (MCU_FLASH_BASE + 0x108)
#define MCU_FEARH_ADDR	               (MCU_FLASH_BASE + 0x120)
#define MCU_FEARL_ADDR	               (MCU_FLASH_BASE + 0x118)
#define MCU_FWBL0_ADDR	               (MCU_FLASH_BASE + 0x130)
#define MCU_FWBH0_ADDR	               (MCU_FLASH_BASE + 0x138)
#define MCU_FRBL0_ADDR	               (MCU_FLASH_BASE + 0x188)
#define MCU_FRBH0_ADDR	               (MCU_FLASH_BASE + 0x190)
#define MCU_FSTATR2_ADDR               (MCU_FLASH_BASE + 0x1F0)
		#define FSTATR2_ERERR_MASK         (0x1)
		#define FSTATR2_PRGERR_MASK        (0x2)
		#define FSTATR2_PRGERR01_MASK      (0x4)
		#define FSTATR2_BCERR_MASK 	       (0x8)
		#define FSTATR2_ILGLERR_MASK       (0x10)
		#define FSTATR2_EILGLERR_MASK      (0x20)
#define MCU_FSTATR1_ADDR               (MCU_FLASH_BASE + 0x12C)
		#define FSTATR1_DRRDY_MASK         (0x2)
		#define FSTATR1_FRDY_MASK          (0x40)
		#define FSTATR1_EXRDY_MASK         (0x80)
#define MCU_DFLCTL_ADDR								 (MCU_FLASH_BASE + 0x090)
	  #define DFLCTL_DFLEN_ENA					 (0x01)
	  #define DFLCTL_DFLEN_DIS					 (0x00)
#define MCU_FPR_ADDR		               (MCU_FLASH_BASE + 0x180)
	  #define FPR_UNLOCK		             (0xA5)
#define MCU_FPMCR_ADDR								 (MCU_FLASH_BASE + 0x100)
  	#define FPMCR_FMS_FMS0						 (0x02)
  	#define FPMCR_FMS_FMS1						 (0x10)
  	#define FPMCR_FMS_FMS							 (0x12)
	  #define FPMCR_FMS_READMODE				 (0x00)
  	#define FPMCR_FMS_CODEPE					 (0x02)
	  #define FPMCR_FMS_DATAPE					 (0x10)
  	#define FPMCR_FMS_RPDIS						 (0x08)
  	#define FPMCR_FMS_RPDIS_DIS				 (0x08)
	  #define FPMCR_FMS_RPDIS_ENA				 (0x00)
#define MCU_FASR_ADDR									 (MCU_FLASH_BASE + 0x104)
	  #define FASR_EXS_USR							 (0x00)
  	#define FASR_EXS_EXTRA						 (0x01)
	  #define MCU_FEXCR_ADDR		         (MCU_FLASH_BASE + 0x1DC)
  	#define FEXCR_CMD				           (0x07)
		#define FEXCR_CMD_STARTUP_AREA_SELECT		(0x02) 
  	#define FEXCR_CMD_ACCESS_WINDOW	   (0x02)
	  #define FEXCR_CMD_OCDID1	         (0x03)
  	#define FEXCR_CMD_OCDID2	         (0x04)
  	#define FEXCR_CMD_OCDID3	         (0x05)
	  #define FEXCR_CMD_OCDID4	         (0x06)
	  #define FEXCR_OPST_START	         (0x80)
	  #define FEXCR_OPST_STOP		         (0x00)
/*
   Other parameters
*/
#define MAX_OPERATING_FREQUENCY					(48000000)
#define RES_ERROR												(1)
#define RES_OK													(0)
#define FLASH_WRITEABLE_MIN_CLOCK_HZ		(1000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH		(48000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_MID		(4000000)
#define FLASH_WRITEABLE_MAX_CLOCK_HZ_LOW		(2000000)
#define FENTRYR_CODEF_START_ADDR				(0x00000000)
#define FENTRYR_DATAF_START_ADDR				(0x40100000)
#define CODEF_PE_MASK_ADDR							(0x000FFFFF)
#define DATAF_PE_MASK_ADDR							(0x0000FFFF)
#define PROGRAMMING_PAGE_SIZE						(128) // Should re-define this value base on user's marco
#define BLANK_VALUE											(0xFF)
#define SECTOR_SIZE											(0x000800) // Should re-define this value base on user's marco
#define CODEF_START_ADDR								(0x00000000)
#define DATAF_START_ADDR								(0x40100000)
#define CONFIG_START_ADDR								(0x01010000)
#define CODEF_SIZE											(0x00010000) // Should re-define this value base on user's marco
#define DATAF_SIZE											(0x00000800) // Should re-define this value base on user's marco
#define MCU_AWS_ADDR		                (CONFIG_START_ADDR + 0x10)
#define MCU_OSIS1_ADDR		              (CONFIG_START_ADDR + 0x18)
#define MCU_OSIS2_ADDR		              (CONFIG_START_ADDR + 0x20)
#define MCU_OSIS3_ADDR		              (CONFIG_START_ADDR + 0x28)
#define MCU_OSIS4_ADDR		              (CONFIG_START_ADDR + 0x30)
#ifdef RA2E3_64K
	#undef CODEF_SIZE											// Undef macro first
	#undef SECTOR_SIZE										// Undef macro first
	#undef PROGRAMMING_PAGE_SIZE					// Undef macro first
	#define CODEF_SIZE										(0x00010000)
	#define SECTOR_SIZE										(0x000800)
	#define PROGRAMMING_PAGE_SIZE					(2048)
#endif
#ifdef RA2E3_32K
	#undef CODEF_SIZE											// Undef macro first
	#undef SECTOR_SIZE										// Undef macro first
	#undef PROGRAMMING_PAGE_SIZE					// Undef macro first
	#define CODEF_SIZE										(0x00008000)
	#define SECTOR_SIZE										(0x000800)
	#define PROGRAMMING_PAGE_SIZE					(2048)
#endif
#ifdef RA2E3_DATA
	#undef DATAF_SIZE											// Undef macro first
	#undef SECTOR_SIZE										// Undef macro first
	#undef PROGRAMMING_PAGE_SIZE					// Undef macro first
	#define DATAF_SIZE										(0x00000800)
	#define SECTOR_SIZE										(0x000400)
	#define PROGRAMMING_PAGE_SIZE					(1024)
#endif
#ifdef RA2E3_CONF
	#undef SECTOR_SIZE										// Undef macro first
	#undef PROGRAMMING_PAGE_SIZE					// Undef macro first
	#define SECTOR_SIZE										(0x00000004)
	#define PROGRAMMING_PAGE_SIZE					(4)
#endif
#define TARGET_CONFIG										(0)
#define TARGET_DATAF										(1)
#define TARGET_CODEF										(2)

/*
   Type declaration
*/
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef enum REG_DATA {
		PRCR = 0,
    SCKSCR,
    SCKDIVCR,
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
static DWORD userRegisterValue[REG_MAX];

/*
   Current ICLK frequency
*/
static DWORD dwICLKValue;

/*
   Internal function prototype
*/
static int EraseChipArea(unsigned long adr, unsigned long sz);
static BYTE checkModeAndClock(DWORD *clk);
static BYTE changeModeAndClock(DWORD *clk);
static BYTE saveModeAndClock(void);
static BYTE restoreModeAndClock(void);
static BYTE enterPEMode(DWORD adr, DWORD *adrRet);
static BYTE checkInternalError(void);
static void Wait_us(DWORD us);
static DWORD extractBits(DWORD dwValue, unsigned int startBit, unsigned int bitCount);
static void SpecificSequence(unsigned char ucFpmcr);
static void SetReadMode(void);
static void read_page(unsigned long adr, unsigned long sz, unsigned char *buf);

/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify) - Ignored
 *    Return Value:   0 - OK,  1 - Failed
 */
int Init (unsigned long adr, unsigned long clk, unsigned long fnc)
{
	// Temporary variable to store mode & clock checking result
	BYTE byteClockCheckResult;
	BYTE byteUserClockValue;
	DWORD dwPEAddr;
	
	dwICLKValue = 0;
	
	// Save current settings
	saveModeAndClock();
	
	// Check if flash is rewritable with current clock and power control mode
	byteClockCheckResult = checkModeAndClock(&clk);
	// Save current frequency for dwICLKValue
	dwICLKValue = clk; 
	
	// If the flash is not rewritable with current settings, switch to a possible state
	#ifdef CHG_CLK_AND_MOD_ENA	
	if(byteClockCheckResult == RES_ERROR)
	{
		// Change power mode and clock to state that allows rewrite flash
		byteClockCheckResult = changeModeAndClock(&clk);
		
		// Save ICLK frequency for wait processing
		dwICLKValue = clk;
  }
	#endif
  
	// Return Failure if the flash cannot be rewritten and stop other processing
	if(byteClockCheckResult)
	{
		return (RES_ERROR); // Finished with Errors
  }
	
	// Convert clock from Hz to MHz unit
	byteUserClockValue = clk / 1000000;
	// Round up the first decimal place in MHz
	if (clk % 1000000)
	{
		byteUserClockValue += 1;
	}
	
	// Enter PE Mode
	enterPEMode(adr, &dwPEAddr);
	
	// Set Peripheral Clock Notification
	// Because (FlashIF Clock Frequency [MHz]) 1.0 = (PCKA [5:0] Bit Setting) 00000b
	// So we decrease clock value by 1 before convert to bit string.
	// Note: This register can be set only in PE Mode
	if(byteUserClockValue <= 32)
	{
		M8(MCU_FISR_ADDR) = (M8(MCU_FISR_ADDR) | (byteUserClockValue - 1));
	} else
	{
		M8(MCU_FISR_ADDR) = (M8(MCU_FISR_ADDR) | ((byteUserClockValue >> 1) + 15));
	}
	
	// Reset all Flash register to apply new FCLK
	M8(MCU_FRESETR_ADDR) = 1;
	M8(MCU_FRESETR_ADDR) = 0;
	
	// Go to READ Mode
	SetReadMode();
	
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
	// Unprotect some registers
	M16(MCU_PRCR_ADDR) = PRCR_KEY | PRCR_PRC0_ENABLE | PRCR_PRC1_ENABLE;
	
	if (restoreModeAndClock()) {
    return (RES_ERROR); // Finished with Errors
  }
	
	M16(MCU_PRCR_ADDR) = PRCR_KEY | userRegisterValue[PRCR];
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
	BYTE byteResult = RES_OK;
	
	// Just delete Code flash area or Data flash area base on user's macro
	#ifdef RA2E3_64K
		// Clear Code flash
		byteResult = EraseChipArea(CODEF_START_ADDR, CODEF_SIZE);
	#endif
	#ifdef RA2E3_32K
		// Clear Code flash
		byteResult = EraseChipArea(CODEF_START_ADDR, CODEF_SIZE);
	#endif
	#ifdef RA2E3_DATA
		// Clear Data flash
		byteResult = EraseChipArea(DATAF_START_ADDR, DATAF_SIZE);
	#endif
	// Configuration Area does not support Erase
	
  return (byteResult); // Return status
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
	if (adr < CONFIG_START_ADDR)
	{
		// This is Code flash - Support Erase
	} else if (adr < DATAF_START_ADDR)
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

	// Write FCR with 04h
	M8(MCU_FCR_ADDR) = 0x04;

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
 *                    sz:   Page Size (Data count of <code>buf</code>)
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
	unsigned long dataIndex;

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

	if (adr < CONFIG_START_ADDR)
	{
		// This is Code flash
		byteTargetArea = TARGET_CODEF;
		byteDataInBlock = 4;
	} else if (adr < DATAF_START_ADDR)
	{
		// This is Configuration Area
		byteTargetArea = TARGET_CONFIG;
		byteDataInBlock = 4;
	} else
	{
		// This is Data flash
		byteTargetArea = TARGET_DATAF;
		byteDataInBlock = 1;
	}

	// Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);

	if (byteTargetArea != TARGET_CONFIG){
		// Set programming address
		M16(MCU_FSARL_ADDR) = (WORD)extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
		M16(MCU_FSARH_ADDR) = (WORD)extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits
	}
	
	// Loop until end of data
	do {
		// Set programming data (Code flash: 4 data, Data flash: 1 data, Config Area: 4 data)
		if (byteTargetArea == TARGET_CODEF)
		{
			M16(MCU_FWBL0_ADDR) = (buf[dataIndex] | (buf[dataIndex + 1] << 8)); // 16 lower bits
			M16(MCU_FWBH0_ADDR) = (buf[dataIndex + 2] | (buf[dataIndex + 3] << 8)); // 16 higher bits
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

			// Write FCR with 01h
			M8(MCU_FCR_ADDR) = 0x01;

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
				// Write FEXCR with various start commands
				M8(MCU_FEXCR_ADDR) = byteExCmd | FEXCR_OPST_START;
				
				// Wait for ready flag (FSTATR1.EXRDY = 1)
				while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_EXRDY_MASK) != FSTATR1_EXRDY_MASK)
				{
					// Do nothing
					;
				}

				// Write FEXCR with various stop commands
				M8(MCU_FEXCR_ADDR) = byteExCmd | FEXCR_OPST_STOP;

				// Write FEXCR with 00h
				M8(MCU_FEXCR_ADDR) = 0x00;

				// Wait for ready flag (FSTATR1.EXRDY = 0)
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
		read_page(adr_tmp, 128, &byte_rd_buf[0]);

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
 *  Read the data written in the flash
 *    Parameter:      udw_addr:  Read start address
 *    Parameter:      udw_sz:   	Read size
 *    Parameter:      ubp_buf:   Buffer to store read data
 *    Return Value:   None
 */
static void read_page(unsigned long udw_addr, unsigned long udw_sz, unsigned char *ubp_buf)
{

    unsigned long i;

    for(i = 0; i < udw_sz ; i++)
    {
        ubp_buf[i] = M8(udw_addr + i); // Store read data byte by byte
    }

}

/*
 *  Blank Check
 *    Parameter:      adr: Address
 *                    sz:  Block Size
 *                    pat: Pattern (skip)
 *    Return Value:   0 - OK,  1 - Failed
 */
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat)
{
	// Temporary variables
	DWORD dwTargetAddressPE;
	BYTE byteTargetArea;
	
	// Detect Area
	if (adr < CONFIG_START_ADDR)
	{
		// This is Code flash - Support BlankCheck
		byteTargetArea = TARGET_CODEF;
	} else if (adr < DATAF_START_ADDR)
	{
		// This is Configuration Area - Not support BlankCheck
		return (RES_OK);
	} else
	{
		// This is Data flash - Support BlankCheck
		byteTargetArea = TARGET_DATAF;
	}

  // Enter PE Mode
	enterPEMode(adr, &dwTargetAddressPE);
	
	// Set Start address
	M16(MCU_FSARL_ADDR) = (WORD)extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = (WORD)extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits
	
	// Set End address
	M16(MCU_FEARL_ADDR) = (WORD)extractBits(dwTargetAddressPE + sz - 1, 0, 16); // 16 lower bits
	M16(MCU_FEARH_ADDR) = (WORD)extractBits(dwTargetAddressPE + sz - 1, 16, 16); // 16 higher bits
	
	if (byteTargetArea == TARGET_CODEF)
	{
		// Write FCR with 83h
		// Start BlankCheck for Code flash
		M8(MCU_FCR_ADDR) = 0x83;
	} else
	{
		// Write FCR with 8Bh
		// Start BlankCheck for Data flash
		M8(MCU_FCR_ADDR) = 0x8B;
	}
	// Internal BlankCheck exeucting
	// Wait for ready flag (FSTATR1.FRDY = 1)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}
	
	if (byteTargetArea == TARGET_CODEF)
	{
		// Write FCR with 03h
		// Stop BlankCheck for Code flash
		M8(MCU_FCR_ADDR) = 0x03;
	} else
	{
		// Write FCR with 0Bh
		// Stop BlankCheck for Data flash
		M8(MCU_FCR_ADDR) = 0x0B;
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
	M16(MCU_FSARL_ADDR) = (WORD)extractBits(dwTargetAddressPE, 0, 16); // 16 lower bits
	M16(MCU_FSARH_ADDR) = (WORD)extractBits(dwTargetAddressPE, 16, 16); // 16 higher bits

	// Set End address
	M16(MCU_FEARL_ADDR) = (WORD)extractBits(dwTargetAddressPE + sz - 1, 0, 16); // 16 lower bits
	M16(MCU_FEARH_ADDR) = (WORD)extractBits(dwTargetAddressPE + sz - 1, 16, 16); // 16 higher bits

	// Write FCR with 86h
	M8(MCU_FCR_ADDR) = 0x86;

	// Wait for Ready flag (FSTATR1.FRDY = 1)
	while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
	{
		// Do nothing
		;
	}

	// Write FCR with 06h
	M8(MCU_FCR_ADDR) = 0x06;

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
 *  Check Mode & Clock for rewriting flash
 *    Parameter:      clk: Input clock value (Hz)
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE checkModeAndClock(DWORD *clk)
{
	// Temporary variable to store user settings
	DWORD dwUserClockValue;
	WORD wUserClockDivisionIndex;
	WORD wUserClockDivisionValue;
	DWORD dwWriteableMaxClock;
	
	// Check and Get current clock speed
	switch (userRegisterValue[SCKSCR] & SCKSCR_CKSEL_MASK)
	{
		case SCKSCR_CKSEL_MOSC:
			// Input clock source is External clock
      dwUserClockValue = *clk; 
      break;
		case SCKSCR_CKSEL_HOCO:
			// Get settings frequency
			if ((userRegisterValue[OFS1] & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_24MHZ)
			{
				dwUserClockValue = 24000000;
			} else if ((userRegisterValue[OFS1] & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_32MHZ)
			{
				dwUserClockValue = 32000000;
			} else if ((userRegisterValue[OFS1] & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_48MHZ)
			{
				dwUserClockValue = 48000000;
			} else if ((userRegisterValue[OFS1] & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_64MHZ)
			{
				dwUserClockValue = 64000000;
			} else
			{
				// Invalid settings
				return (RES_ERROR); // Finished with Errors
			}
			break;
		case SCKSCR_CKSEL_MOCO:
			// MOCO frequency is fixed as 8MHz
			dwUserClockValue = 8000000;
			break;
		case SCKSCR_CKSEL_LOCO: // Flash is not rewriteable in LOCO
			dwUserClockValue = 32768;
			break;
		case SCKSCR_CKSEL_SOSC: // Flash is not rewriteable in SOSC
			dwUserClockValue = 32768;
			break;
		default:
			return (RES_ERROR); // Finished with Errors
	}
	// Calculate actual clock speed (by combining with Frequency Division)
	wUserClockDivisionIndex = (userRegisterValue[SCKDIVCR] & SCKDIVCR_ICK_MASK) >> 24;
	if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_1)
	{
		wUserClockDivisionValue = 1;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_2)
	{
		wUserClockDivisionValue = 2;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_4)
	{
		wUserClockDivisionValue = 4;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_8)
	{
		wUserClockDivisionValue = 8;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_16)
	{
		wUserClockDivisionValue = 16;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_32)
	{
		wUserClockDivisionValue = 32;
	} else if (wUserClockDivisionIndex == SCKDIVCR_ICK_DIV_64)
	{
		wUserClockDivisionValue = 64;
	} else
	{
		// Invalid setting
		return (RES_ERROR); // Finished with Errors
	}
	
	*clk = dwUserClockValue / wUserClockDivisionValue;
	
	// Return Failure if Sub Operating Power Control Mode = Subosc-speed
	if ((userRegisterValue[SOPCCR] & SOPCCR_SOPCM_MASK) == SOPCCR_SOPCM_SUBMODE)
	{
		return (RES_ERROR); // Finished with Errors
	}
	
	// Update the flash rewrite upper limit frequency for each power control mode
	if ((userRegisterValue[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH)
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_HIGH;
  } else if ((userRegisterValue[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_MID;
  } else
  {
    dwWriteableMaxClock = FLASH_WRITEABLE_MAX_CLOCK_HZ_LOW;
  }
	
	// Return Failure if current clock speed out of accepted range
	if (((*clk) < FLASH_WRITEABLE_MIN_CLOCK_HZ) || ((*clk) > dwWriteableMaxClock))
	{
		return (RES_ERROR); // Finished with Errors
	}
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Change Mode & Clock for rewriting flash
 *    Parameter:      clk: Input clock value (Hz)
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE changeModeAndClock(DWORD *clk)
{
	// Unprotect some registers
	M16(MCU_PRCR_ADDR) = PRCR_KEY | PRCR_PRC0_ENABLE | PRCR_PRC1_ENABLE;

	// If Sub Operating Power Control Mode = Subosc-speed, set to Other
	if ((M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCM_MASK) == SOPCCR_SOPCM_SUBMODE)
	{
		M8(MCU_SOPCCR_ADDR) = SOPCCR_SOPCM_OTHMODE;
		// Wait until it successfully sets
		while ((M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF_MASK) != SOPCCR_SOPCMTSF_COMPL)
		{
			// Do nothing
			;
		}
	}
	
	if ((userRegisterValue[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_HIGH){
	// If OFS1 is in initial state, use MOCO
	if ((M32(MCU_OFS1_ADDR) & OFS1_HOCOFRQ1_MASK) == OFS1_HOCOFRQ1_MASK)
	{
		// Change clock source to MOCO
		if (M8(MCU_SCKSCR_ADDR) != SCKSCR_CKSEL_MOCO)
		{
			// If MOCO is stoped, start it
			if (M8(MCU_MOCOCR_ADDR) == MOCOCR_STOP)
			{
				M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
				// Wait until MOCO is stable
				Wait_us(1);
			}
			// Select clock as MOCO
			M8(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;
		}
		// Frequency is 8MHz
		*clk = 8000000;
	} else
	{ // Otherwise, use HOCO
		// Change clock source to HOCO
		if (M8(MCU_SCKSCR_ADDR) != SCKSCR_CKSEL_HOCO)
		{
			// If HOCO is stoped, start it
			if (M8(MCU_HOCOCR_ADDR) == HOCOCR_STOP)
			{
				M8(MCU_HOCOCR_ADDR) = HOCOCR_ON;
				// Wait until HOCO is stable
				while ((M8(MCU_OSCSF_ADDR) & OSCSF_HOCOSF_MASK) != OSCSF_HOCOSF_STABLE)
				{
					// Do nothing
					;
				}
			}
			// Select clock as HOCO
			M8(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_HOCO;
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
	
	if (*clk != 64000000)
	{
	// Change clock division to 1
		M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_ICK_DIV1_MASK;
	} else
	{
		// Change clock division to 2
		M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_ICK_DIV2_MASK;
		*clk = 32000000;
	}
	}else
	{
		// Change clock source to MOCO
		if (M8(MCU_SCKSCR_ADDR) != SCKSCR_CKSEL_MOCO)
		{
			// If MOCO is stoped, start it
			if (M8(MCU_MOCOCR_ADDR) == MOCOCR_STOP)
			{
				M8(MCU_MOCOCR_ADDR) = MOCOCR_ON;
				// Wait until MOCO is stable
				Wait_us(1);		
			}
			// Select clock as MOCO
			M8(MCU_SCKSCR_ADDR) = SCKSCR_CKSEL_MOCO;
		}
		if ((userRegisterValue[OPCCR] & OPCCR_OPCM) == OPCCR_OPCM_MID)
		{
			// Change clock division to 2
			M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_ICK_DIV2_MASK;
			*clk = 4000000;
		} else // Low-speed mode
		{
			// Change clock division to 4
			M32(MCU_SCKDIVCR_ADDR) = SCKDIVCR_ICK_DIV4_MASK;
			*clk = 2000000;
		}
	}
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Save Mode & Clock before rewriting flash
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE saveModeAndClock(void)
{
	userRegisterValue[PRCR] = (DWORD)M16(MCU_PRCR_ADDR);
	userRegisterValue[SCKSCR] = (DWORD)M8(MCU_SCKSCR_ADDR);
  userRegisterValue[SCKDIVCR] = M32(MCU_SCKDIVCR_ADDR);
	userRegisterValue[HOCOCR] = (DWORD)M8(MCU_HOCOCR_ADDR);
	userRegisterValue[MOCOCR] = (DWORD)M8(MCU_MOCOCR_ADDR);
	userRegisterValue[OFS1] = M32(MCU_OFS1_ADDR);
	userRegisterValue[SOPCCR] = (DWORD)M8(MCU_SOPCCR_ADDR);
	userRegisterValue[OPCCR] = (DWORD)M8(MCU_OPCCR_ADDR);
	userRegisterValue[DFLCTL] = (DWORD)M8(MCU_DFLCTL_ADDR);  
	userRegisterValue[FASR] = (DWORD)M8(MCU_FASR_ADDR);  
  userRegisterValue[FISR] = (DWORD)M8(MCU_FISR_ADDR);
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Restore Mode & Clock after rewriting flash
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE restoreModeAndClock(void)
{
	M8(MCU_SCKSCR_ADDR) = userRegisterValue[SCKSCR];
	M32(MCU_SCKDIVCR_ADDR) = userRegisterValue[SCKDIVCR];
	M8(MCU_MOCOCR_ADDR) = userRegisterValue[MOCOCR]; 
	M8(MCU_HOCOCR_ADDR) = userRegisterValue[HOCOCR]; 
	
	M8(MCU_OPCCR_ADDR) = userRegisterValue[OPCCR];
	// Wait until it successfully sets
	while (M8(MCU_OPCCR_ADDR) & OPCCR_OPCMTSF)
  {
		// Do nothing
    ; 
  }
	
	M8(MCU_SOPCCR_ADDR) = userRegisterValue[SOPCCR];
	// Wait until it successfully sets
	while ((M8(MCU_SOPCCR_ADDR) & SOPCCR_SOPCMTSF_MASK) != SOPCCR_SOPCMTSF_COMPL)
	{
		// Do nothing
		;
	}
	M8(MCU_DFLCTL_ADDR) = userRegisterValue[DFLCTL];
	M8(MCU_FASR_ADDR) = userRegisterValue[FASR];
	M8(MCU_FISR_ADDR) = userRegisterValue[FISR];
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Wait processing in us units
 *    Parameter:      us: wait time
 *    Return Value:   none
 *	  Note:			  Please note that the weight processing is less accurate.
 */
#pragma GCC push_options
#pragma GCC optimize ("O0")

static void Wait_us(DWORD us)
{
	DWORD dwCount;
	DWORD dwTotalNsPerCycle;

	dwTotalNsPerCycle = 1000000000 / dwICLKValue; // Calculate ns per cycle
	if (dwTotalNsPerCycle < (1000000000 / MAX_OPERATING_FREQUENCY))
	{
		// Fail-safe when exceeding 48MHz
		dwTotalNsPerCycle = 21;	
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
	while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_READ_MODE)
	{
		// Do nothing
		;
	}
	
	// Detect target area is Code Flash and Config Area
	if (adr < FENTRYR_DATAF_START_ADDR)
	{
		if((CONFIG_START_ADDR <= adr) && (adr < FENTRYR_DATAF_START_ADDR))
		{
			// When rewriting the Config Area, set the DFLEN bit to 1
			M8(MCU_DFLCTL_ADDR) = DFLCTL_DFLEN_ENA;
		}
		
		// Enter Code Flash PE Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_CODE_PE_MODE;

		// Code Flash
		byFpmcr = FPMCR_FMS_CODEPE;
		SpecificSequence(byFpmcr);
		// Wait for tDIS
		Wait_us(2);
		// Set the Start PE address of Code flash
		*adrRet = adr & CODEF_PE_MASK_ADDR;
		// Wait until successfully entering Code Flash PE Mode
		while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_CODE_PE_MODE)
		{
			// Do nothing
			;
		}
	} else
	{
		M8(MCU_DFLCTL_ADDR) = DFLCTL_DFLEN_ENA;
		// Enter Data Flash PE Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_DATA_PE_MODE;
		// Wait for tDSTOP(Wait 250ns or more)
		Wait_us(1);
		byFpmcr = FPMCR_FMS_DATAPE;
		SpecificSequence(byFpmcr);
		// Wait for tDIS
		Wait_us(2);
		// Set the Start PE address of Data flash
		*adrRet = (0xFE000000) | (adr & DATAF_PE_MASK_ADDR);
		// Wait until successfully entering Data Flash PE Mode
		while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_DATA_PE_MODE)
		{
			// Do nothing
			;
		}
	}
	
	if((CONFIG_START_ADDR <= adr) && (adr < FENTRYR_DATAF_START_ADDR))
	{
		M8(MCU_FASR_ADDR) = FASR_EXS_EXTRA;
	} else
	{
		M8(MCU_FASR_ADDR) = FASR_EXS_USR;
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
static void SetReadMode(void)
{

	//ReadMode
	SpecificSequence(FPMCR_FMS_RPDIS | FPMCR_FMS_READMODE);
	//Wait fot tMS
	Wait_us(15);
	//FENTRYR
	M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;
	//FENTRYR=0
	while(M16(MCU_FENTRYR_ADDR))
	{
		// Do nothing
		;
	}

	return;
}


/*
 *  Check internal errors
 *    Parameter:      none
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE checkInternalError(void)
{
	// Check for ILGLERR
	if (M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK)
	{
		return (RES_ERROR);
	}
	
	return (RES_OK); // Finished without Errors
}

/*
 *  Extract some bits from input
 *    Parameter:      dwValue:	Input value
 *                    startBit:	Start bit
 *                    bitCount:	Number of bits to get
 *    Return Value:   extracted bits
 */
static DWORD extractBits(DWORD dwValue, unsigned int startBit, unsigned int bitCount)
{
	return (((1 << bitCount) - 1) & (dwValue >> ((startBit + 1) - 1)));
}