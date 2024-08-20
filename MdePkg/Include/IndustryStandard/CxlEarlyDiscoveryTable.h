/** @file
  ACPI CXL Early Discovery Table (CEDT) definitions.

  This file contains the register definitions based on the Compute Express Link
  (CXL) Specification Revision 3.1.

  Copyright (c) 2024, Phytium Technology Co Ltd. All rights reserved.

  @par Reference(s):
    - Compute Express Link (CXL) Specification Revision 3.1.
      (https://computeexpresslink.org/cxl-specification/)
**/

#ifndef CXL_EARLY_DISCOVERY_TABLE_H_
#define CXL_EARLY_DISCOVERY_TABLE_H_

#include <IndustryStandard/Acpi.h>

///
/// "CEDT" CXL Early Discovery Table
///
#define CXL_3_1_CXL_EARLY_DISCOVERY_TABLE_SIGNATURE  SIGNATURE_32 ('C', 'E', 'D', 'T')

#define CXL_EARLY_DISCOVERY_TABLE_REVISION_01  0x1   // CXL2.0 ~ CXL3.0
#define CXL_EARLY_DISCOVERY_TABLE_REVISION_02  0x2   // CXL3.1

#define CEDT_TYPE_CHBS     0x0
#define CEDT_TYPE_CFMWS    0x1
#define CEDT_TYPE_CXIMS    0x2
#define CEDT_TYPE_RDPAS    0x3
#define CEDT_TYPE_CSDS     0x4

#pragma pack(1)

///
/// Table header
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
} CXL_3_1_CXL_EARLY_DISCOVERY_TABLE;

///
/// Node header definition shared by all structure types
///
typedef struct {
  UINT8     Type;
  UINT8     Reserved;
  UINT16    Length;
} CXL_3_1_CEDT_STRUCTURE;

///
/// Definition for CXL Host Bridge Structure (CHBS)
///
typedef struct {
  CXL_3_1_CEDT_STRUCTURE           Header;
  UINT32                           UID;
  UINT32                           CXLVersion;
  UINT32                           Reserved;
  UINT64                           Base;
  UINT64                           Length;
} CXL_3_1_CXL_HOST_BRIDGE_STRUCTURE;

///
/// Definition for CXL Fixed Memory Window Structure (CFMWS)
///
typedef struct {
  CXL_3_1_CEDT_STRUCTURE               Header;
  UINT32                               Reserved;
  UINT64                               BaseHPA;
  UINT64                               WindowSize;
  UINT8                                InterleaveMembers;
  UINT8                                InterleaveArithmetic;
  UINT16                               Reserved1;
  UINT32                               Granularity;
  UINT16                               Restrictions;
  UINT16                               QtgId;
  UINT32                               TargetList[16];
} CXL_3_1_CXL_FIXED_MEMORY_WINDOW_STRUCTURE;

///
/// Definition for CXL XOR Interleave Math Structure (CXIMS)
///
typedef struct {
  CXL_3_1_CEDT_STRUCTURE               Header;
  UINT16                               Reserved;
  UINT8                                HBIG;
  UINT8                                NIB;
  UINT64                               XORMAPLIST[4];
} CXL_3_1_CXL_XOR_INTERLEAVE_MATH_STRUCTURE;

///
/// Definition for RCEC Downstream Port Association Structure (RDPAS)
///
typedef struct {
  CXL_3_1_CEDT_STRUCTURE               Header;
  UINT16                               SegmentNumber;
  UINT16                               BDF;
  UINT8                                ProtocolType;
  UINT64                               BaseAddress;
} CXL_3_1_RCEC_DOWNSTREAM_PORT_ASSOCIATION_STRUCTURE;

///
/// Definition for CXL System Description Structure (CSDS)
///
typedef struct {
  CXL_3_1_CEDT_STRUCTURE               Header;
  UINT16                               Capabilities;
  UINT16                               Reserved;
} CXL_3_1_CXL_DOWNSTREAM_PORT_ASSOCIATION_STRUCTURE;

#pragma pack()

#endif
