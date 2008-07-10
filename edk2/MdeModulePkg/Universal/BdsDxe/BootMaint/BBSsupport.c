/** @file
  This function deal with the legacy boot option, it create, delete
  and manage the legacy boot option, all legacy boot option is getting from
  the legacy BBS table.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BBSsupport.h"

/**

  Translate the first n characters of an Ascii string to
  Unicode characters. The count n is indicated by parameter
  Size. If Size is greater than the length of string, then
  the entire string is translated.


  @param a               Pointer to input Ascii string.
  @param Size            The number of characters to translate.
  @param u               Pointer to output Unicode string buffer.

  @return None

**/
VOID
AsciiToUnicodeSize (
  IN UINT8              *a,
  IN UINTN              Size,
  OUT UINT16            *u
  )
{
  UINTN i;

  i = 0;
  while (a[i] != 0) {
    u[i] = (CHAR16) a[i];
    if (i == Size) {
      break;
    }

    i++;
  }
  u[i] = 0;
}

/**

  change a Unicode string t ASCII string


  @param UStr            Unicode string
                         Lenght - most possible length of AStr
  @param Length          The length of UStr.
  @param AStr            ASCII string to pass out

  @return Actual length

**/
UINTN
UnicodeToAscii (
  IN  CHAR16  *UStr,
  IN  UINTN   Length,
  OUT CHAR8   *AStr
  )
{
  UINTN Index;

  //
  // just buffer copy, not character copy
  //
  for (Index = 0; Index < Length; Index++) {
    *AStr++ = (CHAR8) *UStr++;
  }

  return Index;
}

/**
  EDES_TODO: Add function description.

  @param CurBBSEntry     EDES_TODO: Add parameter description
  @param Index           EDES_TODO: Add parameter description
  @param BufSize         EDES_TODO: Add parameter description
  @param BootString      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BdsBuildLegacyDevNameString (
  IN BBS_TABLE                 *CurBBSEntry,
  IN UINTN                     Index,
  IN UINTN                     BufSize,
  OUT CHAR16                   *BootString
  )
{
  CHAR16  *Fmt;
  CHAR16  *Type;
  UINT8   *StringDesc;
  CHAR16  temp[80];

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
  StringDesc = (UINT8 *) (UINTN) ((CurBBSEntry->DescStringSegment << 4) + CurBBSEntry->DescStringOffset);
  if (NULL != StringDesc) {
    //
    // Only get fisrt 32 characters, this is suggested by BBS spec
    //
    AsciiToUnicodeSize (StringDesc, 32, temp);
    Fmt   = L"%s";
    Type  = temp;
  }

  //
  // BbsTable 16 entries are for onboard IDE.
  // Set description string for SATA harddisks, Harddisk 0 ~ Harddisk 11
  //
  if (Index >= 5 && Index <= 16 && CurBBSEntry->DeviceType == BBS_HARDDISK) {
    Fmt = L"%s %d";
    UnicodeSPrint (BootString, BufSize, Fmt, Type, Index - 5);
  } else {
    UnicodeSPrint (BootString, BufSize, Fmt, Type);
  }
}

/**

  Create a legacy boot option for the specified entry of
  BBS table, save it as variable, and append it to the boot
  order list.


  @param CurrentBbsEntry Pointer to current BBS table.
  @param CurrentBbsDevPath Pointer to the Device Path Protocol instance of BBS
  @param Index           Index of the specified entry in BBS table.
  @param BootOrderList   On input, the original boot order list.
                         On output, the new boot order list attached with the
                         created node.
  @param BootOrderListSize On input, the original size of boot order list.
                         - On output, the size of new boot order list.

  @retval  EFI_SUCCESS             Boot Option successfully created.
  @retval  EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval  Other                   Error occurs while setting variable.

**/
EFI_STATUS
BdsCreateLegacyBootOption (
  IN BBS_TABLE                        *CurrentBbsEntry,
  IN EFI_DEVICE_PATH_PROTOCOL         *CurrentBbsDevPath,
  IN UINTN                            Index,
  IN OUT UINT16                       **BootOrderList,
  IN OUT UINTN                        *BootOrderListSize
  )
{
  EFI_STATUS           Status;
  UINT16               CurrentBootOptionNo;
  UINT16               BootString[10];
  UINT16               BootDesc[100];
  CHAR8                HelpString[100];
  UINT16               *NewBootOrderList;
  UINTN                BufferSize;
  UINTN                StringLen;
  VOID                 *Buffer;
  UINT8                *Ptr;
  UINT16               CurrentBbsDevPathSize;
  UINTN                BootOrderIndex;
  UINTN                BootOrderLastIndex;
  UINTN                ArrayIndex;
  BOOLEAN              IndexNotFound;
  BBS_BBS_DEVICE_PATH  *NewBbsDevPathNode;

  if (NULL == (*BootOrderList)) {
    CurrentBootOptionNo = 0;
  } else {
    for (ArrayIndex = 0; ArrayIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); ArrayIndex++) {
      IndexNotFound = TRUE;
      for (BootOrderIndex = 0; BootOrderIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); BootOrderIndex++) {
        if ((*BootOrderList)[BootOrderIndex] == ArrayIndex) {
          IndexNotFound = FALSE;
          break;
        }
      }

      if (!IndexNotFound) {
        continue;
      } else {
        break;
      }
    }

    CurrentBootOptionNo = (UINT16) ArrayIndex;
  }

  UnicodeSPrint (
    BootString,
    sizeof (BootString),
    L"Boot%04x",
    CurrentBootOptionNo
    );

  BdsBuildLegacyDevNameString (CurrentBbsEntry, Index, sizeof (BootDesc), BootDesc);

  //
  // Create new BBS device path node with description string
  //
  UnicodeToAscii (BootDesc, StrSize (BootDesc), HelpString);
  StringLen = AsciiStrLen (HelpString);
  NewBbsDevPathNode = AllocateZeroPool (sizeof (BBS_BBS_DEVICE_PATH) + StringLen);
  if (NewBbsDevPathNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (NewBbsDevPathNode, CurrentBbsDevPath, sizeof (BBS_BBS_DEVICE_PATH));
  CopyMem (NewBbsDevPathNode->String, HelpString, StringLen + 1);
  SetDevicePathNodeLength (&(NewBbsDevPathNode->Header), sizeof (BBS_BBS_DEVICE_PATH) + StringLen);

  //
  // Create entire new CurrentBbsDevPath with end node
  //
  CurrentBbsDevPath = AppendDevicePathNode (
                        EndDevicePath,
                        (EFI_DEVICE_PATH_PROTOCOL *) NewBbsDevPathNode
                        );
   if (CurrentBbsDevPath == NULL) {
    FreePool (NewBbsDevPathNode);
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentBbsDevPathSize = (UINT16) (GetDevicePathSize (CurrentBbsDevPath));

  BufferSize = sizeof (UINT32) +
    sizeof (UINT16) +
    StrSize (BootDesc) +
    CurrentBbsDevPathSize +
    sizeof (BBS_TABLE) +
    sizeof (UINT16);

  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr               = (UINT8 *) Buffer;

  *((UINT32 *) Ptr) = LOAD_OPTION_ACTIVE;
  Ptr += sizeof (UINT32);

  *((UINT16 *) Ptr) = CurrentBbsDevPathSize;
  Ptr += sizeof (UINT16);

  CopyMem (
    Ptr,
    BootDesc,
    StrSize (BootDesc)
    );
  Ptr += StrSize (BootDesc);

  CopyMem (
    Ptr,
    CurrentBbsDevPath,
    CurrentBbsDevPathSize
    );
  Ptr += CurrentBbsDevPathSize;

  CopyMem (
    Ptr,
    CurrentBbsEntry,
    sizeof (BBS_TABLE)
    );

  Ptr += sizeof (BBS_TABLE);
  *((UINT16 *) Ptr) = (UINT16) Index;

  Status = gRT->SetVariable (
                  BootString,
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BufferSize,
                  Buffer
                  );

  SafeFreePool (Buffer);
  Buffer = NULL;

  NewBootOrderList = AllocateZeroPool (*BootOrderListSize + sizeof (UINT16));
  if (NULL == NewBootOrderList) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  if (NULL != *BootOrderList) {
    CopyMem (NewBootOrderList, *BootOrderList, *BootOrderListSize);
  }

  SafeFreePool (*BootOrderList);

  BootOrderLastIndex                    = (UINTN) (*BootOrderListSize / sizeof (UINT16));
  NewBootOrderList[BootOrderLastIndex]  = CurrentBootOptionNo;
  *BootOrderListSize += sizeof (UINT16);
  *BootOrderList = NewBootOrderList;

  FreePool (NewBbsDevPathNode);
  FreePool (CurrentBbsDevPath);
  return Status;
}

/**
  EDES_TODO: Add function description.

  @param BootOptionVar   EDES_TODO: Add parameter description
  @param BbsEntry        EDES_TODO: Add parameter description
  @param BbsIndex        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
BdsIsLegacyBootOption (
  IN UINT8                 *BootOptionVar,
  OUT BBS_TABLE            **BbsEntry,
  OUT UINT16               *BbsIndex
  )
{
  UINT8                     *Ptr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Ret;
  UINT16                    DevPathLen;

  Ptr = BootOptionVar;
  Ptr += sizeof (UINT32);
  DevPathLen = *(UINT16 *) Ptr;
  Ptr += sizeof (UINT16);
  Ptr += StrSize ((UINT16 *) Ptr);
  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
  if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
    Ptr += DevPathLen;
    *BbsEntry = (BBS_TABLE *) Ptr;
    Ptr += sizeof (BBS_TABLE);
    *BbsIndex = *(UINT16 *) Ptr;
    Ret       = TRUE;
  } else {
    *BbsEntry = NULL;
    Ret       = FALSE;
  }

  return Ret;
}

/**

  Delete boot option specified by OptionNumber and adjust the boot order.

  @param  OptionNumber      The boot option to be deleted.
  @param  BootOrder         Boot order list to be adjusted by remove this boot option.
  @param  BootOrderSize     The size of Boot order list will be modified.
  
  @retval EFI_SUCCESS       The boot option is deleted successfully.

**/
EFI_STATUS
EFIAPI
BdsDeleteBootOption (
  IN UINTN                       OptionNumber,
  IN OUT UINT16                  *BootOrder,
  IN OUT UINTN                   *BootOrderSize
  )
{
  UINT16      BootOption[100];
  UINTN       Index;
  EFI_STATUS  Status;
  UINTN       Index2Del;

  Status    = EFI_SUCCESS;
  Index2Del = 0;

  UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", OptionNumber);
  Status = EfiLibDeleteVariable (BootOption, &gEfiGlobalVariableGuid);
  //
  // adjust boot order array
  //
  for (Index = 0; Index < *BootOrderSize / sizeof (UINT16); Index++) {
    if (BootOrder[Index] == OptionNumber) {
      Index2Del = Index;
      break;
    }
  }

  if (Index != *BootOrderSize / sizeof (UINT16)) {
    for (Index = 0; Index < *BootOrderSize / sizeof (UINT16) - 1; Index++) {
      if (Index >= Index2Del) {
        BootOrder[Index] = BootOrder[Index + 1];
      }
    }

    *BootOrderSize -= sizeof (UINT16);
  }

  return Status;

}

/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.
**/
EFI_STATUS
EFIAPI
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  BBS_TABLE                 *BbsEntry;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT16                    BootOption[10];
  UINT16                    BootDesc[100];
  BOOLEAN                   DescStringMatch;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  BbsEntry      = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      SafeFreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, &BbsIndex)) {
      SafeFreePool (BootOptionVar);
      Index++;
      continue;
    }

    //
    // Check if BBS Description String is changed
    //
    DescStringMatch = FALSE;

    BdsBuildLegacyDevNameString (
      &LocalBbsTable[BbsIndex],
      BbsIndex,
      sizeof(BootDesc),
      BootDesc
      );

    if (StrCmp (BootDesc, (UINT16*)(BootOptionVar + sizeof (UINT32) + sizeof (UINT16))) == 0) {
      DescStringMatch = TRUE;
    }

    if (!((LocalBbsTable[BbsIndex].BootPriority == BBS_IGNORE_ENTRY) ||
          (LocalBbsTable[BbsIndex].BootPriority == BBS_DO_NOT_BOOT_FROM)) &&
        (LocalBbsTable[BbsIndex].DeviceType == BbsEntry->DeviceType) &&
        DescStringMatch) {
      Index++;
      continue;
    }

    SafeFreePool (BootOptionVar);
    //
    // should delete
    //
    BdsDeleteBootOption (
      BootOrder[Index],
      BootOrder,
      &BootOrderSize
      );
  }

  if (BootOrderSize != 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  SafeFreePool (BootOrder);

  return Status;
}

/**
  EDES_TODO: Add function description.

  @param BootOrder       EDES_TODO: Add parameter description
  @param BootOptionNum   EDES_TODO: Add parameter description
  @param DevType         EDES_TODO: Add parameter description
  @param Attribute       EDES_TODO: Add parameter description
  @param BbsIndex        EDES_TODO: Add parameter description
  @param OptionNumber    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
BdsFindLegacyBootOptionByDevType (
  IN UINT16                 *BootOrder,
  IN UINTN                  BootOptionNum,
  IN UINT16                 DevType,
  OUT UINT32                *Attribute,
  OUT UINT16                *BbsIndex,
  OUT UINTN                 *OptionNumber
  )
{
  UINTN     Index;
  UINTN     BootOrderIndex;
  UINT16    BootOption[100];
  UINTN     BootOptionSize;
  UINT8     *BootOptionVar;
  BBS_TABLE *BbsEntry;
  BOOLEAN   Found;

  BbsEntry  = NULL;
  Found     = FALSE;

  if (NULL == BootOrder) {
    return Found;
  }

  for (BootOrderIndex = 0; BootOrderIndex < BootOptionNum; BootOrderIndex++) {
    Index = (UINTN) BootOrder[BootOrderIndex];
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", Index);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      continue;
    }

    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, BbsIndex)) {
      SafeFreePool (BootOptionVar);
      continue;
    }

    if (BbsEntry->DeviceType != DevType) {
      SafeFreePool (BootOptionVar);
      continue;
    }

    *Attribute    = *(UINT32 *) BootOptionVar;
    *OptionNumber = Index;
    Found         = TRUE;
    SafeFreePool (BootOptionVar);
    break;
  }

  return Found;
}

/**
  EDES_TODO: Add function description.

  @param BbsItem         EDES_TODO: Add parameter description
  @param Index           EDES_TODO: Add parameter description
  @param BootOrderList   EDES_TODO: Add parameter description
  @param BootOrderListSize EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsCreateOneLegacyBootOption (
  IN BBS_TABLE              *BbsItem,
  IN UINTN                  Index,
  IN OUT UINT16             **BootOrderList,
  IN OUT UINTN              *BootOrderListSize
  )
{
  BBS_BBS_DEVICE_PATH       BbsDevPathNode;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  DevPath                       = NULL;

  BbsDevPathNode.Header.Type    = BBS_DEVICE_PATH;
  BbsDevPathNode.Header.SubType = BBS_BBS_DP;
  SetDevicePathNodeLength (&BbsDevPathNode.Header, sizeof (BBS_BBS_DEVICE_PATH));
  BbsDevPathNode.DeviceType = BbsItem->DeviceType;
  CopyMem (&BbsDevPathNode.StatusFlag, &BbsItem->StatusFlags, sizeof (UINT16));

  DevPath = AppendDevicePathNode (
              EndDevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) &BbsDevPathNode
              );
  if (NULL == DevPath) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BdsCreateLegacyBootOption (
            BbsItem,
            DevPath,
            Index,
            BootOrderList,
            BootOrderListSize
            );
  BbsItem->BootPriority = 0x00;

  FreePool (DevPath);

  return Status;
}

/**

  Add the legacy boot options from BBS table if they do not exist.

  @retval EFI_SUCCESS       The boot options are added successfully 
                            or they are already in boot options.

**/
EFI_STATUS
EFIAPI
BdsAddNonExistingLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINTN                     BootOrderSize;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT32                    Attribute;
  UINTN                     OptionNumber;
  BOOLEAN                   Ret;

  BootOrder     = NULL;
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    BootOrderSize = 0;
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    Ret = BdsFindLegacyBootOptionByDevType (
            BootOrder,
            BootOrderSize / sizeof (UINT16),
            LocalBbsTable[Index].DeviceType,
            &Attribute,
            &BbsIndex,
            &OptionNumber
            );
    if (Ret) {
      continue;
    }

    //
    // Not found such type of legacy device in boot options or we found but it's disabled
    // so we have to create one and put it to the tail of boot order list
    //
    Status = BdsCreateOneLegacyBootOption (
              &LocalBbsTable[Index],
              Index,
              &BootOrder,
              &BootOrderSize
              );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (BootOrderSize > 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  if (BootOrder != NULL) {
    SafeFreePool (BootOrder);
  }

  return Status;
}

/**
  EDES_TODO: Add function description.

  @param BbsTable        EDES_TODO: Add parameter description
  @param BbsType         EDES_TODO: Add parameter description
  @param BbsCount        EDES_TODO: Add parameter description
  @param Buf             EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINT16 *
BdsFillDevOrderBuf (
  IN BBS_TABLE                    *BbsTable,
  IN BBS_TYPE                     BbsType,
  IN UINTN                        BbsCount,
  IN UINT16                       *Buf
  )
{
  UINTN Index;

  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    if (BbsTable[Index].DeviceType != BbsType) {
      continue;
    }

    *Buf = (UINT16) (Index & 0xFF);
    Buf++;
  }

  return Buf;
}

/**
  EDES_TODO: Add function description.

  @param BbsTable        EDES_TODO: Add parameter description
  @param BbsCount        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsCreateDevOrder (
  IN BBS_TABLE                  *BbsTable,
  IN UINT16                     BbsCount
  )
{
  UINTN       Index;
  UINTN       FDCount;
  UINTN       HDCount;
  UINTN       CDCount;
  UINTN       NETCount;
  UINTN       BEVCount;
  UINTN       TotalSize;
  UINTN       HeaderSize;
  UINT8       *DevOrder;
  UINT8       *Ptr;
  EFI_STATUS  Status;

  FDCount     = 0;
  HDCount     = 0;
  CDCount     = 0;
  NETCount    = 0;
  BEVCount    = 0;
  TotalSize   = 0;
  HeaderSize  = sizeof (BBS_TYPE) + sizeof (UINT16);
  DevOrder    = NULL;
  Ptr         = NULL;
  Status      = EFI_SUCCESS;

  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
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

  DevOrder = AllocateZeroPool (TotalSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr                 = DevOrder;

  *((BBS_TYPE *) Ptr) = BBS_FLOPPY;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (FDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_FLOPPY, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_HARDDISK;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (HDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_HARDDISK, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_CDROM;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (CDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_CDROM, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_EMBED_NETWORK;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (NETCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_EMBED_NETWORK, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_BEV_DEVICE;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (BEVCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_BEV_DEVICE, BbsCount, (UINT16 *) Ptr);
  }

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &EfiLegacyDevOrderGuid,
                  VAR_FLAG,
                  TotalSize,
                  DevOrder
                  );
  SafeFreePool (DevOrder);

  return Status;
}

/**

  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS       The boot devices are added successfully.

**/
EFI_STATUS
EFIAPI
BdsUpdateLegacyDevOrder (
  VOID
  )
{
  UINT8                     *DevOrder;
  UINT8                     *NewDevOrder;
  UINTN                     DevOrderSize;
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
  UINT8                     *Ptr;
  UINT8                     *NewPtr;
  UINT16                    *NewFDPtr;
  UINT16                    *NewHDPtr;
  UINT16                    *NewCDPtr;
  UINT16                    *NewNETPtr;
  UINT16                    *NewBEVPtr;
  UINT16                    *NewDevPtr;
  UINT16                    Length;
  UINT16                    tmp;
  UINTN                     FDIndex;
  UINTN                     HDIndex;
  UINTN                     CDIndex;
  UINTN                     NETIndex;
  UINTN                     BEVIndex;

  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  Idx           = NULL;
  FDCount       = 0;
  HDCount       = 0;
  CDCount       = 0;
  NETCount      = 0;
  BEVCount      = 0;
  TotalSize     = 0;
  HeaderSize    = sizeof (BBS_TYPE) + sizeof (UINT16);
  FDIndex       = 0;
  HDIndex       = 0;
  CDIndex       = 0;
  NETIndex      = 0;
  BEVIndex      = 0;
  NewDevPtr     = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  DevOrder = (UINT8 *) BdsLibGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER,
                        &EfiLegacyDevOrderGuid,
                        &DevOrderSize
                        );
  if (NULL == DevOrder) {
    return BdsCreateDevOrder (LocalBbsTable, BbsCount);
  }
  //
  // First we figure out how many boot devices with same device type respectively
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
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

  NewFDPtr  = (UINT16 *) (NewDevOrder + HeaderSize);
  NewHDPtr  = (UINT16 *) ((UINT8 *) NewFDPtr + FDCount * sizeof (UINT16) + HeaderSize);
  NewCDPtr  = (UINT16 *) ((UINT8 *) NewHDPtr + HDCount * sizeof (UINT16) + HeaderSize);
  NewNETPtr = (UINT16 *) ((UINT8 *) NewCDPtr + CDCount * sizeof (UINT16) + HeaderSize);
  NewBEVPtr = (UINT16 *) ((UINT8 *) NewNETPtr + NETCount * sizeof (UINT16) + HeaderSize);

  //
  // copy FD
  //
  Ptr                     = DevOrder;
  NewPtr                  = NewDevOrder;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].DeviceType != BBS_FLOPPY
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewFDPtr[FDIndex] = *(UINT16 *) Ptr;
    FDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy HD
  //
  NewPtr                  = (UINT8 *) NewHDPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_HARDDISK
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewHDPtr[HDIndex] = *(UINT16 *) Ptr;
    HDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy CD
  //
  NewPtr                  = (UINT8 *) NewCDPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_CDROM
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewCDPtr[CDIndex] = *(UINT16 *) Ptr;
    CDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy NET
  //
  NewPtr                  = (UINT8 *) NewNETPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_EMBED_NETWORK
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewNETPtr[NETIndex] = *(UINT16 *) Ptr;
    NETIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy BEV
  //
  NewPtr                  = (UINT8 *) NewBEVPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_BEV_DEVICE
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewBEVPtr[BEVIndex] = *(UINT16 *) Ptr;
    BEVIndex++;
    Ptr += sizeof (UINT16);
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
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
    if (Idx != 0) {
      for (Index2 = 0; Index2 < *Idx; Index2++) {
        if ((NewDevPtr[Index2] & 0xFF) == (UINT16) Index) {
          break;
        }
      }

      if (Index2 == *Idx) {
        //
        // Index2 == *Idx means we didn't find Index
        // so Index is a new appeared device's index in BBS table
        // save it.
        //
        NewDevPtr[*Idx] = (UINT16) (Index & 0xFF);
        (*Idx)++;
      }
    }
  }

  if (FDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < FDIndex - 1; Index++) {
      if (0xFF00 != (NewFDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < FDIndex; Index2++) {
        if (0 == (NewFDPtr[Index2] & 0xFF00)) {
          tmp               = NewFDPtr[Index];
          NewFDPtr[Index]   = NewFDPtr[Index2];
          NewFDPtr[Index2]  = tmp;
          break;
        }
      }
    }
  }

  if (HDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < HDIndex - 1; Index++) {
      if (0xFF00 != (NewHDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < HDIndex; Index2++) {
        if (0 == (NewHDPtr[Index2] & 0xFF00)) {
          tmp               = NewHDPtr[Index];
          NewHDPtr[Index]   = NewHDPtr[Index2];
          NewHDPtr[Index2]  = tmp;
          break;
        }
      }
    }
  }

  if (CDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < CDIndex - 1; Index++) {
      if (0xFF00 != (NewCDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < CDIndex; Index2++) {
        if (0 == (NewCDPtr[Index2] & 0xFF00)) {
          tmp               = NewCDPtr[Index];
          NewCDPtr[Index]   = NewCDPtr[Index2];
          NewCDPtr[Index2]  = tmp;
          break;
        }
      }
    }
  }

  if (NETCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < NETIndex - 1; Index++) {
      if (0xFF00 != (NewNETPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < NETIndex; Index2++) {
        if (0 == (NewNETPtr[Index2] & 0xFF00)) {
          tmp               = NewNETPtr[Index];
          NewNETPtr[Index]  = NewNETPtr[Index2];
          NewNETPtr[Index2] = tmp;
          break;
        }
      }
    }
  }

  if (BEVCount!= 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < BEVIndex - 1; Index++) {
      if (0xFF00 != (NewBEVPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < BEVIndex; Index2++) {
        if (0 == (NewBEVPtr[Index2] & 0xFF00)) {
          tmp               = NewBEVPtr[Index];
          NewBEVPtr[Index]  = NewBEVPtr[Index2];
          NewBEVPtr[Index2] = tmp;
          break;
        }
      }
    }
  }

  SafeFreePool (DevOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &EfiLegacyDevOrderGuid,
                  VAR_FLAG,
                  TotalSize,
                  NewDevOrder
                  );
  SafeFreePool (NewDevOrder);

  return Status;
}

/**
  EDES_TODO: Add function description.

  @param DeviceType      EDES_TODO: Add parameter description
  @param LocalBbsTable   EDES_TODO: Add parameter description
  @param Priority        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsSetBootPriority4SameTypeDev (
  IN UINT16                                              DeviceType,
  IN OUT BBS_TABLE                                       *LocalBbsTable,
  IN OUT UINT16                                          *Priority
  )
{
  UINT8   *DevOrder;

  UINT8   *OrigBuffer;
  UINT16  *DevIndex;
  UINTN   DevOrderSize;
  UINTN   DevCount;
  UINTN   Index;

  DevOrder = BdsLibGetVariableAndSize (
              VAR_LEGACY_DEV_ORDER,
              &EfiLegacyDevOrderGuid,
              &DevOrderSize
              );
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  OrigBuffer = DevOrder;
  while (DevOrder < OrigBuffer + DevOrderSize) {
    if (DeviceType == * (BBS_TYPE *) DevOrder) {
      break;
    }

    DevOrder += sizeof (BBS_TYPE);
    DevOrder += *(UINT16 *) DevOrder;
  }

  if (DevOrder >= OrigBuffer + DevOrderSize) {
    SafeFreePool (OrigBuffer);
    return EFI_NOT_FOUND;
  }

  DevOrder += sizeof (BBS_TYPE);
  DevCount  = (*((UINT16 *) DevOrder) - sizeof (UINT16)) / sizeof (UINT16);
  DevIndex  = (UINT16 *) (DevOrder + sizeof (UINT16));
  //
  // If the high byte of the DevIndex is 0xFF, it indicates that this device has been disabled.
  //
  for (Index = 0; Index < DevCount; Index++) {
    if ((DevIndex[Index] & 0xFF00) == 0xFF00) {
      //
      // LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = BBS_DISABLED_ENTRY;
      //
    } else {
      LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = *Priority;
      (*Priority)++;
    }
  }

  SafeFreePool (OrigBuffer);
  return EFI_SUCCESS;
}

/**
  EDES_TODO: Add function description.

  @param LocalBbsTable   EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
PrintBbsTable (
  IN BBS_TABLE                      *LocalBbsTable
  )
{
  UINT16  Idx;

  DEBUG ((DEBUG_ERROR, "\n"));
  DEBUG ((DEBUG_ERROR, " NO  Prio bb/dd/ff cl/sc Type Stat segm:offs\n"));
  DEBUG ((DEBUG_ERROR, "=============================================\n"));
  for (Idx = 0; Idx < MAX_BBS_ENTRIES; Idx++) {
    if ((LocalBbsTable[Idx].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Idx].BootPriority == BBS_DO_NOT_BOOT_FROM) ||
        (LocalBbsTable[Idx].BootPriority == BBS_LOWEST_PRIORITY)
        ) {
      continue;
    }

    DEBUG (
      (DEBUG_ERROR,
      " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x\n",
      (UINTN) Idx,
      (UINTN) LocalBbsTable[Idx].BootPriority,
      (UINTN) LocalBbsTable[Idx].Bus,
      (UINTN) LocalBbsTable[Idx].Device,
      (UINTN) LocalBbsTable[Idx].Function,
      (UINTN) LocalBbsTable[Idx].Class,
      (UINTN) LocalBbsTable[Idx].SubClass,
      (UINTN) LocalBbsTable[Idx].DeviceType,
      (UINTN) * (UINT16 *) &LocalBbsTable[Idx].StatusFlags,
      (UINTN) LocalBbsTable[Idx].BootHandlerSegment,
      (UINTN) LocalBbsTable[Idx].BootHandlerOffset,
      (UINTN) ((LocalBbsTable[Idx].MfgStringSegment << 4) + LocalBbsTable[Idx].MfgStringOffset),
      (UINTN) ((LocalBbsTable[Idx].DescStringSegment << 4) + LocalBbsTable[Idx].DescStringOffset))
      );
  }

  DEBUG ((DEBUG_ERROR, "\n"));
}

/**

  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  Entry             The boot option is to be checked for refresh BBS table.
  
  @retval EFI_SUCCESS       The boot priority for BBS entries is refreshed successfully.

**/
EFI_STATUS
EFIAPI
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  )
{
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINT16                    DevType;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT16                    Priority;
  UINT16                    *BootOrder;
  UINTN                     BootOrderSize;
  UINT8                     *BootOptionVar;
  UINTN                     BootOptionSize;
  UINT16                    BootOption[100];
  UINT8                     *Ptr;
  UINT16                    DevPathLen;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  DevType       = BBS_UNKNOWN;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );
  //
  // First, set all the present devices' boot priority to BBS_UNPRIORITIZED_ENTRY
  // We will set them according to the settings setup by user
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!((BBS_IGNORE_ENTRY == LocalBbsTable[Index].BootPriority) ||
        (BBS_DO_NOT_BOOT_FROM == LocalBbsTable[Index].BootPriority) ||
         (BBS_LOWEST_PRIORITY == LocalBbsTable[Index].BootPriority))) {
      LocalBbsTable[Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
    }
  }
  //
  // boot priority always starts at 0
  //
  Priority = 0;
  if (Entry->LoadOptionsSize == sizeof (BBS_TABLE) + sizeof (UINT16)) {
    //
    // If Entry stands for a legacy boot option, we prioritize the devices with the same type first.
    //
    DevType = ((BBS_TABLE *) Entry->LoadOptions)->DeviceType;
    Status = BdsSetBootPriority4SameTypeDev (
              DevType,
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
  BootOrder = (UINT16 *) BdsLibGetVariableAndSize (
                          L"BootOrder",
                          &gEfiGlobalVariableGuid,
                          &BootOrderSize
                          );
  for (Index = 0; ((BootOrder != NULL) && (Index < BootOrderSize / sizeof (UINT16))); Index++) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      continue;
    }

    Ptr = BootOptionVar;

    Ptr += sizeof (UINT32);
    DevPathLen = *(UINT16 *) Ptr;
    Ptr += sizeof (UINT16);
    Ptr += StrSize ((UINT16 *) Ptr);
    DevPath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    if (BBS_DEVICE_PATH != DevPath->Type || BBS_BBS_DP != DevPath->SubType) {
      SafeFreePool (BootOptionVar);
      continue;
    }

    Ptr += DevPathLen;
    if (DevType == ((BBS_TABLE *) Ptr)->DeviceType) {
      //
      // We don't want to process twice for a device type
      //
      SafeFreePool (BootOptionVar);
      continue;
    }

    Status = BdsSetBootPriority4SameTypeDev (
              ((BBS_TABLE *) Ptr)->DeviceType,
              LocalBbsTable,
              &Priority
              );
    SafeFreePool (BootOptionVar);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (BootOrder != NULL) {
    SafeFreePool (BootOrder);
  }
  //
  // For debug
  //
  PrintBbsTable (LocalBbsTable);

  return Status;
}
