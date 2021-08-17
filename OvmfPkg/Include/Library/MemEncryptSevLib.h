/** @file

  Define Secure Encrypted Virtualization (SEV) base library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEM_ENCRYPT_SEV_LIB_H_
#define _MEM_ENCRYPT_SEV_LIB_H_

#include <Base.h>
#include <WorkArea.h>

//
// Define the maximum number of #VCs allowed (e.g. the level of nesting
// that is allowed => 2 allows for 1 nested #VCs). I this value is changed,
// be sure to increase the size of
//   gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbBackupSize
// in any FDF file using this PCD.
//
#define VMGEXIT_MAXIMUM_VC_COUNT   2

//
// Per-CPU data mapping structure
//   Use UINT32 for cached indicators and compare to a specific value
//   so that the hypervisor can't indicate a value is cached by just
//   writing random data to that area.
//
typedef struct {
  UINT32  Dr7Cached;
  UINT64  Dr7;

  UINTN   VcCount;
  VOID    *GhcbBackupPages;
} SEV_ES_PER_CPU_DATA;

//
// Memory encryption address range states.
//
typedef enum {
  MemEncryptSevAddressRangeUnencrypted,
  MemEncryptSevAddressRangeEncrypted,
  MemEncryptSevAddressRangeMixed,
  MemEncryptSevAddressRangeError,
} MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE;

/**
  Returns a boolean to indicate whether SEV-ES is enabled.

  @retval TRUE           SEV-ES is enabled
  @retval FALSE          SEV-ES is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevEsIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV is enabled

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevIsEnabled (
  VOID
  );

/**
  This function clears memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevClearPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );

/**
  This function sets memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevSetPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );


/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptSevLocateInitialSmramSaveStateMapPages (
  OUT UINTN *BaseAddress,
  OUT UINTN *NumberOfPages
  );

/**
  Returns the SEV encryption mask.

  @return  The SEV pagetable encryption mask
**/
UINT64
EFIAPI
MemEncryptSevGetEncryptionMask (
  VOID
  );

/**
  Returns the encryption state of the specified virtual address range.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             Base address to check
  @param[in]  Length                  Length of virtual address range

  @retval MemEncryptSevAddressRangeUnencrypted  Address range is mapped
                                                unencrypted
  @retval MemEncryptSevAddressRangeEncrypted    Address range is mapped
                                                encrypted
  @retval MemEncryptSevAddressRangeMixed        Address range is mapped mixed
  @retval MemEncryptSevAddressRangeError        Address range is not mapped
**/
MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE
EFIAPI
MemEncryptSevGetAddressRangeState (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    Length
  );

/**
  This function clears memory encryption bit for the MMIO region specified by
  BaseAddress and NumPages.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a MMIO region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevClearMmioPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );

#endif // _MEM_ENCRYPT_SEV_LIB_H_
