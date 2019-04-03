/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBiosInterface.h"
#include <IndustryStandard/Pci.h>

// Give floppy 3 states
// FLOPPY_PRESENT_WITH_MEDIA  = Floppy controller present and media is inserted
// FLOPPY_NOT_PRESENT = No floppy controller present
// FLOPPY_PRESENT_NO_MEDIA = Floppy controller present but no media inserted
//
#define FLOPPY_NOT_PRESENT           0
#define FLOPPY_PRESENT_WITH_MEDIA    1
#define FLOPPY_PRESENT_NO_MEDIA      2

BBS_TABLE           *mBbsTable;
BOOLEAN             mBbsTableDoneFlag   = FALSE;
BOOLEAN             IsHaveMediaInFloppy = TRUE;

/**
  Checks the state of the floppy and if media is inserted.

  This routine checks the state of the floppy and if media is inserted.
  There are 3 cases:
  No floppy present         - Set BBS entry to ignore
  Floppy present & no media - Set BBS entry to lowest priority. We cannot
  set it to ignore since 16-bit CSM will
  indicate no floppy and thus drive A: is
  unusable. CSM-16 will not try floppy since
  lowest priority and thus not incur boot
  time penality.
  Floppy present & media    - Set BBS entry to some priority.

  @return  State of floppy media

**/
UINT8
HasMediaInFloppy (
  VOID
  )
{
  EFI_STATUS                            Status;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  UINTN                                 Index;
  EFI_ISA_IO_PROTOCOL                   *IsaIo;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;

  HandleBuffer  = NULL;
  HandleCount   = 0;

  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiIsaIoProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
        );

  //
  // If don't find any ISA/IO protocol assume no floppy. Need for floppy
  // free system
  //
  if (HandleCount == 0) {
    return FLOPPY_NOT_PRESENT;
  }

  ASSERT (HandleBuffer != NULL);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiIsaIoProtocolGuid,
                    (VOID **) &IsaIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (IsaIo->ResourceList->Device.HID != EISA_PNP_ID (0x604)) {
      continue;
    }
    //
    // Update blockio in case the floppy is inserted in during BdsTimeout
    //
    Status = gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (BlkIo->Media->MediaPresent) {
      FreePool (HandleBuffer);
      return FLOPPY_PRESENT_WITH_MEDIA;
    } else {
      FreePool (HandleBuffer);
      return FLOPPY_PRESENT_NO_MEDIA;
    }
  }

  FreePool (HandleBuffer);

  return FLOPPY_NOT_PRESENT;

}


/**
  Complete build of BBS TABLE.

  @param  Private                 Legacy BIOS Instance data
  @param  BbsTable                BBS Table passed to 16-bit code

  @retval EFI_SUCCESS             Removable media not present

**/
EFI_STATUS
LegacyBiosBuildBbs (
  IN  LEGACY_BIOS_INSTANCE      *Private,
  IN  BBS_TABLE                 *BbsTable
  )
{
  UINTN     BbsIndex;
  HDD_INFO  *HddInfo;
  UINTN     HddIndex;
  UINTN     Index;

  //
  // First entry is floppy.
  // Next 2*MAX_IDE_CONTROLLER entries are for onboard IDE.
  // Next n entries are filled in after each ROM is dispatched.
  //   Entry filled in if follow BBS spec. See LegacyPci.c
  // Next entries are for non-BBS compliant ROMS. They are filled in by
  //   16-bit code during Legacy16UpdateBbs invocation. Final BootPriority
  //   occurs after that invocation.
  //
  // Floppy
  // Set default state.
  //
  IsHaveMediaInFloppy = HasMediaInFloppy ();
  if (IsHaveMediaInFloppy == FLOPPY_PRESENT_WITH_MEDIA) {
    BbsTable[0].BootPriority = BBS_UNPRIORITIZED_ENTRY;
  } else {
    if (IsHaveMediaInFloppy == FLOPPY_PRESENT_NO_MEDIA) {
      BbsTable[0].BootPriority = BBS_LOWEST_PRIORITY;
    } else {
      BbsTable[0].BootPriority = BBS_IGNORE_ENTRY;
    }
  }

  BbsTable[0].Bus                       = 0xff;
  BbsTable[0].Device                    = 0xff;
  BbsTable[0].Function                  = 0xff;
  BbsTable[0].DeviceType                = BBS_FLOPPY;
  BbsTable[0].Class                     = 01;
  BbsTable[0].SubClass                  = 02;
  BbsTable[0].StatusFlags.OldPosition   = 0;
  BbsTable[0].StatusFlags.Reserved1     = 0;
  BbsTable[0].StatusFlags.Enabled       = 0;
  BbsTable[0].StatusFlags.Failed        = 0;
  BbsTable[0].StatusFlags.MediaPresent  = 0;
  BbsTable[0].StatusFlags.Reserved2     = 0;

  //
  // Onboard HDD - Note Each HDD controller controls 2 drives
  //               Master & Slave
  //
  HddInfo = &Private->IntThunk->EfiToLegacy16BootTable.HddInfo[0];
  //
  // Get IDE Drive Info
  //
  LegacyBiosBuildIdeData (Private, &HddInfo, 0);

  for (HddIndex = 0; HddIndex < MAX_IDE_CONTROLLER; HddIndex++) {

    BbsIndex = HddIndex * 2 + 1;
    for (Index = 0; Index < 2; ++Index) {

      BbsTable[BbsIndex + Index].Bus                      = HddInfo[HddIndex].Bus;
      BbsTable[BbsIndex + Index].Device                   = HddInfo[HddIndex].Device;
      BbsTable[BbsIndex + Index].Function                 = HddInfo[HddIndex].Function;
      BbsTable[BbsIndex + Index].Class                    = 01;
      BbsTable[BbsIndex + Index].SubClass                 = 01;
      BbsTable[BbsIndex + Index].StatusFlags.OldPosition  = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Reserved1    = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Enabled      = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Failed       = 0;
      BbsTable[BbsIndex + Index].StatusFlags.MediaPresent = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Reserved2    = 0;

      //
      // If no controller found or no device found set to ignore
      // else set to unprioritized and set device type
      //
      if (HddInfo[HddIndex].CommandBaseAddress == 0) {
        BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
      } else {
        if (Index == 0) {
          if ((HddInfo[HddIndex].Status & (HDD_MASTER_IDE | HDD_MASTER_ATAPI_CDROM | HDD_MASTER_ATAPI_ZIPDISK)) != 0) {
            BbsTable[BbsIndex + Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
            if ((HddInfo[HddIndex].Status & HDD_MASTER_IDE) != 0) {
              BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
            } else if ((HddInfo[HddIndex].Status & HDD_MASTER_ATAPI_CDROM) != 0) {
              BbsTable[BbsIndex + Index].DeviceType = BBS_CDROM;
            } else {
              //
              // for ZIPDISK
              //
              BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
            }
          } else {
            BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
          }
        } else {
          if ((HddInfo[HddIndex].Status & (HDD_SLAVE_IDE | HDD_SLAVE_ATAPI_CDROM | HDD_SLAVE_ATAPI_ZIPDISK)) != 0) {
            BbsTable[BbsIndex + Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
            if ((HddInfo[HddIndex].Status & HDD_SLAVE_IDE) != 0) {
              BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
            } else if ((HddInfo[HddIndex].Status & HDD_SLAVE_ATAPI_CDROM) != 0) {
              BbsTable[BbsIndex + Index].DeviceType = BBS_CDROM;
            } else {
              //
              // for ZIPDISK
              //
              BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
            }
          } else {
            BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
          }
        }
      }
    }
  }

  return EFI_SUCCESS;

}


/**
  Get all BBS info

  @param  This                    Protocol instance pointer.
  @param  HddCount                Number of HDD_INFO structures
  @param  HddInfo                 Onboard IDE controller information
  @param  BbsCount                Number of BBS_TABLE structures
  @param  BbsTable                List BBS entries

  @retval EFI_SUCCESS             Tables returned
  @retval EFI_NOT_FOUND           resource not found
  @retval EFI_DEVICE_ERROR        can not get BBS table

**/
EFI_STATUS
EFIAPI
LegacyBiosGetBbsInfo (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *HddCount,
  OUT HDD_INFO                        **HddInfo,
  OUT UINT16                          *BbsCount,
  OUT BBS_TABLE                       **BbsTable
  )
{
  LEGACY_BIOS_INSTANCE              *Private;
  EFI_IA32_REGISTER_SET             Regs;
  EFI_TO_COMPATIBILITY16_BOOT_TABLE *EfiToLegacy16BootTable;
//  HDD_INFO                          *LocalHddInfo;
//  IN BBS_TABLE                      *LocalBbsTable;
  UINTN                             NumHandles;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  UINTN                             TempData;
  UINT32                            Granularity;

  HandleBuffer            = NULL;

  Private                 = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  EfiToLegacy16BootTable  = &Private->IntThunk->EfiToLegacy16BootTable;
//  LocalHddInfo            = EfiToLegacy16BootTable->HddInfo;
//  LocalBbsTable           = (BBS_TABLE*)(UINTN)EfiToLegacy16BootTable->BbsTable;

  if (!mBbsTableDoneFlag) {
    mBbsTable = Private->BbsTablePtr;

    //
    // Always enable disk controllers so 16-bit CSM code has valid information for all
    // drives.
    //
    //
    // Get PciRootBridgeIO protocol
    //
    gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiPciRootBridgeIoProtocolGuid,
          NULL,
          &NumHandles,
          &HandleBuffer
          );

    if (NumHandles == 0) {
      return EFI_NOT_FOUND;
    }

    mBbsTableDoneFlag = TRUE;
    for (Index = 0; Index < NumHandles; Index++) {
      //
      // Connect PciRootBridgeIO protocol handle with FALSE parameter to let
      // PCI bus driver enumerate all subsequent handles
      //
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);

    }

    LegacyBiosBuildBbs (Private, mBbsTable);

    Private->LegacyRegion->UnLock (Private->LegacyRegion, 0xe0000, 0x20000, &Granularity);

    //
    // Call into Legacy16 code to add to BBS table for non BBS compliant OPROMs.
    //
    ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
    Regs.X.AX = Legacy16UpdateBbs;

    //
    // Pass in handoff data
    //
    TempData  = (UINTN) EfiToLegacy16BootTable;
    Regs.X.ES = NORMALIZE_EFI_SEGMENT ((UINT32) TempData);
    Regs.X.BX = NORMALIZE_EFI_OFFSET ((UINT32) TempData);

    Private->LegacyBios.FarCall86 (
      This,
      Private->Legacy16CallSegment,
      Private->Legacy16CallOffset,
      &Regs,
      NULL,
      0
      );

    Private->Cpu->FlushDataCache (Private->Cpu, 0xE0000, 0x20000, EfiCpuFlushTypeWriteBackInvalidate);
    Private->LegacyRegion->Lock (Private->LegacyRegion, 0xe0000, 0x20000, &Granularity);

    if (Regs.X.AX != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  *HddCount = MAX_IDE_CONTROLLER;
  *HddInfo  = EfiToLegacy16BootTable->HddInfo;
  *BbsTable = (BBS_TABLE*)(UINTN)EfiToLegacy16BootTable->BbsTable;
  *BbsCount = (UINT16) (sizeof (Private->IntThunk->BbsTable) / sizeof (BBS_TABLE));
  return EFI_SUCCESS;
}
