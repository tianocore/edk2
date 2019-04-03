/** @file
Definition of the global NVS area protocol.  This protocol
publishes the address and format of a global ACPI NVS buffer
used as a communications buffer between SMM code and ASL code.
The format is derived from the ACPI reference code, version 0.95.
Note:  Data structures defined in this protocol are not naturally aligned.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GLOBAL_NVS_AREA_H_
#define _GLOBAL_NVS_AREA_H_

//
// Forward reference for pure ANSI compatability
//

typedef struct _EFI_GLOBAL_NVS_AREA_PROTOCOL EFI_GLOBAL_NVS_AREA_PROTOCOL;

//
// Global NVS Area Protocol GUID
//
#define EFI_GLOBAL_NVS_AREA_PROTOCOL_GUID \
{ 0x74e1e48, 0x8132, 0x47a1, {0x8c, 0x2c, 0x3f, 0x14, 0xad, 0x9a, 0x66, 0xdc} }


//
// Global NVS Area definition
//
#pragma pack (1)
typedef struct {
  //
  // Miscellaneous Dynamic Values
  //
  UINT32      OperatingSystemType;    // Os type indicator
  UINT32      Cfgd;                   // System configuration description
  UINT32      HpetEnable;

  UINT32      Pm1blkIoBaseAddress;
  UINT32      PmbaIoBaseAddress;
  UINT32      Gpe0blkIoBaseAddress;
  UINT32      GbaIoBaseAddress;

  UINT32      SmbaIoBaseAddress;
  UINT32      Reserved1;
  UINT32      WdtbaIoBaseAddress;

  UINT32      HpetBaseAddress;
  UINT32      HpetSize;
  UINT32      PciExpressBaseAddress;
  UINT32      PciExpressSize;

  UINT32      RcbaMmioBaseAddress;
  UINT32      RcbaMmioSize;
  UINT32      IoApicBaseAddress;
  UINT32      IoApicSize;

  UINT32      TpmPresent;
  UINT32      DBG2Present;
  UINT32      PlatformType;           // Set to one of EFI_PLATFORM_TYPE enums.
  UINT32      AlternateSla;           // If TRUE use alternate I2C Slave addresses.

  UINT8       Reserved[512 - 4 * 22]; // Total 512 Bytes
} EFI_GLOBAL_NVS_AREA;
#pragma pack ()

//
// Global NVS Area Protocol
//
struct _EFI_GLOBAL_NVS_AREA_PROTOCOL {
  EFI_GLOBAL_NVS_AREA     *Area;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiGlobalNvsAreaProtocolGuid;

#endif
