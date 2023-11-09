/** @file
  This library will parse the Slim Bootloader to get required information.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BlParseLib.h>
#include <IndustryStandard/Acpi.h>
#include <UniversalPayload/PciRootBridges.h>
#include <Guid/SmmRegisterInfoGuid.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/SpiFlashInfoGuid.h>
#include <Guid/NvVariableInfoGuid.h>
#include <Guid/SmmS3CommunicationInfoGuid.h>

/**
  This function retrieves the parameter base address from boot loader.

  This function will get bootloader specific parameter address for UEFI payload.
  e.g. HobList pointer for Slim Bootloader, and coreboot table header for Coreboot.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
EFIAPI
GetParameterBase (
  VOID
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HandoffTable;

  HandoffTable = (EFI_HOB_HANDOFF_INFO_TABLE *)(UINTN)GET_BOOTLOADER_PARAMETER ();
  if ((HandoffTable->Header.HobType == EFI_HOB_TYPE_HANDOFF) &&
      (HandoffTable->Header.HobLength == sizeof (EFI_HOB_HANDOFF_INFO_TABLE)) &&
      (HandoffTable->Header.Reserved == 0))
  {
    return (VOID *)HandoffTable;
  }

  return NULL;
}

/**
  This function retrieves a GUIDed HOB data from Slim Bootloader.

  This function will search SBL HOB list to find the first GUIDed HOB that
  its GUID matches Guid.

  @param[in]  Guid        A pointer to HOB GUID to search.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
GetGuidHobDataFromSbl (
  IN       EFI_GUID  *Guid
  )
{
  UINT8       *GuidHob;
  CONST VOID  *HobList;

  HobList = GetParameterBase ();
  ASSERT (HobList != NULL);
  GuidHob = GetNextGuidHob (Guid, HobList);
  if (GuidHob != NULL) {
    return GET_GUID_HOB_DATA (GuidHob);
  }

  return NULL;
}

/**
  Acquire the memory map information.

  @param  MemInfoCallback     The callback routine
  @param  Params              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
ParseMemoryInfo (
  IN  BL_MEM_INFO_CALLBACK  MemInfoCallback,
  IN  VOID                  *Params
  )
{
  MEMORY_MAP_INFO  *MemoryMapInfo;
  UINTN            Idx;

  MemoryMapInfo = (MEMORY_MAP_INFO *)GetGuidHobDataFromSbl (&gLoaderMemoryMapInfoGuid);
  if (MemoryMapInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  for (Idx = 0; Idx < MemoryMapInfo->Count; Idx++) {
    MemInfoCallback (&MemoryMapInfo->Entry[Idx], Params);
  }

  return RETURN_SUCCESS;
}

/**
  Acquire SMBIOS table from slim bootloader.

  @param  SmbiosTable           Pointer to the SMBIOS table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseSmbiosTable (
  OUT UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmbiosTable
  )
{
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *TableInfo;

  TableInfo = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *)GetGuidHobDataFromSbl (&gUniversalPayloadSmbiosTableGuid);
  if (TableInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  SmbiosTable->SmBiosEntryPoint = TableInfo->SmBiosEntryPoint;

  return RETURN_SUCCESS;
}

/**
  Acquire ACPI table from slim bootloader.

  @param  AcpiTableHob              Pointer to the ACPI table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseAcpiTableInfo (
  OUT UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob
  )
{
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *TableInfo;

  TableInfo = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GetGuidHobDataFromSbl (&gUniversalPayloadAcpiTableGuid);
  if (TableInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  AcpiTableHob->Rsdp = TableInfo->Rsdp;

  return RETURN_SUCCESS;
}

/**
  Find the serial port information

  @param[out]  SerialPortInfo     Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
ParseSerialInfo (
  OUT SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  SERIAL_PORT_INFO  *BlSerialInfo;

  BlSerialInfo = (SERIAL_PORT_INFO *)GetGuidHobDataFromSbl (&gUefiSerialPortInfoGuid);
  if (BlSerialInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  CopyMem (SerialPortInfo, BlSerialInfo, sizeof (SERIAL_PORT_INFO));

  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer information

  @param  GfxInfo             Pointer to the EFI_PEI_GRAPHICS_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxInfo (
  OUT EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfo
  )
{
  EFI_PEI_GRAPHICS_INFO_HOB  *BlGfxInfo;

  BlGfxInfo = (EFI_PEI_GRAPHICS_INFO_HOB *)GetGuidHobDataFromSbl (&gEfiGraphicsInfoHobGuid);
  if (BlGfxInfo == NULL) {
    return RETURN_NOT_FOUND;
  }

  CopyMem (GfxInfo, BlGfxInfo, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));

  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer device information

  @param  GfxDeviceInfo      Pointer to the EFI_PEI_GRAPHICS_DEVICE_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information.

**/
RETURN_STATUS
EFIAPI
ParseGfxDeviceInfo (
  OUT EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *GfxDeviceInfo
  )
{
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *BlGfxDeviceInfo;

  BlGfxDeviceInfo = (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *)GetGuidHobDataFromSbl (&gEfiGraphicsDeviceInfoHobGuid);
  if (BlGfxDeviceInfo == NULL) {
    return RETURN_NOT_FOUND;
  }

  CopyMem (GfxDeviceInfo, BlGfxDeviceInfo, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB));

  return RETURN_SUCCESS;
}

/**
  Parse and handle the misc info provided by bootloader

  @retval RETURN_SUCCESS           The misc information was parsed successfully.
  @retval RETURN_NOT_FOUND         Could not find required misc info.
  @retval RETURN_OUT_OF_RESOURCES  Insufficant memory space.

**/
RETURN_STATUS
EFIAPI
ParseMiscInfo (
  VOID
  )
{
  RETURN_STATUS                       Status;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *BlRootBridgesHob;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PldRootBridgesHob;
  PLD_SMM_REGISTERS                   *BlSmmRegisterHob;
  PLD_SMM_REGISTERS                   *PldSmmRegisterHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK      *BlSmmMemoryHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK      *PldSmmMemoryHob;
  SPI_FLASH_INFO                      *BlSpiFlashInfoHob;
  SPI_FLASH_INFO                      *PldSpiFlashInfoHob;
  NV_VARIABLE_INFO                    *BlNvVariableHob;
  NV_VARIABLE_INFO                    *PldNvVariableHob;
  PLD_S3_COMMUNICATION                *BlS3CommunicationHob;
  PLD_S3_COMMUNICATION                *PldS3CommunicationHob;
  UINT32                              Length;

  Status           = RETURN_NOT_FOUND;
  BlRootBridgesHob = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GetGuidHobDataFromSbl (
                                                             &gUniversalPayloadPciRootBridgeInfoGuid
                                                             );
  if (BlRootBridgesHob != NULL) {
    //
    // Migrate bootloader root bridge info hob from bootloader to payload.
    //
    PldRootBridgesHob = BuildGuidHob (
                          &gUniversalPayloadPciRootBridgeInfoGuid,
                          BlRootBridgesHob->Header.Length
                          );
    ASSERT (PldRootBridgesHob != NULL);
    if (PldRootBridgesHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldRootBridgesHob, BlRootBridgesHob, BlRootBridgesHob->Header.Length);
    DEBUG ((DEBUG_INFO, "Create PCI root bridge info guid hob\n"));
    Status = RETURN_SUCCESS;
  }

  //
  // Create SMM info hob.
  //
  BlSmmRegisterHob = (PLD_SMM_REGISTERS *)GetGuidHobDataFromSbl (&gSmmRegisterInfoGuid);
  if (BlSmmRegisterHob != NULL) {
    Length            = sizeof (PLD_SMM_REGISTERS) + 5 * sizeof (PLD_GENERIC_REGISTER);
    PldSmmRegisterHob = BuildGuidHob (&gSmmRegisterInfoGuid, Length);
    ASSERT (PldSmmRegisterHob != NULL);
    if (PldSmmRegisterHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldSmmRegisterHob, BlSmmRegisterHob, Length);
    DEBUG ((DEBUG_INFO, "Created SMM info hob\n"));
    Status = RETURN_SUCCESS;
  }

  //
  // Create SMM memory hob.
  //
  BlSmmMemoryHob = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)GetGuidHobDataFromSbl (&gEfiSmmSmramMemoryGuid);
  if (BlSmmMemoryHob != NULL) {
    Length          = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + sizeof (EFI_SMRAM_DESCRIPTOR);
    PldSmmMemoryHob = BuildGuidHob (&gEfiSmmSmramMemoryGuid, Length);
    ASSERT (PldSmmMemoryHob != NULL);
    if (PldSmmMemoryHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldSmmMemoryHob, BlSmmMemoryHob, Length);
    DEBUG ((DEBUG_INFO, "Created SMM memory hob\n"));
    Status = RETURN_SUCCESS;
  }

  //
  // Create SPI flash info hob.
  //
  BlSpiFlashInfoHob = (SPI_FLASH_INFO *)GetGuidHobDataFromSbl (&gSpiFlashInfoGuid);
  if (BlSpiFlashInfoHob != NULL) {
    Length             = sizeof (SPI_FLASH_INFO);
    PldSpiFlashInfoHob = BuildGuidHob (&gSpiFlashInfoGuid, Length);
    ASSERT (PldSpiFlashInfoHob != NULL);
    if (PldSpiFlashInfoHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldSpiFlashInfoHob, BlSpiFlashInfoHob, Length);
    DEBUG ((DEBUG_INFO, "Created SPI flash info hob\n"));
    Status = RETURN_SUCCESS;
  }

  //
  // Create SPI flash variable info hob.
  //
  BlNvVariableHob = (NV_VARIABLE_INFO *)GetGuidHobDataFromSbl (&gNvVariableInfoGuid);
  if (BlNvVariableHob != NULL) {
    Length           = sizeof (NV_VARIABLE_INFO);
    PldNvVariableHob = BuildGuidHob (&gNvVariableInfoGuid, Length);
    ASSERT (PldNvVariableHob != NULL);
    if (PldNvVariableHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldNvVariableHob, BlNvVariableHob, Length);
    DEBUG ((DEBUG_INFO, "Created SPI flash variable info hob\n"));
    Status = RETURN_SUCCESS;
  }

  //
  // Create SMM S3 communication hob.
  //
  BlS3CommunicationHob = (PLD_S3_COMMUNICATION *)GetGuidHobDataFromSbl (&gS3CommunicationGuid);
  if (BlS3CommunicationHob != NULL) {
    Length                = sizeof (PLD_S3_COMMUNICATION);
    PldS3CommunicationHob = BuildGuidHob (&gS3CommunicationGuid, Length);
    ASSERT (PldS3CommunicationHob != NULL);
    if (PldS3CommunicationHob == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (PldS3CommunicationHob, BlS3CommunicationHob, Length);
    DEBUG ((DEBUG_INFO, "Created SMM S3 communication hob\n"));
    Status = RETURN_SUCCESS;
  }

  return Status;
}
