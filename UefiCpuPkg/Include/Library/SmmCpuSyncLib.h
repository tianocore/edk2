/** @file
Library that provides SMM CPU Sync related operations.
The lib provides 3 sets of APIs:
1. ContextInit/ContextDeinit/ContextReset:
ContextInit() is called in driver's entrypoint to allocate and initialize the SMM CPU Sync context.
ContextDeinit() is called in driver's unload function to deinitialize the SMM CPU Sync context.
ContextReset() is called before CPU exist SMI, which allows CPU to check into the next SMI from this point.

2. GetArrivedCpuCount/CheckInCpu/CheckOutCpu/LockDoor:
When SMI happens, all processors including BSP enter to SMM mode by calling CheckInCpu().
The elected BSP calls LockDoor() so that CheckInCpu() will return the error code after that.
CheckOutCpu() can be called in error handling flow for the CPU who calls CheckInCpu() earlier.
GetArrivedCpuCount() returns the number of checked-in CPUs.

3. WaitForAPs/ReleaseOneAp/WaitForBsp/ReleaseBsp
WaitForAPs() & ReleaseOneAp() are called from BSP to wait the number of APs and release one specific AP.
WaitForBsp() & ReleaseBsp() are called from APs to wait and release BSP.
The 4 APIs are used to synchronize the running flow among BSP and APs. BSP and AP Sync flow can be
easy understand as below:
BSP: ReleaseOneAp  -->  AP: WaitForBsp
BSP: WaitForAPs    <--  AP: ReleaseBsp

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CPU_SYNC_LIB_H_
#define SMM_CPU_SYNC_LIB_H_

#include <Uefi/UefiBaseType.h>

//
// Opaque structure for SMM CPU Sync context.
//
typedef struct SMM_CPU_SYNC_CTX SMM_CPU_SYNC_CTX;

/**
  Create and initialize the SMM CPU Sync context.

  SmmCpuSyncContextInit() function is to allocate and initialize the SMM CPU Sync context.

  @param[in]  NumberOfCpus          The number of Logical Processors in the system.
  @param[out] SmmCpuSyncCtx         Pointer to the new created and initialized SMM CPU Sync context object.
                                    NULL will be returned if any error happen during init.

  @retval RETURN_SUCCESS            The SMM CPU Sync context was successful created and initialized.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL.
  @retval RETURN_BUFFER_TOO_SMALL   Overflow happen
  @retval RETURN_OUT_OF_RESOURCES   There are not enough resources available to create and initialize SMM CPU Sync context.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncContextInit (
  IN   UINTN             NumberOfCpus,
  OUT  SMM_CPU_SYNC_CTX  **SmmCpuSyncCtx
  );

/**
  Deinit an allocated SMM CPU Sync context.

  SmmCpuSyncContextDeinit() function is to deinitialize SMM CPU Sync context, the resources allocated in
  SmmCpuSyncContextInit() will be freed.

  Note: This function only can be called after SmmCpuSyncContextInit() return success.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object to be deinitialized.

  @retval RETURN_SUCCESS            The SMM CPU Sync context was successful deinitialized.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncContextDeinit (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx
  );

/**
  Reset SMM CPU Sync context.

  SmmCpuSyncContextReset() function is to reset SMM CPU Sync context to the initialized state.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object to be reset.

  @retval RETURN_SUCCESS            The SMM CPU Sync context was successful reset.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncContextReset (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx
  );

/**
  Get current number of arrived CPU in SMI.

  For traditional CPU synchronization method, BSP might need to know the current number of arrived CPU in
  SMI to make sure all APs in SMI. This API can be for that purpose.

  @param[in]      SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in,out]  CpuCount          Current count of arrived CPU in SMI.

  @retval RETURN_SUCCESS            Get current number of arrived CPU in SMI successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx or CpuCount is NULL.
  @retval RETURN_ABORTED            Function Aborted due to the door has been locked by SmmCpuSyncLockDoor() function.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncGetArrivedCpuCount (
  IN     SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN OUT UINTN             *CpuCount
  );

/**
  Performs an atomic operation to check in CPU.

  When SMI happens, all processors including BSP enter to SMM mode by calling SmmCpuSyncCheckInCpu().

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check in CPU index.

  @retval RETURN_SUCCESS            Check in CPU (CpuIndex) successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL.
  @retval RETURN_ABORTED            Check in CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckInCpu (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex
  );

/**
  Performs an atomic operation to check out CPU.

  CheckOutCpu() can be called in error handling flow for the CPU who calls CheckInCpu() earlier.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check out CPU index.

  @retval RETURN_SUCCESS            Check out CPU (CpuIndex) successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL.
  @retval RETURN_NOT_READY          The CPU is not checked-in.
  @retval RETURN_ABORTED            Check out CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckOutCpu (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex
  );

/**
  Performs an atomic operation lock door for CPU checkin or checkout.

  After this function:
  CPU can not check in via SmmCpuSyncCheckInCpu().
  CPU can not check out via SmmCpuSyncCheckOutCpu().
  CPU can not get number of arrived CPU in SMI via SmmCpuSyncGetArrivedCpuCount(). The number of
  arrived CPU in SMI will be returned in CpuCount.

  The CPU specified by CpuIndex is elected to lock door.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which CPU to lock door.
  @param[in,out]  CpuCount          Number of arrived CPU in SMI after look door.

  @retval RETURN_SUCCESS            Lock door for CPU successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx or CpuCount is NULL.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncLockDoor (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex,
  IN OUT UINTN             *CpuCount
  );

/**
  Used by the BSP to wait for APs.

  The number of APs need to be waited is specified by NumberOfAPs. The BSP is specified by BspIndex.

  Note: This function is blocking mode, and it will return only after the number of APs released by
  calling SmmCpuSyncReleaseBsp():
  BSP: WaitForAPs    <--  AP: ReleaseBsp

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      NumberOfAPs       Number of APs need to be waited by BSP.
  @param[in]      BspIndex          The BSP Index to wait for APs.

  @retval RETURN_SUCCESS            BSP to wait for APs successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL or NumberOfAPs > total number of processors in system.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncWaitForAPs (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             NumberOfAPs,
  IN     UINTN             BspIndex
  );

/**
  Used by the BSP to release one AP.

  The AP is specified by CpuIndex. The BSP is specified by BspIndex.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP need to be released.
  @param[in]      BspIndex          The BSP Index to release AP.

  @retval RETURN_SUCCESS            BSP to release one AP successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL or CpuIndex is same as BspIndex.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncReleaseOneAp   (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex,
  IN     UINTN             BspIndex
  );

/**
  Used by the AP to wait BSP.

  The AP is specified by CpuIndex. The BSP is specified by BspIndex.

  Note: This function is blocking mode, and it will return only after the AP released by
  calling SmmCpuSyncReleaseOneAp():
  BSP: ReleaseOneAp  -->  AP: WaitForBsp

  @param[in,out]  SmmCpuSyncCtx    Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex         Indicate which AP wait BSP.
  @param[in]      BspIndex         The BSP Index to be waited.

  @retval RETURN_SUCCESS            AP to wait BSP successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL or CpuIndex is same as BspIndex.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncWaitForBsp (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex,
  IN     UINTN             BspIndex
  );

/**
  Used by the AP to release BSP.

  The AP is specified by CpuIndex. The BSP is specified by BspIndex.

  @param[in,out]  SmmCpuSyncCtx     Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP release BSP.
  @param[in]      BspIndex          The BSP Index to be released.

  @retval RETURN_SUCCESS            AP to release BSP successfully.
  @retval RETURN_INVALID_PARAMETER  SmmCpuSyncCtx is NULL or CpuIndex is same as BspIndex.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncReleaseBsp (
  IN OUT SMM_CPU_SYNC_CTX  *SmmCpuSyncCtx,
  IN     UINTN             CpuIndex,
  IN     UINTN             BspIndex
  );

#endif
