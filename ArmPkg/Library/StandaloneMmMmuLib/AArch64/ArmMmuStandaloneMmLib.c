/** @file
*  File managing the MMU for ARMv8 architecture in S-EL0
*
*  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <IndustryStandard/ArmMmSvc.h>

#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

STATIC
EFI_STATUS
GetMemoryPermissions (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  OUT UINT32                    *MemoryAttributes
  )
{
  ARM_SVC_ARGS  GetMemoryPermissionsSvcArgs = {0};

  GetMemoryPermissionsSvcArgs.Arg0 = ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH64;
  GetMemoryPermissionsSvcArgs.Arg1 = BaseAddress;
  GetMemoryPermissionsSvcArgs.Arg2 = 0;
  GetMemoryPermissionsSvcArgs.Arg3 = 0;

  ArmCallSvc (&GetMemoryPermissionsSvcArgs);
  if (GetMemoryPermissionsSvcArgs.Arg0 == ARM_SVC_SPM_RET_INVALID_PARAMS) {
    *MemoryAttributes = 0;
    return EFI_INVALID_PARAMETER;
  }

  *MemoryAttributes = GetMemoryPermissionsSvcArgs.Arg0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
RequestMemoryPermissionChange (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINTN                     Permissions
  )
{
  EFI_STATUS    Status;
  ARM_SVC_ARGS  ChangeMemoryPermissionsSvcArgs = {0};

  ChangeMemoryPermissionsSvcArgs.Arg0 = ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH64;
  ChangeMemoryPermissionsSvcArgs.Arg1 = BaseAddress;
  ChangeMemoryPermissionsSvcArgs.Arg2 = EFI_SIZE_TO_PAGES(Length);
  ChangeMemoryPermissionsSvcArgs.Arg3 = Permissions;

  ArmCallSvc (&ChangeMemoryPermissionsSvcArgs);

  Status = ChangeMemoryPermissionsSvcArgs.Arg0;

  switch (Status) {
  case ARM_SVC_SPM_RET_SUCCESS:
    Status = EFI_SUCCESS;
    break;

  case ARM_SVC_SPM_RET_NOT_SUPPORTED:
    Status = EFI_UNSUPPORTED;
    break;

  case ARM_SVC_SPM_RET_INVALID_PARAMS:
    Status = EFI_INVALID_PARAMETER;
    break;

  case ARM_SVC_SPM_RET_DENIED:
    Status = EFI_ACCESS_DENIED;
    break;

  case ARM_SVC_SPM_RET_NO_MEMORY:
    Status = EFI_BAD_BUFFER_SIZE;
    break;

  default:
    Status = EFI_ACCESS_DENIED;
    ASSERT (0);
  }

  return Status;
}

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  EFI_STATUS    Status;
  UINT32 MemoryAttributes;
  UINT32 CodePermission;

  Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
  if (Status != EFI_INVALID_PARAMETER) {
    CodePermission = SET_MEM_ATTR_CODE_PERM_XN << SET_MEM_ATTR_CODE_PERM_SHIFT;
    return RequestMemoryPermissionChange (
             BaseAddress,
             Length,
             MemoryAttributes | CodePermission
             );
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  EFI_STATUS    Status;
  UINT32 MemoryAttributes;
  UINT32 CodePermission;

  Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
  if (Status != EFI_INVALID_PARAMETER) {
    CodePermission = SET_MEM_ATTR_CODE_PERM_XN << SET_MEM_ATTR_CODE_PERM_SHIFT;
    return RequestMemoryPermissionChange (
             BaseAddress,
             Length,
             MemoryAttributes & ~CodePermission
             );
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  EFI_STATUS    Status;
  UINT32 MemoryAttributes;
  UINT32 DataPermission;

  Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
  if (Status != EFI_INVALID_PARAMETER) {
    DataPermission = SET_MEM_ATTR_DATA_PERM_RO << SET_MEM_ATTR_DATA_PERM_SHIFT;
    return RequestMemoryPermissionChange (
             BaseAddress,
             Length,
             MemoryAttributes | DataPermission
             );
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  EFI_STATUS    Status;
  UINT32 MemoryAttributes;
  UINT32 PermissionRequest;

  Status = GetMemoryPermissions (BaseAddress, &MemoryAttributes);
  if (Status != EFI_INVALID_PARAMETER) {
    PermissionRequest = SET_MEM_ATTR_MAKE_PERM_REQUEST (SET_MEM_ATTR_DATA_PERM_RW,
                                                        MemoryAttributes);
    return RequestMemoryPermissionChange (
             BaseAddress,
             Length,
             PermissionRequest
             );
  }
  return EFI_INVALID_PARAMETER;
}
