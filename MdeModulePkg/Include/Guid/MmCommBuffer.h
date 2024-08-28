/** @file
  MM Communication buffer data.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMM_BUFFER_H_
#define MM_COMM_BUFFER_H_

///
/// The GUID of the MM Communication buffer HOB.
///
#define MM_COMM_BUFFER_HOB_GUID \
  { 0x6c2a2520, 0x0131, 0x4aee, { 0xa7, 0x50, 0xcc, 0x38, 0x4a, 0xac, 0xe8, 0xc6 }}

///
/// The MM communicate buffer facilitates data sharing between non-MM and MM code.
/// The MM IPL code allocates a "fixed" runtime type memory as the MM communication buffer,
/// and communicates its address and size to MM Core via MmCommBuffer GUIDed HOB.
/// Here, "fixed" implies that the buffer's location remains constant throughout the boot process.
/// Data is exchanged between the MM Communication PPI/Protocol and a software MMI handler
/// using this fixed MM communication buffer.
///
typedef struct {
  ///
  /// The address of the 4-KiB aligned fixed MM communication buffer.
  ///
  EFI_PHYSICAL_ADDRESS    PhysicalStart;

  ///
  /// Size of the fixed MM communication buffer, in 4KiB pages.
  ///
  UINT64                  NumberOfPages;

  ///
  /// Point to MM_COMM_BUFFER_STATUS structure.
  ///
  EFI_PHYSICAL_ADDRESS    Status;
} MM_COMM_BUFFER;

typedef struct {
  ///
  /// Whether the data in the fixed MM communication buffer is valid when entering from non-MM to MM.
  ///
  BOOLEAN    IsCommBufferValid;

  ///
  /// The return status when returning from MM to non-MM.
  ///
  UINT64     ReturnStatus;

  ///
  /// The size in bytes of the output buffer when returning from MM to non-MM.
  ///
  UINT64     ReturnBufferSize;
} MM_COMM_BUFFER_STATUS;

extern EFI_GUID  gMmCommBufferHobGuid;

#endif
