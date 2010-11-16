/** @file
  Main file for bcfg shell install1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellInstall1CommandsLib.h"
#include <Guid/GlobalVariable.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/DevicePathLib.h>

typedef enum {
  BCFG_TARGET_BOOT_ORDER    = 0,
  BCFG_TARGET_DRIVER_ORDER  = 1,
  BCFG_TARGET_MAX           = 2
} BCFG_OPERATION_TARGET;

typedef enum {
  BCFG_TYPE_DUMP       = 0,
  BCFG_TYPE_ADD        = 1,
  BCFG_TYPE_ADDP       = 2,
  BCFG_TYPE_ADDH       = 3,
  BCFG_TYPE_RM         = 4,
  BCFG_TYPE_MV         = 5,
  BCFG_TYPE_OPT        = 6,
  BCFG_TYPE_MAX        = 7
} BCFG_OPERATION_TYPE;

typedef struct {
  BCFG_OPERATION_TARGET Target;
  BCFG_OPERATION_TYPE   Type;
  UINT16                Number1;
  UINT16                Number2;
  UINTN                 HandleIndex;
  CHAR16                *FileName;
  CHAR16                *Description;
  UINT16                *Order;
  CONST CHAR16          *OptData;
} BGFG_OPERATION;

/**
  Function to update the optional data associated with an option.

  @param[in] Target     The type being modified.  BOOT or DRIVER
  @param[in] OptData    The pointer to the data to modify with.

  @retval SHELL_SUCCESS The optional data was updated sucessfully.
**/
SHELL_STATUS
EFIAPI
BcfgAddOptInstall(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST CHAR16 *OptData
  )
{
  ShellPrintEx(-1, -1, L"use of -opt is not currently supported.");
  return (SHELL_UNSUPPORTED);
}

/**
  Function to add an option.

  @param[in] Position     The position to add this in at.
  @param[in] File         The file to add as the option.
  @param[in] Desc         The description.
  @param[in] CurrentOrder The current order of that type.
  @param[in] OrderCount   The number of items in the current order.
  @param[in] Target       The type being modified.  BOOT or DRIVER
  @param[in] UseHandle    Add something by a handle.  Uses HandleNumber if TRUE and File if FALSE.
  @param[in] UsePath      Add something by local path.  Only used of UseHandle is FALSE.
  @param[in] HandleNumber The HandleIndex to use.

  @retval SHELL_SUCCESS The option was added sucessfully.
**/
SHELL_STATUS
EFIAPI
BcfgAddInstall (
  IN       UINTN                  Position,
  IN CONST CHAR16                 *File,
  IN CONST CHAR16                 *Desc,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST BOOLEAN                UseHandle,
  IN CONST BOOLEAN                UseFullPath,
  IN CONST UINTN                  HandleNumber
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath, *FilePath, *FileNode;
  CHAR16                    *Str;
  CONST CHAR16              *p;
  UINT8                     *p8;
  UINT8                     *p8Copy;
  EFI_SHELL_FILE_INFO       *Arg;
  EFI_SHELL_FILE_INFO       *FileList;
  CHAR16                    OptionStr[40];
  UINTN                     DescSize, FilePathSize;
  BOOLEAN                   Found;
  UINTN                     TargetLocation;
  UINTN                     Index;
  EFI_HANDLE                *Handles;
  EFI_HANDLE                CurHandle;
  SHELL_STATUS              ShellStatus;
  UINT16                    *NewOrder;
  EFI_LOADED_IMAGE_PROTOCOL *Image;

  if (!UseHandle) {
    ASSERT(File != NULL);
    ASSERT(Desc != NULL);
  } else {
    ASSERT(HandleNumber != 0);
  }

  Str                         = NULL;
  FilePath                    = NULL;
  FileNode                    = NULL;
  FileList                    = NULL;
  Handles                     = NULL;
  ShellStatus                 = SHELL_SUCCESS;
  TargetLocation              = 0xFFFF;

  if (UseHandle) {
    CurHandle = ConvertHandleIndexToHandle(HandleNumber);
    if (CurHandle == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, L"<HandleNumber>");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      FilePath = NULL;
      Status = gBS->HandleProtocol (CurHandle, &gEfiLoadedImageDevicePathProtocolGuid, (VOID**)&FilePath);
      if (EFI_ERROR (Status)) {
        Status = EFI_SUCCESS;
        //
        // Try to construct the DevicePath
        //
        Status = gBS->OpenProtocol(CurHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&Image, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_HANDLE), gShellInstall1HiiHandle, HandleNumber);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = gBS->HandleProtocol (Image->DeviceHandle, &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_HANDLE), gShellInstall1HiiHandle, HandleNumber);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            FilePath = AppendDevicePath(DevicePath, Image->FilePath);
          }
        }
      }
    }
  } else {
    //
    // Get file info
    //
    Status = ShellOpenFileMetaArg ((CHAR16*)File, EFI_FILE_MODE_READ, &FileList);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellInstall1HiiHandle, File);
      ShellStatus = SHELL_NOT_FOUND;
    } else if (FileList == NULL || FileList->Link.ForwardLink != FileList->Link.BackLink) {
      //
      // If filename expanded to multiple names, fail
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE), gShellInstall1HiiHandle, File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Arg = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link);
      if (EFI_ERROR(Arg->Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_OPEN), gShellInstall1HiiHandle, File, Arg->Status);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Build FilePath to the filename
        //

        //
        // get the device path
        //
        DevicePath = mEfiShellProtocol->GetDevicePathFromFilePath(Arg->FullName);
        if (DevicePath == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_DP), gShellInstall1HiiHandle, Arg->FullName);
          ShellStatus = SHELL_UNSUPPORTED;
        } else {
          if (UseFullPath) {
            FilePath = DuplicateDevicePath(DevicePath);
          } else {
            for(p=Arg->FullName; *p != CHAR_NULL && *p != ':'; p++);
            FilePath = FileDevicePath(NULL, p+1);
          }

          FreePool(DevicePath);
        }
      }
    }
  }


  if (ShellStatus == SHELL_SUCCESS) {
    //
    // Find a free target ,a brute force implementation
    //
    Found = FALSE;
    for (TargetLocation=0; TargetLocation < 0xFFFF; TargetLocation++) {
      Found = TRUE;
      for (Index=0; Index < OrderCount; Index++) {
        if (CurrentOrder[Index] == TargetLocation) {
          Found = FALSE;
          break;
        }
      }

      if (Found) {
        break;
      }
    }

    if (TargetLocation == 0xFFFF) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET_NF), gShellInstall1HiiHandle);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET), gShellInstall1HiiHandle, TargetLocation);
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    //
    // Add the option
    //
    DescSize = StrSize(Desc);
    FilePathSize = GetDevicePathSize (FilePath);

    p8 = AllocateZeroPool(sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize);
    if (p8 != NULL) {
      p8Copy = p8;
      *((UINT32 *) p8) = LOAD_OPTION_ACTIVE;      // Attributes
      p8 += sizeof (UINT32);

      *((UINT16 *) p8) = (UINT16)FilePathSize;    // FilePathListLength
      p8 += sizeof (UINT16);

      CopyMem (p8, Desc, DescSize);
      p8 += DescSize;
      CopyMem (p8, FilePath, FilePathSize);

      UnicodeSPrint (OptionStr, sizeof(OptionStr), L"%s%04x", Target == BCFG_TARGET_BOOT_ORDER?L"Boot":L"Driver", TargetLocation);
      Status = gRT->SetVariable (
            OptionStr,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
            sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize,
            p8Copy
           );

      FreePool(p8Copy);

      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellInstall1HiiHandle, OptionStr, Status);
      } else {
        NewOrder = AllocateZeroPool((OrderCount+1)*sizeof(UINT16));
        ASSERT(NewOrder != NULL);
        CopyMem(NewOrder, CurrentOrder, (OrderCount)*sizeof(UINT16));

        //
        // Insert target into order list
        //
        for (Index=OrderCount; Index > Position; Index--) {
          NewOrder[Index] = NewOrder[Index-1];
        }

        NewOrder[Index] = (UINT16)TargetLocation;
        Status = gRT->SetVariable (
          Target == BCFG_TARGET_BOOT_ORDER?L"BootOrder":L"DriverOrder",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
          (OrderCount+1) * sizeof(UINT16),
          NewOrder
         );

        FreePool(NewOrder);

        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellInstall1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?L"BootOrder":L"DriverOrder", Status);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ShellPrintEx (-1, -1, L"bcfg: Add %s as %x\n", OptionStr, Index);
        }
      }
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellInstall1HiiHandle);
      ShellStatus = SHELL_OUT_OF_RESOURCES;
    }
  }

//
//If always Free FilePath, will free devicepath in system when use "addh"
//

  if (FilePath!=NULL && !UseHandle) {
    FreePool (FilePath);
  }

  if (Str != NULL) {
    FreePool(Str);
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  if (FileList != NULL) {
    ShellCloseFileMetaArg (&FileList);
  }

  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
BcfgRemoveInstall(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                  Location
  )
{
  CHAR16      VariableName[12];
  UINT16      *NewOrder;
  EFI_STATUS  Status;

  NewOrder  = AllocatePool(OrderCount*sizeof(CurrentOrder[0]));
  if (NewOrder == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }

  CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
  CopyMem(NewOrder+Location, NewOrder+Location+1, (OrderCount - Location - 1)*sizeof(CurrentOrder[0]));

  Status = gRT->SetVariable(
    Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    (OrderCount-1)*sizeof(CurrentOrder[0]), // drop 1 off since the list is 1 shorter
    NewOrder);

  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellInstall1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }

  UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Target == BCFG_TARGET_BOOT_ORDER?L"Boot":L"Driver", CurrentOrder[Location]);
  Status = gRT->SetVariable(
    VariableName,
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    0,
    NULL);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellInstall1HiiHandle, VariableName, Status);
    return (SHELL_INVALID_PARAMETER);
  }

  return (SHELL_SUCCESS);
}

SHELL_STATUS
EFIAPI
BcfgMoveInstall(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                 OldLocation,
  IN CONST UINT16                 NewLocation
  )
{
  UINT16            *NewOrder;
  EFI_STATUS        Status;
  UINT16            Temp;

  NewOrder = AllocatePool(OrderCount*sizeof(CurrentOrder[0]));
  ASSERT(NewOrder != NULL);

  Temp = CurrentOrder[OldLocation];
  CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
  CopyMem(NewOrder+OldLocation, NewOrder+OldLocation+1, (OrderCount - OldLocation - 1)*sizeof(CurrentOrder[0]));
  if (NewLocation == OrderCount) {
    NewOrder[OrderCount-1] = Temp;
  } else {
    CopyMem(NewOrder+NewLocation+1, NewOrder+NewLocation, (OrderCount - NewLocation - 1)*sizeof(CurrentOrder[0]));
    NewOrder[NewLocation] = Temp;
  }


  Status = gRT->SetVariable(
    Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    OrderCount*sizeof(CurrentOrder[0]),
    NewOrder);

  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellInstall1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

SHELL_STATUS
EFIAPI
BcfgDisplayDumpInstall(
  IN CONST CHAR16   *Op,
  IN CONST UINT16   *CurrentOrder,
  IN CONST UINTN    OrderCount,
  IN CONST BOOLEAN  VerboseOutput
  )
{
  EFI_STATUS  Status;
  UINT8       *Buffer;
  UINTN       BufferSize;
  CHAR16      VariableName[12];
  UINTN       LoopVar;
  UINTN       LoopVar2;
  CHAR16      *DevPathString;
  VOID        *DevPath;

  for (LoopVar = 0 ; LoopVar < OrderCount ; LoopVar++) {
    Buffer      = NULL;
    BufferSize  = 0;
    UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Op, CurrentOrder[LoopVar]);

    Status = gRT->GetVariable(
        VariableName,
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &BufferSize,
        Buffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocatePool(BufferSize);
      Status = gRT->GetVariable(
          VariableName,
          (EFI_GUID*)&gEfiGlobalVariableGuid,
          NULL,
          &BufferSize,
          Buffer);
    }

    if (EFI_ERROR(Status) || Buffer == NULL) {
      SHELL_FREE_NON_NULL(Buffer);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_READ_FAIL), gShellInstall1HiiHandle, VariableName, Status);
      return (SHELL_INVALID_PARAMETER);
    }

    DevPath = AllocatePool(*(UINT16*)(Buffer+4));
    CopyMem(DevPath, Buffer+6+StrSize((CHAR16*)(Buffer+6)), *(UINT16*)(Buffer+4));
    DevPathString = gDevPathToText->ConvertDevicePathToText(DevPath, TRUE, FALSE);
    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN(STR_BCFG_LOAD_OPTIONS),
      gShellInstall1HiiHandle,
      Op,
      LoopVar,
      (CHAR16*)(Buffer+6),
      DevPathString,
      (StrSize((CHAR16*)(Buffer+6)) + *(UINT16*)(Buffer+4) + 6) <= BufferSize?L'N':L'Y');
    if (VerboseOutput) {
      for (LoopVar2 = (StrSize((CHAR16*)(Buffer+6)) + *(UINT16*)(Buffer+4) + 6);LoopVar2<BufferSize;LoopVar2++){
        ShellPrintEx(-1, -1, L"%02x", Buffer[LoopVar2]);
      }
      ShellPrintEx(-1, -1, L"\r\n");
    }

    if (Buffer != NULL) {
      FreePool(Buffer);
    }
    if (DevPath != NULL) {
      FreePool(DevPath);
    }
    if (DevPathString != NULL) {
      FreePool(DevPathString);
    }
  }
  return (SHELL_SUCCESS);
}

VOID
EFIAPI
InitBcfgStructInstall(
  IN BGFG_OPERATION *Struct
  )
{
  ASSERT(Struct != NULL);
  Struct->Target      = BCFG_TARGET_MAX;
  Struct->Type        = BCFG_TYPE_MAX;
  Struct->Number1     = 0;
  Struct->Number2     = 0;
  Struct->HandleIndex = 0;
  Struct->FileName    = NULL;
  Struct->Description = NULL;
  Struct->Order       = NULL;
  Struct->OptData     = NULL;
}


STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},
  {L"-opt", TypeMaxValue},
  {NULL, TypeMax}
  };

/**
  Function for 'bcfg' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunBcfgInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Package;
  CHAR16                *ProblemParam;
  SHELL_STATUS          ShellStatus;
  UINTN                 ParamNumber;
  CONST CHAR16          *CurrentParam;
  BGFG_OPERATION        CurrentOperation;
  UINTN                 Length;

  Length              = 0;
  ProblemParam        = NULL;
  Package             = NULL;
  ShellStatus         = SHELL_SUCCESS;

  InitBcfgStructInstall(&CurrentOperation);

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // small block to read the target of the operation
    //
    if (ShellCommandLineGetFlag(Package, L"-opt") && ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (!ShellCommandLineGetFlag(Package, L"-opt") && ShellCommandLineGetCount(Package) < 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"driver") == 0) {
      CurrentOperation.Target = BCFG_TARGET_DRIVER_ORDER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"boot") == 0) {
      CurrentOperation.Target = BCFG_TARGET_BOOT_ORDER;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_DRIVER_BOOT), gShellInstall1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }

    //
    // Read in if we are doing -OPT
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BCFG_TARGET_MAX && ShellCommandLineGetFlag(Package, L"-opt")) {
      CurrentOperation.OptData = ShellCommandLineGetValue(Package, L"-opt");
      CurrentOperation.Type = BCFG_TYPE_OPT;
    }

    //
    // Read in the boot or driver order environment variable (not needed for opt)
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BCFG_TARGET_MAX && CurrentOperation.Type != BCFG_TYPE_OPT) {
      Length = 0;
      Status = gRT->GetVariable(
        CurrentOperation.Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &Length,
        CurrentOperation.Order);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        CurrentOperation.Order = AllocatePool(Length+(4*sizeof(CurrentOperation.Order[0])));
        Status = gRT->GetVariable(
          CurrentOperation.Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
          (EFI_GUID*)&gEfiGlobalVariableGuid,
          NULL,
          &Length,
          CurrentOperation.Order);
      }
    }

    //
    // large block to read the type of operation and verify parameter types for the info.
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BCFG_TARGET_MAX) {
      for (ParamNumber = 2 ; ParamNumber < ShellCommandLineGetCount(Package) && ShellStatus == SHELL_SUCCESS; ParamNumber++) {
        CurrentParam = ShellCommandLineGetRawValue(Package, ParamNumber);
        if        (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"dump") == 0)    {
          if (ShellCommandLineGetCount(Package) > 3) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_DUMP;
        } else if (ShellCommandLineGetFlag(Package, L"-v")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, L"-v (without dump)");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"add") == 0)     {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_ADD;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            ASSERT(CurrentOperation.FileName == NULL);
            CurrentOperation.FileName    = StrnCatGrow(&CurrentOperation.FileName   , NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            ASSERT(CurrentOperation.Description == NULL);
            CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"addp") == 0)    {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Type = BCFG_TYPE_ADDP;
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
              ASSERT(CurrentOperation.FileName == NULL);
              CurrentOperation.FileName    = StrnCatGrow(&CurrentOperation.FileName   , NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
              ASSERT(CurrentOperation.Description == NULL);
              CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"addh") == 0)    {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_ADDH;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentOperation.HandleIndex = (UINT16)StrHexToUintn(CurrentParam);
              ASSERT(CurrentOperation.Description == NULL);
              CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"rm") == 0)      {
          if ((ParamNumber + 1) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_RM;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            if (CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_LOCATION_RANGE), gShellInstall1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"mv") == 0)      {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellInstall1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_MV;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            if (CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_LOCATION_RANGE), gShellInstall1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentOperation.Number2     = (UINT16)StrHexToUintn(CurrentParam);
            }
            if (CurrentOperation.Number2 == CurrentOperation.Number1
              ||CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))
              ||CurrentOperation.Number2 > (Length / sizeof(CurrentOperation.Order[0]))
             ){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_LOCATION_RANGE), gShellInstall1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellInstall1HiiHandle, CurrentParam);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
    }
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BCFG_TARGET_MAX && CurrentOperation.Type < BCFG_TYPE_MAX) {
      //
      // we have all the info.  Do the work
      //
      switch (CurrentOperation.Type) {
        case   BCFG_TYPE_DUMP:
          ShellStatus = BcfgDisplayDumpInstall(
            CurrentOperation.Target == BCFG_TARGET_BOOT_ORDER?L"Boot":L"Driver",
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            ShellCommandLineGetFlag(Package, L"-v"));
          break;
        case   BCFG_TYPE_MV:
          ShellStatus = BcfgMoveInstall(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Number1,
            CurrentOperation.Number2);
          break;
        case   BCFG_TYPE_RM:
          ShellStatus = BcfgRemoveInstall(
            CurrentOperation.Target,
            CurrentOperation.Order,
            (Length / sizeof(CurrentOperation.Order[0])),
            CurrentOperation.Number1);
          break;
        case   BCFG_TYPE_ADD:
        case   BCFG_TYPE_ADDP:
        case   BCFG_TYPE_ADDH:
          ShellStatus = BcfgAddInstall(
            CurrentOperation.Number1,
            CurrentOperation.FileName,
            CurrentOperation.Description,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Target,
            (BOOLEAN)(CurrentOperation.Type == BCFG_TYPE_ADDH),
            (BOOLEAN)(CurrentOperation.Type == BCFG_TYPE_ADD ),
            CurrentOperation.HandleIndex);
          break;
        case   BCFG_TYPE_OPT:
          ShellStatus = BcfgAddOptInstall(
            CurrentOperation.Target,
            CurrentOperation.OptData);
          break;
        default:
          ASSERT(FALSE);
      }
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  if (CurrentOperation.FileName != NULL) {
    FreePool(CurrentOperation.FileName);
  }
  if (CurrentOperation.Description != NULL) {
    FreePool(CurrentOperation.Description);
  }
  if (CurrentOperation.Order != NULL) {
    FreePool(CurrentOperation.Order);
  }

  return (ShellStatus);
}
