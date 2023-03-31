/** @file
  Implement Risc-V Cache Management Operations

  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2023, Rivos Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define RV64_CACHE_BLOCK_SIZE  64

typedef enum {
  Clean,
  Flush,
  Invld,
} CACHE_OP;

/**
  RISC-V invalidate instruction cache.

**/
VOID
EFIAPI
RiscVInvalidateInstCacheAsm_Fence (
  VOID
  );

/**
  RISC-V invalidate data cache.

**/
VOID
EFIAPI
RiscVInvalidateDataCacheAsm_Fence (
  VOID
  );

/**
  RISC-V flush cache block. Atomically perform a clean operation
  followed by an invalidate operation

**/
VOID
EFIAPI
RiscVCpuCacheFlushAsm_Cbo (
  UINTN
  );

/**
Perform a write transfer to another cache or to memory if the
data in the copy of the cache block have been modified by a store
operation

**/
VOID
EFIAPI
RiscVCpuCacheCleanAsm_Cbo (
  UINTN
  );

/**
Deallocate the copy of the cache block

**/
VOID
EFIAPI
RiscVCpuCacheInvalAsm_Cbo (
  UINTN
  );

/**
Verify CBOs are supported by this HW
CBCFE == Cache Block Clean and Flush instruction Enable
CBIE == Cache Block Invalidate instruction Enable

**/
UINTN
RiscvIsCbcfeEnabledAsm (
  VOID
  );

UINTN
RiscvIsCbiEnabledAsm (
  VOID
  );

/**
  Performs required opeartion on cache lines in the cache coherency domain
  of the calling CPU. If Address is not aligned on a cache line boundary,
  then entire cache line containing Address is operated. If Address + Length
  is not aligned on a cache line boundary, then the entire cache line
  containing Address + Length -1 is operated.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the cache lines to
          invalidate. If the CPU is in a physical addressing mode,
          then Address is a physical address. If the CPU is in a virtual
          addressing mode, then Address is a virtual address.

  @param  Length  The number of bytes to invalidate from the instruction
          cache.

  @param  Op  Type of CMO operation to be performed

  @return Address.

**/
VOID *
EFIAPI
CacheOpCacheRange (
  IN VOID      *Address,
  IN UINTN     Length,
  IN CACHE_OP  Op
  )
{
  UINTN  CacheLineSize;
  UINTN  Start;
  UINTN  End;

  if (Length == 0) {
    return Address;
  }

  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Address));

  //
  // Cache line size is 8 * Bits 15-08 of EBX returned from CPUID 01H
  //
  CacheLineSize = RV64_CACHE_BLOCK_SIZE;

  Start = (UINTN)Address;
  //
  // Calculate the cache line alignment
  //
  End    = (Start + Length + (CacheLineSize - 1)) & ~(CacheLineSize - 1);
  Start &= ~((UINTN)CacheLineSize - 1);

  DEBUG (
    (DEBUG_INFO,
     "%a Performing Cache Management Operation %d \n", __func__, Op)
    );

  do {
    switch (Op) {
      case Invld:
        RiscVCpuCacheInvalAsm_Cbo (Start);
        break;
      case Flush:
        RiscVCpuCacheFlushAsm_Cbo (Start);
        break;
      case Clean:
        RiscVCpuCacheCleanAsm_Cbo (Start);
        break;
      default:
        DEBUG ((DEBUG_ERROR, "%a:RISC-V unsupported operation\n"));
        break;
    }

    Start = Start + CacheLineSize;
  } while (Start != End);

  return Address;
}

/**
  Invalidates the entire instruction cache in cache coherency domain of the
  calling CPU. Risc-V does not have currently an CBO implementation which can
  invalidate entire I-cache. Hence using Fence instruction for now. P.S. Fence
  instruction may or may not implement full I-cache invd functionality on all
  implementations.

**/
VOID
EFIAPI
InvalidateInstructionCache (
  VOID
  )
{
  RiscVInvalidateInstCacheAsm_Fence ();
}

/**
  Invalidates a range of instruction cache lines in the cache coherency domain
  of the calling CPU.

  Invalidates the instruction cache lines specified by Address and Length. If
  Address is not aligned on a cache line boundary, then entire instruction
  cache line containing Address is invalidated. If Address + Length is not
  aligned on a cache line boundary, then the entire instruction cache line
  containing Address + Length -1 is invalidated. This function may choose to
  invalidate the entire instruction cache if that is more efficient than
  invalidating the specified range. If Length is 0, then no instruction cache
  lines are invalidated. Address is returned.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the instruction cache lines to
                  invalidate. If the CPU is in a physical addressing mode, then
                  Address is a physical address. If the CPU is in a virtual
                  addressing mode, then Address is a virtual address.

  @param  Length  The number of bytes to invalidate from the instruction cache.

  @return Address.

**/
VOID *
EFIAPI
InvalidateInstructionCacheRange (
  IN VOID   *Address,
  IN UINTN  Length
  )
{
  if (RiscvIsCbiEnabledAsm () == RETURN_SUCCESS) {
    CacheOpCacheRange (Address, Length, Invld);
  } else {
    DEBUG (
      (DEBUG_WARN,
       "%a:RISC-V unsupported function.\n"
       "Invalidating the whole instruction cache instead.\n", __func__)
      );
    InvalidateInstructionCache ();
  }

  return Address;
}

/**
  Writes back and invalidates the entire data cache in cache coherency domain
  of the calling CPU.

  Writes back and invalidates the entire data cache in cache coherency domain
  of the calling CPU. This function guarantees that all dirty cache lines are
  written back to system memory, and also invalidates all the data cache lines
  in the cache coherency domain of the calling CPU.

**/
VOID
EFIAPI
WriteBackInvalidateDataCache (
  VOID
  )
{
  DEBUG ((DEBUG_ERROR, "%a:RISC-V unsupported function.\n", __func__));
}

/**
  Writes back and invalidates a range of data cache lines in the cache
  coherency domain of the calling CPU.

  Writes back and invalidates the data cache lines specified by Address and
  Length. If Address is not aligned on a cache line boundary, then entire data
  cache line containing Address is written back and invalidated. If Address +
  Length is not aligned on a cache line boundary, then the entire data cache
  line containing Address + Length -1 is written back and invalidated. This
  function may choose to write back and invalidate the entire data cache if
  that is more efficient than writing back and invalidating the specified
  range. If Length is 0, then no data cache lines are written back and
  invalidated. Address is returned.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the data cache lines to write back and
                  invalidate. If the CPU is in a physical addressing mode, then
                  Address is a physical address. If the CPU is in a virtual
                  addressing mode, then Address is a virtual address.
  @param  Length  The number of bytes to write back and invalidate from the
                  data cache.

  @return Address of cache invalidation.

**/
VOID *
EFIAPI
WriteBackInvalidateDataCacheRange (
  IN      VOID   *Address,
  IN      UINTN  Length
  )
{
  if (RiscvIsCbcfeEnabledAsm () == RETURN_SUCCESS) {
    CacheOpCacheRange (Address, Length, Flush);
  } else {
    DEBUG ((DEBUG_ERROR, "%a:RISC-V unsupported function.\n", __func__));
  }

  return Address;
}

/**
  Writes back the entire data cache in cache coherency domain of the calling
  CPU.

  Writes back the entire data cache in cache coherency domain of the calling
  CPU. This function guarantees that all dirty cache lines are written back to
  system memory. This function may also invalidate all the data cache lines in
  the cache coherency domain of the calling CPU.

**/
VOID
EFIAPI
WriteBackDataCache (
  VOID
  )
{
  DEBUG ((DEBUG_ERROR, "%a:RISC-V unsupported function.\n", __func__));
}

/**
  Writes back a range of data cache lines in the cache coherency domain of the
  calling CPU.

  Writes back the data cache lines specified by Address and Length. If Address
  is not aligned on a cache line boundary, then entire data cache line
  containing Address is written back. If Address + Length is not aligned on a
  cache line boundary, then the entire data cache line containing Address +
  Length -1 is written back. This function may choose to write back the entire
  data cache if that is more efficient than writing back the specified range.
  If Length is 0, then no data cache lines are written back. This function may
  also invalidate all the data cache lines in the specified range of the cache
  coherency domain of the calling CPU. Address is returned.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the data cache lines to write back. If
                  the CPU is in a physical addressing mode, then Address is a
                  physical address. If the CPU is in a virtual addressing
                  mode, then Address is a virtual address.
  @param  Length  The number of bytes to write back from the data cache.

  @return Address of cache written in main memory.

**/
VOID *
EFIAPI
WriteBackDataCacheRange (
  IN      VOID   *Address,
  IN      UINTN  Length
  )
{
  if (RiscvIsCbcfeEnabledAsm () == RETURN_SUCCESS) {
    CacheOpCacheRange (Address, Length, Clean);
  } else {
    DEBUG ((DEBUG_ERROR, "%a:RISC-V unsupported function.\n", __func__));
  }

  return Address;
}

/**
  Invalidates the entire data cache in cache coherency domain of the calling
  CPU.

  Invalidates the entire data cache in cache coherency domain of the calling
  CPU. This function must be used with care because dirty cache lines are not
  written back to system memory. It is typically used for cache diagnostics. If
  the CPU does not support invalidation of the entire data cache, then a write
  back and invalidate operation should be performed on the entire data cache.

**/
VOID
EFIAPI
InvalidateDataCache (
  VOID
  )
{
  DEBUG (
    (DEBUG_WARN,
     "%a:RISC-V unsupported function.\n"
     "Invalidating the whole instruction cache instead.\n", __func__)
    );
  RiscVInvalidateDataCacheAsm_Fence ();
}

/**
  Invalidates a range of data cache lines in the cache coherency domain of the
  calling CPU.

  Invalidates the data cache lines specified by Address and Length. If Address
  is not aligned on a cache line boundary, then entire data cache line
  containing Address is invalidated. If Address + Length is not aligned on a
  cache line boundary, then the entire data cache line containing Address +
  Length -1 is invalidated. This function must never invalidate any cache lines
  outside the specified range. If Length is 0, then no data cache lines are
  invalidated. Address is returned. This function must be used with care
  because dirty cache lines are not written back to system memory. It is
  typically used for cache diagnostics. If the CPU does not support
  invalidation of a data cache range, then a write back and invalidate
  operation should be performed on the data cache range.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the data cache lines to invalidate. If
                  the CPU is in a physical addressing mode, then Address is a
                  physical address. If the CPU is in a virtual addressing mode,
                  then Address is a virtual address.
  @param  Length  The number of bytes to invalidate from the data cache.

  @return Address.

**/
VOID *
EFIAPI
InvalidateDataCacheRange (
  IN      VOID   *Address,
  IN      UINTN  Length
  )
{
  if (RiscvIsCbiEnabledAsm () == RETURN_SUCCESS) {
    CacheOpCacheRange (Address, Length, Invld);
  } else {
    DEBUG (
      (DEBUG_WARN,
       "%a:RISC-V unsupported function.\n"
       "Invalidating the whole instruction cache instead.\n", __func__)
      );
    InvalidateDataCache ();
  }

  return Address;
}
