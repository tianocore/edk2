/** @file
  Library that provides SMM CPU Sync related operations.

  The lib provides 3 sets of APIs:
  1. ContextInit/ContextDeinit/ContextReset:

    ContextInit() is called in driver's entrypoint to allocate and initialize the SMM CPU Sync context.
    ContextDeinit() is called in driver's unload function to deinitialize the SMM CPU Sync context.
    ContextReset() is called by one of CPUs after all CPUs are ready to exit SMI, which allows CPU to
    check into the next SMI from this point.

  2. GetArrivedCpuCount/CheckInCpu/CheckOutCpu/LockDoor:
    When SMI happens, all processors including BSP enter to SMM mode by calling CheckInCpu().
    CheckOutCpu() can be called in error handling flow for the CPU who calls CheckInCpu() earlier.
    The elected BSP calls LockDoor() so that CheckInCpu() and CheckOutCpu() will return the error code after that.
    GetArrivedCpuCount() returns the number of checked-in CPUs.

  3. WaitForAPs/ReleaseOneAp/WaitForBsp/ReleaseBsp
    WaitForAPs() & ReleaseOneAp() are called from BSP to wait the number of APs and release one specific AP.
    WaitForBsp() & ReleaseBsp() are called from APs to wait and release BSP.
    The 4 APIs are used to synchronize the running flow among BSP and APs.
    BSP and AP Sync flow can be easy understand as below:
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
typedef struct SMM_CPU_SYNC_CONTEXT SMM_CPU_SYNC_CONTEXT;

/**
  Create and initialize the SMM CPU Sync context. It is to allocate and initialize the
  SMM CPU Sync context.

  If Context is NULL, then ASSERT().

  @param[in]  NumberOfCpus          The number of Logical Processors in the system.
  @param[out] Context               Pointer to the new created and initialized SMM CPU Sync context object.
                                    NULL will be returned if any error happen during init.

  @retval RETURN_SUCCESS            The SMM CPU Sync context was successful created and initialized.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough resources available to create and initialize SMM CPU Sync context.
  @retval RETURN_BUFFER_TOO_SMALL   Overflow happen

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncContextInit (
  IN   UINTN                 NumberOfCpus,
  OUT  SMM_CPU_SYNC_CONTEXT  **Context
  );

/**
  Deinit an allocated SMM CPU Sync context. The resources allocated in SmmCpuSyncContextInit() will
  be freed.

  If Context is NULL, then ASSERT().

  @param[in,out]  Context     Pointer to the SMM CPU Sync context object to be deinitialized.

**/
VOID
EFIAPI
SmmCpuSyncContextDeinit (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context
  );

/**
  Reset SMM CPU Sync context. SMM CPU Sync context will be reset to the initialized state.

  This function is called by one of CPUs after all CPUs are ready to exit SMI, which allows CPU to
  check into the next SMI from this point.

  If Context is NULL, then ASSERT().

  @param[in,out]  Context     Pointer to the SMM CPU Sync context object to be reset.

**/
VOID
EFIAPI
SmmCpuSyncContextReset (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context
  );

/**
  Get current number of arrived CPU in SMI.

  BSP might need to know the current number of arrived CPU in SMI to make sure all APs
  in SMI. This API can be for that purpose.

  If Context is NULL, then ASSERT().

  @param[in]      Context     Pointer to the SMM CPU Sync context object.

  @retval    Current number of arrived CPU in SMI.

**/
UINTN
EFIAPI
SmmCpuSyncGetArrivedCpuCount (
  IN  SMM_CPU_SYNC_CONTEXT  *Context
  );

/**
  Performs an atomic operation to check in CPU.

  When SMI happens, all processors including BSP enter to SMM mode by calling SmmCpuSyncCheckInCpu().

  If Context is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check in CPU index.

  @retval RETURN_SUCCESS            Check in CPU (CpuIndex) successfully.
  @retval RETURN_ABORTED            Check in CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckInCpu (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex
  );

/**
  Performs an atomic operation to check out CPU.

  This function can be called in error handling flow for the CPU who calls CheckInCpu() earlier.
  The caller shall make sure the CPU specified by CpuIndex has already checked-in.

  If Context is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check out CPU index.

  @retval RETURN_SUCCESS            Check out CPU (CpuIndex) successfully.
  @retval RETURN_ABORTED            Check out CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckOutCpu (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex
  );

/**
  Performs an atomic operation lock door for CPU checkin and checkout. After this function:
  CPU can not check in via SmmCpuSyncCheckInCpu().
  CPU can not check out via SmmCpuSyncCheckOutCpu().

  The CPU specified by CpuIndex is elected to lock door. The caller shall make sure the CpuIndex
  is the actual CPU calling this function to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If CpuCount is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which CPU to lock door.
  @param[out]     CpuCount          Number of arrived CPU in SMI after look door.

**/
VOID
EFIAPI
SmmCpuSyncLockDoor (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  OUT UINTN                    *CpuCount
  );

/**
  Used by the BSP to wait for APs.

  The number of APs need to be waited is specified by NumberOfAPs. The BSP is specified by BspIndex.
  The caller shall make sure the BspIndex is the actual CPU calling this function to avoid the undefined behavior.
  The caller shall make sure the NumberOfAPs have already checked-in to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If NumberOfAPs >= All CPUs in system, then ASSERT().
  If BspIndex exceeds the range of all CPUs in the system, then ASSERT().

  Note:
  This function is blocking mode, and it will return only after the number of APs released by
  calling SmmCpuSyncReleaseBsp():
  BSP: WaitForAPs    <--  AP: ReleaseBsp

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      NumberOfAPs       Number of APs need to be waited by BSP.
  @param[in]      BspIndex          The BSP Index to wait for APs.

**/
VOID
EFIAPI
SmmCpuSyncWaitForAPs (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 NumberOfAPs,
  IN     UINTN                 BspIndex
  );

/**
  Used by the BSP to release one AP.

  The AP is specified by CpuIndex. The BSP is specified by BspIndex.
  The caller shall make sure the BspIndex is the actual CPU calling this function to avoid the undefined behavior.
  The caller shall make sure the CpuIndex has already checked-in to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP need to be released.
  @param[in]      BspIndex          The BSP Index to release AP.

**/
VOID
EFIAPI
SmmCpuSyncReleaseOneAp   (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  );

/**
  Used by the AP to wait BSP.

  The AP is specified by CpuIndex.
  The caller shall make sure the CpuIndex is the actual CPU calling this function to avoid the undefined behavior.
  The BSP is specified by BspIndex.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  Note:
  This function is blocking mode, and it will return only after the AP released by
  calling SmmCpuSyncReleaseOneAp():
  BSP: ReleaseOneAp  -->  AP: WaitForBsp

  @param[in,out]  Context          Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex         Indicate which AP wait BSP.
  @param[in]      BspIndex         The BSP Index to be waited.

**/
VOID
EFIAPI
SmmCpuSyncWaitForBsp (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  );

/**
  Used by the AP to release BSP.

  The AP is specified by CpuIndex.
  The caller shall make sure the CpuIndex is the actual CPU calling this function to avoid the undefined behavior.
  The BSP is specified by BspIndex.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP release BSP.
  @param[in]      BspIndex          The BSP Index to be released.

**/
VOID
EFIAPI
SmmCpuSyncReleaseBsp (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  );

#endif
