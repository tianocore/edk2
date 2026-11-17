/** @file // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SPI Host Controller entry point for PEI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of SpiHcDxe/SpiHcSmm. It publishes // [CODE_FIRST] 13226
  EFI_PEI_SPI_HC_PPI (a structural alias of EFI_SPI_HC_PROTOCOL) and reuses the // [CODE_FIRST] 13226
  phase-neutral ChipSelect/Clock/Transaction shells in SpiHc.c, which delegate // [CODE_FIRST] 13226
  all hardware work to the platform-specific SpiHcPlatformLib. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR> // [CODE_FIRST] 13226
  SPDX-License-Identifier: BSD-2-Clause-Patent // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#include <Base.h> // [CODE_FIRST] 13226
#include <PiPei.h> // [CODE_FIRST] 13226
#include <Library/DebugLib.h> // [CODE_FIRST] 13226
#include <Library/PeiServicesLib.h> // [CODE_FIRST] 13226
#include <Library/MemoryAllocationLib.h> // [CODE_FIRST] 13226
#include <Library/SpiHcPlatformLib.h> // [CODE_FIRST] 13226
#include <Ppi/SpiHc.h> // [CODE_FIRST] 13226
#include "SpiHc.h" // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
// // [CODE_FIRST] 13226
// Single allocation holding both the PPI data and its descriptor, so the two // [CODE_FIRST] 13226
// are always freed together and only one NULL-check is required. // [CODE_FIRST] 13226
// // [CODE_FIRST] 13226
typedef struct { // [CODE_FIRST] 13226
  EFI_PEI_SPI_HC_PPI        Ppi; // [CODE_FIRST] 13226
  EFI_PEI_PPI_DESCRIPTOR    Descriptor; // [CODE_FIRST] 13226
} SPI_HC_PPI_INSTANCE; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Entry point of the SPI Host Controller PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Queries the platform library for host controller capabilities, builds the // [CODE_FIRST] 13226
  EFI_PEI_SPI_HC_PPI instance (reusing the shared ChipSelect/Clock/Transaction // [CODE_FIRST] 13226
  shells), and installs it for consumption by the SPI bus PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] FileHandle   Handle of the file being invoked. // [CODE_FIRST] 13226
  @param[in] PeiServices  Pointer to the PEI Services Table. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS           The PPI was installed successfully. // [CODE_FIRST] 13226
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the PPI instance. // [CODE_FIRST] 13226
  @retval EFI_DEVICE_ERROR      Platform library returned zero FrameSizeSupportMask // [CODE_FIRST] 13226
                                or MaximumTransferBytes. // [CODE_FIRST] 13226
  @retval Otherwise             Failed to obtain host controller details or // [CODE_FIRST] 13226
                                install the PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
EFIAPI // [CODE_FIRST] 13226
SpiHcPeiEntry ( // [CODE_FIRST] 13226
  IN EFI_PEI_FILE_HANDLE     FileHandle, // [CODE_FIRST] 13226
  IN CONST EFI_PEI_SERVICES  **PeiServices // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS           Status; // [CODE_FIRST] 13226
  SPI_HC_PPI_INSTANCE  *Instance; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Instance = AllocateZeroPool (sizeof (SPI_HC_PPI_INSTANCE)); // [CODE_FIRST] 13226
  if (Instance == NULL) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Out of memory resources for SPI HC PPI\n", __func__)); // [CODE_FIRST] 13226
    return EFI_OUT_OF_RESOURCES; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = GetPlatformSpiHcDetails ( // [CODE_FIRST] 13226
             &Instance->Ppi.Attributes, // [CODE_FIRST] 13226
             &Instance->Ppi.FrameSizeSupportMask, // [CODE_FIRST] 13226
             &Instance->Ppi.MaximumTransferBytes // [CODE_FIRST] 13226
             ); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to get SPI HC details - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
    return Status; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  ASSERT (Instance->Ppi.FrameSizeSupportMask != 0); // [CODE_FIRST] 13226
  ASSERT (Instance->Ppi.MaximumTransferBytes != 0); // [CODE_FIRST] 13226
  if ((Instance->Ppi.FrameSizeSupportMask == 0) || // [CODE_FIRST] 13226
      (Instance->Ppi.MaximumTransferBytes  == 0)) // [CODE_FIRST] 13226
  { // [CODE_FIRST] 13226
    DEBUG (( // [CODE_FIRST] 13226
      DEBUG_ERROR, // [CODE_FIRST] 13226
      "%a: Platform SPI HC lib returned invalid capabilities " // [CODE_FIRST] 13226
      "(FrameSizeSupportMask=0x%x MaximumTransferBytes=0x%x)\n", // [CODE_FIRST] 13226
      __func__, // [CODE_FIRST] 13226
      Instance->Ppi.FrameSizeSupportMask, // [CODE_FIRST] 13226
      Instance->Ppi.MaximumTransferBytes // [CODE_FIRST] 13226
      )); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
    return EFI_DEVICE_ERROR; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // // [CODE_FIRST] 13226
  // EFI_PEI_SPI_HC_PPI aliases EFI_SPI_HC_PROTOCOL, so the shared shells apply // [CODE_FIRST] 13226
  // directly. // [CODE_FIRST] 13226
  // // [CODE_FIRST] 13226
  Instance->Ppi.ChipSelect   = ChipSelect; // [CODE_FIRST] 13226
  Instance->Ppi.Clock        = Clock; // [CODE_FIRST] 13226
  Instance->Ppi.Transaction  = Transaction; // [CODE_FIRST] 13226
  Instance->Descriptor.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | // [CODE_FIRST] 13226
                               EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST; // [CODE_FIRST] 13226
  Instance->Descriptor.Guid = &gEfiPeiSpiHcPpiGuid; // [CODE_FIRST] 13226
  Instance->Descriptor.Ppi  = &Instance->Ppi; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = PeiServicesInstallPpi (&Instance->Descriptor); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to install SPI HC PPI - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return Status; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
