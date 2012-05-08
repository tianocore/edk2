/** @file
  This module collects performance data for SMM driver boot records and S3 Suspend Performance Record.

  This module registers report status code listener to collect performance data
  for SMM driver boot records and S3 Suspend Performance Record.

  Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>

#include <IndustryStandard/Acpi50.h>

#include <Protocol/SmmReportStatusCodeHandler.h>
#include <Protocol/SmmAccess2.h>

#include <Guid/FirmwarePerformance.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/LockBoxLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SynchronizationLib.h>

#define EXTENSION_RECORD_SIZE     0x1000

EFI_SMM_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol    = NULL;
UINT64                        mSuspendStartTime       = 0;
BOOLEAN                       mS3SuspendLockBoxSaved  = FALSE;
UINT32                        mBootRecordSize = 0;
UINT32                        mBootRecordMaxSize = 0;
UINT8                         *mBootRecordBuffer = NULL;

EFI_SMRAM_DESCRIPTOR          *mSmramRanges;
UINTN                         mSmramRangeCount;
SPIN_LOCK                     mSmmFpdtLock;
BOOLEAN                       mSmramIsOutOfResource = FALSE;

/**
  Report status code listener for SMM. This is used to record the performance
  data for S3 Suspend Start and S3 Suspend End in FPDT.

  @param[in]  CodeType            Indicates the type of status code being reported.
  @param[in]  Value               Describes the current status of a hardware or software entity.
                                  This included information about the class and subclass that is used to
                                  classify the entity as well as an operation.
  @param[in]  Instance            The enumeration of a hardware or software entity within
                                  the system. Valid instance numbers start with 1.
  @param[in]  CallerId            This optional parameter may be used to identify the caller.
                                  This parameter allows the status code driver to apply different rules to
                                  different callers.
  @param[in]  Data                This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS             Status code is what we expected.
  @retval EFI_UNSUPPORTED         Status code not supported.

**/
EFI_STATUS
EFIAPI
FpdtStatusCodeListenerSmm (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data
  )
{
  EFI_STATUS                           Status;
  UINT64                               CurrentTime;
  EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD  S3SuspendRecord;
  UINT8                                *NewRecordBuffer;

  //
  // Check whether status code is what we are interested in.
  //
  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) != EFI_PROGRESS_CODE) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Collect one or more Boot records in boot time
  //
  if (Data != NULL && CompareGuid (&Data->Type, &gEfiFirmwarePerformanceGuid)) {
    AcquireSpinLock (&mSmmFpdtLock);
    
    if (mBootRecordSize + Data->Size > mBootRecordMaxSize) {
      //
      // Try to allocate big SMRAM data to store Boot record. 
      //
      if (mSmramIsOutOfResource) {
        return EFI_OUT_OF_RESOURCES;
      }
      NewRecordBuffer = AllocatePool (mBootRecordSize + Data->Size + EXTENSION_RECORD_SIZE); 
      if (NewRecordBuffer == NULL) {
        mSmramIsOutOfResource = TRUE;
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (NewRecordBuffer, mBootRecordBuffer, mBootRecordSize);
      mBootRecordBuffer  = NewRecordBuffer;
      mBootRecordMaxSize = mBootRecordSize + Data->Size + EXTENSION_RECORD_SIZE;
    }
    //
    // Save boot record into the temp memory space.
    //
    CopyMem (mBootRecordBuffer + mBootRecordSize, Data + 1, Data->Size);
    mBootRecordSize += Data->Size;
    
    ReleaseSpinLock (&mSmmFpdtLock);
    return EFI_SUCCESS;
  }

  if ((Value != PcdGet32 (PcdProgressCodeS3SuspendStart)) &&
      (Value != PcdGet32 (PcdProgressCodeS3SuspendEnd))) {
    return EFI_UNSUPPORTED;
  }

  //
  // Retrieve current time.
  //
  CurrentTime = GetTimeInNanoSecond (GetPerformanceCounter ());

  if (Value == PcdGet32 (PcdProgressCodeS3SuspendStart)) {
    //
    // S3 Suspend started, record the performance data and return.
    //
    mSuspendStartTime = CurrentTime;
    return EFI_SUCCESS;
  }

  //
  // We are going to S3 sleep, record S3 Suspend End performance data.
  //
  S3SuspendRecord.SuspendStart = mSuspendStartTime;
  S3SuspendRecord.SuspendEnd   = CurrentTime;

  //
  // Save S3 suspend performance data to lock box, it will be used by Firmware Performance PEIM.
  //
  if (!mS3SuspendLockBoxSaved) {
    Status = SaveLockBox (
               &gEfiFirmwarePerformanceGuid,
               &S3SuspendRecord,
               sizeof (EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD)
               );
    ASSERT_EFI_ERROR (Status);

    mS3SuspendLockBoxSaved = TRUE;
  } else {
    Status = UpdateLockBox (
               &gEfiFirmwarePerformanceGuid,
               0,
               &S3SuspendRecord,
               sizeof (EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD)
               );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  This function check if the address is in SMRAM.

  @param Buffer  the buffer address to be checked.
  @param Length  the buffer length to be checked.

  @retval TRUE  this address is in SMRAM.
  @retval FALSE this address is NOT in SMRAM.
**/
BOOLEAN
InternalIsAddressInSmram (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  UINTN  Index;

  for (Index = 0; Index < mSmramRangeCount; Index ++) {
    if (((Buffer >= mSmramRanges[Index].CpuStart) && (Buffer < mSmramRanges[Index].CpuStart + mSmramRanges[Index].PhysicalSize)) ||
        ((mSmramRanges[Index].CpuStart >= Buffer) && (mSmramRanges[Index].CpuStart < Buffer + Length))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for report SMM boot records. 

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS            The interrupt was handled and quiesced. No other handlers should still be called.
  @retval EFI_INVALID_PARAMETER  The interrupt parameter is not valid. 
  @retval EFI_ACCESS_DENIED      The interrupt buffer can't be written. 
  @retval EFI_UNSUPPORTED        The interrupt is not supported. 
**/
EFI_STATUS
EFIAPI
FpdtSmiHandler (
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
  )
{
  EFI_STATUS                   Status;
  SMM_BOOT_RECORD_COMMUNICATE  *SmmCommData;
  
  ASSERT (CommBuffer != NULL);
  if (CommBuffer == NULL || *CommBufferSize < sizeof (SMM_BOOT_RECORD_COMMUNICATE)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  SmmCommData = (SMM_BOOT_RECORD_COMMUNICATE*)CommBuffer;

  switch (SmmCommData->Function) {
    case SMM_FPDT_FUNCTION_GET_BOOT_RECORD_SIZE :
       SmmCommData->BootRecordSize = mBootRecordSize;
       break;

    case SMM_FPDT_FUNCTION_GET_BOOT_RECORD_DATA :
       if (SmmCommData->BootRecordData == NULL || SmmCommData->BootRecordSize < mBootRecordSize) {
         Status = EFI_INVALID_PARAMETER;
         break;
       } 
	   
       //
       // Sanity check
       //
       SmmCommData->BootRecordSize = mBootRecordSize;
       if (InternalIsAddressInSmram ((EFI_PHYSICAL_ADDRESS)(UINTN)SmmCommData->BootRecordData, mBootRecordSize)) {
         DEBUG ((EFI_D_ERROR, "Smm Data buffer is in SMRAM!\n"));
         Status = EFI_ACCESS_DENIED;
         break;
       }

       CopyMem (
         (UINT8*)SmmCommData->BootRecordData, 
         mBootRecordBuffer, 
         mBootRecordSize
         );
       break;

    default:
       ASSERT (FALSE);
       Status = EFI_UNSUPPORTED;
  }

  SmmCommData->ReturnStatus = Status;
  return EFI_SUCCESS;
}

/**
  The module Entry Point of the Firmware Performance Data Table SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FirmwarePerformanceSmmEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_SMM_ACCESS2_PROTOCOL  *SmmAccess;
  UINTN                     Size;

  //
  // Initialize spin lock
  //
  InitializeSpinLock (&mSmmFpdtLock); 
   
  //
  // Get SMM Report Status Code Handler Protocol.
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **) &mRscHandlerProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register report status code listener for BootRecords and S3 Suspend Start and End.
  //
  Status = mRscHandlerProtocol->Register (FpdtStatusCodeListenerSmm);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMRAM information
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&SmmAccess);
  ASSERT_EFI_ERROR (Status);

  Size = 0;
  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **)&mSmramRanges
                    );
  ASSERT_EFI_ERROR (Status);

  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, mSmramRanges);
  ASSERT_EFI_ERROR (Status);

  mSmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Register SMI handler.
  //
  Handle = NULL;
  Status = gSmst->SmiHandlerRegister (FpdtSmiHandler, &gEfiFirmwarePerformanceGuid, &Handle);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
