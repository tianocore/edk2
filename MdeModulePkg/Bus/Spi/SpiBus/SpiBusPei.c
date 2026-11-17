/** @file // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SPI bus PEIM. This is the PEI-phase analogue of the SpiBus DXE/SMM driver. It // [CODE_FIRST] 13226
  consumes EFI_PEI_SPI_CONFIGURATION_PPI (board table) and EFI_PEI_SPI_HC_PPI (host // [CODE_FIRST] 13226
  controller), matches the host controller device path against the configured // [CODE_FIRST] 13226
  buses, and installs one EFI_PEI_SPI_IO_PPI per SPI peripheral, keyed on the // [CODE_FIRST] 13226
  peripheral's SpiPeripheralDriverGuid. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  EFI_SPI_*_PPI are structural aliases of the matching EFI_SPI_*_PROTOCOL, so // [CODE_FIRST] 13226
  the shared SpiBus.c transaction/chip-select logic is reused unchanged; only // [CODE_FIRST] 13226
  the installation mechanism differs (PPI descriptor vs protocol handle). // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR> // [CODE_FIRST] 13226
  SPDX-License-Identifier: BSD-2-Clause-Patent // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#include <Base.h> // [CODE_FIRST] 13226
#include <PiPei.h> // [CODE_FIRST] 13226
#include <Library/BaseLib.h> // [CODE_FIRST] 13226
#include <Library/DebugLib.h> // [CODE_FIRST] 13226
#include <Library/MemoryAllocationLib.h> // [CODE_FIRST] 13226
#include <Library/PeiServicesLib.h> // [CODE_FIRST] 13226
#include <Library/SpiHcPlatformLib.h> // [CODE_FIRST] 13226
#include <Ppi/SpiConfiguration.h> // [CODE_FIRST] 13226
#include <Ppi/SpiHc.h> // [CODE_FIRST] 13226
#include <Ppi/SpiIo.h> // [CODE_FIRST] 13226
#include "SpiBusPei.h" // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Install an EFI_PEI_SPI_IO_PPI instance for a single SPI peripheral. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] SpiConfig     The SPI configuration PPI. // [CODE_FIRST] 13226
  @param[in] SpiHc         The SPI host controller PPI. // [CODE_FIRST] 13226
  @param[in] Bus           The SPI bus the peripheral resides on. // [CODE_FIRST] 13226
  @param[in] SpiPeripheral The peripheral to install an I/O PPI for. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS           The I/O PPI was installed successfully. // [CODE_FIRST] 13226
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the SPI_IO_CHIP_PEI instance. // [CODE_FIRST] 13226
  @retval Otherwise             Failed to install the I/O PPI. // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
STATIC // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
InstallSpiIoPpi ( // [CODE_FIRST] 13226
  IN EFI_PEI_SPI_CONFIGURATION_PPI  *SpiConfig, // [CODE_FIRST] 13226
  IN EFI_PEI_SPI_HC_PPI             *SpiHc, // [CODE_FIRST] 13226
  IN EFI_SPI_BUS                    *Bus, // [CODE_FIRST] 13226
  IN EFI_SPI_PERIPHERAL             *SpiPeripheral // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS       Status; // [CODE_FIRST] 13226
  SPI_IO_CHIP_PEI  *Instance; // [CODE_FIRST] 13226
  SPI_IO_CHIP      *SpiChip; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Instance = AllocateZeroPool (sizeof (SPI_IO_CHIP_PEI)); // [CODE_FIRST] 13226
  if (Instance == NULL) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Out of Memory resources\n", __func__)); // [CODE_FIRST] 13226
    return EFI_OUT_OF_RESOURCES; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SpiChip = &Instance->Chip; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SpiChip->Signature                      = SPI_IO_SIGNATURE; // [CODE_FIRST] 13226
  SpiChip->SpiConfig                      = SpiConfig; // [CODE_FIRST] 13226
  SpiChip->SpiHc                          = SpiHc; // [CODE_FIRST] 13226
  SpiChip->SpiBus                         = Bus; // [CODE_FIRST] 13226
  SpiChip->Protocol.SpiPeripheral         = SpiPeripheral; // [CODE_FIRST] 13226
  SpiChip->Protocol.OriginalSpiPeripheral = SpiPeripheral; // [CODE_FIRST] 13226
  SpiChip->Protocol.FrameSizeSupportMask  = SpiHc->FrameSizeSupportMask; // [CODE_FIRST] 13226
  SpiChip->Protocol.MaximumTransferBytes  = SpiHc->MaximumTransferBytes; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) { // [CODE_FIRST] 13226
    SpiChip->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) { // [CODE_FIRST] 13226
    SpiChip->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  if ((SpiHc->Attributes & HC_SUPPORTS_8_BIT_DATA_BUS_WIDTH) != 0) { // [CODE_FIRST] 13226
    SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_8_BIT_DATA_BUS_WIDTH; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  if ((SpiHc->Attributes & HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != 0) { // [CODE_FIRST] 13226
    SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  if ((SpiHc->Attributes & HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != 0) { // [CODE_FIRST] 13226
    SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  SpiChip->Protocol.Transaction         = Transaction; // [CODE_FIRST] 13226
  SpiChip->Protocol.UpdateSpiPeripheral = UpdateSpiPeripheral; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Instance->PpiDescriptor.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | // [CODE_FIRST] 13226
                                  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST; // [CODE_FIRST] 13226
  Instance->PpiDescriptor.Guid = (EFI_GUID *)SpiPeripheral->SpiPeripheralDriverGuid; // [CODE_FIRST] 13226
  Instance->PpiDescriptor.Ppi  = &SpiChip->Protocol; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = PeiServicesInstallPpi (&Instance->PpiDescriptor); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Error installing SpiIoPpi - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    FreePool (Instance); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return Status; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/** // [CODE_FIRST] 13226
  Entry point of the SPI bus PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @param[in] FileHandle   Handle of the file being invoked. // [CODE_FIRST] 13226
  @param[in] PeiServices  Pointer to the PEI Services Table. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  @retval EFI_SUCCESS       Succeed. // [CODE_FIRST] 13226
  @retval EFI_NOT_FOUND     Fail to locate the SPI HC or configuration PPI. // [CODE_FIRST] 13226
  @retval EFI_DEVICE_ERROR  A configured bus has no peripherals. // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
EFI_STATUS // [CODE_FIRST] 13226
EFIAPI // [CODE_FIRST] 13226
SpiBusPeiEntry ( // [CODE_FIRST] 13226
  IN EFI_PEI_FILE_HANDLE     FileHandle, // [CODE_FIRST] 13226
  IN CONST EFI_PEI_SERVICES  **PeiServices // [CODE_FIRST] 13226
  ) // [CODE_FIRST] 13226
{ // [CODE_FIRST] 13226
  EFI_STATUS                     Status; // [CODE_FIRST] 13226
  EFI_PEI_SPI_HC_PPI             *SpiHc; // [CODE_FIRST] 13226
  EFI_PEI_SPI_CONFIGURATION_PPI  *SpiConfiguration; // [CODE_FIRST] 13226
  EFI_SPI_PERIPHERAL             *SpiPeripheral; // [CODE_FIRST] 13226
  EFI_SPI_BUS                    *Bus; // [CODE_FIRST] 13226
  UINTN                          BusIndex; // [CODE_FIRST] 13226
  EFI_DEVICE_PATH_PROTOCOL       *SpiHcDevicePath; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // Locate the SPI HC PPI // [CODE_FIRST] 13226
  Status = PeiServicesLocatePpi ( // [CODE_FIRST] 13226
             &gEfiPeiSpiHcPpiGuid, // [CODE_FIRST] 13226
             0, // [CODE_FIRST] 13226
             NULL, // [CODE_FIRST] 13226
             (VOID **)&SpiHc // [CODE_FIRST] 13226
             ); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: No SpiHcPpi is found - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    return EFI_NOT_FOUND; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // Locate the SPI Configuration PPI // [CODE_FIRST] 13226
  Status = PeiServicesLocatePpi ( // [CODE_FIRST] 13226
             &gEfiPeiSpiConfigurationPpiGuid, // [CODE_FIRST] 13226
             0, // [CODE_FIRST] 13226
             NULL, // [CODE_FIRST] 13226
             (VOID **)&SpiConfiguration // [CODE_FIRST] 13226
             ); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: No SpiConfigurationPpi is found - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    return EFI_NOT_FOUND; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // Obtain the host controller device path from the platform library // [CODE_FIRST] 13226
  Status = GetSpiHcDevicePath (&SpiHcDevicePath); // [CODE_FIRST] 13226
  if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
    DEBUG ((DEBUG_ERROR, "%a: Fail to get SPI HC device path - %r\n", __func__, Status)); // [CODE_FIRST] 13226
    return Status; // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Status = EFI_NOT_FOUND; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  // Parse through SpiConfiguration's SpiBuses, find matching device path // [CODE_FIRST] 13226
  for (BusIndex = 0; BusIndex < SpiConfiguration->BusCount; BusIndex++) { // [CODE_FIRST] 13226
    Bus = (EFI_SPI_BUS *)SpiConfiguration->Buslist[BusIndex]; // [CODE_FIRST] 13226
    if (!DevicePathsAreEqual (SpiHcDevicePath, Bus->ControllerPath)) { // [CODE_FIRST] 13226
      continue; // [CODE_FIRST] 13226
    } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
    DEBUG (( // [CODE_FIRST] 13226
      DEBUG_INFO, // [CODE_FIRST] 13226
      "%a: Found matching device paths, Enumerating SPI BUS: %s\n", // [CODE_FIRST] 13226
      __func__, // [CODE_FIRST] 13226
      Bus->FriendlyName // [CODE_FIRST] 13226
      )); // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
    SpiPeripheral = (EFI_SPI_PERIPHERAL *)Bus->Peripherallist; // [CODE_FIRST] 13226
    if (SpiPeripheral == NULL) { // [CODE_FIRST] 13226
      Status = EFI_DEVICE_ERROR; // [CODE_FIRST] 13226
      continue; // [CODE_FIRST] 13226
    } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
    do { // [CODE_FIRST] 13226
      DEBUG (( // [CODE_FIRST] 13226
        DEBUG_INFO, // [CODE_FIRST] 13226
        "%a: Installing SPI IO PPI for %s, by %s, PN=%s\n", // [CODE_FIRST] 13226
        __func__, // [CODE_FIRST] 13226
        SpiPeripheral->FriendlyName, // [CODE_FIRST] 13226
        SpiPeripheral->SpiPart->Vendor, // [CODE_FIRST] 13226
        SpiPeripheral->SpiPart->PartNumber // [CODE_FIRST] 13226
        )); // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
      Status = InstallSpiIoPpi (SpiConfiguration, SpiHc, Bus, SpiPeripheral); // [CODE_FIRST] 13226
      if (EFI_ERROR (Status)) { // [CODE_FIRST] 13226
        break; // [CODE_FIRST] 13226
      } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
      SpiPeripheral = (EFI_SPI_PERIPHERAL *)SpiPeripheral->NextSpiPeripheral; // [CODE_FIRST] 13226
    } while (SpiPeripheral != NULL); // [CODE_FIRST] 13226
  } // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  return Status; // [CODE_FIRST] 13226
} // [CODE_FIRST] 13226
