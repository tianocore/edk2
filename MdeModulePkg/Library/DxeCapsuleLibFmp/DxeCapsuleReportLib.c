/** @file
  DXE capsule report related function.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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
#include <Library/CapsuleLib.h>

#include <IndustryStandard/WindowsUxCapsule.h>

typedef struct {
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  CapsuleResultHeader;
  EFI_CAPSULE_RESULT_VARIABLE_FMP     CapsuleResultFmp;
} CAPSULE_RESULT_VARIABLE_CACHE;

#define CAPSULE_RESULT_VARIABLE_CACHE_COUNT   0x10

CAPSULE_RESULT_VARIABLE_CACHE *mCapsuleResultVariableCache;
UINTN                         mCapsuleResultVariableCacheMaxCount;
UINTN                         mCapsuleResultVariableCacheCount;

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
  Check if this FMP capsule is processed.

  @param[in] CapsuleHeader  The capsule image header
  @param[in] PayloadIndex   FMP payload index
  @param[in] ImageHeader    FMP image header

  @retval TRUE  This FMP capsule is processed.
  @retval FALSE This FMP capsule is not processed.
**/
BOOLEAN
IsFmpCapsuleProcessed (
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader,
  IN UINTN                                         PayloadIndex,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader
  )
{
  UINTN                               Index;
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  *CapsuleResult;
  EFI_CAPSULE_RESULT_VARIABLE_FMP     *CapsuleResultFmp;

  for (Index = 0; Index < mCapsuleResultVariableCacheCount; Index++) {
    //
    // Check
    //
    CapsuleResult = &mCapsuleResultVariableCache[Index].CapsuleResultHeader;
    if (CapsuleResult->VariableTotalSize >= sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER)) {
      if (CompareGuid(&CapsuleResult->CapsuleGuid, &gEfiFmpCapsuleGuid)) {
        if (CapsuleResult->VariableTotalSize >= sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER) + sizeof(EFI_CAPSULE_RESULT_VARIABLE_FMP)) {
          CapsuleResultFmp = (EFI_CAPSULE_RESULT_VARIABLE_FMP *)(CapsuleResult + 1);
          if (CompareGuid(&CapsuleResultFmp->UpdateImageTypeId, &ImageHeader->UpdateImageTypeId) &&
              (CapsuleResultFmp->UpdateImageIndex == ImageHeader->UpdateImageIndex) &&
              (CapsuleResultFmp->PayloadIndex == PayloadIndex) ) {
            return TRUE;
          }
        }
      }
    }
  }

  return FALSE;
}

/**
  Write a new capsule status variable cache.

  @param[in] CapsuleResult      The capsule status variable
  @param[in] CapsuleResultSize  The size of the capsule stauts variable in bytes

  @retval EFI_SUCCESS          The capsule status variable is cached.
  @retval EFI_OUT_OF_RESOURCES No resource to cache the capsule status variable.
**/
EFI_STATUS
WriteNewCapsuleResultVariableCache (
  IN VOID    *CapsuleResult,
  IN UINTN   CapsuleResultSize
  )
{
  if (CapsuleResultSize > sizeof(CAPSULE_RESULT_VARIABLE_CACHE)) {
    CapsuleResultSize = sizeof(CAPSULE_RESULT_VARIABLE_CACHE);
  }

  if (mCapsuleResultVariableCacheCount == mCapsuleResultVariableCacheMaxCount) {
    mCapsuleResultVariableCache = ReallocatePool(
                                    mCapsuleResultVariableCacheMaxCount * sizeof(CAPSULE_RESULT_VARIABLE_CACHE),
                                    (mCapsuleResultVariableCacheMaxCount + CAPSULE_RESULT_VARIABLE_CACHE_COUNT) * sizeof(CAPSULE_RESULT_VARIABLE_CACHE),
                                    mCapsuleResultVariableCache
                                    );
    if (mCapsuleResultVariableCache == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    mCapsuleResultVariableCacheMaxCount += CAPSULE_RESULT_VARIABLE_CACHE_COUNT;
  }

  ASSERT(mCapsuleResultVariableCacheCount < mCapsuleResultVariableCacheMaxCount);
  ASSERT(mCapsuleResultVariableCache != NULL);
  CopyMem(
    &mCapsuleResultVariableCache[mCapsuleResultVariableCacheCount],
    CapsuleResult,
    CapsuleResultSize
    );
  mCapsuleResultVariableCacheCount++;

  return EFI_SUCCESS;
}

/**
  Get a new capsule status variable index.

  @return A new capsule status variable index.
  @retval -1  No new capsule status variable index.
**/
INTN
GetNewCapsuleResultIndex (
  VOID
  )
{
  INTN                             CurrentIndex;

  CurrentIndex = GetCurrentCapsuleLastIndex();
  if (CurrentIndex >= PcdGet16(PcdCapsuleMax)) {
    return -1;
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
  if (CapsuleResultIndex == -1) {
    return EFI_OUT_OF_RESOURCES;
  }
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
  CopyGuid (&CapsuleResultVariable.CapsuleGuid, &CapsuleHeader->CapsuleGuid);
  ZeroMem(&CapsuleResultVariable.CapsuleProcessed, sizeof(CapsuleResultVariable.CapsuleProcessed));
  gRT->GetTime(&CapsuleResultVariable.CapsuleProcessed, NULL);
  CapsuleResultVariable.CapsuleStatus = CapsuleStatus;

  //
  // Save Local Cache
  //
  Status = WriteNewCapsuleResultVariableCache(&CapsuleResultVariable, sizeof(CapsuleResultVariable));

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

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
RecordFmpCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader,
  IN EFI_STATUS                                    CapsuleStatus,
  IN UINTN                                         PayloadIndex,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader
  )
{
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  *CapsuleResultVariableHeader;
  EFI_CAPSULE_RESULT_VARIABLE_FMP     *CapsuleResultVariableFmp;
  EFI_STATUS                          Status;
  UINT8                               *CapsuleResultVariable;
  UINT32                              CapsuleResultVariableSize;

  CapsuleResultVariable     = NULL;
  CapsuleResultVariableSize = sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER) + sizeof(EFI_CAPSULE_RESULT_VARIABLE_FMP);
  CapsuleResultVariable     = AllocatePool (CapsuleResultVariableSize);
  if (CapsuleResultVariable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CapsuleResultVariableHeader = (VOID *)CapsuleResultVariable;
  CapsuleResultVariableHeader->VariableTotalSize = CapsuleResultVariableSize;
  CopyGuid(&CapsuleResultVariableHeader->CapsuleGuid, &CapsuleHeader->CapsuleGuid);
  ZeroMem(&CapsuleResultVariableHeader->CapsuleProcessed, sizeof(CapsuleResultVariableHeader->CapsuleProcessed));
  gRT->GetTime(&CapsuleResultVariableHeader->CapsuleProcessed, NULL);
  CapsuleResultVariableHeader->CapsuleStatus = CapsuleStatus;

  CapsuleResultVariableFmp = (VOID *)(CapsuleResultVariable + sizeof(EFI_CAPSULE_RESULT_VARIABLE_HEADER));
  CapsuleResultVariableFmp->Version = 0x1;
  CapsuleResultVariableFmp->PayloadIndex = (UINT8)PayloadIndex;
  CapsuleResultVariableFmp->UpdateImageIndex = ImageHeader->UpdateImageIndex;
  CopyGuid (&CapsuleResultVariableFmp->UpdateImageTypeId, &ImageHeader->UpdateImageTypeId);

  //
  // Save Local Cache
  //
  Status = WriteNewCapsuleResultVariableCache(CapsuleResultVariable, CapsuleResultVariableSize);

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
      UnicodeValueToString (TempVarName, 0, Index, 0);
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
