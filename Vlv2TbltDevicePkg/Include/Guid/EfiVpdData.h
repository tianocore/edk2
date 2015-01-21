/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  EfiVpdData.h

Abstract:

  Constants and declarations that are common accross PEI and DXE.
--*/

#ifndef _EFI_VPD_DATA_H_
#define _EFI_VPD_DATA_H_


#pragma pack(1)

//
// DMI data
//
typedef struct {

  CHAR8 DmiGpnvHeader[4];             // $DMI
  CHAR8 SystemInfoManufacturer[0x20]; // Structure Type 1 String 1
  CHAR8 SystemInfoProductName[0x20];  // Structure Type 1 String 2
  CHAR8 SystemInfoVersion[0x18];      // Structure Type 1 String 3
  CHAR8 SystemInfoSerialNumber[0x20]; // Structure Type 1 String 4
  CHAR8 BaseBoardManufacturer[0x20];  // Structure Type 2 String 1
  CHAR8 BaseBoardProductName[0x20];   // Structure Type 2 String 2
  CHAR8 BaseBoardVersion[0x18];       // Structure Type 2 String 3
  CHAR8 BaseBoardSerialNumber[0x20];  // Structure Type 2 String 4
  CHAR8 ChassisManufacturer[0x20];    // Structure Type 3 String 1
  UINT8 ChassisType;                  // Enumerated
  CHAR8 ChassisVersion[0x18];         // Structure Type 3 String 2
  CHAR8 ChassisSerialNumber[0x20];    // Structure Type 3 String 3
  CHAR8 ChassisAssetTag[0x20];        // Structure Type 3 String 4
  UINT8 MfgAccessKeyWorkspace;

  UINT8 ChecksumFixupPool[0xd];       // Checksum Fix-ups
  UINT8 SwitchboardData[4];           // 32 switch switchboard
  UINT8 IntelReserved;                // Reserved for Future Use
} DMI_DATA;

#define DMI_DATA_GUID \
  { \
    0x70e56c5e, 0x280c, 0x44b0, 0xa4, 0x97, 0x09, 0x68, 0x1a, 0xbc, 0x37, 0x5e \
  }

#define DMI_DATA_NAME       (L"DmiData")
#define ASCII_DMI_DATA_NAME ("DmiData")

extern EFI_GUID gDmiDataGuid;
extern CHAR16   gDmiDataName[];

//
// UUID - universally unique system id.
//
#define UUID_VARIABLE_GUID \
  { \
    0xd357c710, 0x0ada, 0x4717, 0x8d, 0xba, 0xc6, 0xad, 0xc7, 0xcd, 0x2b, 0x2a \
  }

#define UUID_VARIABLE_NAME        (L"UUID")
#define ASCII_UUID_VARIABLE_NAME  ("UUID")

//
// UUID data
//
typedef struct {
  UINT32        UuidHigh;
  UINT32        UuidLow;
} SYSTEM_1394_UUID;

typedef struct {
  EFI_GUID          SystemUuid;                   // System Unique ID
  SYSTEM_1394_UUID  System1394Uuid;               // Onboard 1394 UUID
} UUID_DATA;

extern EFI_GUID gUuidVariableGuid;
extern CHAR16   gUuidVariableName[];

//
// MB32GUID for Computrace.
//

#define               MB32_GUID \
  { 0x539D62BA, 0xDE35, 0x453E, 0xBA, 0xB0, 0x85, 0xDB, 0x8D, 0xA2, 0x42, 0xF9 }

#define MB32_VARIABLE_NAME (L"MB32")
#define ASCII_MB32_VARIABLE_NAME ("MB32")

extern EFI_GUID gMb32Guid;
extern CHAR16   gMb32VariableName[];

//
// ACPI OSFR Manufacturer String.
//
// {72234213-0FD7-48a1-A59F-B41BC107FBCD}
//
#define  ACPI_OSFR_MFG_STRING_VARIABLE_GUID \
  {0x72234213, 0xfd7, 0x48a1, 0xa5, 0x9f, 0xb4, 0x1b, 0xc1, 0x7, 0xfb, 0xcd}
#define ACPI_OSFR_MFG_STRING_VARIABLE_NAME (L"OcurMfg")
#define ASCII_ACPI_OSFR_MF_STRING_VARIABLE_NAME ("OcurMfg")

extern EFI_GUID gACPIOSFRMfgStringVariableGuid;


//
// ACPI OSFR Model String.
//
// {72234213-0FD7-48a1-A59F-B41BC107FBCD}
//
#define  ACPI_OSFR_MODEL_STRING_VARIABLE_GUID \
  {0x72234213, 0xfd7, 0x48a1, 0xa5, 0x9f, 0xb4, 0x1b, 0xc1, 0x7, 0xfb, 0xcd}
#define ACPI_OSFR_MODEL_STRING_VARIABLE_NAME (L"OcurModel")
#define ASCII_ACPI_OSFR_MODEL_STRING_VARIABLE_NAME ("OcurModel")

extern EFI_GUID gACPIOSFRModelStringVariableGuid;

//
// ACPI OSFR Reference Data Block.
//
// {72234213-0FD7-48a1-A59F-B41BC107FBCD}
//
#define  ACPI_OSFR_REF_DATA_BLOCK_VARIABLE_GUID \
  {0x72234213, 0xfd7, 0x48a1, 0xa5, 0x9f, 0xb4, 0x1b, 0xc1, 0x7, 0xfb, 0xcd}
#define ACPI_OSFR_REF_DATA_BLOCK_VARIABLE_NAME (L"OcurRef")
#define ASCII_ACPI_OSFR_REF_DATA_BLOCK_VARIABLE_NAME ("OcurRef")
extern EFI_GUID gACPIOSFRRefDataBlockVariableGuid;

//
// Manufacturing mode GUID
//
#define MfgMode_GUID \
  { 0xEF14FD78, 0x0793, 0x4e2b, 0xAC, 0x6D, 0x06, 0x28, 0x47, 0xE0, 0x17, 0x91 }

#define MFGMODE_VARIABLE_NAME (L"MfgMode")
#define ASCII_MFGMODE_VARIABLE_NAME ("MfgMode")

typedef struct {
	UINT8 MfgModeData;
} MFG_MODE_VAR;

extern EFI_GUID gMfgModeVariableGuid;
extern CHAR16   gMfgModeVariableName[];

#pragma pack()

#endif
