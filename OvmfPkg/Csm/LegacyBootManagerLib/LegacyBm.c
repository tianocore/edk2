/** @file
  This function deal with the legacy boot option, it create, delete
  and manage the legacy boot option, all legacy boot option is getting from
  the legacy BBS table.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalLegacyBm.h"

#define  LEGACY_BM_BOOT_DESCRIPTION_LENGTH  32

/**
  Initialize legacy boot manager library by call EfiBootManagerRegisterLegacyBootSupport
  function to export two function pointer.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval EFI_SUCCESS    The legacy boot manager library is initialized correctly.
  @return Other value if failed to initialize the legacy boot manager library.
**/
EFI_STATUS
EFIAPI
LegacyBootManagerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiBootManagerRegisterLegacyBootSupport (
    LegacyBmRefreshAllBootOption,
    LegacyBmBoot
    );
  return EFI_SUCCESS;
}

/**
  Get the device type from the input legacy device path.

  @param DevicePath     The legacy device path.

  @retval               The legacy device type.
**/
UINT16
LegacyBmDeviceType (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  ASSERT (
    (DevicePathType (DevicePath) == BBS_DEVICE_PATH) &&
    (DevicePathSubType (DevicePath) == BBS_BBS_DP)
    );
  return ((BBS_BBS_DEVICE_PATH *)DevicePath)->DeviceType;
}

/**
  Validate the BbsEntry base on the Boot Priority info in the BbsEntry.

  @param BbsEntry       The input bbs entry info.

  @retval TRUE          The BbsEntry is valid.
  @retval FALSE         The BbsEntry is invalid.
**/
BOOLEAN
LegacyBmValidBbsEntry (
  IN BBS_TABLE  *BbsEntry
  )
{
  switch (BbsEntry->BootPriority) {
    case BBS_IGNORE_ENTRY:
    case BBS_DO_NOT_BOOT_FROM:
    case BBS_LOWEST_PRIORITY:
      return FALSE;
    default:
      return TRUE;
  }
}

/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

**/
VOID
LegacyBmBuildLegacyDevNameString (
  IN  BBS_TABLE  *CurBBSEntry,
  IN  UINTN      Index,
  IN  UINTN      BufSize,
  OUT CHAR16     *BootString
  )
{
  CHAR16  *Fmt;
  CHAR16  *Type;
  CHAR8   *StringDesc;
  CHAR8   StringBufferA[LEGACY_BM_BOOT_DESCRIPTION_LENGTH + 1];
  CHAR16  StringBufferU[LEGACY_BM_BOOT_DESCRIPTION_LENGTH + 1];

  switch (Index) {
    //
    // Primary Master
    //
    case 1:
      Fmt = L"Primary Master %s";
      break;

    //
    // Primary Slave
    //
    case 2:
      Fmt = L"Primary Slave %s";
      break;

    //
    // Secondary Master
    //
    case 3:
      Fmt = L"Secondary Master %s";
      break;

    //
    // Secondary Slave
    //
    case 4:
      Fmt = L"Secondary Slave %s";
      break;

    default:
      Fmt = L"%s";
      break;
  }

  switch (CurBBSEntry->DeviceType) {
    case BBS_FLOPPY:
      Type = L"Floppy";
      break;

    case BBS_HARDDISK:
      Type = L"Harddisk";
      break;

    case BBS_CDROM:
      Type = L"CDROM";
      break;

    case BBS_PCMCIA:
      Type = L"PCMCIAe";
      break;

    case BBS_USB:
      Type = L"USB";
      break;

    case BBS_EMBED_NETWORK:
      Type = L"Network";
      break;

    case BBS_BEV_DEVICE:
      Type = L"BEVe";
      break;

    case BBS_UNKNOWN:
    default:
      Type = L"Unknown";
      break;
  }

  //
  // If current BBS entry has its description then use it.
  //
  StringDesc = (CHAR8 *)(((UINTN)CurBBSEntry->DescStringSegment << 4) + CurBBSEntry->DescStringOffset);
  if (NULL != StringDesc) {
    //
    // Only get first 32 characters, this is suggested by BBS spec
    //
    CopyMem (StringBufferA, StringDesc, LEGACY_BM_BOOT_DESCRIPTION_LENGTH);
    StringBufferA[LEGACY_BM_BOOT_DESCRIPTION_LENGTH] = 0;
    AsciiStrToUnicodeStrS (StringBufferA, StringBufferU, ARRAY_SIZE (StringBufferU));
    Fmt  = L"%s";
    Type = StringBufferU;
  }

  //
  // BbsTable 16 entries are for onboard IDE.
  // Set description string for SATA harddisks, Harddisk 0 ~ Harddisk 11
  //
  if ((Index >= 5) && (Index <= 16) && ((CurBBSEntry->DeviceType == BBS_HARDDISK) || (CurBBSEntry->DeviceType == BBS_CDROM))) {
    Fmt = L"%s %d";
    UnicodeSPrint (BootString, BufSize, Fmt, Type, Index - 5);
  } else {
    UnicodeSPrint (BootString, BufSize, Fmt, Type);
  }
}

/**
  Get the Bbs index for the input boot option.

  @param BootOption     The input boot option info.
  @param BbsTable       The input Bbs table.
  @param BbsCount       The input total bbs entry number.
  @param BbsIndexUsed   The array shows how many BBS table indexs have been used.

  @retval The index for the input boot option.
**/
UINT16
LegacyBmFuzzyMatch (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  BBS_TABLE                     *BbsTable,
  UINT16                        BbsCount,
  BOOLEAN                       *BbsIndexUsed
  )
{
  UINT16                          Index;
  LEGACY_BM_BOOT_OPTION_BBS_DATA  *BbsData;
  CHAR16                          Description[LEGACY_BM_BOOT_DESCRIPTION_LENGTH + 1];

  BbsData = (LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption->OptionalData;

  //
  // Directly check the BBS index stored in BootOption
  //
  if ((BbsData->BbsIndex < BbsCount) &&
      (LegacyBmDeviceType (BootOption->FilePath) == BbsTable[BbsData->BbsIndex].DeviceType))
  {
    LegacyBmBuildLegacyDevNameString (
      &BbsTable[BbsData->BbsIndex],
      BbsData->BbsIndex,
      sizeof (Description),
      Description
      );
    if ((StrCmp (Description, BootOption->Description) == 0) && !BbsIndexUsed[BbsData->BbsIndex]) {
      //
      // If devices with the same description string are connected,
      // the BbsIndex of the first device is returned for the other device also.
      // So, check if the BbsIndex is already being used, before assigning the BbsIndex.
      //
      BbsIndexUsed[BbsData->BbsIndex] = TRUE;
      return BbsData->BbsIndex;
    }
  }

  //
  // BBS table could be changed (entry removed/moved)
  // find the correct BBS index
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&BbsTable[Index]) ||
        (BbsTable[Index].DeviceType != LegacyBmDeviceType (BootOption->FilePath)))
    {
      continue;
    }

    LegacyBmBuildLegacyDevNameString (
      &BbsTable[Index],
      Index,
      sizeof (Description),
      Description
      );
    if ((StrCmp (Description, BootOption->Description) == 0) && !BbsIndexUsed[Index]) {
      //
      // If devices with the same description string are connected,
      // the BbsIndex of the first device is assigned for the other device also.
      // So, check if the BbsIndex is already being used, before assigning the corrected BbsIndex.
      //
      break;
    }
  }

  //
  // Add the corrected BbsIndex in the UsedBbsIndex Buffer
  //
  if (Index != BbsCount) {
    BbsIndexUsed[Index] = TRUE;
  }

  return Index;
}

/**

  Update legacy device order base on the input info.

  @param   LegacyDevOrder     Legacy device order data buffer.
  @param   LegacyDevOrderSize Legacy device order data buffer size.
  @param   DeviceType         Device type which need to check.
  @param   OldBbsIndex        Old Bds Index.
  @param   NewBbsIndex        New Bds Index, if it is -1,means remove this option.

**/
VOID
LegacyBmUpdateBbsIndex (
  LEGACY_DEV_ORDER_ENTRY  *LegacyDevOrder,
  UINTN                   *LegacyDevOrderSize,
  UINT16                  DeviceType,
  UINT16                  OldBbsIndex,
  UINT16                  NewBbsIndex  // Delete entry if -1
  )
{
  LEGACY_DEV_ORDER_ENTRY  *Entry;
  UINTN                   Index;

  ASSERT (
    ((LegacyDevOrder == NULL) && (*LegacyDevOrderSize == 0)) ||
    ((LegacyDevOrder != NULL) && (*LegacyDevOrderSize != 0))
    );

  for (Entry = LegacyDevOrder;
       Entry < (LEGACY_DEV_ORDER_ENTRY *)((UINT8 *)LegacyDevOrder + *LegacyDevOrderSize);
       Entry = (LEGACY_DEV_ORDER_ENTRY *)((UINTN)Entry + sizeof (BBS_TYPE) + Entry->Length)
       )
  {
    if (Entry->BbsType == DeviceType) {
      for (Index = 0; Index < Entry->Length / sizeof (UINT16) - 1; Index++) {
        if (Entry->Data[Index] == OldBbsIndex) {
          if (NewBbsIndex == (UINT16)-1) {
            //
            // Delete the old entry
            //
            CopyMem (
              &Entry->Data[Index],
              &Entry->Data[Index + 1],
              (UINT8 *)LegacyDevOrder + *LegacyDevOrderSize - (UINT8 *)&Entry->Data[Index + 1]
              );
            Entry->Length       -= sizeof (UINT16);
            *LegacyDevOrderSize -= sizeof (UINT16);
          } else {
            Entry->Data[Index] = NewBbsIndex;
          }

          break;
        }
      }

      break;
    }
  }
}

/**
  Delete all the legacy boot options.

  @retval EFI_SUCCESS            All legacy boot options are deleted.
**/
EFI_STATUS
LegacyBmDeleteAllBootOptions (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOptionCount; Index++) {
    if ((DevicePathType (BootOption[Index].FilePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (BootOption[Index].FilePath) == BBS_BBS_DP))
    {
      Status = EfiBootManagerDeleteLoadOptionVariable (BootOption[Index].OptionNumber, BootOption[Index].OptionType);
      //
      // Deleting variable with current variable implementation shouldn't fail.
      //
      ASSERT_EFI_ERROR (Status);
    }
  }

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  0,
                  0,
                  NULL
                  );
  //
  // Deleting variable with current variable implementation shouldn't fail.
  //
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);

  return EFI_SUCCESS;
}

/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalid legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrieve variable of boot order.
**/
EFI_STATUS
LegacyBmDeleteAllInvalidBootOptions (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *HddInfo;
  BBS_TABLE                     *BbsTable;
  UINT16                        BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  LEGACY_DEV_ORDER_ENTRY        *LegacyDevOrder;
  UINTN                         LegacyDevOrderSize;
  BOOLEAN                       *BbsIndexUsed;

  HddCount = 0;
  BbsCount = 0;
  HddInfo  = NULL;
  BbsTable = NULL;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &HddInfo,
                         &BbsCount,
                         &BbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetVariable2 (VAR_LEGACY_DEV_ORDER, &gEfiLegacyDevOrderVariableGuid, (VOID **)&LegacyDevOrder, &LegacyDevOrderSize);

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  BbsIndexUsed = AllocateZeroPool (BbsCount * sizeof (BOOLEAN));
  ASSERT (BbsIndexUsed != NULL);

  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // Skip non legacy boot option
    //
    if ((DevicePathType (BootOption[Index].FilePath) != BBS_DEVICE_PATH) ||
        (DevicePathSubType (BootOption[Index].FilePath) != BBS_BBS_DP))
    {
      continue;
    }

    BbsIndex = LegacyBmFuzzyMatch (&BootOption[Index], BbsTable, BbsCount, BbsIndexUsed);
    if (BbsIndex == BbsCount) {
      DEBUG ((DEBUG_INFO, "[LegacyBds] Delete Boot Option Boot%04x: %s\n", (UINTN)BootOption[Index].OptionNumber, BootOption[Index].Description));
      //
      // Delete entry from LegacyDevOrder
      //
      LegacyBmUpdateBbsIndex (
        LegacyDevOrder,
        &LegacyDevOrderSize,
        LegacyBmDeviceType (BootOption[Index].FilePath),
        ((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex,
        (UINT16)-1
        );
      EfiBootManagerDeleteLoadOptionVariable (BootOption[Index].OptionNumber, BootOption[Index].OptionType);
    } else {
      if (((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex != BbsIndex) {
        DEBUG ((
          DEBUG_INFO,
          "[LegacyBds] Update Boot Option Boot%04x: %s Bbs0x%04x->Bbs0x%04x\n",
          (UINTN)BootOption[Index].OptionNumber,
          BootOption[Index].Description,
          (UINTN)((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex,
          (UINTN)BbsIndex
          ));
        //
        // Update the BBS index in LegacyDevOrder
        //
        LegacyBmUpdateBbsIndex (
          LegacyDevOrder,
          &LegacyDevOrderSize,
          LegacyBmDeviceType (BootOption[Index].FilePath),
          ((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex,
          BbsIndex
          );

        //
        // Update the OptionalData in the Boot#### variable
        //
        ((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex = BbsIndex;
        EfiBootManagerLoadOptionToVariable (&BootOption[Index]);
      }
    }
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (LegacyDevOrder != NULL) {
    Status = gRT->SetVariable (
                    VAR_LEGACY_DEV_ORDER,
                    &gEfiLegacyDevOrderVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    LegacyDevOrderSize,
                    LegacyDevOrder
                    );
    //
    // Shrink variable with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);

    FreePool (LegacyDevOrder);
  }

  FreePool (BbsIndexUsed);
  return Status;
}

/**
  Create legacy boot option.

  @param BootOption        Pointer to the boot option which will be crated.
  @param BbsEntry          The input bbs entry info.
  @param BbsIndex          The BBS index.

  @retval EFI_SUCCESS            Create legacy boot option successfully.
  @retval EFI_INVALID_PARAMETER  Invalid input parameter.

**/
EFI_STATUS
LegacyBmCreateLegacyBootOption (
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  IN BBS_TABLE                         *BbsEntry,
  IN UINT16                            BbsIndex
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  CHAR16                          Description[LEGACY_BM_BOOT_DESCRIPTION_LENGTH + 1];
  CHAR8                           HelpString[LEGACY_BM_BOOT_DESCRIPTION_LENGTH + 1];
  UINTN                           StringLen;
  LEGACY_BM_BOOT_OPTION_BBS_DATA  *OptionalData;
  BBS_BBS_DEVICE_PATH             *BbsNode;

  if ((BootOption == NULL) || (BbsEntry == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  LegacyBmBuildLegacyDevNameString (BbsEntry, BbsIndex, sizeof (Description), Description);

  //
  // Create the BBS device path with description string
  //
  UnicodeStrToAsciiStrS (Description, HelpString, sizeof (HelpString));
  StringLen  = AsciiStrLen (HelpString);
  DevicePath = AllocatePool (sizeof (BBS_BBS_DEVICE_PATH) + StringLen + END_DEVICE_PATH_LENGTH);
  ASSERT (DevicePath != NULL);

  BbsNode = (BBS_BBS_DEVICE_PATH *)DevicePath;
  SetDevicePathNodeLength (BbsNode, sizeof (BBS_BBS_DEVICE_PATH) + StringLen);
  BbsNode->Header.Type    = BBS_DEVICE_PATH;
  BbsNode->Header.SubType = BBS_BBS_DP;
  BbsNode->DeviceType     = BbsEntry->DeviceType;
  CopyMem (&BbsNode->StatusFlag, &BbsEntry->StatusFlags, sizeof (BBS_STATUS_FLAGS));
  CopyMem (BbsNode->String, HelpString, StringLen + 1);

  SetDevicePathEndNode (NextDevicePathNode (BbsNode));

  //
  // Create the OptionalData
  //
  OptionalData = AllocatePool (sizeof (LEGACY_BM_BOOT_OPTION_BBS_DATA));
  ASSERT (OptionalData != NULL);
  OptionalData->BbsIndex = BbsIndex;

  //
  // Create the BootOption
  //
  Status = EfiBootManagerInitializeLoadOption (
             BootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             Description,
             DevicePath,
             (UINT8 *)OptionalData,
             sizeof (LEGACY_BM_BOOT_OPTION_BBS_DATA)
             );
  FreePool (DevicePath);
  FreePool (OptionalData);

  return Status;
}

/**
  Fill the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsType         The BBS Type.
  @param BbsCount        The BBS Count.
  @param Buf             device order buffer.

  @return The device order buffer.

**/
UINT16 *
LegacyBmFillDevOrderBuf (
  IN BBS_TABLE  *BbsTable,
  IN BBS_TYPE   BbsType,
  IN UINTN      BbsCount,
  OUT UINT16    *Buf
  )
{
  UINTN  Index;

  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&BbsTable[Index])) {
      continue;
    }

    if (BbsTable[Index].DeviceType != BbsType) {
      continue;
    }

    *Buf = (UINT16)(Index & 0xFF);
    Buf++;
  }

  return Buf;
}

/**
  Create the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsCount        The BBS Count.

  @retval EFI_SUCCESS            The buffer is created and the EFI variable named
                                 VAR_LEGACY_DEV_ORDER and EfiLegacyDevOrderGuid is
                                 set correctly.
  @retval EFI_OUT_OF_RESOURCES   Memory or storage is not enough.
  @retval EFI_DEVICE_ERROR       Fail to add the device order into EFI variable fail
                                 because of hardware error.
**/
EFI_STATUS
LegacyBmCreateDevOrder (
  IN BBS_TABLE  *BbsTable,
  IN UINT16     BbsCount
  )
{
  UINTN                   Index;
  UINTN                   FDCount;
  UINTN                   HDCount;
  UINTN                   CDCount;
  UINTN                   NETCount;
  UINTN                   BEVCount;
  UINTN                   TotalSize;
  UINTN                   HeaderSize;
  LEGACY_DEV_ORDER_ENTRY  *DevOrder;
  LEGACY_DEV_ORDER_ENTRY  *DevOrderPtr;
  EFI_STATUS              Status;

  FDCount    = 0;
  HDCount    = 0;
  CDCount    = 0;
  NETCount   = 0;
  BEVCount   = 0;
  TotalSize  = 0;
  HeaderSize = sizeof (BBS_TYPE) + sizeof (UINT16);
  DevOrder   = NULL;
  Status     = EFI_SUCCESS;

  //
  // Count all boot devices
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&BbsTable[Index])) {
      continue;
    }

    switch (BbsTable[Index].DeviceType) {
      case BBS_FLOPPY:
        FDCount++;
        break;

      case BBS_HARDDISK:
        HDCount++;
        break;

      case BBS_CDROM:
        CDCount++;
        break;

      case BBS_EMBED_NETWORK:
        NETCount++;
        break;

      case BBS_BEV_DEVICE:
        BEVCount++;
        break;

      default:
        break;
    }
  }

  TotalSize += (HeaderSize + sizeof (UINT16) * FDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * HDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * CDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * NETCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * BEVCount);

  //
  // Create buffer to hold all boot device order
  //
  DevOrder = AllocateZeroPool (TotalSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  DevOrderPtr = DevOrder;

  DevOrderPtr->BbsType = BBS_FLOPPY;
  DevOrderPtr->Length  = (UINT16)(sizeof (DevOrderPtr->Length) + FDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *)LegacyBmFillDevOrderBuf (BbsTable, BBS_FLOPPY, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_HARDDISK;
  DevOrderPtr->Length  = (UINT16)(sizeof (UINT16) + HDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *)LegacyBmFillDevOrderBuf (BbsTable, BBS_HARDDISK, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_CDROM;
  DevOrderPtr->Length  = (UINT16)(sizeof (UINT16) + CDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *)LegacyBmFillDevOrderBuf (BbsTable, BBS_CDROM, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_EMBED_NETWORK;
  DevOrderPtr->Length  = (UINT16)(sizeof (UINT16) + NETCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *)LegacyBmFillDevOrderBuf (BbsTable, BBS_EMBED_NETWORK, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_BEV_DEVICE;
  DevOrderPtr->Length  = (UINT16)(sizeof (UINT16) + BEVCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *)LegacyBmFillDevOrderBuf (BbsTable, BBS_BEV_DEVICE, BbsCount, DevOrderPtr->Data);

  ASSERT (TotalSize == ((UINTN)DevOrderPtr - (UINTN)DevOrder));

  //
  // Save device order for legacy boot device to variable.
  //
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  TotalSize,
                  DevOrder
                  );
  FreePool (DevOrder);

  return Status;
}

/**
  Add the legacy boot devices from BBS table into
  the legacy device boot order.

  @retval EFI_SUCCESS           The boot devices are added successfully.
  @retval EFI_NOT_FOUND         The legacy boot devices are not found.
  @retval EFI_OUT_OF_RESOURCES  Memory or storage is not enough.
  @retval EFI_DEVICE_ERROR      Fail to add the legacy device boot order into EFI variable
                                because of hardware error.
**/
EFI_STATUS
LegacyBmUpdateDevOrder (
  VOID
  )
{
  LEGACY_DEV_ORDER_ENTRY    *DevOrder;
  LEGACY_DEV_ORDER_ENTRY    *NewDevOrder;
  LEGACY_DEV_ORDER_ENTRY    *Ptr;
  LEGACY_DEV_ORDER_ENTRY    *NewPtr;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     *Idx;
  UINTN                     FDCount;
  UINTN                     HDCount;
  UINTN                     CDCount;
  UINTN                     NETCount;
  UINTN                     BEVCount;
  UINTN                     TotalSize;
  UINTN                     HeaderSize;
  UINT16                    *NewFDPtr;
  UINT16                    *NewHDPtr;
  UINT16                    *NewCDPtr;
  UINT16                    *NewNETPtr;
  UINT16                    *NewBEVPtr;
  UINT16                    *NewDevPtr;
  UINTN                     FDIndex;
  UINTN                     HDIndex;
  UINTN                     CDIndex;
  UINTN                     NETIndex;
  UINTN                     BEVIndex;

  Idx        = NULL;
  FDCount    = 0;
  HDCount    = 0;
  CDCount    = 0;
  NETCount   = 0;
  BEVCount   = 0;
  TotalSize  = 0;
  HeaderSize = sizeof (BBS_TYPE) + sizeof (UINT16);
  FDIndex    = 0;
  HDIndex    = 0;
  CDIndex    = 0;
  NETIndex   = 0;
  BEVIndex   = 0;
  NewDevPtr  = NULL;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetVariable2 (VAR_LEGACY_DEV_ORDER, &gEfiLegacyDevOrderVariableGuid, (VOID **)&DevOrder, NULL);
  if (NULL == DevOrder) {
    return LegacyBmCreateDevOrder (LocalBbsTable, BbsCount);
  }

  //
  // First we figure out how many boot devices with same device type respectively
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
      case BBS_FLOPPY:
        FDCount++;
        break;

      case BBS_HARDDISK:
        HDCount++;
        break;

      case BBS_CDROM:
        CDCount++;
        break;

      case BBS_EMBED_NETWORK:
        NETCount++;
        break;

      case BBS_BEV_DEVICE:
        BEVCount++;
        break;

      default:
        break;
    }
  }

  TotalSize += (HeaderSize + FDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + HDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + CDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + NETCount * sizeof (UINT16));
  TotalSize += (HeaderSize + BEVCount * sizeof (UINT16));

  NewDevOrder = AllocateZeroPool (TotalSize);
  if (NULL == NewDevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // copy FD
  //
  Ptr             = DevOrder;
  NewPtr          = NewDevOrder;
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16)(sizeof (UINT16) + FDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        (LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_FLOPPY)
        )
    {
      continue;
    }

    NewPtr->Data[FDIndex] = Ptr->Data[Index];
    FDIndex++;
  }

  NewFDPtr = NewPtr->Data;

  //
  // copy HD
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *)(&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *)(&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16)(sizeof (UINT16) + HDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        (LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_HARDDISK)
        )
    {
      continue;
    }

    NewPtr->Data[HDIndex] = Ptr->Data[Index];
    HDIndex++;
  }

  NewHDPtr = NewPtr->Data;

  //
  // copy CD
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *)(&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *)(&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16)(sizeof (UINT16) + CDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        (LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_CDROM)
        )
    {
      continue;
    }

    NewPtr->Data[CDIndex] = Ptr->Data[Index];
    CDIndex++;
  }

  NewCDPtr = NewPtr->Data;

  //
  // copy NET
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *)(&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *)(&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16)(sizeof (UINT16) + NETCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        (LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_EMBED_NETWORK)
        )
    {
      continue;
    }

    NewPtr->Data[NETIndex] = Ptr->Data[Index];
    NETIndex++;
  }

  NewNETPtr = NewPtr->Data;

  //
  // copy BEV
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *)(&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *)(&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16)(sizeof (UINT16) + BEVCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        (LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_BEV_DEVICE)
        )
    {
      continue;
    }

    NewPtr->Data[BEVIndex] = Ptr->Data[Index];
    BEVIndex++;
  }

  NewBEVPtr = NewPtr->Data;

  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
      case BBS_FLOPPY:
        Idx       = &FDIndex;
        NewDevPtr = NewFDPtr;
        break;

      case BBS_HARDDISK:
        Idx       = &HDIndex;
        NewDevPtr = NewHDPtr;
        break;

      case BBS_CDROM:
        Idx       = &CDIndex;
        NewDevPtr = NewCDPtr;
        break;

      case BBS_EMBED_NETWORK:
        Idx       = &NETIndex;
        NewDevPtr = NewNETPtr;
        break;

      case BBS_BEV_DEVICE:
        Idx       = &BEVIndex;
        NewDevPtr = NewBEVPtr;
        break;

      default:
        Idx = NULL;
        break;
    }

    //
    // at this point we have copied those valid indexes to new buffer
    // and we should check if there is any new appeared boot device
    //
    if (Idx != NULL) {
      for (Index2 = 0; Index2 < *Idx; Index2++) {
        if ((NewDevPtr[Index2] & 0xFF) == (UINT16)Index) {
          break;
        }
      }

      if (Index2 == *Idx) {
        //
        // Index2 == *Idx means we didn't find Index
        // so Index is a new appeared device's index in BBS table
        // insert it before disabled indexes.
        //
        for (Index2 = 0; Index2 < *Idx; Index2++) {
          if ((NewDevPtr[Index2] & 0xFF00) == 0xFF00) {
            break;
          }
        }

        CopyMem (&NewDevPtr[Index2 + 1], &NewDevPtr[Index2], (*Idx - Index2) * sizeof (UINT16));
        NewDevPtr[Index2] = (UINT16)(Index & 0xFF);
        (*Idx)++;
      }
    }
  }

  FreePool (DevOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  TotalSize,
                  NewDevOrder
                  );
  FreePool (NewDevOrder);

  return Status;
}

/**
  Set Boot Priority for specified device type.

  @param DeviceType      The device type.
  @param BbsIndex        The BBS index to set the highest priority. Ignore when -1.
  @param LocalBbsTable   The BBS table.
  @param Priority        The priority table.

  @retval EFI_SUCCESS           The function completes successfully.
  @retval EFI_NOT_FOUND         Failed to find device.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the efi variable of device order.

**/
EFI_STATUS
LegacyBmSetPriorityForSameTypeDev (
  IN UINT16         DeviceType,
  IN UINTN          BbsIndex,
  IN OUT BBS_TABLE  *LocalBbsTable,
  IN OUT UINT16     *Priority
  )
{
  LEGACY_DEV_ORDER_ENTRY  *DevOrder;
  LEGACY_DEV_ORDER_ENTRY  *DevOrderPtr;
  UINTN                   DevOrderSize;
  UINTN                   Index;

  GetVariable2 (VAR_LEGACY_DEV_ORDER, &gEfiLegacyDevOrderVariableGuid, (VOID **)&DevOrder, &DevOrderSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  DevOrderPtr = DevOrder;
  while ((UINT8 *)DevOrderPtr < (UINT8 *)DevOrder + DevOrderSize) {
    if (DevOrderPtr->BbsType == DeviceType) {
      break;
    }

    DevOrderPtr = (LEGACY_DEV_ORDER_ENTRY *)((UINTN)DevOrderPtr + sizeof (BBS_TYPE) + DevOrderPtr->Length);
  }

  if ((UINT8 *)DevOrderPtr >= (UINT8 *)DevOrder + DevOrderSize) {
    FreePool (DevOrder);
    return EFI_NOT_FOUND;
  }

  if (BbsIndex != (UINTN)-1) {
    //
    // In case the BBS entry isn't valid because devices were plugged or removed.
    //
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[BbsIndex]) || (LocalBbsTable[BbsIndex].DeviceType != DeviceType)) {
      FreePool (DevOrder);
      return EFI_NOT_FOUND;
    }

    LocalBbsTable[BbsIndex].BootPriority = *Priority;
    (*Priority)++;
  }

  //
  // If the high byte of the DevIndex is 0xFF, it indicates that this device has been disabled.
  //
  for (Index = 0; Index < DevOrderPtr->Length / sizeof (UINT16) - 1; Index++) {
    if ((DevOrderPtr->Data[Index] & 0xFF00) == 0xFF00) {
      //
      // LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = BBS_DISABLED_ENTRY;
      //
    } else if (DevOrderPtr->Data[Index] != BbsIndex) {
      LocalBbsTable[DevOrderPtr->Data[Index]].BootPriority = *Priority;
      (*Priority)++;
    }
  }

  FreePool (DevOrder);
  return EFI_SUCCESS;
}

/**
  Print the BBS Table.

  @param LocalBbsTable   The BBS table.
  @param BbsCount        The count of entry in BBS table.
**/
VOID
LegacyBmPrintBbsTable (
  IN BBS_TABLE  *LocalBbsTable,
  IN UINT16     BbsCount
  )
{
  UINT16  Index;

  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, " NO  Prio bb/dd/ff cl/sc Type Stat segm:offs mseg dseg\n"));
  DEBUG ((DEBUG_INFO, "======================================================\n"));
  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    DEBUG (
      (DEBUG_INFO,
       " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x %04x %04x\n",
       (UINTN)Index,
       (UINTN)LocalBbsTable[Index].BootPriority,
       (UINTN)LocalBbsTable[Index].Bus,
       (UINTN)LocalBbsTable[Index].Device,
       (UINTN)LocalBbsTable[Index].Function,
       (UINTN)LocalBbsTable[Index].Class,
       (UINTN)LocalBbsTable[Index].SubClass,
       (UINTN)LocalBbsTable[Index].DeviceType,
       (UINTN)*(UINT16 *)&LocalBbsTable[Index].StatusFlags,
       (UINTN)LocalBbsTable[Index].BootHandlerSegment,
       (UINTN)LocalBbsTable[Index].BootHandlerOffset,
       (UINTN)((LocalBbsTable[Index].MfgStringSegment << 4) + LocalBbsTable[Index].MfgStringOffset),
       (UINTN)((LocalBbsTable[Index].DescStringSegment << 4) + LocalBbsTable[Index].DescStringOffset))
      );
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  BootOption            The boot option is to be checked for refresh BBS table.

  @retval EFI_SUCCESS           The boot priority for BBS entries is refreshed successfully.
  @retval EFI_NOT_FOUND         BBS entries can't be found.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the legacy device boot order.
**/
EFI_STATUS
LegacyBmRefreshBbsTableForBoot (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                    Status;
  UINT16                        BbsIndex;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *LocalHddInfo;
  BBS_TABLE                     *LocalBbsTable;
  UINT16                        DevType;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINTN                         Index;
  UINT16                        Priority;
  UINT16                        *DeviceType;
  UINTN                         DeviceTypeCount;
  UINTN                         DeviceTypeIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION  *Option;
  UINTN                         OptionCount;

  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  DevType       = BBS_UNKNOWN;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // First, set all the present devices' boot priority to BBS_UNPRIORITIZED_ENTRY
  // We will set them according to the settings setup by user
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (LegacyBmValidBbsEntry (&LocalBbsTable[Index])) {
      LocalBbsTable[Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
    }
  }

  //
  // boot priority always starts at 0
  //
  Priority = 0;
  if ((DevicePathType (BootOption->FilePath) == BBS_DEVICE_PATH) &&
      (DevicePathSubType (BootOption->FilePath) == BBS_BBS_DP))
  {
    //
    // If BootOption stands for a legacy boot option, we prioritize the devices with the same type first.
    //
    DevType  = LegacyBmDeviceType (BootOption->FilePath);
    BbsIndex = ((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOption->OptionalData)->BbsIndex;
    Status   = LegacyBmSetPriorityForSameTypeDev (
                 DevType,
                 BbsIndex,
                 LocalBbsTable,
                 &Priority
                 );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // we have to set the boot priority for other BBS entries with different device types
  //
  Option     = EfiBootManagerGetLoadOptions (&OptionCount, LoadOptionTypeBoot);
  DeviceType = AllocatePool (sizeof (UINT16) * OptionCount);
  ASSERT (DeviceType != NULL);
  DeviceType[0]   = DevType;
  DeviceTypeCount = 1;
  for (Index = 0; Index < OptionCount; Index++) {
    if ((DevicePathType (Option[Index].FilePath) != BBS_DEVICE_PATH) ||
        (DevicePathSubType (Option[Index].FilePath) != BBS_BBS_DP))
    {
      continue;
    }

    DevType = LegacyBmDeviceType (Option[Index].FilePath);
    for (DeviceTypeIndex = 0; DeviceTypeIndex < DeviceTypeCount; DeviceTypeIndex++) {
      if (DeviceType[DeviceTypeIndex] == DevType) {
        break;
      }
    }

    if (DeviceTypeIndex < DeviceTypeCount) {
      //
      // We don't want to process twice for a device type
      //
      continue;
    }

    DeviceType[DeviceTypeCount] = DevType;
    DeviceTypeCount++;

    Status = LegacyBmSetPriorityForSameTypeDev (
               DevType,
               (UINTN)-1,
               LocalBbsTable,
               &Priority
               );
  }

  EfiBootManagerFreeLoadOptions (Option, OptionCount);

  DEBUG_CODE_BEGIN ();
  LegacyBmPrintBbsTable (LocalBbsTable, BbsCount);
  DEBUG_CODE_END ();

  return Status;
}

/**
  Boot the legacy system with the boot option.

  @param  BootOption The legacy boot option which have BBS device path
                     On return, BootOption->Status contains the boot status.
                     EFI_UNSUPPORTED    There is no legacybios protocol, do not support
                                        legacy boot.
                     EFI_STATUS         The status of LegacyBios->LegacyBoot ().
**/
VOID
EFIAPI
LegacyBmBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    //
    // If no LegacyBios protocol we do not support legacy boot
    //
    BootOption->Status = EFI_UNSUPPORTED;
    return;
  }

  //
  // Notes: if we separate the int 19, then we don't need to refresh BBS
  //
  Status = LegacyBmRefreshBbsTableForBoot (BootOption);
  if (EFI_ERROR (Status)) {
    BootOption->Status = Status;
    return;
  }

  BootOption->Status = LegacyBios->LegacyBoot (
                                     LegacyBios,
                                     (BBS_BBS_DEVICE_PATH *)BootOption->FilePath,
                                     BootOption->OptionalDataSize,
                                     BootOption->OptionalData
                                     );
}

/**
  This function enumerates all the legacy boot options.

  @param BootOptionCount   Return the legacy boot option count.

  @retval    Pointer to the legacy boot option buffer.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
LegacyBmEnumerateAllBootOptions (
  UINTN  *BootOptionCount
  )
{
  EFI_STATUS                    Status;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *HddInfo;
  BBS_TABLE                     *BbsTable;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINT16                        Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;

  ASSERT (BootOptionCount != NULL);

  BootOptions      = NULL;
  *BootOptionCount = 0;
  BbsCount         = 0;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &HddInfo,
                         &BbsCount,
                         &BbsTable
                         );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if (!LegacyBmValidBbsEntry (&BbsTable[Index])) {
      continue;
    }

    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = LegacyBmCreateLegacyBootOption (&BootOptions[(*BootOptionCount)++], &BbsTable[Index], Index);
    ASSERT_EFI_ERROR (Status);
  }

  return BootOptions;
}

/**
  Return the index of the boot option in the boot option array.

  The function compares the Description, FilePath, OptionalData.

  @param Key         The input boot option which is compared with.
  @param Array       The input boot option array.
  @param Count       The count of the input boot options.

  @retval  The index of the input boot option in the array.

**/
INTN
LegacyBmFindBootOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
  IN UINTN                               Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++) {
    if ((StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0))
    {
      return (INTN)Index;
    }
  }

  return -1;
}

/**
  Refresh all legacy boot options.

**/
VOID
EFIAPI
LegacyBmRefreshAllBootOption (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINTN                         RootBridgeHandleCount;
  EFI_HANDLE                    *RootBridgeHandleBuffer;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         RootBridgeIndex;
  UINTN                         Index;
  UINTN                         Flags;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *ExistingBootOptions;
  UINTN                         ExistingBootOptionCount;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    LegacyBmDeleteAllBootOptions ();
    return;
  }

  PERF_START (NULL, "LegacyBootOptionEnum", "BDS", 0);

  //
  // Before enumerating the legacy boot option, we need to dispatch all the legacy option roms
  // to ensure the GetBbsInfo() counts all the legacy devices.
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);
    gBS->LocateHandleBuffer (
           ByProtocol,
           &gEfiPciIoProtocolGuid,
           NULL,
           &HandleCount,
           &HandleBuffer
           );
    for (Index = 0; Index < HandleCount; Index++) {
      //
      // Start the thunk driver so that the legacy option rom gets dispatched.
      // Note: We don't directly call InstallPciRom because some thunk drivers
      // (e.g. BlockIo thunk driver) depend on the immediate result after dispatching
      //
      Status = LegacyBios->CheckPciRom (
                             LegacyBios,
                             HandleBuffer[Index],
                             NULL,
                             NULL,
                             &Flags
                             );
      if (!EFI_ERROR (Status)) {
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);
      }
    }
  }

  //
  // Same algorithm pattern as the EfiBootManagerRefreshAllBootOption
  // Firstly delete the invalid legacy boot options,
  // then enumerate and save the newly appeared legacy boot options
  // the last step is legacy boot option special action to refresh the LegacyDevOrder variable
  //
  LegacyBmDeleteAllInvalidBootOptions ();

  ExistingBootOptions = EfiBootManagerGetLoadOptions (&ExistingBootOptionCount, LoadOptionTypeBoot);
  BootOptions         = LegacyBmEnumerateAllBootOptions (&BootOptionCount);

  for (Index = 0; Index < BootOptionCount; Index++) {
    if (LegacyBmFindBootOption (&BootOptions[Index], ExistingBootOptions, ExistingBootOptionCount) == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&BootOptions[Index], (UINTN)-1);
      DEBUG ((
        DEBUG_INFO,
        "[LegacyBds] New Boot Option: Boot%04x Bbs0x%04x %s %r\n",
        (UINTN)BootOptions[Index].OptionNumber,
        (UINTN)((LEGACY_BM_BOOT_OPTION_BBS_DATA *)BootOptions[Index].OptionalData)->BbsIndex,
        BootOptions[Index].Description,
        Status
        ));
      //
      // Continue upon failure to add boot option.
      //
    }
  }

  EfiBootManagerFreeLoadOptions (ExistingBootOptions, ExistingBootOptionCount);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

  //
  // Failure to create LegacyDevOrder variable only impacts the boot order.
  //
  LegacyBmUpdateDevOrder ();

  PERF_END (NULL, "LegacyBootOptionEnum", "BDS", 0);
}
