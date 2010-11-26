/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    AcpiDescription.h
    
Abstract:


   GUIDs used for ACPI Description

--*/

#ifndef _EFI_ACPI_DESCRIPTION_H_
#define _EFI_ACPI_DESCRIPTION_H_

#define EFI_ACPI_DESCRIPTION_GUID \
  { \
    0x3c699197, 0x93c, 0x4c69, {0xb0, 0x6b, 0x12, 0x8a, 0xe3, 0x48, 0x1d, 0xc9} \
  }

#pragma pack(1)
typedef struct {
  UINT8   AddressSpaceId;
  UINT8   RegisterBitWidth;
  UINT8   RegisterBitOffset;
  UINT8   AccessSize;
  UINT64  Address;
} EFI_ACPI_GENERIC_ADDRESS_STRUCTURE;

#define ACPI_ADDRESS_ID_MEMORY    0
#define ACPI_ADDRESS_ID_IO        1
#define ACPI_ADDRESS_ID_PCI       2
#define ACPI_ADDRESS_ID_EC        3
#define ACPI_ADDRESS_ID_SMBUS     4

#define ACPI_ADDRESS_ACCESS_ANY   0
#define ACPI_ADDRESS_ACCESS_BYTE  1
#define ACPI_ADDRESS_ACCESS_WORD  2
#define ACPI_ADDRESS_ACCESS_DWORD 3
#define ACPI_ADDRESS_ACCESS_QWORD 4

//
// Following structure defines ACPI Description information.
// This information is platform specific, may be consumed by DXE generic driver.
//
typedef struct _EFI_ACPI_DESCRIPTION {
  //
  // For Timer
  //
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM_TMR_BLK;
  UINT8                                    PM_TMR_LEN;
  UINT8                                    TMR_VAL_EXT;

  //
  // For RTC
  //
  UINT8                                    DAY_ALRM;
  UINT8                                    MON_ALRM;
  UINT8                                    CENTURY;

  //
  // For Reset
  //
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       RESET_REG;
  UINT8                                    RESET_VALUE;

  //
  // For Shutdown
  //
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM1a_EVT_BLK;
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM1b_EVT_BLK;
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM1a_CNT_BLK;
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM1b_CNT_BLK;
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       PM2_CNT_BLK;
  UINT8                                    PM1_EVT_LEN;
  UINT8                                    PM1_CNT_LEN;
  UINT8                                    PM2_CNT_LEN;
  UINT8                                    SLP_TYPa;
  UINT8                                    SLP_TYPb;

  //
  // For sleep
  //
  UINT8                                    SLP1_TYPa;
  UINT8                                    SLP1_TYPb;
  UINT8                                    SLP2_TYPa;
  UINT8                                    SLP2_TYPb;
  UINT8                                    SLP3_TYPa;
  UINT8                                    SLP3_TYPb;
  UINT8                                    SLP4_TYPa;
  UINT8                                    SLP4_TYPb;

  //
  // GPE
  //
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       GPE0_BLK;
  EFI_ACPI_GENERIC_ADDRESS_STRUCTURE       GPE1_BLK;
  UINT8                                    GPE0_BLK_LEN;
  UINT8                                    GPE1_BLK_LEN;
  UINT8                                    GPE1_BASE;

  //
  // IAPC Boot Arch
  //
  UINT16                                   IAPC_BOOT_ARCH;

  //
  // Flags
  //
  UINT32                                   Flags;

} EFI_ACPI_DESCRIPTION;
#pragma pack()

extern EFI_GUID gEfiAcpiDescriptionGuid;

#endif
