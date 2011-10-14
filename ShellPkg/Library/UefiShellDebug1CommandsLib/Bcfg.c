/** @file
  Main file for bcfg shell Debug1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
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
  BcfgTargetBootOrder    = 0,
  BcfgTargetDriverOrder  = 1,
  BcfgTargetMax          = 2
} BCFG_OPERATION_TARGET;

typedef enum {
  BcfgTypeDump       = 0,
  BcfgTypeAdd        = 1,
  BcfgTypeAddp       = 2,
  BcfgTypeAddh       = 3,
  BcfgTypeRm         = 4,
  BcfgTypeMv         = 5,
  BcfgTypeOpt        = 6,
  BcfgTypeMax        = 7
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
  This function will populate the device path protocol parameter based on TheHandle.

  @param[in]      TheHandle     Driver handle.
  @param[in, out] FilePath      On a sucessful return the device path to the handle.

  @retval EFI_SUCCESS           The device path was sucessfully returned.
  @retval other                 A error from gBS->HandleProtocol.

  @sa HandleProtocol
**/
EFI_STATUS
EFIAPI
GetDevicePathForDriverHandleDebug1 (
  IN EFI_HANDLE                   TheHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath
  )
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;

  Status = gBS->OpenProtocol (
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID**)&LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ImageDevicePath,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
    if (!EFI_ERROR (Status)) {
//      *DevPath  = DuplicateDevicePath (ImageDevicePath);
//      *FilePath = DuplicateDevicePath (LoadedImage->FilePath);
        *FilePath = AppendDevicePath(ImageDevicePath,LoadedImage->FilePath);
      gBS->CloseProtocol(
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  gImageHandle,
                  NULL);
    }
    gBS->CloseProtocol(
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                gImageHandle,
                NULL);
  }
  return (Status);
}

/**
  Function to add a option.

  @param[in] Position       The position to add Target at.
  @param[in] File           The file to make the target.
  @param[in] Desc           The description text.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] Target         The info on the option to add.
  @param[in] UseHandle      TRUE to use HandleNumber, FALSE to use File and Desc.
  @param[in] UsePath        TRUE to convert to devicepath.
  @param[in] HandleNumber   The handle number to add.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
EFIAPI
BcfgAddDebug1(
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
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *FileNode;
//  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  CHAR16                    *Str;
//  CONST CHAR16              *StringWalker;
  UINT8                     *TempByteBuffer;
  UINT8                     *TempByteStart;
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
    if (File == NULL || Desc == NULL) {
      return (SHELL_INVALID_PARAMETER);
    }
  } else {
    if (HandleNumber == 0) {
      return (SHELL_INVALID_PARAMETER);
    }
  }

  if (Position > OrderCount) {
    Position =  OrderCount;
  }

  Str             = NULL;
  FilePath        = NULL;
  FileNode        = NULL;
  FileList        = NULL;
  Handles         = NULL;
  ShellStatus     = SHELL_SUCCESS;
  TargetLocation  = 0xFFFF;

  if (UseHandle) {
    CurHandle = ConvertHandleIndexToHandle(HandleNumber);
    if (CurHandle == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"Handle Number");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (Target == BcfgTargetBootOrder) {
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
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_HANDLE), gShellDebug1HiiHandle, HandleNumber);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        //
        //Make sure that the handle should point to driver, not a controller.
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

        Status = gBS->HandleProtocol (
                   CurHandle,
                   &gEfiDevicePathProtocolGuid,
                   (VOID**)&FilePath);

        if (DriverBindingHandleCount > 0
              || ParentControllerHandleCount > 0
              || ChildControllerHandleCount > 0
              || !EFI_ERROR(Status) ) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"Handle Number");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // Get the DevicePath from the loaded image information.
          //
          Status = GetDevicePathForDriverHandleDebug1(CurHandle, &FilePath);
        }
      }
    }
  } else {
    //
    // Get file info
    //
    ShellOpenFileMetaArg ((CHAR16*)File, EFI_FILE_MODE_READ, &FileList);

    if (FileList == NULL) {
      //
      // If filename matched nothing fail
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (FileList->Link.ForwardLink != FileList->Link.BackLink) {
      //
      // If filename expanded to multiple names, fail
      //
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
        DevicePath = gEfiShellProtocol->GetDevicePathFromFilePath(Arg->FullName);
        if (DevicePath == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_DP), gShellDebug1HiiHandle, Arg->FullName);
          ShellStatus = SHELL_UNSUPPORTED;
        } else {
/*
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
            for(StringWalker=Arg->FullName; *StringWalker != CHAR_NULL && *StringWalker != ':'; StringWalker++);
            FileNode = FileDevicePath(NULL, StringWalker+1);
            FilePath = AppendDevicePath(DevicePath, FileNode);
            FreePool(FileNode);
          } else {
*/
            FilePath = DuplicateDevicePath(DevicePath);
/*
          }
*/
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

    TempByteBuffer = AllocateZeroPool(sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize);
    if (TempByteBuffer != NULL) {
      TempByteStart  = TempByteBuffer;
      *((UINT32 *) TempByteBuffer) = LOAD_OPTION_ACTIVE;      // Attributes
      TempByteBuffer += sizeof (UINT32);

      *((UINT16 *) TempByteBuffer) = (UINT16)FilePathSize;    // FilePathListLength
      TempByteBuffer += sizeof (UINT16);

      CopyMem (TempByteBuffer, Desc, DescSize);
      TempByteBuffer += DescSize;
      CopyMem (TempByteBuffer, FilePath, FilePathSize);

      UnicodeSPrint (OptionStr, sizeof(OptionStr), L"%s%04x", Target == BcfgTargetBootOrder?L"Boot":L"Driver", TargetLocation);
      Status = gRT->SetVariable (
            OptionStr,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
            sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize,
            TempByteStart
           );

      FreePool(TempByteStart);
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }

    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellDebug1HiiHandle, OptionStr, Status);
    } else {
      NewOrder = AllocateZeroPool((OrderCount+1)*sizeof(NewOrder[0]));
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
        Target == BcfgTargetBootOrder?L"BootOrder":L"DriverOrder",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
        (OrderCount+1) * sizeof(UINT16),
        NewOrder
       );

      FreePool(NewOrder);

      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BcfgTargetBootOrder?L"BootOrder":L"DriverOrder", Status);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Print (L"bcfg: Add %s as %x\n", OptionStr, Position);
      }
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

/**
  Funciton to remove an item.

  @param[in] Target         The target item to move.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] Location       The current location of the Target.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
EFIAPI
BcfgRemoveDebug1(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                 Location
  )
{
  CHAR16      VariableName[12];
  UINT16      *NewOrder;
  EFI_STATUS  Status;
  UINTN       NewCount;

  UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Target == BcfgTargetBootOrder?L"Boot":L"Driver", CurrentOrder[Location]);
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
  NewOrder = AllocateZeroPool(OrderCount*sizeof(CurrentOrder[0]));
  if (NewOrder != NULL) {
    NewCount = OrderCount;
    CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
    CopyMem(NewOrder+Location, NewOrder+Location+1, (OrderCount - Location - 1)*sizeof(CurrentOrder[0]));
    NewCount--;

    Status = gRT->SetVariable(
      Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
      (EFI_GUID*)&gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
      NewCount*sizeof(NewOrder[0]),
      NewOrder);
    FreePool(NewOrder);
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

/**
  Funciton to move a item to another location.

  @param[in] Target         The target item to move.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] OldLocation    The current location of the Target.
  @param[in] NewLocation    The desired location of the Target.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
EFIAPI
BcfgMoveDebug1(
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

  NewOrder = AllocateZeroPool(OrderCount*sizeof(CurrentOrder[0]));
  ASSERT(NewOrder != NULL);

  Temp = CurrentOrder[OldLocation];
  CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
  CopyMem(NewOrder+OldLocation, NewOrder+OldLocation+1, (OrderCount - OldLocation - 1)*sizeof(CurrentOrder[0]));
  CopyMem(NewOrder+NewLocation+1, NewOrder+NewLocation, (OrderCount - NewLocation - 1)*sizeof(CurrentOrder[0]));
  NewOrder[NewLocation] = Temp;


  Status = gRT->SetVariable(
    Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    OrderCount*sizeof(CurrentOrder[0]),
    NewOrder);

  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellDebug1HiiHandle, Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder", Status);
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

/**
  Function to add optional data to an option.

  @param[in] OptData      The optional data to add.
  @param[in] Target       The target of the operation.

  @retval SHELL_SUCCESS   The operation was succesful.
**/
SHELL_STATUS
EFIAPI
BcfgAddOptDebug1(
  IN CONST CHAR16                 *OptData,
  IN CONST BCFG_OPERATION_TARGET  Target
  )
{
  ASSERT(OptData != NULL);
  return SHELL_SUCCESS;
}

/**
  Function to dump the Bcfg information.

  @param[in] Op             The operation.
  @param[in] OrderCount     How many to dump.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] VerboseOutput  TRUE for extra output.  FALSE otherwise.

  @retval SHELL_SUCCESS           The dump was successful.
  @retval SHELL_INVALID_PARAMETER A parameter was invalid.
**/
SHELL_STATUS
EFIAPI
BcfgDisplayDumpDebug1(
  IN CONST CHAR16   *Op,
  IN CONST UINTN    OrderCount,
  IN CONST UINT16   *CurrentOrder,
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

  if (OrderCount == 0) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_BCFG_NONE), gShellDebug1HiiHandle);
    return (SHELL_SUCCESS);
  }

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
      Buffer = AllocateZeroPool(BufferSize);
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

    if ((*(UINT16*)(Buffer+4)) != 0) {
      DevPath = AllocateZeroPool(*(UINT16*)(Buffer+4));
      CopyMem(DevPath, Buffer+6+StrSize((CHAR16*)(Buffer+6)), *(UINT16*)(Buffer+4));
      DevPathString = gDevPathToText->ConvertDevicePathToText(DevPath, TRUE, FALSE);
    } else {
      DevPath       = NULL;
      DevPathString = NULL;
    }
    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN(STR_BCFG_LOAD_OPTIONS),
      gShellDebug1HiiHandle,
      LoopVar,
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

/**
  Function to initialize the BCFG operation structure.

  @param[in] Struct   The stuct to initialize.
**/
VOID
EFIAPI
InitBcfgStructDebug1(
  IN BGFG_OPERATION *Struct
  )
{
  ASSERT(Struct != NULL);
  Struct->Target      = BcfgTargetMax;
  Struct->Type        = BcfgTypeMax;
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
  UINT64                Intermediate;

  Length              = 0;
  ProblemParam        = NULL;
  Package             = NULL;
  ShellStatus         = SHELL_SUCCESS;

  InitBcfgStructDebug1(&CurrentOperation);

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
    // Read in if we are doing -OPT
    //
    if (ShellCommandLineGetFlag(Package, L"-opt")) {
      CurrentOperation.OptData = ShellCommandLineGetValue(Package, L"-opt");
      if (CurrentOperation.OptData == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDebug1HiiHandle, L"-opt");
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      CurrentOperation.Type = BcfgTypeOpt;
    }

    //
    // small block to read the target of the operation
    //
    if ((ShellCommandLineGetCount(Package) < 3 && CurrentOperation.Type != BcfgTypeOpt) ||
        (ShellCommandLineGetCount(Package) < 2 && CurrentOperation.Type == BcfgTypeOpt)
       ){
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"driver") == 0) {
      CurrentOperation.Target = BcfgTargetDriverOrder;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"boot") == 0) {
      CurrentOperation.Target = BcfgTargetBootOrder;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_DRIVER_BOOT), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }


    //
    // Read in the boot or driver order environment variable (not needed for opt)
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax && CurrentOperation.Type != BcfgTypeOpt) {
      Length = 0;
      Status = gRT->GetVariable(
        CurrentOperation.Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &Length,
        CurrentOperation.Order);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        CurrentOperation.Order = AllocateZeroPool(Length+(4*sizeof(CurrentOperation.Order[0])));
        Status = gRT->GetVariable(
          CurrentOperation.Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
          (EFI_GUID*)&gEfiGlobalVariableGuid,
          NULL,
          &Length,
          CurrentOperation.Order);
      }
    }

    //
    // large block to read the type of operation and verify parameter types for the info.
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax) {
      for (ParamNumber = 2 ; ParamNumber < ShellCommandLineGetCount(Package) && ShellStatus == SHELL_SUCCESS; ParamNumber++) {
        CurrentParam = ShellCommandLineGetRawValue(Package, ParamNumber);
        if        (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"dump") == 0)    {
          CurrentOperation.Type = BcfgTypeDump;
        } else if (ShellCommandLineGetFlag(Package, L"-v")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"-v (without dump)");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"add") == 0)     {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeAdd;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
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
          CurrentOperation.Type = BcfgTypeAddp;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
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
          CurrentOperation.Type = BcfgTypeAddh;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.HandleIndex = (UINT16)Intermediate;
              ASSERT(CurrentOperation.Description == NULL);
              CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"rm") == 0)      {
          if ((ParamNumber + 1) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeRm;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
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
          CurrentOperation.Type = BcfgTypeMv;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            if (CurrentOperation.Number1 > (Length / sizeof(CurrentOperation.Order[0]))){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellDebug1HiiHandle, Length / sizeof(CurrentOperation.Order[0]));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.Number2     = (UINT16)Intermediate;
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
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax && CurrentOperation.Type < BcfgTypeMax) {
      //
      // we have all the info.  Do the work
      //
      switch (CurrentOperation.Type) {
        case   BcfgTypeDump:
          ShellStatus = BcfgDisplayDumpDebug1(
            CurrentOperation.Target == BcfgTargetBootOrder?L"Boot":L"Driver",
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Order,
            ShellCommandLineGetFlag(Package, L"-v"));
          break;
        case   BcfgTypeMv:
          ShellStatus = BcfgMoveDebug1(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Number1,
            CurrentOperation.Number2);
          break;
        case   BcfgTypeRm:
          ShellStatus = BcfgRemoveDebug1(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Number1);
          break;
        case   BcfgTypeAdd:
        case   BcfgTypeAddp:
        case   BcfgTypeAddh:
          ShellStatus = BcfgAddDebug1(
            CurrentOperation.Number1,
            CurrentOperation.FileName,
            CurrentOperation.Description==NULL?L"":CurrentOperation.Description,
            CurrentOperation.Order,
            Length / sizeof(CurrentOperation.Order[0]),
            CurrentOperation.Target,
            (BOOLEAN)(CurrentOperation.Type == BcfgTypeAddh),
            (BOOLEAN)(CurrentOperation.Type == BcfgTypeAddp),
            CurrentOperation.HandleIndex);
          break;
        case   BcfgTypeOpt:
          ShellStatus = BcfgAddOptDebug1(
            CurrentOperation.OptData,
            CurrentOperation.Target);
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
