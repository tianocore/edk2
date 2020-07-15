/** @file
  Contains function prototypes for Memory Services in the SMM Core.

  This header file borrows the PiSmmCore Memory Allocation services as the primitive
  for memory allocation.

  Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PI_SMM_CORE_MEMORY_ALLOCATION_SERVICES_H_
#define _PI_SMM_CORE_MEMORY_ALLOCATION_SERVICES_H_

//
// It should be aligned with the definition in PiSmmCore.
//
typedef struct {
  UINTN                           Signature;

  ///
  /// The ImageHandle passed into the entry point of the SMM IPL.  This ImageHandle
  /// is used by the SMM Core to fill in the ParentImageHandle field of the Loaded
  /// Image Protocol for each SMM Driver that is dispatched by the SMM Core.
  ///
  EFI_HANDLE                      SmmIplImageHandle;

  ///
  /// The number of SMRAM ranges passed from the SMM IPL to the SMM Core.  The SMM
  /// Core uses these ranges of SMRAM to initialize the SMM Core memory manager.
  ///
  UINTN                           SmramRangeCount;

  ///
  /// A table of SMRAM ranges passed from the SMM IPL to the SMM Core.  The SMM
  /// Core uses these ranges of SMRAM to initialize the SMM Core memory manager.
  ///
  EFI_SMRAM_DESCRIPTOR            *SmramRanges;

  ///
  /// The SMM Foundation Entry Point.  The SMM Core fills in this field when the
  /// SMM Core is initialized.  The SMM IPL is responsbile for registering this entry
  /// point with the SMM Configuration Protocol.  The SMM Configuration Protocol may
  /// not be available at the time the SMM IPL and SMM Core are started, so the SMM IPL
  /// sets up a protocol notification on the SMM Configuration Protocol and registers
  /// the SMM Foundation Entry Point as soon as the SMM Configuration Protocol is
  /// available.
  ///
  EFI_SMM_ENTRY_POINT             SmmEntryPoint;

  ///
  /// Boolean flag set to TRUE while an SMI is being processed by the SMM Core.
  ///
  BOOLEAN                         SmmEntryPointRegistered;

  ///
  /// Boolean flag set to TRUE while an SMI is being processed by the SMM Core.
  ///
  BOOLEAN                         InSmm;

  ///
  /// This field is set by the SMM Core then the SMM Core is initialized.  This field is
  /// used by the SMM Base 2 Protocol and SMM Communication Protocol implementations in
  /// the SMM IPL.
  ///
  EFI_SMM_SYSTEM_TABLE2           *Smst;

  ///
  /// This field is used by the SMM Communicatioon Protocol to pass a buffer into
  /// a software SMI handler and for the software SMI handler to pass a buffer back to
  /// the caller of the SMM Communication Protocol.
  ///
  VOID                            *CommunicationBuffer;

  ///
  /// This field is used by the SMM Communicatioon Protocol to pass the size of a buffer,
  /// in bytes, into a software SMI handler and for the software SMI handler to pass the
  /// size, in bytes, of a buffer back to the caller of the SMM Communication Protocol.
  ///
  UINTN                           BufferSize;

  ///
  /// This field is used by the SMM Communication Protocol to pass the return status from
  /// a software SMI handler back to the caller of the SMM Communication Protocol.
  ///
  EFI_STATUS                      ReturnStatus;

  EFI_PHYSICAL_ADDRESS            PiSmmCoreImageBase;
  UINT64                          PiSmmCoreImageSize;
  EFI_PHYSICAL_ADDRESS            PiSmmCoreEntryPoint;
} SMM_CORE_PRIVATE_DATA;

/**
  Called to initialize the memory service.

  @param   SmramRangeCount       Number of SMRAM Regions
  @param   SmramRanges           Pointer to SMRAM Descriptors

**/
VOID
SmmInitializeMemoryServices (
  IN UINTN                 SmramRangeCount,
  IN EFI_SMRAM_DESCRIPTOR  *SmramRanges
  );

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmAllocatePages (
  IN      EFI_ALLOCATE_TYPE         Type,
  IN      EFI_MEMORY_TYPE           MemoryType,
  IN      UINTN                     NumberOfPages,
  OUT     EFI_PHYSICAL_ADDRESS      *Memory
  );

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePages (
  IN      EFI_PHYSICAL_ADDRESS      Memory,
  IN      UINTN                     NumberOfPages
  );

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  PoolType not valid
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmAllocatePool (
  IN      EFI_MEMORY_TYPE           PoolType,
  IN      UINTN                     Size,
  OUT     VOID                      **Buffer
  );

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePool (
  IN      VOID                      *Buffer
  );

#endif
