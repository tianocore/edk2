/** @file
  Library definition for FF-A memory management protocol.

  Copyright (c) 2020 - 2026, Arm Ltd. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <IndustryStandard/ArmFfaMemMgmt.h>

/**
  Starts a transaction to transfer of ownership of a memory region from a Sender
  endpoint to a Receiver endpoint.

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
  Starts a transaction to transfer of ownership of a memory region from a Sender
  endpoint to a Receiver endpoint.

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
  Starts a transaction to transfer an Owner's access to a memory region and grant
  access to it to one or more Borrowers.

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
  Starts a transaction to transfer an Owner's access to a memory region and grant
  access to it to one or more Borrowers through Rx/Tx buffer.

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
  Starts a transaction to grant access to a memory region to one or more Borrowers.

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
  Starts a transaction to grant access to a memory region to one or more Borrowers
  through Rx/Tx buffer.

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
  @brief      Builds and sends an FF-A memory transaction descriptor using the
              Rx/Tx buffer to share a memory region with a receiver endpoint.

              This helper builds a single-receiver memory transaction descriptor
              in the FF-A Tx buffer and then invokes the FFA_MEM_SHARE ABI.

  @param[in]  ReceiverId       FF-A receiver endpoint/partition ID.
  @param[in]  RegionPhysBase   Physical base address of the memory region to
                               share. Must be 4K aligned.
  @param[in]  RegionSizeBytes  Size in bytes of the memory region to share.
                               Rounded up to a whole number of 4 KiB pages.
  @param[in]  Attributes       Memory attributes to use for the receiver mapping.
                               Security must be 0 (reserved for FFA_MEM_* ABIs).
  @param[in]  Permissions      Access permissions to request for the receiver.
  @param[out] Handle           Globally unique handle to identify the memory
                               region upon successful transmission.

  @retval EFI_SUCCESS          Success.
  @retval Others               Failure.
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibShareRxTxSingleRegion (
  IN UINT16                               ReceiverId,
  IN EFI_PHYSICAL_ADDRESS                 RegionPhysBase,
  IN UINTN                                RegionSizeBytes,
  IN CONST FFA_MEMORY_ATTRIBUTES          *Attributes,
  IN CONST FFA_MEMORY_ACCESS_PERMISSIONS  *Permissions,
  OUT UINT64                              *Handle
  );

/**
  Requests completion of a donate, lend or share memory management transaction.

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
  Requests completion of a donate, lend or share memory management transaction
  through Rx/Tx buffer.

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
  Starts a transaction to transfer access to a shared or lent memory region from
  a Borrower back to its Owner.

  @retval     The translated FF-A error status code
**/
EFI_STATUS
EFIAPI
ArmFfaMemLibRelinquish (
  VOID
  );

/**
  Restores exclusive access to a memory region back to its Owner.

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
  Queries the memory attributes of a memory region. This function can only access
  the regions of the SP's own translation regine. Moreover this interface is only
  available in the boot phase, i.e. before invoking FFA_MSG_WAIT interface.

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
  Sets the memory attributes of a memory regions. This function can only access
  the regions of the SP's own translation regine. Moreover this interface is only
  available in the boot phase, i.e. before invoking FFA_MSG_WAIT interface.

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
