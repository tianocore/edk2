/** @file
  CxlFirmwareMgmt Application is used to send and receive FMP commands
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlFwMgmtApp.h"

/**
  Opens the file and read its buffer value

  @param[in] FileName                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] BufferSize              size of buffer to be read

  @retval Buffer                     input file is read and buffer is returned

  **/
EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16  *FileName,
  OUT UINTN   *BufferSize,
  OUT VOID    **Buffer
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   FileHandle;
  UINT64              FileSize;
  VOID                *TempBuffer;
  UINTN               Position;
  EFI_SHELL_PROTOCOL  *ShellProtocol;
  UINTN               TempBufferSize;

  FileSize      = 0;
  Position      = 0;
  ShellProtocol = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiShellProtocolGuid,
                  NULL,
                  (VOID **)&ShellProtocol
                  );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error gEfiShellProtocolGuid %r\n", Status);
    ShellProtocol = NULL;
    *BufferSize   = 0;
    *Buffer       = NULL;
    return Status;
  }

  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &FileHandle,
                            EFI_FILE_MODE_READ
                            );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error open file by name %r\n", Status);
    return Status;
  }

  Status = ShellProtocol->GetFileSize (FileHandle, &FileSize);

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  if (FileSize > CXL_FW_SIZE) {
    Print (L"ReadFileToBuffer: Error FileSize = %d is greater then 32 MB\n", FileSize);
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  TempBufferSize = (UINTN)FileSize;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
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
  *Buffer     = TempBuffer;
  return EFI_SUCCESS;
}

/**
  Get lots of info about a device from its handle.

  @param[in]  NumOfHandles         Number of handle to loop among all

  @retval Handles                  return handle of firmware management protocol

  **/
EFI_STATUS
GetHandleInfo (
  UINTN       *NumOfHandles,
  EFI_HANDLE  **Handles
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  NumOfHandles,
                  Handles
                  );

  if (EFI_ERROR (Status)) {
    Print (L"GetHandleInfo: LocateHandleBuffer failed status = %r, NumOfHandles = %d\n", Status, *NumOfHandles);
    return Status;
  }

  if (0 == *NumOfHandles) {
    Print (L"GetHandleInfo: Handle not found status = %r, NoHandles = %d\n", Status, *NumOfHandles);
    return EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Get CXL controller private data structure from firmware management

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Handles                  Number of handle to loop among all
  @param[in] Index                    Index of handle

  @retval Status                      Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetCxlPrivateData (
  CXL_CONTROLLER_PRIVATE_DATA  **Private1,
  EFI_HANDLE                   *Handles,
  UINTN                        Index
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *FirmwareMgmt;

  Status       = EFI_SUCCESS;
  FirmwareMgmt = NULL;

  Status = gBS->HandleProtocol (
                  Handles[Index],
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID **)&FirmwareMgmt
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Private1 = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (FirmwareMgmt);
  if (CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE != (*Private1)->Signature) {
    Print (L"\ngetPrivateStr: Error, Private Data is not for CXL device!\n");
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Gets BUS, DEVICE, FUNCTION value of all CXL supported device

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetCxlDeviceList (
  )
{
  EFI_HANDLE                   *Handles = NULL;
  UINTN                        Index;
  EFI_STATUS                   Status       = EFI_SUCCESS;
  UINTN                        NumOfHandles = 0;
  bool                         printFlag    = FALSE;
  CXL_CONTROLLER_PRIVATE_DATA  *Private     = NULL;

  Handles      = NULL;
  Status       = EFI_SUCCESS;
  NumOfHandles = 0;
  printFlag    = FALSE;
  Private      = NULL;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"GetCxlDeviceList: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if (printFlag == FALSE) {
      Print (L"Device          BUS    DEVICE    FUNCTION   \n");
      printFlag = TRUE;
    }

    Print (L"CXLDevice[%d]:   %d       %d          %d\n", Index, Private->Bus, Private->Device, Private->Function);
  }

  FreePool (Handles);
  return Status;
}

/**
  Prints help page of the CXL Firmware management application

  **/
void
PrintHelpPage (
  )
{
  Print (L" -fGetCXLDeviceList                                          : Get CXL Device List (GetCxlDeviceList).\n");
}

/**
  Get operation type from input arguments in CXL Application

  @retval                          DisplayHelp, ListDevice, FmpGetImgInfo, GetImage, SetPkgInfo, FmpSetImg, FmpCheckImg, FmpGetPkgInfo

  **/
CXL_FMP_OPERATION_TYPE
GetOperationType (
  UINTN   Argc,
  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;
  CHAR16                  *String;

  String = NULL;

  if (1 == Argc) {
    OpType = OpTypeDisplayHelp;
    goto END;
  }

  String = Argv[1];

  if (!StrCmp (String, L"-fGetCXLDeviceList")) {
    OpType = OpTypeListDevice;
  } else {
    Print (L"Invalid argument...\n");
    OpType = OpTypeDisplayHelp;
  }

END:
  return OpType;
}

/**
  Parse the argument from input arguments in CXL Application

  @retval Bus                     Bus value in BDF of device.
  @retval Device                  Device value in BDF of device.
  @retval Function                Function value in BDF of device.
  @retval Slot                    Slot information on which image info is to be taken

  **/
CXL_FMP_OPERATION_TYPE
ParseArguments (
  UINTN   Argc,
  CHAR16  **Argv,
  UINTN   *Bus,
  UINTN   *Device,
  UINTN   *Function,
  UINTN   *Slot,
  CHAR16  **FileName
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;

  OpType = GetOperationType (Argc, Argv);
  return OpType;
}

/**
  CXL Application main function which will call the operation

  @retval Status                      Return EFI_SUCCESS on successfully calling the requested operation
  **/
EFI_STATUS
EFIAPI
CxlFwMain (
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;
  UINTN                   Bus;
  UINTN                   Device;
  UINTN                   Function;
  UINTN                   Slot;
  CHAR16                  *FileName;
  EFI_STATUS              Status;

  FileName = NULL;
  Status   = EFI_SUCCESS;

  OpType = ParseArguments (Argc, Argv, &Bus, &Device, &Function, &Slot, &FileName);

  switch (OpType) {
    case OpTypeDisplayHelp:
      PrintHelpPage ();
      Status = EFI_SUCCESS;
      break;

    case OpTypeListDevice:
      Status = GetCxlDeviceList ();
      break;

    default:
      Print (L"Invalid Operation Type\n");
      break;
  }

  if (NULL != FileName) {
    FreePool (FileName);
  }

  return Status;
}

/**
  Shell Entry library of the function

  @param[in] ImageHandle              Image Handle for entry lib.

  @retval Status                      Return EFI_SUCCESS on successfully calling the requested operation

  **/
EFI_STATUS
EFIAPI
MyShellCEntryLib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SHELL_PARAMETERS_PROTOCOL  *EfiShellParametersProtocol;
  EFI_STATUS                     Status;

  EfiShellParametersProtocol =  NULL;

  Status = SystemTable->BootServices->OpenProtocol (
                                        ImageHandle,
                                        &gEfiShellParametersProtocolGuid,
                                        (VOID **)&EfiShellParametersProtocol,
                                        ImageHandle,
                                        NULL,
                                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                                        );

  if (!EFI_ERROR (Status)) {
    Status = CxlFwMain (EfiShellParametersProtocol->Argc, EfiShellParametersProtocol->Argv);
  } else {
    ASSERT (FALSE);
  }

  return Status;
}

