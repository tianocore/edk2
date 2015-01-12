/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  BoardIdDecode.h

Abstract:

  Header file for Platform Initialization Driver.

Revision History

++*/

//
// Board AA# and Board ID
//
#define DEFAULT_BOARD_AA                  "FFFFFF"

//
// Mount Washington (LVDS) LOEM
//
#define MW_ITX_MPCIE_LVDS_LOEM_AA         "E93081"
#define MW_ITX_MPCIE_LVDS_LOEM_ID         3

//
// Mount Washington (LVDS) Channel
//
#define MW_ITX_MPCIE_LVDS_CHANNEL_AA      "E93080"
#define MW_ITX_MPCIE_LVDS_CHANNEL_ID      3

//
// Mount Washington Channel
//
#define MW_ITX_MPCIE_CHANNEL_AA           "E93082"
#define MW_ITX_MPCIE_CHANNEL_ID           1

//
// Kinston (mPCIe + LVDS) LOEM
//
#define KT_ITX_MPCIE_LVDS_LOEM_AA         "E93085"
#define KT_ITX_MPCIE_LVDS_LOEM_ID         2

//
// Kinston (LVDS) Channel
//
#define KT_ITX_CHANNEL_AA                 "E93083"
#define KT_ITX_CHANNEL_ID                 0

//
// Kinston LOEM
//
#define KT_ITX_LOEM_AA                    "E93084"
#define KT_ITX_LOEM_ID                    0

