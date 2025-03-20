// SPDX-License-Identifier: BSD-3-Clause

/** @file
  This library implements the FF-A memory manage protocol.

  Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile v1.3 ALP1: [https://developer.arm.com/documentation/den0077/l]
     - FF-A Memory Management Protocol [https://developer.arm.com/documentation/den0140/f]

**/

#include <Uefi.h>
#include <IndustryStandard/ArmFfaMemMgmt.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmFfaMemMgmtLib.h>
#include <Library/BaseMemoryLib.h>

/**
  @brief      Starts a transaction to transfer of ownership of a memory region
              from a Sender endpoint to a Receiver endpoint.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction descriptor
                              passed in this ABI invocation
  @param[in]  BufferAddr      Base address of a buffer allocated by the Owner and
                              distinct from the TX buffer
  @param[in]  PageCount       Number of 4K pages in the buffer allocated by the
                              Owner and distinct from the TX buffer
  @param[out] Handle          Globally unique Handle to identify the memory region
                              upon successful transmission of the transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibDonate (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  IN VOID     *BufferAddr,
  IN UINT32   PageCount,
  OUT UINT64  *Handle
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  if (Handle == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Handle is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FfaArgs.Arg0 = ((UINTN)BufferAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_DONATE_AARCH64 : ARM_FID_FFA_MEM_DONATE_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_DONATE feature... Status: %r\n", __func__, Status));
    *Handle = 0U;
    return Status;
  }

  FfaArgs.Arg1 = TotalLength;
  FfaArgs.Arg2 = FragmentLength;
  FfaArgs.Arg3 = (UINTN)BufferAddr;
  FfaArgs.Arg4 = PageCount;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    *Handle = 0U;
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  /**
    There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
    will use the default 32-bit version.
  **/
  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  *Handle = ((UINT64)FfaArgs.Arg3 << 32) | FfaArgs.Arg2;
  return EFI_SUCCESS;
}

/**
  @brief      Starts a transaction to transfer of ownership of a memory region
              from a Sender endpoint to a Receiver endpoint.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction descriptor
                              passed in this ABI invocation
  @param[out] Handle          Globally unique Handle to identify the memory region
                              upon successful transmission of the transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibDonateRxTx (
  IN   UINT32  TotalLength,
  IN   UINT32  FragmentLength,
  OUT  UINT64  *Handle
  )
{
  return ArmFfaMemLibDonate (TotalLength, FragmentLength, NULL, 0, Handle);
}

/**
  @brief      Starts a transaction to transfer an Owner’s access to a memory
              region and  grant access to it to one or more Borrowers.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction
                              descriptor passed in this ABI invocation
  @param[in]  BufferAddr      Base address of a buffer allocated by the Owner
                              and distinct from the TX buffer
  @param[in]  PageCount       Number of 4K pages in the buffer allocated by
                              the Owner and distinct from the TX buffer
  @param[out] Handle          Globally unique Handle to identify the memory
                              region upon successful transmission of the
                              transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibLend (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  IN VOID     *BufferAddr,
  IN UINT32   PageCount,
  OUT UINT64  *Handle
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  if (Handle == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Handle is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FfaArgs.Arg0 = ((UINTN)BufferAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_LEND_AARCH64 : ARM_FID_FFA_MEM_LEND_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_LEND feature... Status: %r\n", __func__, Status));
    *Handle = 0U;
    return Status;
  }

  FfaArgs.Arg1 = TotalLength;
  FfaArgs.Arg2 = FragmentLength;
  FfaArgs.Arg3 = (UINTN)BufferAddr;
  FfaArgs.Arg4 = PageCount;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    *Handle = 0U;
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  /**
    There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
    will use the default 32-bit version.
  **/
  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  *Handle = ((UINT64)FfaArgs.Arg3 << 32) | FfaArgs.Arg2;
  return EFI_SUCCESS;
}

/**
  @brief      Starts a transaction to transfer an Owner’s access to a memory
              region and  grant access to it to one or more Borrowers through
              Rx/Tx buffer.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction descriptor
                              passed in this ABI invocation
  @param[out] Handle          Globally unique Handle to identify the memory region
                              upon successful transmission of the transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibLendRxTx (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  OUT UINT64  *Handle
  )
{
  return ArmFfaMemLibLend (TotalLength, FragmentLength, NULL, 0, Handle);
}

/**
  @brief      Starts a transaction to grant access to a memory region to one or
              more Borrowers.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction descriptor
                              passed in this ABI invocation
  @param[in]  BufferAddr      Base address of a buffer allocated by the Owner and
                              distinct from the TX buffer
  @param[in]  PageCount       Number of 4K pages in the buffer allocated by the
                              Owner and distinct from the TX buffer
  @param[out] Handle          Globally unique Handle to identify the memory region
                              upon successful transmission of the transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibShare (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  IN VOID     *BufferAddr,
  IN UINT32   PageCount,
  OUT UINT64  *Handle
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  if (Handle == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Handle is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FfaArgs.Arg0 = ((UINTN)BufferAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_SHARE_AARCH64 : ARM_FID_FFA_MEM_SHARE_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_SHARE feature... Status: %r\n", __func__, Status));
    *Handle = 0U;
    return Status;
  }

  FfaArgs.Arg1 = TotalLength;
  FfaArgs.Arg2 = FragmentLength;
  FfaArgs.Arg3 = (UINTN)BufferAddr;
  FfaArgs.Arg4 = PageCount;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    *Handle = 0U;
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  /**
    There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
    will use the default 32-bit version.
  **/
  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  *Handle = ((UINT64)FfaArgs.Arg3 << 32) | FfaArgs.Arg2;
  return EFI_SUCCESS;
}

/**
  @brief      Starts a transaction to grant access to a memory region to one or
              more Borrowers through Rx/Tx buffer.

  @param[in]  TotalLength     Total length of the memory transaction descriptor
                              in bytes
  @param[in]  FragmentLength  Length in bytes of the memory transaction descriptor
                              passed in this ABI invocation
  @param[out] Handle          Globally unique Handle to identify the memory region
                              upon successful transmission of the transaction descriptor.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibShareRxTx (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  OUT UINT64  *Handle
  )
{
  return ArmFfaMemLibShare (TotalLength, FragmentLength, NULL, 0, Handle);
}

/**
  @brief      Requests completion of a donate, lend or share memory management
              transaction.

  @param[in]  TotalLength         Total length of the memory transaction descriptor
                                  in bytes
  @param[in]  FragmentLength      Length in bytes of the memory transaction descriptor
                                  passed in this ABI invocation
  @param[in]  BufferAddr          Base address of a buffer allocated by the Owner
                                  and distinct from the TX buffer
  @param[in]  PageCount           Number of 4K pages in the buffer allocated by
                                  the Owner and distinct from the TX buffer
  @param[out] RespTotalLength     Total length of the response memory transaction
                                  descriptor in bytes
  @param[out] RespFragmentLength  Length in bytes of the response memory transaction
                                  descriptor passed in this ABI invocation

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibRetrieveReq (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  IN VOID     *BufferAddr,
  IN UINT32   PageCount,
  OUT UINT32  *RespTotalLength,
  OUT UINT32  *RespFragmentLength
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  if ((RespTotalLength == NULL) || (RespFragmentLength == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Handle is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FfaArgs.Arg0 = ((UINTN)BufferAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH64 : ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_RETRIEVE_REQ feature... Status: %r\n", __func__, Status));
    *RespTotalLength    = 0U;
    *RespFragmentLength = 0U;
    return Status;
  }

  FfaArgs.Arg1 = TotalLength;
  FfaArgs.Arg2 = FragmentLength;
  FfaArgs.Arg3 = (UINTN)BufferAddr;
  FfaArgs.Arg4 = PageCount;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    *RespTotalLength    = 0U;
    *RespFragmentLength = 0U;
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_MEM_RETRIEVE_RESP);
  *RespTotalLength    = FfaArgs.Arg1;
  *RespFragmentLength = FfaArgs.Arg2;
  return EFI_SUCCESS;
}

/**
  @brief      Requests completion of a donate, lend or share memory management
              transaction through Rx/Tx buffer.

  @param[in]  TotalLength         Total length of the memory transaction descriptor
                                  in bytes
  @param[in]  FragmentLength      Length in bytes of the memory transaction descriptor
                                  passed in this ABI invocation
  @param[out] RespTotalLength     Total length of the response memory transaction
                                  descriptor in bytes
  @param[out] RespFragmentLength  Length in bytes of the response memory transaction
                                  descriptor passed in this ABI invocation

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibRetrieveReqRxTx (
  IN UINT32   TotalLength,
  IN UINT32   FragmentLength,
  OUT UINT32  *RespTotalLength,
  OUT UINT32  *RespFragmentLength
  )
{
  return ArmFfaMemLibRetrieveReq (
           TotalLength,
           FragmentLength,
           NULL,
           0,
           RespTotalLength,
           RespFragmentLength
           );
}

/**
  @brief      Starts a transaction to transfer access to a shared or lent
              memory region from a Borrower back to its Owner.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibRelinquish (
  VOID
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  FfaArgs.Arg0 = ARM_FID_FFA_MEM_RETRIEVE_RELINQUISH;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_RETRIEVE_RELINQUISH feature... Status: %r\n", __func__, Status));
    return Status;
  }

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  return EFI_SUCCESS;
}

/**
  @brief      Restores exclusive access to a memory region back to its Owner.

  @param[in]  Handle  Globally unique Handle to identify the memory region
  @param[in]  Flags   Flags for modifying the reclaim behavior

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibReclaim (
  IN UINT64  Handle,
  IN UINT32  Flags
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;
  UINT32        HandleHi = 0;
  UINT32        HandleLo = 0;

  FfaArgs.Arg0 = ARM_FID_FFA_MEM_RETRIEVE_RECLAIM;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_RETRIEVE_RECLAIM feature... Status: %r\n", __func__, Status));
    return Status;
  }

  HandleHi = (Handle >> 32) & MAX_UINT32;
  HandleLo = Handle & MAX_UINT32;

  FfaArgs.Arg1 = HandleLo;
  FfaArgs.Arg2 = HandleHi;
  FfaArgs.Arg3 = Flags;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  return EFI_SUCCESS;
}

/**
  @brief       Queries the memory attributes of a memory region. This function
               can only access the regions of the SP's own translation regine.
               Moreover this interface is only available in the boot phase,
               i.e. before invoking FFA_MSG_WAIT interface.

  @param[in]   BaseAddr     Base VA of a translation granule whose
                            permission attributes must be returned.
  @param[in]   PageCount    Number of translation granule size pages from the
                            Base address whose permissions must be returned.
                            This is calculated as Input Page count + 1.
  @param[out]  MemoryPerm   Permission attributes of the memory region

  @retval      The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibPermGet (
  IN CONST VOID  *BaseAddr,
  IN UINT32      PageCount,
  OUT UINT32     *MemoryPerm
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  if (MemoryPerm == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Handle is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FfaArgs.Arg0 = ((UINTN)BaseAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_PERM_GET_AARCH64 : ARM_FID_FFA_MEM_PERM_GET_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_PERM_GET feature... Status: %r\n", __func__, Status));
    return Status;
  }

  FfaArgs.Arg1 = (UINTN)BaseAddr;
  FfaArgs.Arg2 = PageCount;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  *MemoryPerm = FfaArgs.Arg2;
  return EFI_SUCCESS;
}

/**
  @brief       Sets the memory attributes of a memory regions. This function
               can only access the regions of the SP's own translation regine.
               Moreover this interface is only available in the boot phase,
               i.e. before invoking FFA_MSG_WAIT interface.

  @param[in]   BaseAddr     Base VA of a memory region whose permission
                            attributes must be set.
  @param[in]   PageCount    Number of translation granule size pages
                            starting from the Base address whose permissions
                            must be set.
  @param[in]   MemoryPerm   Permission attributes to be set for the memory
                            region.

  @retval      The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibPermSet (
  IN CONST VOID  *BaseAddr,
  IN UINT32      PageCount,
  IN UINT32      MemoryPerm
  )
{
  ARM_FFA_ARGS  FfaArgs;
  EFI_STATUS    Status;

  ASSERT ((MemoryPerm & ARM_FFA_MEM_PERM_RESERVED_MASK) == 0);

  FfaArgs.Arg0 = ((UINTN)BaseAddr > MAX_UINT32) ? ARM_FID_FFA_MEM_PERM_SET_AARCH64 : ARM_FID_FFA_MEM_PERM_SET_AARCH32;

  Status = ArmFfaLibGetFeatures (
             FfaArgs.Arg0,
             0,
             &FfaArgs.Arg1,
             &FfaArgs.Arg2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FFA_MEM_PERM_SET feature... Status: %r\n", __func__, Status));
    return Status;
  }

  FfaArgs.Arg1 = (UINTN)BaseAddr;
  FfaArgs.Arg2 = PageCount;
  FfaArgs.Arg3 = MemoryPerm;

  ArmCallFfa (&FfaArgs);

  if (FfaArgs.Arg0 == ARM_FID_FFA_ERROR) {
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  ASSERT (FfaArgs.Arg0 == ARM_FID_FFA_SUCCESS_AARCH32);
  return EFI_SUCCESS;
}
