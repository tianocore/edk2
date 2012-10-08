/** @file   
  ACPI debug port 2 table definition, defined at 
  Microsoft DebugPort2Specification.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/


#ifndef _DEBUG_PORT_2_TABLE_H_
#define _DEBUG_PORT_2_TABLE_H_

#include <IndustryStandard/Acpi.h>

//
// Ensure proper structure formats
//
#pragma pack(1)

//
// Debug Device Information structure.
//
typedef struct {
  UINT8                                     Revision;
  UINT16                                    Length;
  UINT8                                     NumberofGenericAddressRegisters;
  UINT16                                    NameSpaceStringLength;
  UINT16                                    NameSpaceStringOffset;
  UINT16                                    OemDataLength;
  UINT16                                    OemDataOffset;
  UINT16                                    PortType;
  UINT16                                    PortSubtype;
  UINT8                                     Reserved[2];
  UINT16                                    BaseAddressRegisterOffset;
  UINT16                                    AddressSizeOffset;
} EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT;

#define EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION      0x00

#define EFI_ACPI_DBG2_PORT_TYPE_SERIAL                                                 0x8000
#define   EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550                                 0x0000
#define   EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_SUBSET_COMPATIBLE_WITH_MS_DBGP_SPEC  0x0001
#define EFI_ACPI_DBG2_PORT_TYPE_1394                                                   0x8001
#define   EFI_ACPI_DBG2_PORT_SUBTYPE_1394_STANDARD                                     0x0000
#define EFI_ACPI_DBG2_PORT_TYPE_USB                                                    0x8002
#define   EFI_ACPI_DBG2_PORT_SUBTYPE_USB_XHCI                                          0x0000
#define   EFI_ACPI_DBG2_PORT_SUBTYPE_USB_EHCI                                          0x0001
#define EFI_ACPI_DBG2_PORT_TYPE_NET                                                    0x8003

//
// Debug Port 2 Table definition.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER               Header;
  UINT32                                    OffsetDbgDeviceInfo;
  UINT32                                    NumberDbgDeviceInfo;
} EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE;

#pragma pack()

//
// DBG2 Revision (defined in spec)
//
#define EFI_ACPI_DEBUG_PORT_2_TABLE_REVISION      0x00

#endif
