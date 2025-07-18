/** @file
  Shell application for VLAN configuration.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/VlanConfig.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/NetLib.h>

//
// String token ID of VConfig command help message text.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStringVConfigHelpTokenId = STRING_TOKEN (STR_VCONFIG_HELP);

#define INVALID_NIC_INDEX  0xffff
#define INVALID_VLAN_ID    0xffff

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  VConfigStrings[];

EFI_HANDLE      mImageHandle = NULL;
EFI_HII_HANDLE  mHiiHandle   = NULL;

SHELL_PARAM_ITEM  mParamList[] = {
  {
    L"-l",
    TypeValue
  },
  {
    L"-a",
    TypeMaxValue
  },
  {
    L"-d",
    TypeValue
  },
  {
    NULL,
    TypeMax
  }
};

/**
  Locate the network interface handle buffer.

  @param[out]  NumberOfHandles Pointer to the number of handles.
  @param[out]  HandleBuffer    Pointer to the buffer to store the returned handles.

**/
VOID
LocateNicHandleBuffer (
  OUT UINTN       *NumberOfHandles,
  OUT EFI_HANDLE  **HandleBuffer
  )
{
  EFI_STATUS  Status;

  *NumberOfHandles = 0;
  *HandleBuffer    = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiVlanConfigProtocolGuid,
                  NULL,
                  NumberOfHandles,
                  HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_LOCATE_FAIL), mHiiHandle, Status);
  }
}

/**
  Extract the decimal index from the network interface name.

  @param[in]  Name           Name of the network interface.

  @retval INVALID_NIC_INDEX  Failed to extract the network interface index.
  @return others             The network interface index.

**/
UINTN
NicNameToIndex (
  IN CHAR16  *Name
  )
{
  CHAR16  *Str;

  Str = Name + 3;
  if ((StrnCmp (Name, L"eth", 3) != 0) || (*Str == 0)) {
    return INVALID_NIC_INDEX;
  }

  while (*Str != 0) {
    if ((*Str < L'0') || (*Str > L'9')) {
      return INVALID_NIC_INDEX;
    }

    Str++;
  }

  return (UINT16)StrDecimalToUintn (Name + 3);
}

/**
  Find network interface device handle by its name.

  @param[in]  Name           Name of the network interface.

  @retval NULL               Cannot find the network interface.
  @return others             Handle of the network interface.

**/
EFI_HANDLE
NicNameToHandle (
  IN CHAR16  *Name
  )
{
  UINTN       NumberOfHandles;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  EFI_HANDLE  Handle;

  //
  // Find all NIC handles.
  //
  LocateNicHandleBuffer (&NumberOfHandles, &HandleBuffer);
  if (NumberOfHandles == 0) {
    return NULL;
  }

  Index = NicNameToIndex (Name);
  if (Index >= NumberOfHandles) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_INVALID_IF), mHiiHandle, Name);
    Handle = NULL;
  } else {
    Handle = HandleBuffer[Index];
  }

  FreePool (HandleBuffer);
  return Handle;
}

/**
  Open VlanConfig protocol from a handle.

  @param[in]  Handle         The handle to open the VlanConfig protocol.

  @return The VlanConfig protocol interface.

**/
EFI_VLAN_CONFIG_PROTOCOL *
OpenVlanConfigProtocol (
  IN EFI_HANDLE  Handle
  )
{
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;

  VlanConfig = NULL;
  gBS->OpenProtocol (
         Handle,
         &gEfiVlanConfigProtocolGuid,
         (VOID **)&VlanConfig,
         mImageHandle,
         Handle,
         EFI_OPEN_PROTOCOL_GET_PROTOCOL
         );

  return VlanConfig;
}

/**
  Close VlanConfig protocol of a handle.

  @param[in]  Handle         The handle to close the VlanConfig protocol.

**/
VOID
CloseVlanConfigProtocol (
  IN EFI_HANDLE  Handle
  )
{
  gBS->CloseProtocol (
         Handle,
         &gEfiVlanConfigProtocolGuid,
         mImageHandle,
         Handle
         );
}

/**
  Display VLAN configuration of a network interface.

  @param[in]  Handle         Handle of the network interface.
  @param[in]  NicIndex       Index of the network interface.

**/
VOID
ShowNicVlanInfo (
  IN EFI_HANDLE  Handle,
  IN UINTN       NicIndex
  )
{
  CHAR16                    *MacStr;
  EFI_STATUS                Status;
  UINTN                     Index;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  UINT16                    NumberOfVlan;
  EFI_VLAN_FIND_DATA        *VlanData;

  VlanConfig = OpenVlanConfigProtocol (Handle);
  if (VlanConfig == NULL) {
    return;
  }

  MacStr = NULL;
  Status = NetLibGetMacString (Handle, mImageHandle, &MacStr);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_MAC_FAIL), mHiiHandle, Status);
    goto Exit;
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_ETH_MAC), mHiiHandle, NicIndex, MacStr);

  Status = VlanConfig->Find (VlanConfig, NULL, &NumberOfVlan, &VlanData);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_VLAN), mHiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_FIND_FAIL), mHiiHandle, Status);
    }

    goto Exit;
  }

  for (Index = 0; Index < NumberOfVlan; Index++) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_VCONFIG_VLAN_DISPLAY),
      mHiiHandle,
      VlanData[Index].VlanId,
      VlanData[Index].Priority
      );
  }

  FreePool (VlanData);

Exit:
  CloseVlanConfigProtocol (Handle);

  if (MacStr != NULL) {
    FreePool (MacStr);
  }
}

/**
  Display the VLAN configuration of all, or a specified network interface.

  @param[in]  Name           Name of the network interface. If NULL, the VLAN
                             configuration of all network will be displayed.

**/
VOID
DisplayVlan (
  IN CHAR16  *Name OPTIONAL
  )
{
  UINTN       NumberOfHandles;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  EFI_HANDLE  NicHandle;

  if (Name != NULL) {
    //
    // Display specified NIC
    //
    NicHandle = NicNameToHandle (Name);
    if (NicHandle == NULL) {
      return;
    }

    ShowNicVlanInfo (NicHandle, 0);
    return;
  }

  //
  // Find all NIC handles
  //
  LocateNicHandleBuffer (&NumberOfHandles, &HandleBuffer);
  if (NumberOfHandles == 0) {
    return;
  }

  for (Index = 0; Index < NumberOfHandles; Index++) {
    ShowNicVlanInfo (HandleBuffer[Index], Index);
  }

  FreePool (HandleBuffer);
}

/**
  Convert a NULL-terminated unicode decimal VLAN ID string to VLAN ID.

  @param[in]  String       Pointer to VLAN ID string from user input.

  @retval Value translated from String, or INVALID_VLAN_ID is string is invalid.

**/
UINT16
StrToVlanId (
  IN CHAR16  *String
  )
{
  CHAR16  *Str;

  if (String == NULL) {
    return INVALID_VLAN_ID;
  }

  Str = String;
  while ((*Str >= '0') && (*Str <= '9')) {
    Str++;
  }

  if (*Str != 0) {
    return INVALID_VLAN_ID;
  }

  return (UINT16)StrDecimalToUintn (String);
}

/**
  Add a VLAN device.

  @param[in]  ParamStr       Parameter string from user input.

**/
VOID
AddVlan (
  IN CHAR16  *ParamStr
  )
{
  CHAR16                    *Name;
  CHAR16                    *VlanIdStr;
  CHAR16                    *PriorityStr;
  CHAR16                    *StrPtr;
  BOOLEAN                   IsSpace;
  UINTN                     VlanId;
  UINTN                     Priority;
  EFI_HANDLE                Handle;
  EFI_HANDLE                VlanHandle;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  EFI_STATUS                Status;

  VlanConfig = NULL;
  Priority   = 0;

  if (ParamStr == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_IF), mHiiHandle);
    return;
  }

  StrPtr = AllocateCopyPool (StrSize (ParamStr), ParamStr);
  if (StrPtr == NULL) {
    return;
  }

  Name        = StrPtr;
  VlanIdStr   = NULL;
  PriorityStr = NULL;
  IsSpace     = FALSE;
  while (*StrPtr != 0) {
    if (*StrPtr == L' ') {
      *StrPtr = 0;
      IsSpace = TRUE;
    } else {
      if (IsSpace) {
        //
        // Start of a parameter.
        //
        if (VlanIdStr == NULL) {
          //
          // 2nd parameter is VLAN ID.
          //
          VlanIdStr = StrPtr;
        } else if (PriorityStr == NULL) {
          //
          // 3rd parameter is Priority.
          //
          PriorityStr = StrPtr;
        } else {
          //
          // Ignore else parameters.
          //
          break;
        }
      }

      IsSpace = FALSE;
    }

    StrPtr++;
  }

  Handle = NicNameToHandle (Name);
  if (Handle == NULL) {
    goto Exit;
  }

  VlanConfig = OpenVlanConfigProtocol (Handle);
  if (VlanConfig == NULL) {
    goto Exit;
  }

  //
  // Check VLAN ID.
  //
  if ((VlanIdStr == NULL) || (*VlanIdStr == 0)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_VID), mHiiHandle);
    goto Exit;
  }

  VlanId = StrToVlanId (VlanIdStr);
  if (VlanId > 4094) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_INVALID_VID), mHiiHandle, VlanIdStr);
    goto Exit;
  }

  //
  // Check Priority.
  //
  if ((PriorityStr != NULL) && (*PriorityStr != 0)) {
    Priority = StrDecimalToUintn (PriorityStr);
    if (Priority > 7) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_INVALID_PRIORITY), mHiiHandle, PriorityStr);
      goto Exit;
    }
  }

  //
  // Set VLAN
  //
  Status = VlanConfig->Set (VlanConfig, (UINT16)VlanId, (UINT8)Priority);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_SET_FAIL), mHiiHandle, Status);
    goto Exit;
  }

  //
  // Connect the VLAN device.
  //
  VlanHandle = NetLibGetVlanHandle (Handle, (UINT16)VlanId);
  if (VlanHandle != NULL) {
    gBS->ConnectController (VlanHandle, NULL, NULL, TRUE);
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_SET_SUCCESS), mHiiHandle);

Exit:
  if (VlanConfig != NULL) {
    CloseVlanConfigProtocol (Handle);
  }

  FreePool (Name);
}

/**
  Remove a VLAN device.

  @param[in]  ParamStr       Parameter string from user input.

**/
VOID
DeleteVlan (
  IN CHAR16  *ParamStr
  )
{
  CHAR16                    *Name;
  CHAR16                    *VlanIdStr;
  CHAR16                    *StrPtr;
  UINTN                     VlanId;
  EFI_HANDLE                Handle;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  EFI_STATUS                Status;
  UINT16                    NumberOfVlan;
  EFI_VLAN_FIND_DATA        *VlanData;

  VlanConfig = NULL;

  if (ParamStr == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_IF), mHiiHandle);
    return;
  }

  StrPtr = AllocateCopyPool (StrSize (ParamStr), ParamStr);
  if (StrPtr == NULL) {
    return;
  }

  Name      = StrPtr;
  VlanIdStr = NULL;
  while (*StrPtr != 0) {
    if (*StrPtr == L'.') {
      *StrPtr   = 0;
      VlanIdStr = StrPtr + 1;
      break;
    }

    StrPtr++;
  }

  Handle = NicNameToHandle (Name);
  if (Handle == NULL) {
    goto Exit;
  }

  VlanConfig = OpenVlanConfigProtocol (Handle);
  if (VlanConfig == NULL) {
    goto Exit;
  }

  //
  // Check VLAN ID
  //
  if ((VlanIdStr == NULL) || (*VlanIdStr == 0)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_VID), mHiiHandle);
    goto Exit;
  }

  VlanId = StrToVlanId (VlanIdStr);
  if (VlanId > 4094) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_INVALID_VID), mHiiHandle, VlanIdStr);
    goto Exit;
  }

  //
  // Delete VLAN.
  //
  Status = VlanConfig->Remove (VlanConfig, (UINT16)VlanId);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NOT_FOUND), mHiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_REMOVE_FAIL), mHiiHandle, Status);
    }

    goto Exit;
  }

  //
  // Check whether this is the last VLAN to remove.
  //
  Status = VlanConfig->Find (VlanConfig, NULL, &NumberOfVlan, &VlanData);
  if (EFI_ERROR (Status)) {
    //
    // This is the last VLAN to remove, try to connect the controller handle.
    //
    gBS->ConnectController (Handle, NULL, NULL, TRUE);
  } else {
    FreePool (VlanData);
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_REMOVE_SUCCESS), mHiiHandle);

Exit:
  if (VlanConfig != NULL) {
    CloseVlanConfigProtocol (Handle);
  }

  FreePool (Name);
}

/**
  The actual entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point executed successfully.
  @retval other             Some error occur when executing this entry point.

**/
EFI_STATUS
EFIAPI
VlanConfigMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  LIST_ENTRY                   *List;
  CONST CHAR16                 *Str;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_STATUS                   Status;

  mImageHandle = ImageHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &mHiiHandle
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mHiiHandle == NULL) {
    return EFI_SUCCESS;
  }

  List = NULL;
  ShellCommandLineParseEx (mParamList, &List, NULL, FALSE, FALSE);
  if (List == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_ARG), mHiiHandle);
    goto Exit;
  }

  if (ShellCommandLineGetFlag (List, L"-l")) {
    Str = ShellCommandLineGetValue (List, L"-l");
    if (Str == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_IF), mHiiHandle);
      goto Exit;
    }

    DisplayVlan ((CHAR16 *)Str);
    goto Exit;
  }

  if (ShellCommandLineGetFlag (List, L"-a")) {
    Str = ShellCommandLineGetValue (List, L"-a");
    if (Str == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_IF), mHiiHandle);
      goto Exit;
    }

    AddVlan ((CHAR16 *)Str);

    goto Exit;
  }

  if (ShellCommandLineGetFlag (List, L"-d")) {
    Str = ShellCommandLineGetValue (List, L"-d");
    if (Str == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_IF), mHiiHandle);
      goto Exit;
    }

    DeleteVlan ((CHAR16 *)Str);
    goto Exit;
  }

  //
  // No valid argument till now.
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VCONFIG_NO_ARG), mHiiHandle);

Exit:
  if (List != NULL) {
    ShellCommandLineFreeVarList (List);
  }

  //
  // Remove our string package from HII database.
  //
  HiiRemovePackages (mHiiHandle);

  return EFI_SUCCESS;
}
