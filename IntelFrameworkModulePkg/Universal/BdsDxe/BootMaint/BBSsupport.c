/** @file
  This function deal with the legacy boot option, it create, delete
  and manage the legacy boot option, all legacy boot option is getting from
  the legacy BBS table.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BBSsupport.h"

#pragma pack(1)
typedef struct {
  BBS_TABLE BbsEntry;
  UINT16    BbsIndex;
} LEGACY_BOOT_OPTION_BBS_DATA;
#pragma pack()

/**
  Re-order the Boot Option according to the DevOrder.

  The routine re-orders the Boot Option in BootOption array according to
  the order specified by DevOrder.

  @param DevOrder           Pointer to buffer containing the BBS Index,
                            high 8-bit value 0xFF indicating a disabled boot option
  @param DevOrderCount      Count of the BBS Index
  @param EnBootOption       Callee allocated buffer containing the enabled Boot Option Numbers
  @param EnBootOptionCount  Count of the enabled Boot Option Numbers
  @param DisBootOption      Callee allocated buffer containing the disabled Boot Option Numbers
  @param DisBootOptionCount Count of the disabled Boot Option Numbers
**/
VOID
OrderLegacyBootOption4SameType (
  UINT16                   *DevOrder,
  UINTN                    DevOrderCount,
  UINT16                   **EnBootOption,
  UINTN                    *EnBootOptionCount,
  UINT16                   **DisBootOption,
  UINTN                    *DisBootOptionCount
  )
{
  EFI_STATUS               Status;
  UINT16                   *NewBootOption;
  UINT16                   *BootOrder;
  UINTN                    BootOrderSize;
  UINTN                    Index;
  UINTN                    StartPosition;
  
  BDS_COMMON_OPTION        *BootOption;
  
  CHAR16                   OptionName[sizeof ("Boot####")];
  UINT16                   *BbsIndexArray;
  UINT16                   *DeviceTypeArray;
  LIST_ENTRY               List;

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  ASSERT (BootOrder != NULL);

  BbsIndexArray       = AllocatePool (BootOrderSize);
  DeviceTypeArray     = AllocatePool (BootOrderSize);
  *EnBootOption       = AllocatePool (BootOrderSize);
  *DisBootOption      = AllocatePool (BootOrderSize);
  *DisBootOptionCount = 0;
  *EnBootOptionCount  = 0;
  Index               = 0;

  ASSERT (BbsIndexArray != NULL);
  ASSERT (DeviceTypeArray != NULL);
  ASSERT (*EnBootOption != NULL);
  ASSERT (*DisBootOption != NULL);

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
  
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", BootOrder[Index]);
    InitializeListHead (&List);
    BootOption = BdsLibVariableToOption (&List, OptionName);
    ASSERT (BootOption != NULL);
    
    if ((DevicePathType (BootOption->DevicePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (BootOption->DevicePath) == BBS_BBS_DP)) {
      //
      // Legacy Boot Option
      //
      ASSERT (BootOption->LoadOptionsSize == sizeof (LEGACY_BOOT_OPTION_BBS_DATA));

      DeviceTypeArray[Index] = ((BBS_BBS_DEVICE_PATH *) BootOption->DevicePath)->DeviceType;
      BbsIndexArray  [Index] = ((LEGACY_BOOT_OPTION_BBS_DATA *) BootOption->LoadOptions)->BbsIndex;
    } else {
      DeviceTypeArray[Index] = BBS_TYPE_UNKNOWN;
      BbsIndexArray  [Index] = 0xFFFF;
    }
    FreePool (BootOption->DevicePath);
    FreePool (BootOption->Description);
    FreePool (BootOption->LoadOptions);
    FreePool (BootOption);
  }

  //
  // Record the corresponding Boot Option Numbers according to the DevOrder
  // Record the EnBootOption and DisBootOption according to the DevOrder
  //
  StartPosition = BootOrderSize / sizeof (UINT16);
  NewBootOption = AllocatePool (DevOrderCount * sizeof (UINT16));
  ASSERT (NewBootOption != NULL);
  while (DevOrderCount-- != 0) {
    for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if (BbsIndexArray[Index] == (DevOrder[DevOrderCount] & 0xFF)) {
        StartPosition = MIN (StartPosition, Index);
        NewBootOption[DevOrderCount] = BootOrder[Index];
        
        if ((DevOrder[DevOrderCount] & 0xFF00) == 0xFF00) {
          (*DisBootOption)[*DisBootOptionCount] = BootOrder[Index];
          (*DisBootOptionCount)++;
        } else {
          (*EnBootOption)[*EnBootOptionCount] = BootOrder[Index];
          (*EnBootOptionCount)++;
        }
        break;
      }
    }
  }

  //
  // Overwrite the old BootOption
  //
  CopyMem (&BootOrder[StartPosition], NewBootOption, (*DisBootOptionCount + *EnBootOptionCount) * sizeof (UINT16));
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BootOrderSize,
                  BootOrder
                  );
  //
  // Changing content without increasing its size with current variable implementation shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);

  FreePool (NewBootOption);
  FreePool (DeviceTypeArray);
  FreePool (BbsIndexArray);
  FreePool (BootOrder);
}

/**
  Group the legacy boot options in the BootOption.

  The routine assumes the boot options in the beginning that covers all the device 
  types are ordered properly and re-position the following boot options just after
  the corresponding boot options with the same device type.
  For example:
  1. Input  = [Harddisk1 CdRom2 Efi1 Harddisk0 CdRom0 CdRom1 Harddisk2 Efi0]
     Assuming [Harddisk1 CdRom2 Efi1] is ordered properly
     Output = [Harddisk1 Harddisk0 Harddisk2 CdRom2 CdRom0 CdRom1 Efi1 Efi0]

  2. Input  = [Efi1 Efi0 CdRom1 Harddisk0 Harddisk1 Harddisk2 CdRom0 CdRom2]
     Assuming [Efi1 Efi0 CdRom1 Harddisk0] is ordered properly
     Output = [Efi1 Efi0 CdRom1 CdRom0 CdRom2 Harddisk0 Harddisk1 Harddisk2]

**/
VOID
GroupMultipleLegacyBootOption4SameType (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  UINTN                        DeviceIndex;
  UINTN                        DeviceTypeIndex[7];
  UINTN                        *NextIndex;
  UINT16                       OptionNumber;
  UINT16                       *BootOrder;
  UINTN                        BootOrderSize;
  CHAR16                       OptionName[sizeof ("Boot####")];
  BDS_COMMON_OPTION            *BootOption;
  LIST_ENTRY                   List;

  SetMem (DeviceTypeIndex, sizeof (DeviceTypeIndex), 0xff);

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", BootOrder[Index]);
    InitializeListHead (&List);
    BootOption = BdsLibVariableToOption (&List, OptionName);
    ASSERT (BootOption != NULL);

    if ((DevicePathType (BootOption->DevicePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (BootOption->DevicePath) == BBS_BBS_DP)) {
      //
      // Legacy Boot Option
      //
      ASSERT ((((BBS_BBS_DEVICE_PATH *) BootOption->DevicePath)->DeviceType & 0xF) < sizeof (DeviceTypeIndex) / sizeof (DeviceTypeIndex[0]));
      NextIndex = &DeviceTypeIndex[((BBS_BBS_DEVICE_PATH *) BootOption->DevicePath)->DeviceType & 0xF];

      if (*NextIndex == (UINTN) -1) {
        //
        // *NextIndex is the Index in BootOrder to put the next Option Number for the same type
        //
        *NextIndex = Index + 1;
      } else {
        //
        // insert the current boot option before *NextIndex, causing [*Next .. Index] shift right one position
        //
        OptionNumber = BootOrder[Index];
        CopyMem (&BootOrder[*NextIndex + 1], &BootOrder[*NextIndex], (Index - *NextIndex) * sizeof (UINT16));
        BootOrder[*NextIndex] = OptionNumber;

        //
        // Update the DeviceTypeIndex array to reflect the right shift operation
        //
        for (DeviceIndex = 0; DeviceIndex < sizeof (DeviceTypeIndex) / sizeof (DeviceTypeIndex[0]); DeviceIndex++) {
          if (DeviceTypeIndex[DeviceIndex] != (UINTN) -1 && DeviceTypeIndex[DeviceIndex] >= *NextIndex) {
            DeviceTypeIndex[DeviceIndex]++;
          }
        }
      }
    }
    FreePool (BootOption->DevicePath);
    FreePool (BootOption->Description);
    FreePool (BootOption->LoadOptions);
    FreePool (BootOption);
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BootOrderSize,
                  BootOrder
                  );
  //
  // Changing content without increasing its size with current variable implementation shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);
  FreePool (BootOrder);
}

