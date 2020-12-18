/** @file
  Instance of MM memory check library.

  MM memory check library library implementation. This library consumes MM_ACCESS_PROTOCOL
  to get MMRAM information. In order to use this library instance, the platform should produce
  all MMRAM range via MM_ACCESS_PROTOCOL, including the range for firmware (like MM Core
  and MM driver) and/or specific dedicated hardware.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiMm.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

EFI_MMRAM_DESCRIPTOR *mMmMemLibInternalMmramRanges;
UINTN                mMmMemLibInternalMmramCount;

//
// Maximum support address used to check input buffer
//
EFI_PHYSICAL_ADDRESS  mMmMemLibInternalMaximumSupportAddress = 0;

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibInternalCalculateMaximumSupportAddress (
  VOID
  );

/**
  Initialize cached Mmram Ranges from HOB.

  @retval EFI_UNSUPPORTED   The routine is unable to extract MMRAM information.
  @retval EFI_SUCCESS       MmRanges are populated successfully.

**/
EFI_STATUS
MmMemLibInternalPopulateMmramRanges (
  VOID
  );

/**
  Deinitialize cached Mmram Ranges.

**/
VOID
MmMemLibInternalFreeMmramRanges (
  VOID
  );

/**
  This function check if the buffer is valid per processor architecture and not overlap with MMRAM.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with MMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with MMRAM.
**/
BOOLEAN
EFIAPI
MmIsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  UINTN  Index;

  //
  // Check override.
  // NOTE: (B:0->L:4G) is invalid for IA32, but (B:1->L:4G-1)/(B:4G-1->L:1) is valid.
  //
  if ((Length > mMmMemLibInternalMaximumSupportAddress) ||
      (Buffer > mMmMemLibInternalMaximumSupportAddress) ||
      ((Length != 0) && (Buffer > (mMmMemLibInternalMaximumSupportAddress - (Length - 1)))) ) {
    //
    // Overflow happen
    //
    DEBUG ((
      DEBUG_ERROR,
      "MmIsBufferOutsideMmValid: Overflow: Buffer (0x%lx) - Length (0x%lx), MaximumSupportAddress (0x%lx)\n",
      Buffer,
      Length,
      mMmMemLibInternalMaximumSupportAddress
      ));
    return FALSE;
  }

  for (Index = 0; Index < mMmMemLibInternalMmramCount; Index ++) {
    if (((Buffer >= mMmMemLibInternalMmramRanges[Index].CpuStart) &&
         (Buffer < mMmMemLibInternalMmramRanges[Index].CpuStart + mMmMemLibInternalMmramRanges[Index].PhysicalSize)) ||
        ((mMmMemLibInternalMmramRanges[Index].CpuStart >= Buffer) &&
         (mMmMemLibInternalMmramRanges[Index].CpuStart < Buffer + Length))) {
      DEBUG ((
        DEBUG_ERROR,
        "MmIsBufferOutsideMmValid: Overlap: Buffer (0x%lx) - Length (0x%lx), ",
        Buffer,
        Length
        ));
      DEBUG ((
        DEBUG_ERROR,
        "CpuStart (0x%lx) - PhysicalSize (0x%lx)\n",
        mMmMemLibInternalMmramRanges[Index].CpuStart,
        mMmMemLibInternalMmramRanges[Index].PhysicalSize
        ));
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Copies a source buffer (non-MMRAM) to a destination buffer (MMRAM).

  This function copies a source buffer (non-MMRAM) to a destination buffer (MMRAM).
  It checks if source buffer is valid per processor architecture and not overlap with MMRAM.
  If the check passes, it copies memory and returns EFI_SUCCESS.
  If the check fails, it return EFI_SECURITY_VIOLATION.
  The implementation must be reentrant.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @retval EFI_SECURITY_VIOLATION The SourceBuffer is invalid per processor architecture or overlap with MMRAM.
  @retval EFI_SUCCESS            Memory is copied.

**/
EFI_STATUS
EFIAPI
MmCopyMemToMmram (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  if (!MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)SourceBuffer, Length)) {
    DEBUG ((DEBUG_ERROR, "MmCopyMemToMmram: Security Violation: Source (0x%x), Length (0x%x)\n", SourceBuffer, Length));
    return EFI_SECURITY_VIOLATION;
  }
  CopyMem (DestinationBuffer, SourceBuffer, Length);
  return EFI_SUCCESS;
}

/**
  Copies a source buffer (MMRAM) to a destination buffer (NON-MMRAM).

  This function copies a source buffer (non-MMRAM) to a destination buffer (MMRAM).
  It checks if destination buffer is valid per processor architecture and not overlap with MMRAM.
  If the check passes, it copies memory and returns EFI_SUCCESS.
  If the check fails, it returns EFI_SECURITY_VIOLATION.
  The implementation must be reentrant.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @retval EFI_SECURITY_VIOLATION The DestinationBuffer is invalid per processor architecture or overlap with MMRAM.
  @retval EFI_SUCCESS            Memory is copied.

**/
EFI_STATUS
EFIAPI
MmCopyMemFromMmram (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  if (!MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)DestinationBuffer, Length)) {
    DEBUG ((DEBUG_ERROR, "MmCopyMemFromMmram: Security Violation: Destination (0x%x), Length (0x%x)\n",
            DestinationBuffer, Length));
    return EFI_SECURITY_VIOLATION;
  }
  CopyMem (DestinationBuffer, SourceBuffer, Length);
  return EFI_SUCCESS;
}

/**
  Copies a source buffer (NON-MMRAM) to a destination buffer (NON-MMRAM).

  This function copies a source buffer (non-MMRAM) to a destination buffer (MMRAM).
  It checks if source buffer and destination buffer are valid per processor architecture and not overlap with MMRAM.
  If the check passes, it copies memory and returns EFI_SUCCESS.
  If the check fails, it returns EFI_SECURITY_VIOLATION.
  The implementation must be reentrant, and it must handle the case where source buffer overlaps destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @retval EFI_SECURITY_VIOLATION The DestinationBuffer is invalid per processor architecture or overlap with MMRAM.
  @retval EFI_SECURITY_VIOLATION The SourceBuffer is invalid per processor architecture or overlap with MMRAM.
  @retval EFI_SUCCESS            Memory is copied.

**/
EFI_STATUS
EFIAPI
MmCopyMem (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  if (!MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)DestinationBuffer, Length)) {
    DEBUG ((DEBUG_ERROR, "MmCopyMem: Security Violation: Destination (0x%x), Length (0x%x)\n",
            DestinationBuffer, Length));
    return EFI_SECURITY_VIOLATION;
  }
  if (!MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)SourceBuffer, Length)) {
    DEBUG ((DEBUG_ERROR, "MmCopyMem: Security Violation: Source (0x%x), Length (0x%x)\n", SourceBuffer, Length));
    return EFI_SECURITY_VIOLATION;
  }
  CopyMem (DestinationBuffer, SourceBuffer, Length);
  return EFI_SUCCESS;
}

/**
  Fills a target buffer (NON-MMRAM) with a byte value.

  This function fills a target buffer (non-MMRAM) with a byte value.
  It checks if target buffer is valid per processor architecture and not overlap with MMRAM.
  If the check passes, it fills memory and returns EFI_SUCCESS.
  If the check fails, it returns EFI_SECURITY_VIOLATION.

  @param  Buffer    The memory to set.
  @param  Length    The number of bytes to set.
  @param  Value     The value with which to fill Length bytes of Buffer.

  @retval EFI_SECURITY_VIOLATION The Buffer is invalid per processor architecture or overlap with MMRAM.
  @retval EFI_SUCCESS            Memory is set.

**/
EFI_STATUS
EFIAPI
MmSetMem (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  if (!MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Length)) {
    DEBUG ((DEBUG_ERROR, "MmSetMem: Security Violation: Source (0x%x), Length (0x%x)\n", Buffer, Length));
    return EFI_SECURITY_VIOLATION;
  }
  SetMem (Buffer, Length, Value);
  return EFI_SUCCESS;
}

/**
  The constructor function initializes the Mm Mem library

  @param  [in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param  [in]  MmSystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MemLibConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_MM_SYSTEM_TABLE    *MmSystemTable
  )
{
  EFI_STATUS Status;

  //
  // Calculate and save maximum support address
  //
  MmMemLibInternalCalculateMaximumSupportAddress ();

  //
  // Initialize cached Mmram Ranges from HOB.
  //
  Status = MmMemLibInternalPopulateMmramRanges ();

  return Status;
}

/**
  Destructor for Mm Mem library.

  @param ImageHandle    The image handle of the process.
  @param MmSystemTable  The EFI System Table pointer.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MemLibDestructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_MM_SYSTEM_TABLE    *MmSystemTable
  )
{

  //
  // Deinitialize cached Mmram Ranges.
  //
  MmMemLibInternalFreeMmramRanges ();

  return EFI_SUCCESS;
}
