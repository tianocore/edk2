/** @file
  CxlFirmwareMgmt Application is used to send and receive FMP commands
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlFirmwareMgmt.h"

EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16    *FileName,
  OUT UINTN     *BufferSize,
  OUT VOID      **Buffer
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  SHELL_FILE_HANDLE     FileHandle;
  UINTN                 FileSize = 0;
  VOID                  *TempBuffer;
  UINTN                 Position = 0;
  EFI_SHELL_PROTOCOL    *ShellProtocol = NULL;
  UINTN                 TempBufferSize; 
  Status = gBS->LocateProtocol (
             &gEfiShellProtocolGuid,
             NULL,
             (VOID **)&ShellProtocol
             );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error gEfiShellProtocolGuid %r\n", Status);
    ShellProtocol = NULL;
    *BufferSize = 0; 
    *Buffer = NULL;
    return Status;
   }

  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &FileHandle,
                            EFI_FILE_MODE_READ
                            );

  if (EFI_ERROR (Status)) {
    Print(L"ReadFileToBuffer: Error open file by name %r\n", Status);
    return Status;
  }

  Status = ShellProtocol->GetFileSize (FileHandle, &FileSize);

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  if (FileSize > CXL_FW_SIZE) {
    Print(L"ReadFileToBuffer: Error FileSize = %d is greater then 32 MB\n", FileSize);
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  TempBufferSize = FileSize;
  TempBuffer = AllocateZeroPool (TempBufferSize);
  if (NULL == TempBuffer) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  Status = ShellProtocol->SetFilePosition (FileHandle, Position);
  if (EFI_ERROR (Status)) {
    Print (L"Error in setting position...%r (Position = 0x%X)\n", Status, Position);
    return Status;
  }

  Status = ShellProtocol->ReadFile (
                            FileHandle,
                            &TempBufferSize,
                            TempBuffer
                            );

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  Status = ShellProtocol->CloseFile (FileHandle);

  *BufferSize = TempBufferSize;
  *Buffer = TempBuffer;
  return EFI_SUCCESS;
}

EFI_STATUS getHandleNum(UINTN  *NumOfHandles, EFI_HANDLE  **Handles) {
  EFI_STATUS    Status = EFI_SUCCESS;
  Status = gBS->LocateHandleBuffer(
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  NumOfHandles,
                  Handles
  );

  if (EFI_ERROR(Status)) {
      Print(L"getHandleNum: LocateHandleBuffer failed status = %r, NumOfHandles = %d\n", Status, *NumOfHandles);
      return Status;
  }

  if (0 == *NumOfHandles) {
      Print(L"getHandleNum: Handle not found status = %r, NoHandles = %d\n", Status, *NumOfHandles);
      return EFI_NOT_FOUND;
  }
  return Status;
}

EFI_STATUS getPrivateStr(CXL_CONTROLLER_PRIVATE_DATA  **Private1, EFI_HANDLE  *Handles, UINTN Index) {

  EFI_STATUS                        Status = EFI_SUCCESS;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *FirmwareMgmt = NULL;

  Status = gBS->HandleProtocol(
                  Handles[Index],
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID**)&FirmwareMgmt
                  );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  *Private1 = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(FirmwareMgmt);
  if (CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE != (*Private1)->Signature) {
    Print(L"\ngetPrivateStr: Error, Private Data is not for CXL device!\n");
    Status = EFI_NOT_FOUND;
  }
  return Status;
}

EFI_STATUS GetCXLDeviceList()
{
  EFI_HANDLE                      *Handles = NULL;
  UINTN                           Index;
  EFI_STATUS                      Status = EFI_SUCCESS;
  UINTN                           NumOfHandles = 0;
  bool                            printFlag = FALSE;
  CXL_CONTROLLER_PRIVATE_DATA     *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"GetCXLDeviceList: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }
    
    if (printFlag == FALSE) {
      Print(L"Device          BUS    DEVICE    FUNCTION   \n");
      printFlag = TRUE;
    }
    
    Print(L"CXLDevice[%d]:   %d       %d          %d\n", Index, Private->Bus, Private->Dev, Private->Func);
  }

  FreePool(Handles);
  return Status;
}

void
PrintHelpPage ()
{
  Print (L" -fGetCXLDeviceList                                          : Get CXL Device List (GetCXLDeviceList).\n");
}

CXL_FMP_OPERATION_TYPE GetOptype(UINTN  Argc, CHAR16  **Argv) {

  CXL_FMP_OPERATION_TYPE OpType;
  CHAR16 *str = NULL;

  if (1 == Argc) {
    OpType = OpTypeDisplayHelp;
    goto END;
  }

  str = Argv[1];

  if (!StrCmp(str, L"-fGetCXLDeviceList")) {
    OpType = OpTypeListDevice;
  } else {
      Print(L"Invalid argument...\n");
      OpType = OpTypeDisplayHelp;
    }

END:
  return OpType;
}

CXL_FMP_OPERATION_TYPE ParseArguments(UINTN  Argc, CHAR16  **Argv, UINTN  *Bus, UINTN  *Device, UINTN  *Func, UINTN  *Slot, CHAR16  **FileName) {

  CXL_FMP_OPERATION_TYPE OpType;
  OpType = GetOptype(Argc, Argv);
  return OpType;
}

EFI_STATUS
EFIAPI
cxlFWMain(
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE    OpType;
  UINTN                     Bus, Device, Func, Slot;
  CHAR16                    *FileName = NULL;
  EFI_STATUS                Status = EFI_SUCCESS;
  OpType = ParseArguments(Argc, Argv, &Bus, &Device, &Func, &Slot, &FileName);

  switch (OpType){
    case OpTypeDisplayHelp:
      PrintHelpPage();
      Status = EFI_SUCCESS;
      break;

    case OpTypeListDevice:
      Status = GetCXLDeviceList();
      break;

  default:
    Print(L"Invalid Operation Type\n");
    break;
  }

  if (NULL != FileName) {
    FreePool(FileName);
  }
  return Status;
}

EFI_STATUS
EFIAPI
MyShellCEntryLib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SHELL_PARAMETERS_PROTOCOL  *EfiShellParametersProtocol;
  EFI_STATUS                     Status;
  EfiShellParametersProtocol  =  NULL;

  Status = SystemTable->BootServices->OpenProtocol(ImageHandle,
                          &gEfiShellParametersProtocolGuid,
                          (VOID **)&EfiShellParametersProtocol,
                          ImageHandle,
                          NULL,
                          EFI_OPEN_PROTOCOL_GET_PROTOCOL
                          );

  if (!EFI_ERROR(Status)) {
    Status = cxlFWMain ( EfiShellParametersProtocol->Argc, EfiShellParametersProtocol->Argv);
    } else {
      ASSERT(FALSE);
    }
  return Status;
}
