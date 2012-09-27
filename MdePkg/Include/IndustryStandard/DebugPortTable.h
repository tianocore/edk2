/** @file   
  ACPI debug port table definition, defined at 
  Microsoft DebugPortSpecification.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/


#ifndef _DEBUG_PORT_TABLE_H_
#define _DEBUG_PORT_TABLE_H_

#include <IndustryStandard/Acpi.h>

//
// Ensure proper structure formats
//
#pragma pack(1)

//
// Debug Port Table definition.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER               Header;
  UINT8                                     InterfaceType;
  UINT8                                     Reserved_37[3];
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE    BaseAddress;
} EFI_ACPI_DEBUG_PORT_DESCRIPTION_TABLE;

#pragma pack()

//
// DBGP Revision (defined in spec)
//
#define EFI_ACPI_DEBUG_PORT_TABLE_REVISION      0x01

//
// Interface Type
//
#define EFI_ACPI_DBGP_INTERFACE_TYPE_FULL_16550                                 0
#define EFI_ACPI_DBGP_INTERFACE_TYPE_16550_SUBSET_COMPATIBLE_WITH_MS_DBGP_SPEC  1

#endif
