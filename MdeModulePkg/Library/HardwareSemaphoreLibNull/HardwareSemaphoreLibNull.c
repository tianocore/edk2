/** @file
  Null implementation of SemaphoreLib functions

  This library provides a Null implementation of acquire/release functions for
  global/socket semaphores. It should only be used with code that is guaranteed
  to only be executed on the BSP.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/SemaphoreLib.h>

/**
  Acquire a global (BSP) semaphore for the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireGlobalSemaphore (SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsGlobalSemaphoreOwned (SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseGlobalSemaphore (SystemSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)
  @param[in]  AddToQueue        - specifices whether to add the requesting
                                  socket to the queue (TRUE or FALSE)
  @param[out] QueueNumber       - assigned place in line of semaphore request,
                                  if adding to queue

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
AcquireGlobalSemaphore (
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  BOOLEAN                  AddToQueue,
  OUT UINT32                   *QueueNumber      OPTIONAL
  )
{
  return TRUE;
}

/**
  Checks if a global (BSP) semaphore is owned by the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireGlobalSemaphore (SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsGlobalSemaphoreOwned (SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseGlobalSemaphore (SystemSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)
  @param[in]  QueueNumber       - assigned place in line of semaphore request
                                  that was returned by AcquireGlobalSemaphore

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
IsGlobalSemaphoreOwned (
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  UINT32                   QueueNumber
  )
{
  return TRUE;
}

/**
  Release a global (BSP) semaphore owned by the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireGlobalSemaphore (SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsGlobalSemaphoreOwned (SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseGlobalSemaphore (SystemSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)

  @retval EFI_SUCCESS           - successfully released semaphore
  @retval EFI_DEVICE_ERROR      - error releasing semaphore
**/
EFI_STATUS
EFIAPI
ReleaseGlobalSemaphore (
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber
  )
{
  return EFI_SUCCESS;
}

/**
  Acquire a socket semaphore for the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireSocketSemaphore (Socket, SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsSocketSemaphoreOwned (Socket, SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseSocketSemaphore (Socket, SystemSemaphore0);
      ......

  @param[in]  Socket            - Socket where the semaphore is located
  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)
  @param[in]  AddToQueue        - specifices whether to add the requesting
                                  socket to the queue (TRUE or FALSE)
  @param[out] QueueNumber       - assigned place in line of semaphore request,
                                  if adding to queue

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
AcquireSocketSemaphore (
  IN  UINT8                    Socket,
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  BOOLEAN                  AddToQueue,
  OUT UINT32                   *QueueNumber      OPTIONAL
  )
{
  return TRUE;
}

/**
  Checks if a socket semaphore is owned by the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireSocketSemaphore (Socket, SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsSocketSemaphoreOwned (Socket, SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseSocketSemaphore (Socket, SystemSemaphore0);
      ......

  @param[in]  Socket            - Socket where the semaphore is located
  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)
  @param[in]  QueueNumber       - assigned place in line of semaphore request
                                  that was returned by AcquireSocketSemaphore

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
IsSocketSemaphoreOwned (
  IN  UINT8                    Socket,
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  UINT32                   QueueNumber
  )
{
  return TRUE;
}

/**
  Release a socket semaphore owned by the calling socket.

  Used for ensuring exclusive access to resources among CPU sockets.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireSocketSemaphore (Socket, SystemSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsSocketSemaphoreOwned (Socket, SystemSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseSocketSemaphore (Socket, SystemSemaphore0);
      ......

  @param[in]  Socket            - Socket to release semaphore
  @param[in]  SemaphoreNumber   - SYSTEMSEMAPHORE register number (0 or 1)

  @retval EFI_SUCCESS           - successfully released semaphore
  @retval EFI_DEVICE_ERROR      - error releasing semaphore
**/
EFI_STATUS
EFIAPI
ReleaseSocketSemaphore (
  IN  UINT8                    Socket,
  IN  SYSTEM_SEMAPHORE_NUMBER  SemaphoreNumber
  )
{
  return EFI_SUCCESS;
}

/**
  Acquire a local semaphore for the calling thread.

  Used for ensuring exclusive access to resources among CPU threads.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireLocalSemaphore (LocalSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsLocalSemaphoreOwned (LocalSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseLocalSemaphore (LocalSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - LOCALSEMAPHORE register number (0 or 1)
  @param[in]  AddToQueue        - specifices whether to add the requesting
                                  thread to the queue (TRUE or FALSE)
  @param[out] QueueNumber       - assigned place in line of semaphore request,
                                  if adding to queue

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
AcquireLocalSemaphore (
  IN  LOCAL_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  BOOLEAN                 AddToQueue,
  OUT UINT32                  *QueueNumber      OPTIONAL
  )
{
  return TRUE;
}

/**
  Checks if a local semaphore is owned by the calling thread.

  Used for ensuring exclusive access to resources among CPU threads.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireLocalSemaphore (LocalSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsLocalSemaphoreOwned (LocalSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseLocalSemaphore (LocalSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - LOCALSEMAPHORE register number (0 or 1)
  @param[in]  QueueNumber       - assigned place in line of semaphore request
                                  that was returned by AcquireLocalSemaphore

  @retval TRUE                  - successfully acquired semaphore
  @retval FALSE                 - semaphore not acquired
**/
BOOLEAN
EFIAPI
IsLocalSemaphoreOwned (
  IN  LOCAL_SEMAPHORE_NUMBER  SemaphoreNumber,
  IN  UINT32                  QueueNumber
  )
{
  return TRUE;
}

/**
  Release a local semaphore owned by the calling thread.

  Used for ensuring exclusive access to resources among CPU threads.

  IMPORTANT:
    The functions must be called in the sequence below:
      ......
      Owned = AcquireLocalSemaphore (LocalSemaphore0, ADD_TO_QUEUE, &QNum);
      while (!Owned) {
        Owned = IsLocalSemaphoreOwned (LocalSemaphore0, QNum));
      }
      DoSomething ();
      ReleaseLocalSemaphore (LocalSemaphore0);
      ......

  @param[in]  SemaphoreNumber   - LOCALSEMAPHORE register number (0 or 1)

  @retval EFI_SUCCESS           - successfully released semaphore
  @retval EFI_INVALID_PARAMETER - semaphore number is out of range
  @retval EFI_DEVICE_ERROR      - error releasing semaphore
**/
EFI_STATUS
EFIAPI
ReleaseLocalSemaphore (
  IN  LOCAL_SEMAPHORE_NUMBER  SemaphoreNumber
  )
{
  return EFI_SUCCESS;
}

/**
  Check if any semaphore is owned.

  This function should only be called at a time when no semaphores are expected
  to be owned. If any are owned, it prints a warning message.
**/
VOID
EFIAPI
CheckAnySemaphoreOwned (
  VOID
  )
{
}

/**
  Checks if a GlobalSemaphore is owned by some socket.

  @param[in]  SocketOwned       - socket number
                                  if semaphore is owned by the socket

  @retval TRUE                  - semaphore is owned by the socket
  @retval FALSE                 - semaphore is not owned by the socket
**/
BOOLEAN
EFIAPI
CheckGlobalSemaphoreOwned (
  IN      UINT8  Socket
  )
{
  return TRUE;
}

/**
  Get Bios semaphore Registers' flat address

  @param SocId            - CPU Socket Node number (Socket ID)
  @param RegIndex         - Release semaphore register index
  @param RegAddress       - Register address

  @retval                 - EFI_INVALID_PARAMETER: input Pad index doesn't exist for this SOC
                            EFI_SUCCESS:     the function is excuted successfully

**/
EFI_STATUS
EFIAPI
GetReleaseSemaphoreAddr (
  IN  UINT8  Socket,
  IN  UINT8  RegIndex,
  OUT UINTN  *RegAddress
  )
{
  //
  // Set RegAddress to an invalid data.
  //
  *RegAddress = 0x0;
  return EFI_SUCCESS;
}
