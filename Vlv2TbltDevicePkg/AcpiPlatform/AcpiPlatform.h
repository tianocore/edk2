/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  AcpiPlatform.h

Abstract:

  This is an implementation of the ACPI platform driver.  Requirements for
  this driver are defined in the Tiano ACPI External Product Specification,
  revision 0.3.6.


--*/

#ifndef _ACPI_PLATFORM_H_
#define _ACPI_PLATFORM_H_

//
// Statements that include other header files.
//
#include <FrameworkDxe.h>
#include <PiDxe.h>
#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareVolume.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/MpService.h>
#include <Protocol/CpuIo.h>
#include <IndustryStandard/Acpi30.h>
#include <IndustryStandard/Acpi20.h>
#include <Library/HobLib.h>
#include <AlertStandardFormatTable.h>
#include <Guid/SetupVariable.h>
#include <Protocol/GlobalNvsArea.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <PchRegs.h>
#include <Library/PchPlatformLib.h>
//
// Global variables.
//
EFI_GLOBAL_NVS_AREA_PROTOCOL  mGlobalNvsArea;

//
// ACPI table information used to initialize tables.
#define EFI_ACPI_OEM_REVISION     0x00000003
#define EFI_ACPI_CREATOR_ID       SIGNATURE_32 ('V', 'L', 'V', '2')
#define EFI_ACPI_CREATOR_REVISION 0x0100000D

#define WPCN381U_CONFIG_INDEX     0x2E
#define WPCN381U_CONFIG_DATA      0x2F
#define WPCN381U_CHIP_ID          0xF4
#define WDCP376_CHIP_ID           0xF1

#define MOBILE_PLATFORM 1
#define DESKTOP_PLATFORM 2

//
// Define macros to build data structure signatures from characters.
//
#ifndef EFI_SIGNATURE_16
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#endif
#ifndef EFI_SIGNATURE_32
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#endif
#ifndef EFI_SIGNATURE_64
#define EFI_SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (EFI_SIGNATURE_32 (A, B, C, D) | ((UINT64) (EFI_SIGNATURE_32 (E, F, G, H)) << 32))
#endif


#define GV3_SSDT_OEM_TABLE_IDBASE 0x4000

//
// Private Driver Data.
//
//
// Define Union of IO APIC & Local APIC structure.
//
typedef union {
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE AcpiLocalApic;
  EFI_ACPI_2_0_IO_APIC_STRUCTURE              AcpiIoApic;
  struct {
    UINT8 Type;
    UINT8 Length;
  } AcpiApicCommon;
} ACPI_APIC_STRUCTURE_PTR;

//
// Protocol private structure definition.
//

/**
  Entry point of the ACPI platform driver.

  @param[in]  ImageHandle        EFI_HANDLE: A handle for the image that is initializing this driver.
  @param[in]  SystemTable        EFI_SYSTEM_TABLE: A pointer to the EFI system table.

  @retval  EFI_SUCCESS           Driver initialized successfully.
  @retval  EFI_LOAD_ERROR        Failed to Initialize or has been loaded.
  @retval  EFI_OUT_OF_RESOURCES  Could not allocate needed resources.

**/
EFI_STATUS
InstallAcpiPlatform (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

/**
  Get Acpi Table Version.

  @param[in]  ImageHandle          EFI_HANDLE: A handle for the image that is initializing this driver.
  @param[in]  SystemTable          EFI_SYSTEM_TABLE: A pointer to the EFI system table.

  @retval  EFI_SUCCESS:            Driver initialized successfully.
  @retval  EFI_LOAD_ERROR:         Failed to Initialize or has been loaded.
  @retval  EFI_OUT_OF_RESOURCES:   Could not allocate needed resources.

--*/
EFI_ACPI_TABLE_VERSION
GetAcpiTableVersion (
  VOID
  );

/**
  The funtion returns Oem specific information of Acpi Platform.

  @param[in]  OemId          OemId returned.
  @param[in]  OemTableId     OemTableId returned.
  @param[in]  OemRevision    OemRevision returned.

  @retval  EFI_STATUS        Status of function execution.

**/
EFI_STATUS
AcpiPlatformGetOemFields (
  OUT UINT8   *OemId,
  OUT UINT64  *OemTableId,
  OUT UINT32  *OemRevision
  );

/**
  The function returns Acpi table version.

  @param[in]

  @retval  EFI_ACPI_TABLE_VERSION   Acpi table version encoded as a UINT32.

**/
EFI_ACPI_TABLE_VERSION
AcpiPlatformGetAcpiSetting (
  VOID
  );

/**
  Entry point for Acpi platform driver.

  @param[in]  ImageHandle        A handle for the image that is initializing this driver.
  @param[in]  SystemTable        A pointer to the EFI system table.

  @retval  EFI_SUCCESS           Driver initialized successfully.
  @retval  EFI_LOAD_ERROR        Failed to Initialize or has been loaded.
  @retval  EFI_OUT_OF_RESOURCES  Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

UINT8
ReadCmosBank1Byte (
  IN  UINT8                           Index
  );

VOID
WriteCmosBank1Byte (
  IN  UINT8                           Index,
  IN  UINT8                           Data
  );

VOID
SelectNFCDevice (
  IN VOID
  );

VOID
SettingI2CTouchAddress (
  IN VOID
  );

extern 
EFI_STATUS 
EFIAPI
IsctDxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif
