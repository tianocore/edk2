/** @file
  Host OS Threading Library
 
  @copyright
  INTEL CONFIDENTIAL
  Copyright 2017 Intel Corporation. <BR>
  
  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.
  
  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.
  
  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/

#include <Uefi.h>

/**
  Get the Host OS thread ID.

  This currently always returns zero on Linux. Will need to 
  be updated if we need multithreading support on Linux.

  @param[out]   Ptr to be updated with the thread id.

  @retval EFI_SUCCESS           Thread ID retrieved.
  @retval EFI_INVALID_PARAMETER ThreadID was null on entry.

**/

EFI_STATUS
EFIAPI
HostOsGetCurrentThreadId (
  OUT UINT32 *ThreadId
  )
{
  //
  // Check parameters for sanity.
  //

  if (ThreadId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ThreadId = 0;

  return EFI_SUCCESS;

} // HostOsGetCurrentThreadId

/**
  Create a mutex.

  This function abstracts creating a mutex from the underlying
  Host OS for the simulation. Not functional on Linux.

  @param[in]  MutexName - Ptr to the name of the mutex.
  @param[out] Mutex     - Address of a ptr to the mutex.

  @retval EFI_SUCCESS   - Mutex created successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsCreateMutex (
  IN CHAR8 *MutexName,
  OUT VOID **Mutex
  )
{

  return EFI_SUCCESS;

} // HostOsCreateMutex


/**
  Wait for a mutex to become available.

  This function abstracts waiting on a mutex from the underlying
  Host OS for the simulation. Not functional on Linux yet.

  @param[in] Mutex     - Ptr to the mutex.

  @retval EFI_SUCCESS   - Wait completed successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsWaitForMutex (
  OUT VOID *Mutex
  )
{

  return EFI_SUCCESS;

} // HostOsWaitForMutex

/**
  Release a mutex.

  This function abstracts releasing a mutex from the underlying
  Host OS for the simulation. Not functional on Linux yet.

  @param[in] Mutex     - Ptr to the mutex.

  @retval EFI_SUCCESS   - Wait completed successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsReleaseMutex (
  IN VOID *Mutex
  )
{

  return EFI_SUCCESS;

} // HostOsReleaseMutex

/**
  Open a mutex.

  This function abstracts opening a mutex from the underlying
  Host OS for the simulation. Not functional on Linux yet.

  @param[in]  MutexName - Ptr to the name of the mutex.
  @param[out] Mutex     - Address of a ptr to the mutex.

  @retval EFI_SUCCESS   - Wait completed successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsOpenMutex (
  IN CHAR8 *MutexName,
  OUT VOID **Mutex
  )
{

  return EFI_SUCCESS;

} // HostOsOpenMutex

/**
  Initialize a critical section.

  This function initializes a critical section handler.

  @retval EFI_SUCCESS   - Initialized successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsInitCriticalSection (
  VOID
  )
{

  return EFI_SUCCESS;

} // HostOsInitCriticalSection

/**
  Enter a critical section.

  This function enters a critical section. Other threads
  will not execute the same function(s) until HostOsLeaveCriticalSection
  is called. Used to syncronize access to shared data resources.

  @retval EFI_SUCCESS   - Entered successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsEnterCriticalSection (
  VOID
  )
{

  return EFI_SUCCESS;

} // HostOsEnterCriticalSection

/**
  Leave a critical section.

  This function leaves a critical section. Other threads
  are free to execute.

  @retval EFI_SUCCESS   - Left successfully
  @retval !EFI_SUCCESS  - Failure

**/

EFI_STATUS
EFIAPI
HostOsLeaveCriticalSection (
  VOID
  )
{

  return EFI_SUCCESS;

} // HostOsLeaveCriticalSection
