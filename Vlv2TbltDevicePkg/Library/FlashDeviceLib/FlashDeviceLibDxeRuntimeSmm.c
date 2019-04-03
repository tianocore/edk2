/** @file

  Copyright (c) 2004  - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent



**/

#include <PiDxe.h>

#include <Library/FlashDeviceLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/SmmBase2.h>
#include <Guid/EventGroup.h>
#include "SpiChipDefinitions.h"

extern UINTN FlashDeviceBase;

extern EFI_SPI_PROTOCOL *mSpiProtocol;

VOID
EFIAPI
LibFvbFlashDeviceVirtualAddressChangeNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  gRT->ConvertPointer (0, (VOID **) &mSpiProtocol);
  gRT->ConvertPointer (0, (VOID **) &FlashDeviceBase);
}


/**
  The library constructuor.

  The function does the necessary initialization work for this library
  instance. Please put all initialization works in it.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       The function always return EFI_SUCCESS for now.
                                It will ASSERT on error for debug version.
  @retval     EFI_ERROR         Please reference LocateProtocol for error code details.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceSupportInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  Event;
  UINT8                         SfId[3];
  UINT8                         FlashIndex;
  UINT8                         SpiReadError;
  UINT8                         SpiNotMatchError;
  EFI_SMM_BASE2_PROTOCOL       *SmmBase;
  BOOLEAN                       InSmm;

  SpiReadError     = 0x00;
  SpiNotMatchError = 0x00;

  InSmm = FALSE;
  Status = gBS->LocateProtocol (
                  &gEfiSmmBase2ProtocolGuid,
                  NULL,
                  (void **)&SmmBase
                  );
  if (!EFI_ERROR(Status)) {
    Status = SmmBase->InSmm(SmmBase, &InSmm);
    if (EFI_ERROR(Status)) {
      InSmm = FALSE;
    }
  }

  if (!InSmm) {
    Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LibFvbFlashDeviceVirtualAddressChangeNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
    ASSERT_EFI_ERROR (Status);

  } else {
    Status = gBS->LocateProtocol (
                    &gEfiSmmSpiProtocolGuid,
                    NULL,
                    (VOID **)&mSpiProtocol
                    );
    ASSERT_EFI_ERROR (Status);
  }


  for (FlashIndex = EnumSpiFlashW25Q64; FlashIndex < EnumSpiFlashMax; FlashIndex++) {
    Status = mSpiProtocol->Init (mSpiProtocol, &(mInitTable[FlashIndex]));
    if (!EFI_ERROR (Status)) {
      //
      // Read Vendor/Device IDs to check if the driver supports the Serial Flash device.
      //
      Status = mSpiProtocol->Execute (
                               mSpiProtocol,
                               SPI_READ_ID,
                               SPI_WREN,
                               TRUE,
                               FALSE,
                               FALSE,
                               0,
                               3,
                               SfId,
                               EnumSpiRegionAll
                               );
      if (!EFI_ERROR (Status)) {
        if ((SfId[0] == mInitTable[FlashIndex].VendorId)  &&
            (SfId[1] == mInitTable[FlashIndex].DeviceId0) &&
            (SfId[2] == mInitTable[FlashIndex].DeviceId1)) {
            //
            // Found a matching SPI device, FlashIndex now contains flash device.
            //
            DEBUG ((EFI_D_ERROR, "OK - Found SPI Flash Type in SPI Flash Driver, Device Type ID 0 = 0x%02x!\n", mInitTable[FlashIndex].DeviceId0));
            DEBUG ((EFI_D_ERROR, "Device Type ID 1 = 0x%02x!\n", mInitTable[FlashIndex].DeviceId1));

            if (mInitTable[FlashIndex].BiosStartOffset == (UINTN) (-1)) {
              DEBUG ((EFI_D_ERROR, "ERROR - The size of BIOS image is bigger than SPI Flash device!\n"));
              CpuDeadLoop ();
            }
            break;
        } else {
          SpiNotMatchError++;
        }
      } else {
        SpiReadError++;
      }
    }
  }

  DEBUG ((EFI_D_ERROR, "SPI flash chip VID = 0x%X, DID0 = 0x%X, DID1 = 0x%X\n", SfId[0], SfId[1], SfId[2]));

  if (FlashIndex < EnumSpiFlashMax)  {
    return EFI_SUCCESS;
  } else {
  if (SpiReadError != 0) {
      DEBUG ((EFI_D_ERROR, "ERROR - SPI Read ID execution failed! Error Count = %d\n", SpiReadError));
   }
    else {
      if (SpiNotMatchError != 0) {
        DEBUG ((EFI_D_ERROR, "ERROR - No supported SPI flash chip found! Error Count = %d\n", SpiNotMatchError));
        DEBUG ((EFI_D_ERROR, "SPI flash chip VID = 0x%X, DID0 = 0x%X, DID1 = 0x%X\n", SfId[0], SfId[1], SfId[2]));
      }
    }
    return EFI_UNSUPPORTED;
  }
}

