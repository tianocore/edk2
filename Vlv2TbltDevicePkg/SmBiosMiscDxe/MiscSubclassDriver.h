/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscSubclassDriver.h

Abstract:

  Header file for MiscSubclass Driver.


**/

#ifndef _MISC_SUBCLASS_DRIVER_H
#define _MISC_SUBCLASS_DRIVER_H


#include "CommonHeader.h"

extern UINT8  MiscSubclassStrings[];


#define T14_FVI_STRING          "Driver/firmware version"
#define EFI_SMBIOS_TYPE_FIRMWARE_VERSION_INFO 0x90
#define EFI_SMBIOS_TYPE_MISC_VERSION_INFO 0x94
#define TOUCH_ACPI_ID    "I2C05\\SFFFF\\400K"
#define TOUCH_RESET_GPIO_MMIO  0xFED0C508
#define EFI_SMBIOS_TYPE_SEC_INFO 0x83
#define IntelIdentifer 0x6F725076

//
// Data table entry update function.
//
typedef EFI_STATUS (EFIAPI EFI_MISC_SMBIOS_DATA_FUNCTION) (
  IN  VOID                 *RecordData,
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

//
// Data table entry definition.
//
typedef struct {
  //
  // intermediat input data for SMBIOS record
  //
  VOID                              *RecordData;
  EFI_MISC_SMBIOS_DATA_FUNCTION     *Function;
} EFI_MISC_SMBIOS_DATA_TABLE;

//
// Data Table extern definitions.
//
#define MISC_SMBIOS_TABLE_EXTERNS(NAME1, NAME2, NAME3) \
extern NAME1 NAME2 ## Data; \
extern EFI_MISC_SMBIOS_DATA_FUNCTION NAME3 ## Function


//
// Data Table entries
//
#define MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(NAME1, NAME2) \
{ \
  & NAME1 ## Data, \
  & NAME2 ## Function \
}

//
// Global definition macros.
//
#define MISC_SMBIOS_TABLE_DATA(NAME1, NAME2) \
  NAME1 NAME2 ## Data

#define MISC_SMBIOS_TABLE_FUNCTION(NAME2) \
  EFI_STATUS EFIAPI NAME2 ## Function( \
  IN  VOID                  *RecordData, \
  IN  EFI_SMBIOS_PROTOCOL   *Smbios \
  )

#pragma pack(1)

//
// This is definition for SMBIOS Oem data type 0x90
//
typedef struct {
  STRING_REF                         SECVersion;
  STRING_REF                         uCodeVersion;
  STRING_REF                         GOPVersion;
  STRING_REF                         CpuStepping;
} EFI_MISC_OEM_TYPE_0x90;

//
// This is definition for SMBIOS Oem data type 0x90
//
typedef struct {
  SMBIOS_STRUCTURE          Hdr;
  SMBIOS_TABLE_STRING       SECVersion;
  SMBIOS_TABLE_STRING       uCodeVersion;
  SMBIOS_TABLE_STRING       GOPVersion;
  SMBIOS_TABLE_STRING       CpuStepping;
} SMBIOS_TABLE_TYPE90;

typedef struct {
  STRING_REF                GopVersion;
  STRING_REF                UCodeVersion;
  STRING_REF                MRCVersion;
  STRING_REF                SECVersion;
  STRING_REF                ULPMCVersion;
  STRING_REF                PMCVersion;
  STRING_REF                PUnitVersion;
  STRING_REF                SoCVersion;
  STRING_REF                BoardVersion;
  STRING_REF                FabVersion;
  STRING_REF                CPUFlavor;
  STRING_REF                BiosVersion;
  STRING_REF                PmicVersion;
  STRING_REF                TouchVersion;
  STRING_REF                SecureBoot;
  STRING_REF                BootMode;
  STRING_REF                SpeedStepMode;
  STRING_REF                CPUTurboMode;
  STRING_REF                MaxCState;
  STRING_REF                GfxTurbo;
  STRING_REF                IdleReserve;
  STRING_REF                RC6;
}EFI_MISC_OEM_TYPE_0x94;

typedef struct {
  SMBIOS_STRUCTURE          Hdr;
  SMBIOS_TABLE_STRING       GopVersion;
  SMBIOS_TABLE_STRING       uCodeVersion;
  SMBIOS_TABLE_STRING       MRCVersion;
  SMBIOS_TABLE_STRING       SECVersion;
  SMBIOS_TABLE_STRING       ULPMCVersion;
  SMBIOS_TABLE_STRING       PMCVersion;
  SMBIOS_TABLE_STRING       PUnitVersion;
  SMBIOS_TABLE_STRING       SoCVersion;
  SMBIOS_TABLE_STRING       BoardVersion;
  SMBIOS_TABLE_STRING       FabVersion;
  SMBIOS_TABLE_STRING       CPUFlavor;
  SMBIOS_TABLE_STRING       BiosVersion;
  SMBIOS_TABLE_STRING       PmicVersion;
  SMBIOS_TABLE_STRING       TouchVersion;
  SMBIOS_TABLE_STRING       SecureBoot;
  SMBIOS_TABLE_STRING       BootMode;
  SMBIOS_TABLE_STRING       SpeedStepMode;
  SMBIOS_TABLE_STRING       CPUTurboMode;
  SMBIOS_TABLE_STRING       MaxCState;
  SMBIOS_TABLE_STRING       GfxTurbo;
  SMBIOS_TABLE_STRING       IdleReserve;
  SMBIOS_TABLE_STRING       RC6;
}SMBIOS_TABLE_TYPE94;

#pragma pack()
//
// Data Table Array
//
extern EFI_MISC_SMBIOS_DATA_TABLE mMiscSubclassDataTable[];

//
// Data Table Array Entries
//
extern UINTN                        mMiscSubclassDataTableEntries;
extern EFI_HII_HANDLE               mHiiHandle;

//
// Prototypes
//
EFI_STATUS
EFIAPI
MiscSubclassDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

EFI_STRING
EFIAPI
SmbiosMiscGetString (
  IN EFI_STRING_ID   StringId
  );

#endif
