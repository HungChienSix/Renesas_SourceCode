/**************************************************************************//**
 * @file     FlashDev.c
 * @brief    Flash Device Description for RA8M1
 * @version  V1.0.3
 * @date     14 May 2024
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
 *         : 31.05.2023 1.0.0    First Release
 *				 : 06.02.2024	1.0.1    Increase Programming Page Size
 *         : 28.03.2024 1.0.2    Modify Programming Page Size (Code/Data 8KB/64B)
 *				 : 14.05.2024 1.0.3		 Modify Data Configuration Area Address/Size 
 *********************************************************************************************************************/
#include "../FlashOS.h"        // FlashOS Structures

#ifdef RA8M1_1M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 1M Flash",           // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x00100000,                 // Device Size in Bytes (1MB)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (30 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_1M_DL
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 1M Flash(dual lower side)",   // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x00080000,                 // Device Size in Bytes (1MB/2)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (14 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_1M_DU
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 1M Flash(dual upper side)",   // Device Name 
   ONCHIP,                     // Device Type
   0x02200000,                 // Device Start Address
   0x00080000,                 // Device Size in Bytes (1MB/2)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	 0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (14 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_2M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 2M Flash",           // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x001F8000,                 // Device Size in Bytes (2MB)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (61 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_2M_DL
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 2M Flash(dual lower side)",   // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x000F8000,                 // Device Size in Bytes (2MB/2)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (29 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_2M_DU
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 2M Flash(dual upper side)",   // Device Name 
   ONCHIP,                     // Device Type
   0x02200000,                 // Device Start Address
   0x000F8000,                 // Device Size in Bytes (2MB/2)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	 0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (29 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_DATA
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 12KB DataFlash",     // Device Name 
   ONCHIP,                     // Device Type
   0x27000000,                 // Device Start Address
   0x00003000,                 // Device Size in Bytes (12KB)
   64,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00000040, 0x00000000,     // Sector Size  64B (192 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M1_CCONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 Code flash Config area",        // Device Name 
   ONCHIP,                     // Device Type
   0x0300A100,                 // Device Start Address
   0x00000200,                 // Device Size in Bytes (512B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	0x00000010, 0x00000000,     // Sector Size  16B (32 Sectors)
	SECTOR_END
};
#endif

#ifdef RA8M1_DCONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M1 Data flash Config area",        // Device Name 
   ONCHIP,                     // Device Type
   0x27030080,                 // Device Start Address
   0x000002E0,                 // Device Size in Bytes (736B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	0x00000010, 0x00000000,     // Sector Size  16B (59 Sectors)
	SECTOR_END
};
#endif
