/** @file
  DXE capsule report related function.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/VariableLock.h>
#include <Guid/CapsuleReport.h>
#include <Guid/FmpCapsule.h>
#include <Guid/CapsuleVendor.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DevicePathLib.h>
#include <Library/CapsuleLib.h>

#include <IndustryStandard/WindowsUxCapsule.h>

/**
  Get current capsule last variable index.

  @return Current capsule last variable index.
  @retval -1  No current capsule last variable.
**/
INTN
GetCurrentCapsuleLastIndex (
  VOID
  )
{
  UINTN                            Size;
  CHAR16                           CapsuleLastStr[sizeof("Capsule####")];
  EFI_STATUS                       Status;
  UINT16                           CurrentIndex;

  Size = sizeof(L"Capsule####") - sizeof(CHAR16); // no zero terminator
  Status = gRT->GetVariable(
                  L"CapsuleLast",
                  &gEfiCapsuleReportGuid,
                  NULL,
                  &Size,
                  CapsuleLastStr
                  );
  if (EFI_ERROR(Status)) {
    return -1;
  }
  CurrentIndex = (UINT16)StrHexToUintn(&CapsuleLastStr[sizeof("Capsule") - 1]);
  return CurrentIndex;
}

/**
  Get a new capsule status variable index.

  @return A new capsule status variable index.
  @retval 0  No new capsule status variable index. Rolling over.
**/
INTN
GetNewCapsuleResultIndex (
  VOID
  )
{
  INTN                             CurrentIndex;

  CurrentIndex = GetCurrentCapsuleLastIndex();
  if (CurrentIndex >= PcdGet16(PcdCapsuleMax)) {
    DEBUG((DEBUG_INFO, "  CapsuleResult variable Rolling Over!\n"));
    return 0;
  }

  return CurrentIndex + 1;
}

/**
  Write a new capsule status variable.

  @param[in] CapsuleResult      The capsule status variable
  @param[in] CapsuleResultSize  The size of the capsule stauts variable in bytes

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
WriteNewCapsuleResultVariable (
  IN VOID    *CapsuleResult,
  IN UINTN   CapsuleResultSize
  )
{
  INTN                                CapsuleResultIndex;
  CHAR16                              CapsuleResultStr[sizeof("Capsule####")];
  UINTN                               Size;
  EFI_STATUS                          Status;

  CapsuleResultIndex = GetNewCapsuleResultIndex();
  DEBUG((DEBUG_INFO, "New CapsuleResultIndex - 0x%x\n", CapsuleResultIndex));

  UnicodeSPrint(
    CapsuleResultStr,
    sizeof(CapsuleResultStr),
    L"Capsule%04x",
    CapsuleResultIndex
    );

  Status = gRT->SetVariable(
                  CapsuleResultStr,
                  &gEfiCapsuleReportGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  CapsuleResultSize,
                  CapsuleResult
                  );
  if (!EFI_ERROR(Status)) {
    Size = sizeof(L"Capsule####") - sizeof(CHAR16); // no zero terminator
    DEBUG((DEBUG_INFO, "Set CapsuleLast - %s\n", CapsuleResultStr));
    Status = gRT->SetVariable(
                    L"CapsuleLast",
                    &gEfiCapsuleReportGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    Size,
                    CapsuleResultStr
                    );
  }

  return Status;
}

/**
  Record capsule status variable and to local cache.

  @param[in] CapsuleHeader  The capsule image header
  @param[in] CapsuleStatus  The capsule process stauts

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
RecordCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER                           *CapsuleHeader,
  IN EFI_STATUS                                   CapsuleStatus
  )
{
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  CapsuleResultVariable;
  EFI_STATUS                          Status;

  CapsuleResultVariable.VariableTotalSize = sizeof(CapsuleResultVariable);
  CapsuleResultVariable.Reserved = 0;
  CopyGuid (&CapsuleResultVariable.CapsuleGuid, &CapsuleHeader->CapsuleGuid);
  ZeroMem(&CapsuleResultVariable.CapsuleProcessed, sizeof(CapsuleResultVariable.CapsuleProcessed));
  gRT->GetTime(&CapsuleResultVariable.CapsuleProcessed, NULL);
  CapsuleResultVariable.CapsuleStatus = CapsuleStatus;

  Status = EFI_SUCCESS;
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    Status = WriteNewCapsuleResultVariable(&CapsuleResultVariable, sizeof(CapsuleResultVariable));
  }
  return Status;
}

/**
  Record FMP capsule status variable and to local cache.

  @param[in] CapsuleHeader  The capsule image header
  @param[in] CapsuleStatus  The capsule process stauts
  @param[in] PayloadIndex   FMP payload index
  @param[in] ImageHeader    FMP image header
  @param[in] FmpDevicePath  DevicePath associated with the FMP producer

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
RecordFmpCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader,
  IN EFI_STATUS                                    CapsuleStatus,
  IN UINTN                                         PayloadIndex,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader,
  IN EFI_DEVICE_PATH_PROTOCOL                      *FmpDevicePath OPTIONAL
  )
{
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  *CapsuleResultVariableHeader;
  EFI_CAPSULE_RESULT_VARIABLE_FMP     *CapsuleResultVariableFmp;
  EFI_STATUS                          Status;
  UINT8                               *CapsuleResultVariable;
  UINTN                               CapsuleResultVariableSize;
  CHAR16                              *DevicePathStr;
  UINTN                               DevicePathStrSize;

  DevicePathStr = NULL;
  if (FmpDevicePath != NULL) {
    DevicePathStr = ConvertDevicePathToText (FmpDevicePath, FALSE, FALSE);
  }
  if (DevicePathStr != NULL) {
    DevicePathStrSize = StrSize(DevicePathStr);
  } else {
    DevicePathStrSize = sizeof(CHAR16);
  }
  //
  // Allocate zero CHAR16 for CapsuleFileName.
  //
  CapsuleResultVariableSize = sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER) + sizeof(EFI_CAPSULE_RESULT_VARIABLE_FMP) + sizeof(CHAR16) + DevicePathStrSize;
  CapsuleResultVariable     = AllocateZeroPool (CapsuleResultVariableSize);
  if (CapsuleResultVariable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CapsuleResultVariableHeader = (VOID *)CapsuleResultVariable;
  CapsuleResultVariableHeader->VariableTotalSize = (UINT32)CapsuleResultVariableSize;
  CapsuleResultVariableHeader->Reserved = 0;
  CopyGuid(&CapsuleResultVariableHeader->CapsuleGuid, &CapsuleHeader->CapsuleGuid);
  ZeroMem(&CapsuleResultVariableHeader->CapsuleProcessed, sizeof(CapsuleResultVariableHeader->CapsuleProcessed));
  gRT->GetTime(&CapsuleResultVariableHeader->CapsuleProcessed, NULL);
  CapsuleResultVariableHeader->CapsuleStatus = CapsuleStatus;

  CapsuleResultVariableFmp = (VOID *)(CapsuleResultVariable + sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER));
  CapsuleResultVariableFmp->Version = 0x1;
  CapsuleResultVariableFmp->PayloadIndex = (UINT8)PayloadIndex;
  CapsuleResultVariableFmp->UpdateImageIndex = ImageHeader->UpdateImageIndex;
  CopyGuid (&CapsuleResultVariableFmp->UpdateImageTypeId, &ImageHeader->UpdateImageTypeId);
  if (DevicePathStr != NULL) {
    CopyMem ((UINT8 *)CapsuleResultVariableFmp + sizeof(EFI_CAPSULE_RESULT_VARIABLE_FMP) + sizeof(CHAR16), DevicePathStr, DevicePathStrSize);
    FreePool (DevicePathStr);
    DevicePathStr = NULL;
  }

  Status = EFI_SUCCESS;
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    Status = WriteNewCapsuleResultVariable(CapsuleResultVariable, CapsuleResultVariableSize);
  }
  FreePool (CapsuleResultVariable);
  return Status;
}

/**
  Initialize CapsuleMax variables.
**/
VOID
InitCapsuleMaxVariable (
  VOID
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  CHAR16                           CapsuleMaxStr[sizeof("Capsule####")];
  EDKII_VARIABLE_LOCK_PROTOCOL     *VariableLock;

  UnicodeSPrint(
    CapsuleMaxStr,
    sizeof(CapsuleMaxStr),
    L"Capsule%04x",
    PcdGet16(PcdCapsuleMax)
    );

  Size = sizeof(L"Capsule####") - sizeof(CHAR16); // no zero terminator
  Status = gRT->SetVariable(
                  L"CapsuleMax",
                  &gEfiCapsuleReportGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  Size,
                  CapsuleMaxStr
                  );
  if (!EFI_ERROR(Status)) {
    // Lock it per UEFI spec.
    Status = gBS->LocateProtocol(&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLock);
    if (!EFI_ERROR(Status)) {
      Status = VariableLock->RequestToLock(VariableLock, L"CapsuleMax", &gEfiCapsuleReportGuid);
      ASSERT_EFI_ERROR(Status);
    }
  }
}

/**
  Initialize CapsuleLast variables.
**/
VOID
InitCapsuleLastVariable (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_BOOT_MODE                    BootMode;
  EDKII_VARIABLE_LOCK_PROTOCOL     *VariableLock;
  VOID                             *CapsuleResult;
  UINTN                            Size;
  CHAR16                           CapsuleLastStr[sizeof("Capsule####")];

  BootMode = GetBootModeHob();
  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    Status = gRT->SetVariable(
                    L"CapsuleLast",
                    &gEfiCapsuleReportGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    0,
                    NULL
                    );
    // Do not lock it because it will be updated later.
  } else {
    //
    // Check if OS/APP cleared L"Capsule####"
    //
    ZeroMem(CapsuleLastStr, sizeof(CapsuleLastStr));
    Size = sizeof(L"Capsule####") - sizeof(CHAR16); // no zero terminator
    Status = gRT->GetVariable(
                    L"CapsuleLast",
                    &gEfiCapsuleReportGuid,
                    NULL,
                    &Size,
                    CapsuleLastStr
                    );
    if (!EFI_ERROR(Status)) {
      //
      // L"CapsuleLast" is got, check if data is there.
      //
      Status = GetVariable2 (
                 CapsuleLastStr,
                 &gEfiCapsuleReportGuid,
                 (VOID **) &CapsuleResult,
                 NULL
                 );
      if (EFI_ERROR(Status)) {
        //
        // If no data, delete L"CapsuleLast"
        //
        Status = gRT->SetVariable(
                        L"CapsuleLast",
                        &gEfiCapsuleReportGuid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        0,
                        NULL
                        );
      } else {
        if (CapsuleResult != NULL) {
          FreePool (CapsuleResult);
        }
      }
    }

    // Lock it in normal boot path per UEFI spec.
    Status = gBS->LocateProtocol(&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLock);
    if (!EFI_ERROR(Status)) {
      Status = VariableLock->RequestToLock(VariableLock, L"CapsuleLast", &gEfiCapsuleReportGuid);
      ASSERT_EFI_ERROR(Status);
    }
  }
}

/**
  Initialize capsule update variables.
**/
VOID
InitCapsuleUpdateVariable (
  VOID
  )
{
  EFI_STATUS                     Status;
  UINTN                          Index;
  CHAR16                         CapsuleVarName[30];
  CHAR16                         *TempVarName;

  //
  // Clear all the capsule variables CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...
  // as early as possible which will avoid the next time boot after the capsule update
  // will still into the capsule loop
  //
  StrCpyS (CapsuleVarName, sizeof(CapsuleVarName)/sizeof(CapsuleVarName[0]), EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index = 0;
  while (TRUE) {
    if (Index > 0) {
      UnicodeValueToStringS (
        TempVarName,
        sizeof (CapsuleVarName) - ((UINTN)TempVarName - (UINTN)CapsuleVarName),
        0,
        Index,
        0
        );
    }
    Status = gRT->SetVariable (
                    CapsuleVarName,
                    &gEfiCapsuleVendorGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    (VOID *)NULL
                    );
    if (EFI_ERROR (Status)) {
      //
      // There is no capsule variables, quit
      //
      break;
    }
    Index++;
  }
}

/**
  Initialize capsule related variables.
**/
VOID
InitCapsuleVariable (
  VOID
  )
{
  InitCapsuleUpdateVariable();
  InitCapsuleMaxVariable();
  InitCapsuleLastVariable();
  //
  // No need to clear L"Capsule####", because OS/APP should refer L"CapsuleLast"
  // to check status and delete them.
  //
}
