
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  Valleyview.h

Abstract:

  This header file provides common definitions just for Valleyview-SOC using to avoid including extra module's file.
--*/

#ifndef _MC_H_INCLUDED_
#define _MC_H_INCLUDED_
/*
< Extended Configuration Base Address.*/
#define EC_BASE             0xE0000000

//
// DEVICE 0 (Memroy Controller Hub)
//
#define MC_BUS          0x00
#define MC_DEV          0x00
#define MC_DEV2         0x02
#define MC_FUN          0x00
// NC DEV 0 Vendor and Device IDs
#define MC_VID          0x8086
#define MC_DID_OFFSET   0x2         //Device Identification
#define MC_GGC_OFFSET   0x50        //GMCH Graphics Control Register

//
// Device 2 Register Equates
//
#define IGD_BUS             0x00
#define IGD_DEV             0x02
#define IGD_FUN_0           0x00
#define IGD_FUN_1           0x01
#define IGD_DEV_FUN         (IGD_DEV << 3)
#define IGD_BUS_DEV_FUN     (MC_BUS << 8) + IGD_DEV_FUN
#define IGD_VID             0x8086
#define IGD_DID             0xA001
#define IGD_MGGC_OFFSET     0x0050      //GMCH Graphics Control Register 0x50
#define IGD_BSM_OFFSET      0x005C      //Base of Stolen Memory
#define IGD_SWSCI_OFFSET    0x00E0      //Software SCI 0xE0 2
#define IGD_ASLE_OFFSET     0x00E4      //System Display Event Register 0xE4 4
#define IGD_ASLS_OFFSET     0x00FC      // ASL Storage
#define IGD_DID_QS          0x0BE2      //RCOverride -a: Fix the DID error

#endif
