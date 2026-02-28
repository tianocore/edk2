/** @file
  Tpm Nv Storage part of PlatformTpmLib to use TpmLib.

  To see the plat_XXX interfaces in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__NVNeedsManufacture()

  This function checks whether TPM's NV state needs to be manufactured.

  @return TRUE    TPM's NV storage should be manufactured
  @return FALSE   TPM's NV storage is already manufactured

**/
BOOLEAN
EFIAPI
PlatformTpmLibNVNeedsManufacture (
  VOID
  )
{
  return FALSE;
}

/**
  _plat__NVEnable()

  Enable NV memory.

  the NV state would be read in, decrypted and integrity checked if needs.

  The recovery from an integrity failure depends on where the error occurred. It
  it was in the state that is discarded by TPM Reset, then the error is
  recoverable if the TPM is reset. Otherwise, the TPM must go into failure mode.

  @param [in] PlatParameter   Platform parameter to enable NV storage
  @param [in] ParamSize       Size of PlatParameter

  @return 0        if success
  @return > 0      if receive recoverable error
  @return < 0      if unrecoverable error

**/
INT32
EFIAPI
PlatformTpmLibNVEnable (
  IN VOID   *PlatParameter,
  IN UINTN  ParamSize
  )
{
  return -1;
}

/**
  _plat__NVDisable()

  Disable NV memory.

**/
VOID
EFIAPI
PlatformTpmLibNVDisable (
  VOID
  )
{
  return;
}

/**
  _plat__GetNvReadyState()

  Check if NV is available.

  @return    0               NV is available
  @return    1               NV is not available due to write failure
  @return    2               NV is not available due to rate limit

**/
INT32
EFIAPI
PlatformTpmLibGetNvReadyState (
  VOID
  )
{
  return 1;
}

/**
  _plat__NvMemoryRead()

  Read a chunk of NV memory.

  @param [in] StartOffset   Read start offset
  @param [in] Size          Size to read
  @param [out] Data         Data buffer

  @return 1 Success to read
  @return 0 Failed to read

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryRead (
  IN  UINT32  StartOffset,
  IN  UINT32  Size,
  OUT VOID    *Data
  )
{
  return 0;
}

/**
  _plat__NvGetChangedStatus()

  This function checks to see if the NV is different from the test value
  so that NV will not be written if it has not changed.

  @param [in] StartOffset Start offset to compare
  @param [in] Size        Size for compare
  @param [in] Data        Data to be compared

  @return NV_HAS_CHANGED(1)       the NV location is different from the test value
  @return NV_IS_SAME(0)           the NV location is the same as the test value
  @return NV_INVALID_LOCATION(-1) the NV location is invalid; also triggers failure mode
**/
INT32
EFIAPI
PlatformTpmLibNvGetChangedStatus (
  IN  UINT32  StartOffset,
  IN  UINT32  Size,
  IN  VOID    *Data
  )
{
  return -1;
}

/**
  _plat__NvMemoryWrite()

  This function is used to update NV memory. The "write" is to a memory copy of
  NV. At the end of the current command, any changes are written to
  the actual NV memory.

  NOTE: A useful optimization would be for this code to compare the current
  contents of NV with the local copy and note the blocks that have changed. Then
  only write those blocks when _plat__NvCommit() is called.

  @param [in] StartOffset   Start Offset to write
  @param [in] Size          Size to wrrite
  @param [in] Data          Data

  @return    0               Failed to write
  @return    1               Success to write

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryWrite (
  IN  UINT32  StartOffset,
  IN  UINT32  Size,
  IN  VOID    *Data
  )
{
  return 0;
}

/**
  _plat__NvMemoryClear()

  Function is used to set a range of NV memory bytes to an implementation-dependent
  value. The value represents the erase state of the memory.

  @param [in] StartOffset   Start offset to clear
  @param [in] SIze          Size to be clear


  @return 0 Failed to clear
  @return 1 Success

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryClear (
  IN  UINT32  StartOffset,
  IN  UINT32  Size
  )
{
  return 0;
}

/**
  _plat__NvMemoryMove()

  Function: Move a chunk of NV memory from source to destination
  This function should ensure that if there overlap, the original data is
  copied before it is written.

  @param [in] SourceOffset  Source offset to move
  @param [in] DestOffset    Destination offset to move
  @param [in] Size          Size to be moved

  @return 0 Failed to move
  @return 1 Success

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryMove (
  IN   UINT32  SourceOffset,
  IN   UINT32  DestOffset,
  IN   UINT32  Size
  )
{
  return 0;
}

/**
  _plat__NvCommit()

  This function writes the local copy of NV to NV for permanent store.
  It will write TPM_NV_MEMORY_SIZE bytes to NV.

   @return 0       NV write success
   @return non-0   NV write fail

**/
INT32
EFIAPI
PlatformTpmLibNvCommit (
  VOID
  )
{
  return -1;
}

/**
  _plat__SetNvAvail()

   Set the current NV state to available.
   This function is for testing purpose only.
   It is not part of the platform NV logic.

**/
VOID
EFIAPI
PlatformTpmLibSetNvAvail (
  VOID
  )
{
  // NV will not be made unavailable on this platform
  return;
}

/**
  _plat__ClearNvAvail()

  Set the current NV state to unavailable.
  This function is for testing purpose only.
  It is not part of the platform NV logic.

**/
VOID
EFIAPI
PlatformTpmLibClearNvAvail (
  VOID
  )
{
  // The anti-set; not on this platform.
  return;
}
