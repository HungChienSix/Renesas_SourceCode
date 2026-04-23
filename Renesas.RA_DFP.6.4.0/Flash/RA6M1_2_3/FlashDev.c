/**************************************************************************//**
 * @file     FlashDev.c
 * @brief    Flash Device Description for RA6M1,RA6M2,RA6M3
 * @version  V1.0.2
 * @date     28 Mar 2024
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
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
 /**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 27.07.2020 1.0.0    First Release
 *         : 15.02.2024 1.0.1    Increase Programming Page Size
 *         : 28.03.2024 1.0.2    Modify Programming Page Size (Code/Data 8KB/64B)
 *********************************************************************************************************************/
#include "../FlashOS.h"        // FlashOS Structures

#ifdef RA6M1_512K
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M1 512KB Flash",        // Device Name 
   ONCHIP,                     // Device Type
   0x00000000,                 // Device Start Address
   0x00080000,                 // Device Size in Bytes (512KB)
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

#ifdef RA6M2_512K
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M2 512KB Flash",        // Device Name 
   ONCHIP,                     // Device Type
   0x00000000,                 // Device Start Address
   0x00080000,                 // Device Size in Bytes (512KB)
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

#ifdef RA6M2_1M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M2 1MB Flash",          // Device Name 
   ONCHIP,                     // Device Type
   0x00000000,                 // Device Start Address
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

#ifdef RA6M3_1M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M3 1MB Flash",          // Device Name 
   ONCHIP,                     // Device Type
   0x00000000,                 // Device Start Address
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

#ifdef RA6M3_2M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M3 2MB Flash",          // Device Name 
   ONCHIP,                     // Device Type
   0x00000000,                 // Device Start Address
   0x00200000,                 // Device Size in Bytes (2MB)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00002000, 0x00000000,     // Sector Size  8kB (8 Sectors)
   0x00008000, 0x00010000,     // Sector Size 32kB (62 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M1_CONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M1 Config Area",        // Device Name 
   ONCHIP,                     // Device Type
   0x0100A100,                 // Device Start Address
   0x00000080,                 // Device Size in Bytes (128B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	 0x00000010, 0x00000000,     // Sector Size  16B (8 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M2_CONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M2 Config Area",        // Device Name 
   ONCHIP,                     // Device Type
   0x0100A100,                 // Device Start Address
   0x00000080,                 // Device Size in Bytes (128B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	 0x00000010, 0x00000000,     // Sector Size  16B (8 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M3_CONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M3 Config Area",        // Device Name 
   ONCHIP,                     // Device Type
   0x0100A100,                 // Device Start Address
   0x00000080,                 // Device Size in Bytes (128B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	 0x00000010, 0x00000000,     // Sector Size  16B (8 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M1_DATA
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M1 8KB DataFlash",      // Device Name 
   ONCHIP,                     // Device Type
   0x40100000,                 // Device Start Address
   0x00002000,                 // Device Size in Bytes (8KB)
   64,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00000040, 0x00000000,     // Sector Size  64B (128 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M2_DATA
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M2 32KB DataFlash",     // Device Name 
   ONCHIP,                     // Device Type
   0x40100000,                 // Device Start Address
   0x00008000,                 // Device Size in Bytes (32KB)
   64,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                   		 // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00000040, 0x00000000,     // Sector Size  64B (512 Sectors)
   SECTOR_END
};
#endif

#ifdef RA6M3_DATA
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA6M3 64KB DataFlash",     // Device Name 
   ONCHIP,                     // Device Type
   0x40100000,                 // Device Start Address
   0x00010000,                 // Device Size in Bytes (64KB)
   64,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00000040, 0x00000000,     // Sector Size  64B (1024 Sectors)
   SECTOR_END
};
#endif
