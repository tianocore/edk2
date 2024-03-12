/** @file
  Standalone MM IPL Header file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _STANDALONE_MM_IPL_PEI_H_
#define _STANDALONE_MM_IPL_PEI_H_

/**
  MM Ipl builds a Handoff Information Table HOB

  @param MemoryBegin     - Start Memory Address.
  @param MemoryLength    - Length of Memory.

  @return EFI_SUCCESS Always success to initialize HOB.

**/
EFI_STATUS
MmIplHobConstructor (
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  );

/**
  Create the MM foundation specific HOB list which StandaloneMm Core needed.

  This function build the MM foundation specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      Buffer            The free buffer to be used for HOB creation.
  @param[in, out] BufferSize        The buffer size.
                                    On return, the expected/used size.

  @retval RETURN_INVALID_PARAMETER  BufferSize is NULL.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for HOB creation.
                                    BufferSize is updated to indicate the expected buffer size.
                                    When the input BufferSize is bigger than the expected buffer size,
                                    the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS            HOB List is created/updated successfully or the input Length is 0.

**/
EFI_STATUS
EFIAPI
CreateMmFoundationHobList (
  IN VOID       *Buffer,
  IN OUT UINTN  *BufferSize
  );

/**
  Builds a end of list HOB.

  This function builds a HOB for end of the HOB list.

  @param  EndOfHobList            End of HOB list address.

**/
VOID
EFIAPI
CreateEndOfList (
  IN EFI_PHYSICAL_ADDRESS  EndOfHobList
  );

#endif
