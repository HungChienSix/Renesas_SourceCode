/**************************************************************************//**
 * @file     FlashDev.c
 * @brief    Flash Device Description for RA8M2
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
#include "../FlashOS.h"        // FlashOS Structures

#ifdef RA8M2_512K
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 512KB Code MRAM",    // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x00080000,                 // Device Size in Bytes (512KB)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00008000, 0x00000000,     // Sector Size 32kB (16 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M2_1M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 1M Code MRAM",       // Device Name 
   ONCHIP,                     // Device Type
   0x02000000,                 // Device Start Address
   0x00100000,                 // Device Size in Bytes (1MB)
   8192,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00008000, 0x00000000,     // Sector Size 32kB (32 Sectors)
   SECTOR_END
};
#endif

#ifdef RA8M2_CONF
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 Extra MRAM Config area",        // Device Name 
   ONCHIP,                     // Device Type
   0x02C9F040,                 // Device Start Address
   0x000007C0,                 // Device Size in Bytes (1984B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
    0x00000010, 0x00000000,     // Sector Size  16B (124 Sectors)
    SECTOR_END
};
#endif

#ifdef RA8M2_OTP
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 Extra MRAM OTP area",// Device Name 
   ONCHIP,                     // Device Type
   0x02E07600,                 // Device Start Address
   0x00010334,                 // Device Size in Bytes (66356B)
   16,                         // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   100,                        // Program Page Timeout 100 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
    0x00000010, 0x00000000,     // Sector Size  16B (4148 Sectors)
    SECTOR_END
};
#endif

#ifdef RA8M2_SiP_8M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 8M SiP",             // Device Name 
   ONCHIP,                     // Device Type
   0x08000000,                 // Device Start Address
   0x00800000,                 // Device Size in Bytes (8MB)
   4096,                       // Programming Page Size (4KB)
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00001000, 0x00000000,     // Sector Size 4KB
   SECTOR_END
};
#endif

#ifdef RA8M2_SiP_4M
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "RA8M2 4M SiP",             // Device Name 
   ONCHIP,                     // Device Type
   0x08000000,                 // Device Start Address
   0x00400000,                 // Device Size in Bytes (8MB)
   4096,                       // Programming Page Size (4KB)
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   1000,                       // Program Page Timeout 1000 mSec
   3000,                       // Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
   0x00001000, 0x00000000,     // Sector Size 4KB
   SECTOR_END
};
#endif