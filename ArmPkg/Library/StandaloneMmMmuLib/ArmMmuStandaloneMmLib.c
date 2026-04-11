/** @file
  File managing the MMU for ARMv8 architecture in S-EL0

  Copyright (c) 2017 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2021, Linaro Limited
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] SPM based on the MM interface.
        (https://trustedfirmware-a.readthedocs.io/en/latest/components/
         secure-partition-manager-mm.html)
  - [2] Arm Firmware Framework for Armv8-A, DEN0077, version 1.2
        (https://developer.arm.com/documentation/den0077/latest/)
  - [3] FF-A Memory Management Protocol, DEN0140, version 1.2
        (https://developer.arm.com/documentation/den0140/latest/)
**/

#include <Uefi.h>
#include <IndustryStandard/ArmMmSvc.h>
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

/**
  Convert FFA return code to EFI_STATUS.

  @param [in] SpmMmStatus           SPM_MM return code

  @retval EFI_STATUS             correspond EFI_STATUS to SpmMmStatus
**/
STATIC
EFI_STATUS
SpmMmStatusToEfiStatus (
  IN UINTN  SpmMmStatus
  )
{
  switch (SpmMmStatus) {
    case ARM_SPM_MM_RET_SUCCESS:
      return EFI_SUCCESS;
    case ARM_SPM_MM_RET_INVALID_PARAMS:
      return EFI_INVALID_PARAMETER;
    case ARM_SPM_MM_RET_DENIED:
      return EFI_ACCESS_DENIED;
    case ARM_SPM_MM_RET_NO_MEMORY:
      return EFI_OUT_OF_RESOURCES;
    default:
      return EFI_UNSUPPORTED;
  }
}

/** Send a request the target to get/set the memory permission.

  @param [in]       UseFfaAbis  Use FF-A abis or not.
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
  IN      BOOLEAN       UseFfaAbis,
  IN OUT  ARM_SVC_ARGS  *SvcArgs,
  OUT     INT32         *RetVal
  )
{
  if ((SvcArgs == NULL) || (RetVal == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ArmCallSvc (SvcArgs);

  if (UseFfaAbis) {
    if (IS_FID_FFA_ERROR (SvcArgs->Arg0)) {
      return FfaStatusToEfiStatus (SvcArgs->Arg2);
    }

    *RetVal = SvcArgs->Arg2;
  } else {
    // Check error response from Callee.
    // Bit 31 set means there is an error returned
    // See [1], Section 13.5.5.1 MM_SP_MEMORY_ATTRIBUTES_GET_AARCH64 and
    // Section 13.5.5.2 MM_SP_MEMORY_ATTRIBUTES_SET_AARCH64.
    if ((SvcArgs->Arg0 & BIT31) != 0) {
      return SpmMmStatusToEfiStatus (SvcArgs->Arg0);
    }

    *RetVal = SvcArgs->Arg0;
  }

  return EFI_SUCCESS;
}

/** Request the permission attributes of a memory region from S-EL0.

  @param [in]   UseFfaAbis           Use FF-A abis or not.
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
  IN     BOOLEAN               UseFfaAbis,
  IN     EFI_PHYSICAL_ADDRESS  BaseAddress,
  OUT    UINT32                *MemoryAttributes
  )
{
  EFI_STATUS    Status;
  INT32         Ret;
  ARM_SVC_ARGS  SvcArgs;
  UINTN         Fid;

  if (MemoryAttributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Prepare the message parameters.
  // See [1], Section 13.5.5.1 MM_SP_MEMORY_ATTRIBUTES_GET_AARCH64.
  // See [3], Section 2.8 FFA_MEM_PERM_GET
  Fid = (UseFfaAbis) ? ARM_FID_FFA_MEM_PERM_GET : ARM_FID_SPM_MM_SP_GET_MEM_ATTRIBUTES;

  ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));
  SvcArgs.Arg0 = Fid;
  SvcArgs.Arg1 = BaseAddress;

  Status = SendMemoryPermissionRequest (UseFfaAbis, &SvcArgs, &Ret);
  if (EFI_ERROR (Status)) {
    *MemoryAttributes = 0;
  } else {
    *MemoryAttributes = Ret;
  }

  return Status;
}

/** Request the permission attributes of the S-EL0 memory region to be updated.

  @param [in]  UseFfaAbis      Use FF-A abis or not.
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
  IN  BOOLEAN               UseFfaAbis,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT32                Permissions
  )
{
  INT32         Ret;
  ARM_SVC_ARGS  SvcArgs;
  UINTN         Fid;

  // Prepare the message parameters.
  // See [1], Section 13.5.5.2 MM_SP_MEMORY_ATTRIBUTES_SET_AARCH64.
  // See [3], Section 2.9 FFA_MEM_PERM_SET
  Fid = (UseFfaAbis) ? ARM_FID_FFA_MEM_PERM_SET : ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES;

  ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));

  SvcArgs.Arg0 = Fid;
  SvcArgs.Arg1 = BaseAddress;
  SvcArgs.Arg2 = EFI_SIZE_TO_PAGES (Length);
  SvcArgs.Arg3 = Permissions;

  return SendMemoryPermissionRequest (UseFfaAbis, &SvcArgs, &Ret);
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
  BOOLEAN     UseFfaAbis;
  UINTN       Size;

  UseFfaAbis = IsFfaMemoryAbiSupported ();
  Size       = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (UseFfaAbis, BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (UseFfaAbis) {
      PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            MemoryAttributes,
                            ARM_FFA_SET_MEM_ATTR_CODE_PERM_XN
                            );
    } else {
      PermissionRequest = ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            MemoryAttributes,
                            ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_XN
                            );
    }

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               UseFfaAbis,
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
  BOOLEAN     UseFfaAbis;
  UINTN       Size;

  UseFfaAbis = IsFfaMemoryAbiSupported ();
  Size       = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (UseFfaAbis, BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (UseFfaAbis) {
      PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            MemoryAttributes,
                            ARM_FFA_SET_MEM_ATTR_CODE_PERM_X
                            );
    } else {
      PermissionRequest = ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            MemoryAttributes,
                            ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_X
                            );
    }

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               UseFfaAbis,
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
  BOOLEAN     UseFfaAbis;
  UINTN       Size;

  UseFfaAbis = IsFfaMemoryAbiSupported ();
  Size       = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (UseFfaAbis, BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (UseFfaAbis) {
      PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            ARM_FFA_SET_MEM_ATTR_DATA_PERM_RO,
                            (MemoryAttributes >> ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT)
                            );
    } else {
      PermissionRequest = ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RO,
                            (MemoryAttributes >> ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_SHIFT)
                            );
    }

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               UseFfaAbis,
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
  BOOLEAN     UseFfaAbis;
  UINTN       Size;

  UseFfaAbis = IsFfaMemoryAbiSupported ();
  Size       = EFI_PAGE_SIZE;

  while (Length > 0) {
    Status = GetMemoryPermissions (UseFfaAbis, BaseAddress, &MemoryAttributes);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (UseFfaAbis) {
      PermissionRequest = ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            ARM_FFA_SET_MEM_ATTR_DATA_PERM_RW,
                            (MemoryAttributes >> ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT)
                            );
    } else {
      PermissionRequest = ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
                            ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RW,
                            (MemoryAttributes >> ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_SHIFT)
                            );
    }

    if (Length < Size) {
      Length = Size;
    }

    Status = RequestMemoryPermissionChange (
               UseFfaAbis,
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
