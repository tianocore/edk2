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

#include "UefiShellDebug1CommandsLib.h"
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

SHELL_STATUS
EFIAPI
BcfgAdd (
  IN       UINTN                  Position,
  IN CONST CHAR16                 *File,
  IN CONST CHAR16                 *Desc,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST BOOLEAN                UseHandle,
  IN CONST BOOLEAN                UsePath,
  IN CONST UINTN                  HandleNumber
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath, *FilePath, *FileNode, *DevPath;
  CHAR16                    *Str;
  CONST CHAR16              *p;
  UINT8                     *p8;
  EFI_SHELL_FILE_INFO       *Arg;
  EFI_SHELL_FILE_INFO       *FileList;
  CHAR16                    OptionStr[40];
  UINTN                     DescSize, FilePathSize;
  BOOLEAN                   Found;
  UINTN                     TargetLocation;
  UINTN                     Index;
  EFI_HANDLE                *Handles;
  EFI_HANDLE                CurHandle;
  UINTN                     DriverBindingHandleCount;
  UINTN                     ParentControllerHandleCount;
  UINTN                     ChildControllerHandleCount;
  SHELL_STATUS              ShellStatus;
  UINT16                    *NewOrder;

  if (!UseHandle) {
    ASSERT(File != NULL);
    ASSERT(Desc != NULL);
  } else {
    ASSERT(HandleNumber != 0);
  }

  ASSERT(Position <= (OrderCount+1));

  Str             = NULL;
  FilePath        = NULL;
  FileNode        = NULL;
  FileList        = NULL;
  Handles         = NULL;
  ShellStatus     = SHELL_SUCCESS;
  TargetLocation  = 0xFFFF;

//  if (Position > 0) {
//    Position--;
//  }

  if (UseHandle) {
    CurHandle = ConvertHandleIndexToHandle(StrHexToUintn(File));
    if (CurHandle == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      //Make sure that the handle should point to a real controller
      //
      Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
                 CurHandle,
                 &DriverBindingHandleCount,
                 NULL);

      Status = PARSE_HANDLE_DATABASE_PARENTS (
                 CurHandle,
                 &ParentControllerHandleCount,
                 NULL);

      Status = ParseHandleDatabaseForChildControllers (
                 CurHandle,
                 &ChildControllerHandleCount,
                 NULL);

      if (DriverBindingHandleCount > 0
            || ParentControllerHandleCount > 0
            || ChildControllerHandleCount > 0) {
        FilePath = NULL;
        Status = gBS->HandleProtocol (
                   CurHandle,
                   &gEfiDevicePathProtocolGuid,
                   (VOID**)&FilePath);
      }
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_HANDLE), gShellDebug1HiiHandle, StrHexToUintn(File));
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
    }
  } else {
    //
    // Get file info
    //
    ShellOpenFileMetaArg ((CHAR16*)File, EFI_FILE_MODE_READ, &FileList);

    //
    // If filename expanded to multiple names, fail
    //
    if (FileList == NULL || FileList->Link.ForwardLink != FileList->Link.BackLink) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE), gShellDebug1HiiHandle, File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Arg = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link);
      if (EFI_ERROR(Arg->Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_OPEN), gShellDebug1HiiHandle, File, Arg->Status);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Build FilePath to the filename
        //

        //
        // get the device path
        //
        DevicePath = mEfiShellProtocol->GetDevicePathFromFilePath(Arg->FullName);
        if (DevicePath != NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_DP), gShellDebug1HiiHandle, Arg->FullName);
          ShellStatus = SHELL_UNSUPPORTED;
        } else {
          if (UsePath) {
            DevPath = DevicePath;
            while (!IsDevicePathEnd(DevPath)) {
              if ((DevicePathType(DevPath) == MEDIA_DEVICE_PATH) &&
                (DevicePathSubType(DevPath) == MEDIA_HARDDRIVE_DP)) {

                //
                // If we find it use it instead
                //
                DevicePath = DevPath;
                break;
              }
              DevPath = NextDevicePathNode(DevPath);
            }
            //
            // append the file
            //
            for(p=Arg->FullName; *p != CHAR_NULL && *p != ':'; p++);
            FileNode = FileDevicePath(NULL, p+1);
            FilePath = AppendDevicePath(DevicePath, FileNode);
            FreePool(FileNode);
          } else {
            FilePath = DuplicateDevicePath(DevicePath);
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
    for (TargetLocation=1; TargetLocation < 0xFFFF; TargetLocation++) {
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET_NF), gShellDebug1HiiHandle);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET), gShellDebug1HiiHandle, TargetLocation);
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    //
    // Add the option
    //
    DescSize = StrSize(Desc);
    FilePathSize = GetDevicePathSize (FilePath);

    p8 = AllocatePool(sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize);
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
          p8
         );

    FreePool(p8);

    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellDebug1HiiHandle, OptionStr, Status);
    } else {
      NewOrder = AllocatePool((OrderCount+1)*sizeof(NewOrder[0]));
      ASSERT(NewOrder != NULL);
      CopyMem(NewOrder, CurrentOrder, (OrderCount)*sizeof(NewOrder[0]));

      //
      // Insert target into order list
      //
      for (Index=OrderCount; Index > Position; Index--) {
        NewOrder[Index] = NewOrder[Index-1];
      }

      NewOrder[Position] = (UINT16) TargetLocation;
      Status = gRT->SetVariable (
        Target == BCFG_TARGET_BOOT_ORDER?L"BootOrder":L"DriverOrder",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
        (OrderCount+1) * sizeof(UINT16),
        NewOrder
       );

      FreePool(NewOrder);

      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?L"BootOrder":L"DriverOrder", Status);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Print (L"bcfg: Add %s as %x\n", OptionStr, Position);
      }
    }
  }
  if (FileNode != NULL) {
    FreePool (FileNode);
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
BcfgRemove(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                  Location
  )
{
  CHAR16      VariableName[12];
  UINT16      *NewOrder;
  EFI_STATUS  Status;
  UINTN       LoopVar;
  UINTN       NewCount;

  UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Target == BCFG_TARGET_BOOT_ORDER?L"Boot":L"Driver", Location);
  Status = gRT->SetVariable(
    VariableName,
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    0,
    NULL);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, VariableName, Status);
    return (SHELL_INVALID_PARAMETER);
  }
  NewOrder = AllocatePool(OrderCount*sizeof(CurrentOrder[0]));
  NewCount = OrderCount;
  CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
  for (LoopVar = 0 ; LoopVar < OrderCount ; LoopVar++){
    if (NewOrder[LoopVar] == Location) {
      CopyMem(NewOrder+LoopVar, NewOrder+LoopVar+1, (OrderCount - LoopVar - 1)*sizeof(CurrentOrder[0]));
      NewCount--;
    }
  }
  Status = gRT->SetVariable(
    Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    NewCount*sizeof(NewOrder[0]),
    NewOrder);
  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

SHELL_STATUS
EFIAPI
BcfgMove(
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
  CopyMem(NewOrder+NewLocation+1, NewOrder+NewLocation, (OrderCount - NewLocation - 1)*sizeof(CurrentOrder[0]));
  NewOrder[NewLocation] = Temp;


  Status = gRT->SetVariable(
    Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    OrderCount*sizeof(CurrentOrder[0]),
    NewOrder);

  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BCFG_TARGET_BOOT_ORDER?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

SHELL_STATUS
EFIAPI
BcfgDisplayDump(
  IN CONST CHAR16   *Op,
  IN CONST UINTN   OrderCount,
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
    UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Op, LoopVar);

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_READ_FAIL), gShellDebug1HiiHandle, VariableName, Status);
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
      gShellDebug1HiiHandle,
      VariableName,
      (CHAR16*)(Buffer+6),
      DevPathString,
      (StrSize((CHAR16*)(Buffer+6)) + *(UINT16*)(Buffer+4) + 6) <= BufferSize?L'N':L'Y');
    if (VerboseOutput) {
      for (LoopVar2 = (StrSize((CHAR16*)(Buffer+6)) + *(UINT16*)(Buffer+4) + 6);LoopVar2<BufferSize;LoopVar2++){
        ShellPrintEx(
          -1,
          -1,
          NULL,
          L"%02x",
          Buffer[LoopVar2]);
      }
      ShellPrintEx(
        -1,
        -1,
        NULL,
        L"\r\n");
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
InitBcfgStruct(
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
ShellCommandRunBcfg (
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

  InitBcfgStruct(&CurrentOperation);

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // small block to read the target of the operation
    //
    if (ShellCommandLineGetCount(Package) < 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"driver") == 0) {
      CurrentOperation.Target = BCFG_TARGET_DRIVER_ORDER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"boot") == 0) {
      CurrentOperation.Target = BCFG_TARGET_BOOT_ORDER;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_DRIVER_BOOT), gShellDebug1HiiHandle);
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
          CurrentOperation.Type = BCFG_TYPE_DUMP;
        } else if (ShellCommandLineGetFlag(Package, L"-v")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"-v (without dump)");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"add") == 0)     {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_ADD;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
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
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_ADDP;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            ASSERT(CurrentOperation.FileName == NULL);
            CurrentOperation.FileName    = StrnCatGrow(&CurrentOperation.FileName   , NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            ASSERT(CurrentOperation.Description == NULL);
            CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"addh") == 0)    {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_ADDH;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentOperation.HandleIndex = (UINT16)StrHexToUintn(CurrentParam);
              ASSERT(CurrentOperation.Description == NULL);
              CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"rm") == 0)      {
          if ((ParamNumber + 1) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_RM;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            if (CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellDebug1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"mv") == 0)      {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BCFG_TYPE_MV;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Number1     = (UINT16)StrHexToUintn(CurrentParam);
            if (CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellDebug1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentOperation.Number2     = (UINT16)StrHexToUintn(CurrentParam);
            }
            if (CurrentOperation.Number2 == CurrentOperation.Number1
              ||CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))
              ||CurrentOperation.Number2 > (Length / sizeof(CurrentOperation.Order[0]))
             ){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellDebug1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
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
          ShellStatus = BcfgDisplayDump(
            CurrentOperation.Target == BCFG_TARGET_BOOT_ORDER?L"Boot":L"Driver",
            Length / sizeof(CurrentOperation.Order[0]),
            ShellCommandLineGetFlag(Package, L"-v"));
          break;
        case   BCFG_TYPE_MV:
          ShellStatus = BcfgMove(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Number1,
            CurrentOperation.Number2);
          break;
        case   BCFG_TYPE_RM:
          ShellStatus = BcfgRemove(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Number1);
          break;
        case   BCFG_TYPE_ADD:
        case   BCFG_TYPE_ADDP:
        case   BCFG_TYPE_ADDH:
          ShellStatus = BcfgAdd(
            CurrentOperation.Number1,
            CurrentOperation.FileName,
            CurrentOperation.Description,
            CurrentOperation.Order,
            Length,
            CurrentOperation.Target,
            (BOOLEAN)(CurrentOperation.Type == BCFG_TYPE_ADDH),
            (BOOLEAN)(CurrentOperation.Type == BCFG_TYPE_ADDP),
            CurrentOperation.HandleIndex);
          break;
        case   BCFG_TYPE_OPT:
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
