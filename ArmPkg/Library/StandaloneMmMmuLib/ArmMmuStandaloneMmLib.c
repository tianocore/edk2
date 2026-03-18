/** @file
  File managing the MMU for ARMv8 architecture in S-EL0

  Copyright (c) 2017 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2021, Linaro Limited
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] Arm Firmware Framework for Armv8-A, DEN0077, version 1.2
        (https://developer.arm.com/documentation/den0077/latest/)
  - [2] FF-A Memory Management Protocol, DEN0140, version 1.2
        (https://developer.arm.com/documentation/den0140/latest/)
**/

#include <Uefi.h>
#include <IndustryStandard/ArmFfaSvc.h>

#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

/**
  Utility function to determine whether ABIs in FF-A to set and get
  memory permissions can be used. Ideally, this should be invoked once in the
  library constructor and set a flag that can be used at runtime. However, the
  StMM Core invokes this library before constructors are called and before the
  StMM image itself is relocated.

  @retval TRUE            Use FF-A MemPerm ABIs.
  @retval FALSE           Use MM MemPerm ABIs.

**/
STATIC
BOOLEAN
EFIAPI
IsFfaMemoryAbiSupported (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINT16      CurrentMajorVersion;
  UINT16      CurrentMinorVersion;

  Status = ArmFfaLibGetVersion (
             ARM_FFA_MAJOR_VERSION,
             ARM_FFA_MINOR_VERSION,
             &CurrentMajorVersion,
             &CurrentMinorVersion
             );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/** Send a request the target to get/set the memory permission.

  @param [in, out]  SvcArgs     Pointer to SVC arguments to send. On
                                return it contains the response parameters.
  @param [out]      RetVal      Pointer to return the response value.

  @retval EFI_SUCCESS           Request successfull.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_READY         Callee is busy or not in a state to handle
                                this request.
  @retval EFI_UNSUPPORTED       This function is not implemented by the
                                callee.
  @retval EFI_ABORTED           Message target ran into an unexpected error
                                and has aborted.
  @retval EFI_ACCESS_DENIED     Access denied.
  @retval EFI_OUT_OF_RESOURCES  Out of memory to perform operation.
**/
STATIC
EFI_STATUS
SendMemoryPermissionRequest (
  IN OUT  ARM_SVC_ARGS  *SvcArgs,
  OUT     INT32         *RetVal
  )
{
  if ((SvcArgs == NULL) || (RetVal == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ArmCallSvc (SvcArgs);

  if (IS_FID_FFA_ERROR (SvcArgs->Arg0)) {
    return FfaStatusToEfiStatus (SvcArgs->Arg2);
  }

  *RetVal = SvcArgs->Arg2;

  return EFI_SUCCESS;
}

/** Request the permission attributes of a memory region from S-EL0.

  @param [in]   BaseAddress          Base address for the memory region.
  @param [out]  MemoryAttributes     Pointer to return the memory attributes.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.
**/
STATIC
EFI_STATUS
GetMemoryPermissions (
  IN     EFI_PHYSICAL_ADDRESS  BaseAddress,
  OUT    UINT32                *MemoryAttributes
  )
{
  EFI_STATUS    Status;
  INT32         Ret;
  ARM_SVC_ARGS  SvcArgs;

  if (MemoryAttributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));

  // Prepare the message parameters.
  // See [3], Section 2.8 FFA_MEM_PERM_GET
  SvcArgs.Arg0 = ARM_FID_FFA_MEM_PERM_GET;
  SvcArgs.Arg1 = BaseAddress;

  Status = SendMemoryPermissionRequest (&SvcArgs, &Ret);
  if (EFI_ERROR (Status)) {
    *MemoryAttributes = 0;
  } else {
    *MemoryAttributes = Ret;
  }

  return Status;
}

/** Request the permission attributes of the S-EL0 memory region to be updated.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.
  @param [in]  Permissions     Memory access controls attributes.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.
**/
STATIC
EFI_STATUS
RequestMemoryPermissionChange (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT32                Permissions
  )
{
  INT32         Ret;
  ARM_SVC_ARGS  SvcArgs;

  ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));

  // Prepare the message parameters.
  // See [3], Section 2.9 FFA_MEM_PERM_SET
  SvcArgs.Arg0 = ARM_FID_FFA_MEM_PERM_SET;
  SvcArgs.Arg1 = BaseAddress;
  SvcArgs.Arg2 = EFI_SIZE_TO_PAGES (Length);
  SvcArgs.Arg3 = Permissions;

  return SendMemoryPermissionRequest (&SvcArgs, &Ret);
}

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_STATUS  Status;
  UINT32      MemoryAttributes;
  UINT32      PermissionRequest;
  UINTN       Size;

  if (!IsFfaMemoryAbiSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Size = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                          MemoryAttributes,
                          ARM_FFA_SET_MEM_ATTR_CODE_PERM_XN
                          );

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               BaseAddress,
               Size,
               PermissionRequest
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Length      -= Size;
    BaseAddress += Size;
  } // while

  return Status;
}

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_STATUS  Status;
  UINT32      MemoryAttributes;
  UINT32      PermissionRequest;
  UINTN       Size;

  if (!IsFfaMemoryAbiSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Size = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                          MemoryAttributes,
                          ARM_FFA_SET_MEM_ATTR_CODE_PERM_X
                          );

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               BaseAddress,
               Size,
               PermissionRequest
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Length      -= Size;
    BaseAddress += Size;
  } // while

  return Status;
}

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_STATUS  Status;
  UINT32      MemoryAttributes;
  UINT32      PermissionRequest;
  UINTN       Size;

  if (!IsFfaMemoryAbiSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Size = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                          ARM_FFA_SET_MEM_ATTR_DATA_PERM_RO,
                          (MemoryAttributes >> ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT)
                          );

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               BaseAddress,
               Size,
               PermissionRequest
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Length      -= Size;
    BaseAddress += Size;
  } // while

  return Status;
}

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_STATUS  Status;
  UINT32      MemoryAttributes;
  UINT32      PermissionRequest;
  UINTN       Size;

  if (!IsFfaMemoryAbiSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Size = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                          ARM_FFA_SET_MEM_ATTR_DATA_PERM_RW,
                          (MemoryAttributes >> ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT)
                          );

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               BaseAddress,
               Size,
               PermissionRequest
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Length      -= Size;
    BaseAddress += Size;
  } // while

  return Status;
}
