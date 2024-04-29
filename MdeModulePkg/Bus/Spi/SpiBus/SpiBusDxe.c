/** @file

  SPI bus DXE driver

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiIo.h>
#include "SpiBus.h"

/**
  Entry point of the Spi Bus layer

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS           Succeed.
  @retval EFI_DEVICE_ERROR      SpiPeripheral is NULL.
  @retval EFI_NOT_FOUND         Fail to locate SpiHcProtocol or SpiIoConfigurationProtocol
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate SpiIoChip
**/
EFI_STATUS
EFIAPI
SpiBusEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  SPI_IO_CHIP                     *SpiChip;
  EFI_SPI_HC_PROTOCOL             *SpiHc;
  EFI_SPI_CONFIGURATION_PROTOCOL  *SpiConfiguration;
  EFI_SPI_PERIPHERAL              *SpiPeripheral;
  EFI_SPI_BUS                     *Bus;
  UINTN                           BusIndex;
  UINTN                           HcIndex;
  EFI_HANDLE                      *SpiHcHandles;
  UINTN                           HandleCount;
  EFI_DEVICE_PATH_PROTOCOL        *SpiHcDevicePath;

  DEBUG ((DEBUG_VERBOSE, "%a - ENTRY\n", __func__));

  // Get all SPI HC protocols, could be multiple SPI HC's on a single platform
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSpiHcProtocolGuid,
                  NULL,
                  &HandleCount,
                  &SpiHcHandles
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "No SpiHcProtocol is found\n"));
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  // Locate the SPI Configuration Protocol
  Status = gBS->LocateProtocol (
                  &gEfiSpiConfigurationProtocolGuid,
                  NULL,
                  (VOID **)&SpiConfiguration
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "No SpiConfigurationProtocol is found\n"));
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  // Parse through Hc protocols, find correct device path
  for (HcIndex = 0; HcIndex < HandleCount; HcIndex++) {
    Status = gBS->HandleProtocol (
                    SpiHcHandles[HcIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&SpiHcDevicePath
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "Error locating EFI device path for this SPI controller, status=%r \n", Status));
      continue; // Continue searching
    }

    // Parse through SpiConfiguration's SpiBuses, find matching devicepath for SpiHc
    for (BusIndex = 0; BusIndex < SpiConfiguration->BusCount; BusIndex++) {
      Bus = (EFI_SPI_BUS *)SpiConfiguration->Buslist[BusIndex];
      if (!DevicePathsAreEqual (SpiHcDevicePath, Bus->ControllerPath)) {
        DEBUG ((DEBUG_VERBOSE, "SpiHc and SpiConfig device paths dont match, continue parsing\n"));
        continue;
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "%a: Found matching device paths, Enumerating SPI BUS: %s with DevicePath: %s\n",
        __func__,
        Bus->FriendlyName,
        ConvertDevicePathToText (SpiHcDevicePath, FALSE, FALSE)
        ));

      // Get SpiHc from the SpiHcHandles
      Status = gBS->HandleProtocol (
                      SpiHcHandles[HcIndex],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&SpiHc
                      );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_VERBOSE, "%a - Error getting SpiHc from Handle\n", __func__));
        goto Exit;
      }

      SpiPeripheral = (EFI_SPI_PERIPHERAL *)Bus->Peripherallist;
      if (SpiPeripheral != NULL) {
        do {
          DEBUG ((
            DEBUG_VERBOSE,
            "%a: Installing SPI IO protocol for %s, by %s, PN=%s\n",
            __func__,
            SpiPeripheral->FriendlyName,
            SpiPeripheral->SpiPart->Vendor,
            SpiPeripheral->SpiPart->PartNumber
            ));
          // Allocate the SPI IO Device
          SpiChip = AllocateZeroPool (sizeof (SPI_IO_CHIP));
          ASSERT (SpiChip != NULL);
          if (SpiChip != NULL) {
            // Fill in the SpiChip
            SpiChip->Signature                      = SPI_IO_SIGNATURE;
            SpiChip->SpiConfig                      = SpiConfiguration;
            SpiChip->SpiHc                          = SpiHc;
            SpiChip->SpiBus                         = Bus;
            SpiChip->Protocol.SpiPeripheral         = SpiPeripheral;
            SpiChip->Protocol.OriginalSpiPeripheral = SpiPeripheral;
            SpiChip->Protocol.FrameSizeSupportMask  = SpiHc->FrameSizeSupportMask;
            SpiChip->Protocol.MaximumTransferBytes  = SpiHc->MaximumTransferBytes;
            if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) {
              SpiChip->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS;
            }

            if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) {
              SpiChip->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE;
            }

            if ((SpiHc->Attributes & HC_SUPPORTS_8_BIT_DATA_BUS_WIDTH) != 0) {
              SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_8_BIT_DATA_BUS_WIDTH;
            }

            if ((SpiHc->Attributes & HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != 0) {
              SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH;
            }

            if ((SpiHc->Attributes & HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != 0) {
              SpiChip->Protocol.Attributes |= SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH;
            }

            SpiChip->Protocol.Transaction         = Transaction;
            SpiChip->Protocol.UpdateSpiPeripheral = UpdateSpiPeripheral;
            // Install the SPI IO Protocol
            Status = gBS->InstallProtocolInterface (
                            &SpiChip->Handle,
                            (GUID *)SpiPeripheral->SpiPeripheralDriverGuid,
                            EFI_NATIVE_INTERFACE,
                            &SpiChip->Protocol
                            );
            if (EFI_ERROR (Status)) {
              DEBUG ((DEBUG_VERBOSE, "%a - Error installing SpiIoProtocol\n", __func__));
              continue;
            }
          } else {
            Status = EFI_OUT_OF_RESOURCES;
            DEBUG ((
              DEBUG_ERROR,
              "%a: Out of Memory resources\n",
              __func__
              ));
            break;
          }

          SpiPeripheral = (EFI_SPI_PERIPHERAL *)SpiPeripheral->NextSpiPeripheral;
        } while (SpiPeripheral != NULL);
      } else {
        Status = EFI_DEVICE_ERROR;
      }
    }
  }

Exit:
  DEBUG ((DEBUG_VERBOSE, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
