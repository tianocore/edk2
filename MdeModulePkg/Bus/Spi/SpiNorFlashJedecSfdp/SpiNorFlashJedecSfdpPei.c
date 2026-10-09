/** @file // [CODE_FIRST] 13226
  SPI NOR Flash JEDEC Serial Flash Discoverable Parameters (SFDP) PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of the SpiNorFlashJedecSfdp DXE/SMM driver. It // [CODE_FIRST] 13226
  consumes EFI_PEI_SPI_IO_PPI (installed by the SPI bus PEIM and keyed on // [CODE_FIRST] 13226
  gEdk2JedecSfdpSpiPeiDriverGuid) and produces EFI_PEI_SPI_NOR_FLASH_PPI. The SFDP // [CODE_FIRST] 13226
  discovery and NOR flash operation logic is shared with the DXE/SMM drivers via // [CODE_FIRST] 13226
  SpiNorFlash.c and SpiNorFlashJedecSfdp.c (EFI_SPI_*_PPI are aliases of the // [CODE_FIRST] 13226
  matching protocols). // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SPDX-License-Identifier: BSD-2-Clause-Patent // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @par Revision Reference: // [CODE_FIRST] 13226
    - JEDEC Standard, JESD216F.02 // [CODE_FIRST] 13226
      July 2025 Edition // [CODE_FIRST] 13226
  @par Glossary: // [CODE_FIRST] 13226
    - SFDP - Serial Flash Discoverable Parameters // [CODE_FIRST] 13226
    - PTP  - Parameter Table Pointer // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#include <Base.h> // [CODE_FIRST] 13226
#include <PiPei.h> // [CODE_FIRST] 13226
#include <Library/DebugLib.h> // [CODE_FIRST] 13226
#include <Library/MemoryAllocationLib.h> // [CODE_FIRST] 13226
#include <Library/PeiServicesLib.h> // [CODE_FIRST] 13226
#include <Ppi/SpiConfiguration.h> // [CODE_FIRST] 13226
#include <Ppi/SpiNorFlash.h> // [CODE_FIRST] 13226
#include <Ppi/SpiIo.h> // [CODE_FIRST] 13226
#include <IndustryStandard/SpiNorFlashJedecSfdp.h> // [CODE_FIRST] 13226
#include "SpiNorFlashJedecSfdpInternal.h" // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Create a SPI_NOR_FLASH_INSTANCE for the given SPI I/O PPI and install the // [CODE_FIRST] 13226
  resulting EFI_PEI_SPI_NOR_FLASH_PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] SpiIo  The SPI I/O PPI installed by the SPI bus PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS           Succeed. // [CODE_FIRST] 13226
  @retval EFI_OUT_OF_RESOURCES  Not enough resource to create the instance. // [CODE_FIRST] 13226
  @retval Otherwise             Failed to create the SPI NOR Flash instance. // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
CreateSpiNorFlashSfdpInstance ( // [CODE_FIRST] 13226
  IN EFI_PEI_SPI_IO_PPI  *SpiIo // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS              Status; // [CODE_FIRST] 13226
  SPI_NOR_FLASH_INSTANCE  *Instance; // [CODE_FIRST] 13226
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Instance = AllocateZeroPool (sizeof (SPI_NOR_FLASH_INSTANCE)); // [CODE_FIRST] 13226
  if (Instance == NULL) { // [CODE_FIRST] 13226
    return EFI_OUT_OF_RESOURCES; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  PpiDescriptor = AllocateZeroPool (sizeof (EFI_PEI_PPI_DESCRIPTOR)); // [CODE_FIRST] 13226
  if (PpiDescriptor == NULL) { // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
    return EFI_OUT_OF_RESOURCES; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Instance->SpiIo = SpiIo; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = InitialSpiNorFlashSfdpInstance (Instance); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to initial SPI_NOR_FLASH_INSTANCE - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    FreePool (PpiDescriptor); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
    return Status; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  PpiDescriptor->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | // [CODE_FIRST] 13226
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST; // [CODE_FIRST] 13226
  PpiDescriptor->Guid = &gEfiPeiSpiNorFlashPpiGuid; // [CODE_FIRST] 13226
  PpiDescriptor->Ppi  = &Instance->Protocol; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = PeiServicesInstallPpi (PpiDescriptor); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to install SPI NOR Flash PPI - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    FreePool (PpiDescriptor); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return Status; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Notification callback invoked when the SPI I/O PPI is installed by the SPI // [CODE_FIRST] 13226
  bus PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] PeiServices     Pointer to the PEI Services Table. // [CODE_FIRST] 13226
  @param[in] NotifyDesc      The notification descriptor that was registered. // [CODE_FIRST] 13226
  @param[in] InvokePpi       Pointer to the installed EFI_PEI_SPI_IO_PPI interface. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS  The notification was processed. // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
EFIAPI // [CODE_FIRST] 13226
SpiIoPpiInstalledCallback ( // [CODE_FIRST] 13226
  IN EFI_PEI_SERVICES           **PeiServices, // [CODE_FIRST] 13226
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc, // [CODE_FIRST] 13226
  IN VOID                       *InvokePpi // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS  Status; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = CreateSpiNorFlashSfdpInstance ((EFI_PEI_SPI_IO_PPI *)InvokePpi); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to create SPI NOR Flash SFDP instance - %r\n", __func__, Status)); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return EFI_SUCCESS; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
EFI_PEI_NOTIFY_DESCRIPTOR  mSpiIoPpiNotifyList = { // [CODE_FIRST] 13226
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST, // [CODE_FIRST] 13226
  &gEdk2JedecSfdpSpiPeiDriverGuid, // [CODE_FIRST] 13226
  SpiIoPpiInstalledCallback // [CODE_FIRST] 13226
}; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Entry point of the SPI NOR Flash JEDEC SFDP PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Registers a notification for the SPI I/O PPI. The PEI core invokes the // [CODE_FIRST] 13226
  notification callback for both SPI I/O PPI instances already installed at // [CODE_FIRST] 13226
  registration time (e.g. by an earlier-dispatched SPI bus PEIM) and any // [CODE_FIRST] 13226
  installed later, so a single notify covers all cases without duplicating // [CODE_FIRST] 13226
  flash instances. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] FileHandle   Handle of the file being invoked. // [CODE_FIRST] 13226
  @param[in] PeiServices  Pointer to the PEI Services Table. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS  The PEIM initialized successfully. // [CODE_FIRST] 13226
  @retval Otherwise    Failed to register the SPI I/O PPI notification. // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
EFIAPI // [CODE_FIRST] 13226
SpiNorFlashSfdpPeiEntry ( // [CODE_FIRST] 13226
  IN EFI_PEI_FILE_HANDLE     FileHandle, // [CODE_FIRST] 13226
  IN CONST EFI_PEI_SERVICES  **PeiServices // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS  Status; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // // [CODE_FIRST] 13226
  // Register for the SPI I/O PPI notification. In PEI this callback also fires // [CODE_FIRST] 13226
  // for instances already installed before this registration, so it handles // [CODE_FIRST] 13226
  // both the already-present and later-installed cases. // [CODE_FIRST] 13226
  // // [CODE_FIRST] 13226
  Status = PeiServicesNotifyPpi (&mSpiIoPpiNotifyList); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to register SPI I/O PPI notification - %r\n", __func__, Status)); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return Status; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
