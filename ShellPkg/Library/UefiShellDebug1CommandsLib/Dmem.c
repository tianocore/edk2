/** @file
  Main file for Dmem shell Debug1 function.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/Acpi.h>
#include <Guid/Mps.h>
#include <Guid/SmBios.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/RtPropertiesTable.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/DebugImageInfoTable.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/ConformanceProfiles.h>

/**
  Make a printable character.

  If Char is printable then return it, otherwise return a question mark.

  @param[in] Char     The character to make printable.

  @return A printable character representing Char.
**/
CHAR16
MakePrintable (
  IN CONST CHAR16  Char
  )
{
  if (((Char < 0x20) && (Char > 0)) || (Char > 126)) {
    return (L'?');
  }

  return (Char);
}

/**
  Display some Memory-Mapped-IO memory.

  @param[in] Address    The starting address to display.
  @param[in] Size       The length of memory to display.
**/
SHELL_STATUS
DisplayMmioMemory (
  IN CONST VOID   *Address,
  IN CONST UINTN  Size
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRbIo;
  EFI_STATUS                       Status;
  VOID                             *Buffer;
  SHELL_STATUS                     ShellStatus;

  ShellStatus = SHELL_SUCCESS;

  Status = gBS->LocateProtocol (&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **)&PciRbIo);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle, L"dmem");
    return (SHELL_NOT_FOUND);
  }

  Buffer = AllocateZeroPool (Size);
  if (Buffer == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  Status = PciRbIo->Mem.Read (PciRbIo, EfiPciWidthUint8, (UINT64)(UINTN)Address, Size, Buffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PCIRBIO_ER), gShellDebug1HiiHandle, L"dmem");
    ShellStatus = SHELL_NOT_FOUND;
  } else {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_MMIO_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)(UINTN)Address, Size);
    DumpHex (2, (UINTN)Address, Size, Buffer);
  }

  FreePool (Buffer);
  return (ShellStatus);
}

/**
  Display the RtPropertiesTable entries

  @param[in] RtPropertiesTable    The pointer to the RtPropertiesTable.
**/
STATIC
VOID
DisplayRtProperties (
  IN EFI_RT_PROPERTIES_TABLE  *RtPropertiesTable
  )
{
  UINT32      RtServices;
  EFI_STATUS  Status;

  if (RtPropertiesTable == NULL) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_ERR_NOT_FOUND), gShellDebug1HiiHandle, L"RtPropertiesTable");
    return;
  }

  RtServices = (UINT32)RtPropertiesTable->RuntimeServicesSupported;
  Status     = ShellPrintHiiDefaultEx (
                 STRING_TOKEN (STR_DMEM_RT_PROPERTIES),
                 gShellDebug1HiiHandle,
                 EFI_RT_PROPERTIES_TABLE_VERSION,
                 (RtServices & EFI_RT_SUPPORTED_GET_TIME) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_SET_TIME) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_GET_WAKEUP_TIME) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_SET_WAKEUP_TIME) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_GET_VARIABLE) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_GET_NEXT_VARIABLE_NAME) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_SET_VARIABLE) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_SET_VIRTUAL_ADDRESS_MAP) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_CONVERT_POINTER) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_GET_NEXT_HIGH_MONOTONIC_COUNT) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_RESET_SYSTEM) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_UPDATE_CAPSULE) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_QUERY_CAPSULE_CAPABILITIES) ? 1 : 0,
                 (RtServices & EFI_RT_SUPPORTED_QUERY_VARIABLE_INFO) ? 1 : 0
                 );

  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_ERR_GET_FAIL), gShellDebug1HiiHandle, L"RtPropertiesTable");
  }
}

/**
  Retrieve the ImageExecutionTable Entry ImageName from ImagePath

  @param[in]  FileName    The full path of the image.
  @param[out] BaseName    The name of the image.
**/
EFI_STATUS
GetBaseName (
  IN  CHAR16  *FileName,
  OUT CHAR16  **BaseName
  )
{
  UINTN   StrLen;
  CHAR16  *StrTail;

  StrLen = StrSize (FileName);

  for (StrTail = FileName + StrLen - 1; StrTail != FileName && *StrTail != L'\\'; StrTail--) {
  }

  if (StrTail == FileName) {
    return EFI_NOT_FOUND;
  }

  *BaseName = StrTail+1;

  return EFI_SUCCESS;
}

/**
  Retrieve the ImageExecutionTable entries.

  @param[in] ExecInfoTablePtr    The pointer to the ImageExecutionTable.
**/
EFI_STATUS
GetImageExecutionInfo (
  IN EFI_IMAGE_EXECUTION_INFO_TABLE  *ExecInfoTablePtr
  )
{
  EFI_STATUS                Status;
  EFI_IMAGE_EXECUTION_INFO  *InfoPtr;
  CHAR8                     *ptr;
  CHAR16                    *ImagePath;
  CHAR16                    *ImageName;
  UINTN                     Image;
  UINTN                     *NumberOfImages;
  CHAR16                    *ActionType;

  NumberOfImages = &ExecInfoTablePtr->NumberOfImages;

  ptr = (CHAR8 *)ExecInfoTablePtr + 1;

  Status = EFI_NOT_FOUND;

  for (Image = 0; Image < *NumberOfImages; Image++, ptr += InfoPtr->InfoSize) {
    InfoPtr   = (EFI_IMAGE_EXECUTION_INFO *)ptr;
    ImagePath = (CHAR16 *)(InfoPtr + 1);

    GetBaseName (ImagePath, &ImageName);

    switch (InfoPtr->Action) {
      case EFI_IMAGE_EXECUTION_AUTHENTICATION:
        ActionType = L"AUTHENTICATION";
        break;
      case EFI_IMAGE_EXECUTION_AUTH_UNTESTED:
        ActionType = L"AUTH_UNTESTED";
        break;
      case EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED:
        ActionType = L"AUTH_SIG_FAILED";
        break;
      case EFI_IMAGE_EXECUTION_AUTH_SIG_PASSED:
        ActionType = L"AUTH_SIG_PASSED";
        break;
      case EFI_IMAGE_EXECUTION_AUTH_SIG_NOT_FOUND:
        ActionType = L"AUTH_SIG_NOT_FOUND";
        break;
      case EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND:
        ActionType = L"AUTH_SIG_FOUND";
        break;
      case EFI_IMAGE_EXECUTION_POLICY_FAILED:
        ActionType = L"POLICY_FAILED";
        break;
      case EFI_IMAGE_EXECUTION_INITIALIZED:
        ActionType = L"INITIALIZED";
        break;
      default:
        ActionType = L"invalid action";
    }

    Status = ShellPrintHiiDefaultEx (
               STRING_TOKEN (STR_DMEM_IMG_EXE_ENTRY),
               gShellDebug1HiiHandle,
               ImageName,
               ActionType
               );
  }

  return Status;
}

/**
  Display the ImageExecutionTable entries

  @param[in] ExecInfoTablePtr    The pointer to the ImageExecutionTable.
**/
STATIC
VOID
DisplayImageExecutionEntries (
  IN EFI_IMAGE_EXECUTION_INFO_TABLE  *ExecInfoTablePtr
  )
{
  EFI_STATUS  Status;

  if (ExecInfoTablePtr == NULL) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_ERR_NOT_FOUND), gShellDebug1HiiHandle, L"ImageExecutionTable");
    return;
  }

  ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_IMG_EXE_TABLE), gShellDebug1HiiHandle);
  Status = GetImageExecutionInfo (ExecInfoTablePtr);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_ERR_GET_FAIL), gShellDebug1HiiHandle, L"ImageExecutionTable");
  }
}

/**
  Display the ConformanceProfileTable entries

  @param[in] ConfProfTable    The pointer to the ConformanceProfileTable.
**/
STATIC
VOID
DisplayConformanceProfiles (
  IN  EFI_CONFORMANCE_PROFILES_TABLE  *ConfProfTable
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *EntryGuid;
  CHAR16      *GuidName;
  UINTN       Profile;

  Status = EFI_SUCCESS;

  if (ConfProfTable == NULL) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_CONF_PRO_TABLE), gShellDebug1HiiHandle);
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_DMEM_CONF_PRO_ROW),
      gShellDebug1HiiHandle,
      L"EFI_CONFORMANCE_PROFILES_UEFI_SPEC_GUID",
      &gEfiConfProfilesUefiSpecGuid
      );
    return;
  }

  ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_CONF_PRO_TABLE), gShellDebug1HiiHandle);

  EntryGuid = (EFI_GUID *)(ConfProfTable + 1);

  for (Profile = 0; Profile < ConfProfTable->NumberOfProfiles; Profile++, EntryGuid++) {
    GuidName = L"Unknown_Profile";

    if (CompareGuid (EntryGuid, &gEfiConfProfilesUefiSpecGuid)) {
      GuidName = L"EFI_CONFORMANCE_PROFILE_UEFI_SPEC_GUID";
    }

    if (CompareGuid (EntryGuid, &gEfiConfProfilesEbbrSpec21Guid)) {
      GuidName = L"EBBR_2.1";
    }

    if (CompareGuid (EntryGuid, &gEfiConfProfilesEbbrSpec22Guid)) {
      GuidName = L"EBBR_2.2";
    }

    Status = ShellPrintHiiDefaultEx (
               STRING_TOKEN (STR_DMEM_CONF_PRO_ROW),
               gShellDebug1HiiHandle,
               GuidName,
               EntryGuid
               );
  }

  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_ERR_GET_FAIL), gShellDebug1HiiHandle, L"ComformanceProfilesTable");
  }
}

/** Enum of the System tables to print. */
typedef enum {
  EDstAcpi20Table = 0,
  EDstAcpi10Table,
  EDstSmbiosTable,
  EDstSmbios3Table,
  EDstMpsTable,
  EDstFdtTable,
  EDstMemoryAttributesTable,
  EDstRtPropertiesTable,
  EDstSystemResourceTable,
  EDstDebugImageInfoTable,
  EDstImageSecurityDatabase,
  EDstJsonConfigDataTable,
  EDstJsonCapsuleDataTable,
  EDstJsonCapsuleResultTable,
  EDstHiiDatabaseProtocol,
  EDstConfProfilesTable,
  EDstMax
} DISPLAYED_SYSTEM_TABLES;

STATIC CONST EFI_GUID  *GuidArray[] = {
  [EDstAcpi20Table]            = &gEfiAcpi20TableGuid,
  [EDstAcpi10Table]            = &gEfiAcpi10TableGuid,
  [EDstSmbiosTable]            = &gEfiSmbiosTableGuid,
  [EDstSmbios3Table]           = &gEfiSmbios3TableGuid,
  [EDstMpsTable]               = &gEfiMpsTableGuid,
  [EDstFdtTable]               = &gFdtTableGuid,
  [EDstMemoryAttributesTable]  = &gEfiMemoryAttributesTableGuid,
  [EDstRtPropertiesTable]      = &gEfiRtPropertiesTableGuid,
  [EDstSystemResourceTable]    = &gEfiSystemResourceTableGuid,
  [EDstDebugImageInfoTable]    = &gEfiDebugImageInfoTableGuid,
  [EDstImageSecurityDatabase]  = &gEfiImageSecurityDatabaseGuid,
  [EDstJsonConfigDataTable]    = &gEfiJsonConfigDataTableGuid,
  [EDstJsonCapsuleDataTable]   = &gEfiJsonCapsuleDataTableGuid,
  [EDstJsonCapsuleResultTable] = &gEfiJsonCapsuleResultTableGuid,
  [EDstHiiDatabaseProtocol]    = &gEfiHiiDatabaseProtocolGuid,
  [EDstConfProfilesTable]      = &gEfiConfProfilesTableGuid,
};

/**
  Display the System Tables.

  @param[in] Package    List of input parameters.
  @param[in] Address    The address of the System Table.
**/
STATIC
SHELL_STATUS
DisplaySystemTable (
  IN  LIST_ENTRY  *Package,
  IN  VOID        *Address
  )
{
  UINTN         Index;
  UINTN         TableWalker;
  UINTN         AddressArray[EDstMax];
  SHELL_STATUS  ShellStatus;

  ShellStatus = SHELL_SUCCESS;

  ZeroMem (AddressArray, sizeof (AddressArray));

  for (TableWalker = 0; TableWalker < gST->NumberOfTableEntries; TableWalker++) {
    for (Index = 0; Index < EDstMax; Index++) {
      if (CompareGuid (&gST->ConfigurationTable[TableWalker].VendorGuid, GuidArray[Index])) {
        AddressArray[Index] = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
        continue;
      }
    }
  }

  // If Smbios3 is valid, use it.
  if (AddressArray[EDstSmbios3Table]) {
    ASSERT (AddressArray[EDstSmbiosTable]);
    AddressArray[EDstSmbiosTable] = AddressArray[EDstSmbios3Table];
  }

  ShellPrintHiiDefaultEx (
    STRING_TOKEN (STR_DMEM_SYSTEM_TABLE),
    gShellDebug1HiiHandle,
    (UINT64)(UINTN)Address,
    gST->Hdr.HeaderSize,
    gST->Hdr.Revision,
    (UINT64)(UINTN)gST->ConIn,
    (UINT64)(UINTN)gST->ConOut,
    (UINT64)(UINTN)gST->StdErr,
    (UINT64)(UINTN)gST->RuntimeServices,
    (UINT64)(UINTN)gST->BootServices,
    AddressArray[EDstAcpi10Table],
    AddressArray[EDstAcpi20Table],
    AddressArray[EDstMpsTable],
    AddressArray[EDstSmbiosTable],
    AddressArray[EDstFdtTable],
    AddressArray[EDstMemoryAttributesTable],
    AddressArray[EDstRtPropertiesTable],
    AddressArray[EDstSystemResourceTable],
    AddressArray[EDstDebugImageInfoTable],
    AddressArray[EDstImageSecurityDatabase],
    AddressArray[EDstJsonConfigDataTable],
    AddressArray[EDstJsonCapsuleDataTable],
    AddressArray[EDstJsonCapsuleResultTable],
    AddressArray[EDstHiiDatabaseProtocol],
    AddressArray[EDstConfProfilesTable]
    );

  if (ShellCommandLineGetFlag (Package, L"-verbose")) {
    DisplayRtProperties ((EFI_RT_PROPERTIES_TABLE *)AddressArray[EDstRtPropertiesTable]);
    DisplayImageExecutionEntries ((EFI_IMAGE_EXECUTION_INFO_TABLE *)AddressArray[EDstImageSecurityDatabase]);
    DisplayConformanceProfiles ((EFI_CONFORMANCE_PROFILES_TABLE *)AddressArray[EDstConfProfilesTable]);
  }

  return ShellStatus;
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-mmio",    TypeFlag },
  { L"-verbose", TypeFlag },
  { NULL,        TypeMax  }
};

/**
  Function for 'dmem' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDmem (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  VOID          *Address;
  UINT64        Size;
  CONST CHAR16  *Temp1;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  Address     = NULL;
  Size        = 0;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"dmem", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 3) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"dmem");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Temp1 = ShellCommandLineGetRawValue (Package, 1);
      if (Temp1 == NULL) {
        Address = gST;
        Size    = sizeof (*gST);
      } else {
        if (!ShellIsHexOrDecimalNumber (Temp1, TRUE, FALSE) || EFI_ERROR (ShellConvertStringToUint64 (Temp1, (UINT64 *)&Address, TRUE, FALSE))) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmem", Temp1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }

        Temp1 = ShellCommandLineGetRawValue (Package, 2);
        if (Temp1 == NULL) {
          Size = 512;
        } else {
          if (!ShellIsHexOrDecimalNumber (Temp1, FALSE, FALSE) || EFI_ERROR (ShellConvertStringToUint64 (Temp1, &Size, TRUE, FALSE))) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmem", Temp1);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      }
    }

    if (ShellStatus == SHELL_SUCCESS) {
      if (!ShellCommandLineGetFlag (Package, L"-mmio")) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_DMEM_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)(UINTN)Address, Size);
        DumpHex (2, (UINTN)Address, (UINTN)Size, Address);
        if (Address == (VOID *)gST) {
          ShellStatus = DisplaySystemTable (Package, Address);
        }
      } else {
        ShellStatus = DisplayMmioMemory (Address, (UINTN)Size);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
