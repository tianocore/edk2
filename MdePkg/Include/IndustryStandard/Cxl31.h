/** @file
  CXL 3.1 definitions

  This file contains the register definitions and firmware interface based
  on the Compute Express Link (CXL) Specification Revision 3.1.

  Copyright (c) 2024, Phytium Technology Co Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Compute Express Link (CXL) Specification Revision 3.1.
    (https://computeexpresslink.org/cxl-specification/)

**/

#pragma once

#include <IndustryStandard/Cxl30.h>

//
// "CEDT" CXL Early Discovery Table
// Compute Express Link Specification Revision 3.1  - Chapter 9.18.1
//
#define CXL_EARLY_DISCOVERY_TABLE_REVISION_02  0x2

#define CEDT_TYPE_CSDS  0x4

//
// Ensure proper structure formats
//
#pragma pack(1)

//
// DVSEC CXL Capability3 (Offset 38h)
// Compute Express Link Specification Revision 3.1  - Chapter 8.1.3.9
//
typedef union {
  struct {
    UINT16    DefaultVolatileHdmStateAfterColdReset        : 1;  // bit 0
    UINT16    DefaultVolatileHdmStateAfterWarmReset        : 1;  // bit 1
    UINT16    DefaultVolatileHdmStateAfterHotReset         : 1;  // bit 2
    UINT16    VolatileHdmStateAfterHotResetConfigurability : 1;  // bit 3
    UINT16    DirectP2pMemCapable                          : 1;  // bit 4
    UINT16    Reserved                                     : 11; // bit 5..15
  } Bits;
  UINT16    Uint16;
} CXL_DVSEC_CXL_DEVICE_CAPABILITY3;

//
// Definition for CXL System Description Structure (CSDS)
// Compute Express Link Specification Revision 3.1  - Chapter 9.18.6
//
typedef struct {
  CEDT_STRUCTURE    Header;
  UINT16            Capabilities;
  UINT16            Reserved;
} CXL_DOWNSTREAM_PORT_ASSOCIATION_STRUCTURE;

#pragma pack()
