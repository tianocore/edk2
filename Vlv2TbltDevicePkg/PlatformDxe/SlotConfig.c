/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  SlotConfig.c

Abstract:

  Sets platform/SKU specific expansion slot information.



--*/
#include "SlotConfig.h"

//
// Implementation
//
VOID
InitializeSlotInfo (
  )
{
  UINT16                              BusSaveState;
  UINT16                              Vendor;
  UINT8                               CurrentBus;
  UINTN                               i;
  UINTN                               j;
  EFI_HANDLE                          Handle;
  EFI_STATUS                          Status;
  BOOLEAN                             RunNext;

  //
  // Loop through the slot table and see if any slots have cards in them
  //
  for (i = 0; i < mSlotBridgeTableSize; i++) {
    //
    // Initialize variable
    //
    RunNext = FALSE;

    //
    // Hide mini PCIe slots per SKU
    //
    for (j = 0; j < mSlotInformation.NumberOfEntries; j++) {
      if (mSlotInformation.SlotEntries[j].SmbiosSlotId == mSlotBridgeTable[i].SmbiosSlotId) {
        if ((mSlotInformation.SlotEntries[j].SmbiosSlotId == 0x02) &&
            (mBoardFeatures & B_BOARD_FEATURES_NO_MINIPCIE)
          ) {
          mSlotInformation.SlotEntries[j].Disabled = TRUE;
          RunNext = TRUE;
        }
        break;
      }
    }

    if (RunNext) {
      //
      // Skip slot device detection since the slot is disabled.
      //
      continue;
    }

    //
    // Check to see if the bridge has a bus number and assign one if not
    //
    BusSaveState = MmPci16 (
      0,
      mSlotBridgeTable[i].Bus,
      mSlotBridgeTable[i].Dev,
      mSlotBridgeTable[i].Function,
      PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET
                            );
    if (BusSaveState == 0) {
      //
      // Assign temp bus number
      //
      MmPci16 (
        0,
        mSlotBridgeTable[i].Bus,
        mSlotBridgeTable[i].Dev,
        mSlotBridgeTable[i].Function,
        PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET
        ) = DEF_BUS_CONFIG;
      CurrentBus = DEF_BUS;
    } else if (BusSaveState == 0xFFFF) {
      //
      // Bridge is disabled so continue with next entry in the table
      //
      continue;
    } else {
      //
      // Use existing bus number
      //
      CurrentBus = (UINT8) BusSaveState & 0xFF;
    }

    //
    // Check to see if a device is behind the bridge
    //
    Vendor = MmPci16 (
               0,
               CurrentBus,
               mSlotBridgeTable[i].TargetDevice,
               0,
               0
               );
    if (Vendor != 0xFFFF) {
      //
      // Device found so make sure the slot is marked that way
      //
      for (j = 0; j < mSlotInformation.NumberOfEntries; j++) {
        if (mSlotInformation.SlotEntries[j].SmbiosSlotId == mSlotBridgeTable[i].SmbiosSlotId) {
          mSlotInformation.SlotEntries[j].InUse = TRUE;
          break;
        }
      }
    }

    //
    // Restore previous bus information
    //
    if (BusSaveState == 0) {
      MmPci16 (
        0,
        mSlotBridgeTable[i].Bus,
        mSlotBridgeTable[i].Dev,
        mSlotBridgeTable[i].Function,
        PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET
        ) = 0;
    }
  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSmbiosSlotPopulationGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSlotInformation
                  );
  ASSERT_EFI_ERROR(Status);

}
