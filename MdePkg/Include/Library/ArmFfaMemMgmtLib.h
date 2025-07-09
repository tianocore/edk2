/** @file
  Library definition for FF-A memory management protocol.

  Copyright (c) 2020 - 2022, Arm Ltd. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_FFA_MEM_MGMT_LIB_H_
#define ARM_FFA_MEM_MGMT_LIB_H_

#include <Uefi.h>

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  @brief      Starts a transaction to transfer access to a shared or lent
              memory region from a Borrower back to its Owner.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibRelinquish (
  VOID
  );

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
  );

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
  );

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
  );

#endif // ARM_FFA_MEM_MGMT_LIB_H_
