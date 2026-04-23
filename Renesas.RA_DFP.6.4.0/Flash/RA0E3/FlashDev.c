/**************************************************************************//**
 * @file     FlashDev.c
 * @brief    Flash Device Description for RA0E3
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
 
#include "../FlashOS.h"						// FlashOS Structures

#ifdef RA0E3_16K
struct FlashDevice const FlashDevice  =  {
	FLASH_DRV_VERS,								// Driver Version, do not modify!
	"RA0E3 16KB Flash",						// Device Name 
	ONCHIP	,											// Device Type
	0x00000000,										// Device Start Address
	0x00004000,										// Device Size in Bytes (16KB)
	128,													// Programming Page Size (64 Bytes)
	0,														// Reserved, must be 0
	0xFF,													// Initial Content of Erased Memory
	1000,													// Program Page Timeout 1000 mSec
	3000,													// Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	0x00000800, 0x00000000,						// Sector Size 2KB (8 Sectors)
	SECTOR_END
};
#endif

#ifdef RA0E3_CONF
struct FlashDevice const FlashDevice  =  {
	FLASH_DRV_VERS,								// Driver Version, do not modify!
	"RA0E3 Config Area",					// Device Name 
	ONCHIP,												// Device Type
	0x01010000,										// Device Start Address
	0x00000034,										// Device Size in Bytes (52 Bytes)
	4,														// Programming Page Size (4 Bytes)
	0,														// Reserved, must be 0
	0xFF,													// Initial Content of Erased Memory
	1000,													// Program Page Timeout 100 mSec
	3000,													// Erase Sector Timeout 3000 mSec

// Specify Size and Address of Sectors
	0x00000004, 0x00000000,					// Sector Size 4 Bytes (13 Sectors)
	SECTOR_END
};
#endif
