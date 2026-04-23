/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for RA0E3
 * @version  V1.0.0
 * @date     18 Jun 2025
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
 * Copyright (C) 2025 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 18.06.2025 1.0.0    First Release
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
   Flash related registers adrress and bit definition
*/
#define MCU_FLASH_BASE                 (0x407EC000)
#define MCU_FENTRYR_ADDR               (MCU_FLASH_BASE + 0x21A)
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
		#define FCR_CMD_MASK	             (0x07)
		#define FCR_STOP_MASK	             (0x40)
		#define FCR_OPST_MASK	             (0x80)
#define MCU_FSARH_ADDR	               (MCU_FLASH_BASE + 0x110)
#define MCU_FSARL_ADDR	               (MCU_FLASH_BASE + 0x108)
#define MCU_FEARH_ADDR	               (MCU_FLASH_BASE + 0x120)
#define MCU_FEARL_ADDR	               (MCU_FLASH_BASE + 0x118)
#define MCU_FWBL0_ADDR	               (MCU_FLASH_BASE + 0x130)
#define MCU_FWBH0_ADDR	               (MCU_FLASH_BASE + 0x138)
#define MCU_FSTATR2_ADDR               (MCU_FLASH_BASE + 0x1F0)
		#define FSTATR2_ERERR_MASK         (0x1)
		#define FSTATR2_PRGERR_MASK        (0x2)
		#define FSTATR2_PRGERR01_MASK      (0x4)
		#define FSTATR2_BCERR_MASK 	       (0x8)
		#define FSTATR2_ILGLERR_MASK       (0x10)
		#define FSTATR2_EILGLERR_MASK      (0x20)
#define MCU_FSTATR1_ADDR               (MCU_FLASH_BASE + 0x12C)
		#define FSTATR1_FRDY_MASK	       	 (0x40)
		#define FSTATR1_EXRDY_MASK         (0x80)
#define MCU_FPR_ADDR		               (MCU_FLASH_BASE + 0x180)
	  #define FPR_UNLOCK		             (0xA5)
#define MCU_FPMCR_ADDR								 (MCU_FLASH_BASE + 0x100)
  	#define FPMCR_FMS_FMS0						 (0x02)
  	#define FPMCR_FMS_FMS1						 (0x10)
  	#define FPMCR_FMS_FMS							 (0x12)
	  #define FPMCR_FMS_READMODE				 (0x00)
  	#define FPMCR_FMS_CODEPE					 (0x02)
  	#define FPMCR_FMS_RPDIS						 (0x08)
  	#define FPMCR_FMS_RPDIS_DIS				 (0x08)
	  #define FPMCR_FMS_RPDIS_ENA				 (0x00)
#define MCU_FASR_ADDR									 (MCU_FLASH_BASE + 0x104)
	  #define FASR_EXS_USR							 (0x00)
  	#define FASR_EXS_EXTRA						 (0x01)
#define MCU_FEXCR_ADDR		         		 (MCU_FLASH_BASE + 0x1DC)
  	#define FEXCR_CMD				           (0x07)
  	#define FEXCR_CMD_ACCESS_WINDOW	   (0x02)
		#define FEXCR_CMD_STARTUP_AREA_SELCT		(0x02)
	  #define FEXCR_CMD_OCDID1	         (0x03)
  	#define FEXCR_CMD_OCDID2	         (0x04)
  	#define FEXCR_CMD_OCDID3	         (0x05)
	  #define FEXCR_CMD_OCDID4	         (0x06)
	  #define FEXCR_OPST_START	         (0x80)
	  #define FEXCR_OPST_STOP		         (0x00)
/*
   Other parameters
*/
#define RES_ERROR												(1)
#define RES_OK													(0)
#define CODEF_PE_MASK_ADDR							(0x0000FFFF)
#define PROGRAMMING_PAGE_SIZE						(128)
#define BLANK_VALUE											(0xFF)
#define SECTOR_SIZE											(0x00000800) // Should re-define this value base on user's marco
#define CODEF_START_ADDR								(0x00000000)
#define CONFIG_START_ADDR								(0x01010000)
#define CODEF_SIZE											(0x00004000) // 16KB
#define MCU_AWS_ADDR		                (CONFIG_START_ADDR + 0x10)
#define MCU_OSIS1_ADDR		              (CONFIG_START_ADDR + 0x18)
#define MCU_OSIS2_ADDR		              (CONFIG_START_ADDR + 0x20)
#define MCU_OSIS3_ADDR		              (CONFIG_START_ADDR + 0x28)
#define MCU_OSIS4_ADDR		              (CONFIG_START_ADDR + 0x30)
#define LOWER16(x) 											((WORD)((x) & 0xFFFF))
#define UPPER16(x) 											((WORD)(((x) >> 16) & 0xFFFF))
#define BYTEDATA												(4)

#ifdef RA0E3_16K
	#undef SECTOR_SIZE										// Undef macro first
	#define SECTOR_SIZE										(0x00000800) // 2KB
#endif

#ifdef RA0E3_CONF
	#undef SECTOR_SIZE										// Undef macro first
	#define SECTOR_SIZE										(0x00000004)
#endif
#define TARGET_CONFIG										(0)
#define TARGET_CODEF										(2)

/*
   Type declaration
*/
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

/*
   Internal function prototype
*/
static BYTE enterPEMode(DWORD adr);
static void Wait_15us(void);
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
		BYTE byteUserClockValue;

		clk = 4000000; // System clock using HOCO (32 MHz) with HOCODIV 1/8 after reset.

		// Convert clock from Hz to MHz unit
		byteUserClockValue = clk / 1000000;

		// Enter PE Mode
		enterPEMode(adr);

		// Set Peripheral Clock Notification
		// Because (FlashIF Clock Frequency [MHz]) 1.0 = (PCKA [5:0] Bit Setting) 00000b
		// So we decrease clock value by 1 before convert to bit string.
		// Note: This register can be set only in PE Mode

		M8(MCU_FISR_ADDR) = (M8(MCU_FISR_ADDR) | (byteUserClockValue - 1));

		// Reset all Flash register to apply new ICLK
		M8(MCU_FRESETR_ADDR) = 1;
		M8(MCU_FRESETR_ADDR) = 0;

		// Go to READ Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;

		// Check for ILGLERR
		if (M16(MCU_FSTATR2_ADDR) & FSTATR2_ILGLERR_MASK)
		{
				return (RES_ERROR);
		} 
		
		return (RES_OK);
}

/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */
int UnInit (unsigned long fnc)
{
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
		DWORD TEMP_ADDR;
				
#ifdef RA0E3_CONF
		// Configuration Area does not support Erase
		return (RES_OK);
#endif
		
		// Erase per sector
		for (TEMP_ADDR = CODEF_START_ADDR; TEMP_ADDR < CODEF_START_ADDR + CODEF_SIZE; TEMP_ADDR+= SECTOR_SIZE)
		{
				if (EraseSector(TEMP_ADDR) == 1)
				{
						return (RES_ERROR);
				}
		}
		
		return (RES_OK); // Return status
}

/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */
int EraseSector (unsigned long adr)
{
#ifdef RA0E3_CONF
		// Configuration Area does not support Erase
		return (RES_OK);
#endif

		// Enter PE Mode
		enterPEMode(adr);

		// Set Start address		
		M16(MCU_FSARL_ADDR) = LOWER16(adr); // 16 lower bits
		M16(MCU_FSARH_ADDR) = UPPER16(adr); // 16 higher bits

		// Set End address
		M16(MCU_FEARL_ADDR) = LOWER16(adr + SECTOR_SIZE - 1); // 16 lower bits
		M16(MCU_FEARH_ADDR) = UPPER16(adr + SECTOR_SIZE - 1); // 16 higher bits

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

		// Enter PE Mode
		enterPEMode(adr);
		
		// Loop until end of data
		do {
				if (adr < CONFIG_START_ADDR){
						// Set programming address
						M16(MCU_FSARL_ADDR) = LOWER16(adr + dataIndex); // 16 lower bits
						M16(MCU_FSARH_ADDR) = UPPER16(adr + dataIndex); // 16 higher bits
				}
				
				// Set programming data (Code flash: 4 data, Config Area: 4 data)
				M16(MCU_FWBL0_ADDR) = (buf[dataIndex] | (buf[dataIndex + 1] << 8)); // 16 lower bits
				M16(MCU_FWBH0_ADDR) = (buf[dataIndex + 2] | (buf[dataIndex + 3] << 8)); // 16 higher bits

				if (adr < CONFIG_START_ADDR)
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
				dataIndex += BYTEDATA;
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
		BYTE byte_rd_buf[32];
		BYTE i, j ,k, m = 0;
		
		// Return to READ Mode
		SetReadMode();
		
		for(i = 0; i < sz ; i += 32)
		{
				read_page(adr + m, 32, &byte_rd_buf[0]);

				/** Determine size to compare */
				if ((sz - i) >= 32)
				{
						k = 32;
				} else
				{
						k = (sz - i);     
				}
						
					/** Check up to 32 bytes if equal to read data */
					for (j = 0; j < k; j++)
					{
							if (byte_rd_buf[j] != buf[m])
							{
									return adr + m; // The written data and the actually written flash contents do not match	
							}
							m++;
					}
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
    BYTE i;

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
#ifdef RA0E3_CONF
		// Configuration Area does not support Erase
		return (RES_OK);
#endif

		// Enter PE Mode
		enterPEMode(adr);
		
		// Set Start address
		M16(MCU_FSARL_ADDR) = LOWER16(adr); // 16 lower bits
		M16(MCU_FSARH_ADDR) = UPPER16(adr); // 16 higher bits
		
		// Set End address
		M16(MCU_FEARL_ADDR) = LOWER16(adr + sz - 1); // 16 lower bits
		M16(MCU_FEARH_ADDR) = UPPER16(adr + sz - 1); // 16 higher bits
		
		// Write FCR with 83h
		M8(MCU_FCR_ADDR) = 0x83;

		// Internal BlankCheck exeucting
		// Wait for ready flag (FSTATR1.FRDY = 1)
		while ((M8(MCU_FSTATR1_ADDR) & FSTATR1_FRDY_MASK) != FSTATR1_FRDY_MASK)
		{
				// Do nothing
				;
		}
		
		// Write FCR with 03h
		M8(MCU_FCR_ADDR) = 0x03;

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
 *  Wait processing in us units
 *    Parameter:      us: wait time
 *    Return Value:   none
 *	  Note:			  Please note that the weight processing is less accurate.
 */
static void Wait_15us(void)
{
		volatile BYTE byteCount = 4;

		for ( ; byteCount > 0; byteCount--)
		{
				__asm("NOP");
		}
}

/*
 *  PE Mode entry
 *    Parameter:      adr: Target address area (may be Code flash or Data flash start address)
 *                    adrRet: PE address (as returned value)
 *    Return Value:   0 - OK,  1 - Failed
 */
static BYTE enterPEMode(DWORD adr)
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
		// Enter Code Flash PE Mode
		M16(MCU_FENTRYR_ADDR) = FENTRYR_CODE_PE_MODE;

		// Code Flash
		byFpmcr = FPMCR_FMS_CODEPE;
		SpecificSequence(byFpmcr);
		// Wait for tDIS
		Wait_15us();
		// Wait until successfully entering Code Flash PE Mode
		while (M16(MCU_FENTRYR_ADDR) != FENTRYR_CHK_CODE_PE_MODE)
		{
				// Do nothing
				;
		}
		
		if(CONFIG_START_ADDR <= adr)
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
		Wait_15us();
		//FENTRYR
		M16(MCU_FENTRYR_ADDR) = FENTRYR_READ_MODE;
		//FENTRYR=0
		while(M16(MCU_FENTRYR_ADDR));

		return;
}