/** @file
  This file connects Tcg TPM reference code's platform specific functions
  to correspond functions in PlatformTpmLib.
  about each "_plat__XXX" interfaces in TPM reference libraries, see:
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/Platform/src

  When TPM reference library is build, it links with PlatformTpmLib
  which have correspondant interfaces with "_plat_XXX" to "PlatformTpmLibXXX"
  By this, when TPM reference library calls "_plat_XXX",
  it calls "_plat_XXX" in this file and finally this file calls
  "PlatformTpmLibXXX" implemented in PlatformTpmLib.

  That's why this doesn't follow edk2 coding style
  But remain as it is including comment style in TPM reference library
  for clarification.

  Of course, this doesn't defines all _plat_XXX interfaces in TPM
  reference library because interfaces are enough with default implementation
  in TPM reference library.


  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
typedef unsigned char  uint8_t;
typedef int            BOOL;

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

#include <TpmConfiguration/TpmBuildSwitches.h>
#include <CompilerDependencies.h>
#include <ExecCommand_fp.h>
#include <platform_interface/tpm_to_platform_interface.h>

/**
  This function used to generate Endorsement Seed when
  first initailization of TPM.

  @param [in]   Size              Size of endorsement seed
  @param [out]  EndorsementSeed   Endorsement Seed.

**/
LIB_EXPORT void
_plat__GetEPS (
  UINT16   Size,
  uint8_t  *EndorsementSeed
  )
{
  PlatformTpmLibGetEPS (Size, EndorsementSeed);
}

/**
  This function sets current system clock time as t0 for counting TPM time.
  This function is called at a power on event to reset the clock. When the clock
  is reset, the indication that the clock was stopped is also set.

**/
LIB_EXPORT void
_plat__TimerReset (
  void
  )
{
  PlatformTpmLibTimerReset ();
}

/**
  This function should be called in order to simulate the restart of the timer
  should it be stopped while power is still applied.

**/
LIB_EXPORT void
_plat__TimerRestart (
  void
  )
{
  PlatformTpmLibTimerRestart ();
}

/**
  This is another, probably futile, attempt to define a portable function
  that will return a 64-bit clock value that has mSec resolution.

**/
LIB_EXPORT uint64_t
_plat__RealTime (
  void
  )
{
  return PlatformTpmLibRealTime ();
}

/**
 This function provides access to the tick timer of the platform. The TPM code
 uses this value to drive the TPM Clock.

 The tick timer is supposed to run when power is applied to the device. This timer
 should not be reset by time events including _TPM_Init. It should only be reset
 when TPM power is re-applied.

 If the TPM is run in a protected environment, that environment may provide the
 tick time to the TPM as long as the time provided by the environment is not
 allowed to go backwards. If the time provided by the system can go backwards
 during a power discontinuity, then the _plat__Signal_PowerOn should call

**/
LIB_EXPORT uint64_t
_plat__TimerRead (
  void
  )
{
  return PlatformTpmLibTimerRead ();
}

/**
  This function is used to interrogate the flag indicating if the tick timer has
  been reset.

  If the resetFlag parameter is SET, then the flag will be CLEAR before the
  function returns.

**/
LIB_EXPORT int
_plat__TimerWasReset (
  void
  )
{
  return PlatformTpmLibTimerWasReset ();
}

/**
  This function is used to interrogate the flag indicating if the tick timer has
  been stopped. If so, this is typically a reason to roll the nonce.

  This function will CLEAR the s_timerStopped flag before returning. This provides
  functionality that is similar to status register that is cleared when read. This
  is the model used here because it is the one that has the most impact on the TPM
  code as the flag can only be accessed by one entity in the TPM. Any other
  implementation of the hardware can be made to look like a read-once register.

**/
LIB_EXPORT BOOL
_plat__TimerWasStopped (
  void
  )
{
  return (BOOL)PlatformTpmLibTimerWasStopped ();
}

/**
 ClockRateAdjust uses predefined signal values and encapsulates the platform
 specifics regarding the number of ticks the underlying clock is running at.

 The adjustment must be one of these values. A COARSE adjustment is 1%, MEDIUM
 is 0.1%, and FINE is the smallest amount supported by the platform.  The
 total (cumulative) adjustment is limited to ~15% total.  Attempts to adjust
 the clock further are silently ignored as are any invalid values.  These
 values are defined here to insulate them from spec changes and to avoid
 needing visibility to the doc-generated structure headers.

 @param [in] Adjust  The adjust number.

**/
LIB_EXPORT void
_plat__ClockRateAdjust (
  _plat__ClockAdjustStep  adjust
  )
{
  PlatformTpmLibClockAdjustRate (adjust);
}

/**
  _plat__GetEntropy()

  This function is used to get available hardware entropy. In a hardware
  implementation of this function, there would be no call to the system
  to get entropy.

  @param [out] Entropy
  @param [in]  Amount    amount reuqested.

  @return < 0   Failed to generate entropy
  @return >= 0  The returned amount of entropy (bytes)

**/
LIB_EXPORT int32_t
_plat__GetEntropy (
  unsigned char  *entropy,                                    // output buffer
  uint32_t       amount                                       // amount requested
  )
{
  return PlatformTpmLibGetEntropy (entropy, amount);
}

/**
  Enable NV memory.

  the NV state would be read in, decrypted and integrity checked if needs.

  The recovery from an integrity failure depends on where the error occurred. It
  it was in the state that is discarded by TPM Reset, then the error is
  recoverable if the TPM is reset. Otherwise, the TPM must go into failure mode.

  @param [in] PlatParameter   Platform parameter to enable NV storage
  @param [in] ParamSize

  @return 0        if success
  @return > 0      if receive recoverable error
  @return < 0      if unrecoverable error

**/
LIB_EXPORT int
_plat__NVEnable (
  void    *platParameter, // IN: platform specific parameters
  size_t  paramSize
  )
{
  return PlatformTpmLibNVEnable (platParameter, paramSize);
}

/**
  Disable NV memory.

**/
LIB_EXPORT void
_plat__NVDisable (
  VOID
  )
{
  PlatformTpmLibNVDisable ();
}

/**
  Check if NV is available

  @return    0               NV is available
  @return    1               NV is not available due to write failure
  @return    2               NV is not available due to rate limit

**/
LIB_EXPORT int
_plat__GetNvReadyState (
  void
  )
{
  return PlatformTpmLibGetNvReadyState ();
}

/**
  Read a chunk of NV memory

  @param [in] StartOffset   Read start offset
  @param [in] Size          Size to read
  @param [out] Data         Data buffer

  @return 1 Success to read
  @return 0 Failed to read

**/
LIB_EXPORT int
_plat__NvMemoryRead (
  unsigned int  startOffset,                            // IN: read start
  unsigned int  size,                                   // IN: size of bytes to read
  void          *data                                   // OUT: data buffer
  )
{
  return PlatformTpmLibNvMemoryRead (
           startOffset,
           size,
           data
           );
}

/**
  This function checks to see if the NV is different from the test value
  so that NV will not be written if it has not changed.

  @param [in] StartOffset Start offset to compare
  @param [in] Size        Size for compare
  @param [in] Data        Data to be compared

  @return NV_HAS_CHANGED(1)       the NV location is different from the test value
  @return NV_IS_SAME(0)           the NV location is the same as the test value
  @return NV_INVALID_LOCATION(-1) the NV location is invalid; also triggers failure mode

**/
LIB_EXPORT int
_plat__NvGetChangedStatus (
  unsigned int  startOffset,                            // IN: read start
  unsigned int  size,                                   // IN: size of bytes to read
  void          *data                                   // IN: data buffer
  )
{
  return PlatformTpmLibNvGetChangedStatus (
           startOffset,
           size,
           data
           );
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
LIB_EXPORT int
_plat__NvMemoryWrite (
  unsigned int  startOffset,                            // IN: write start
  unsigned int  size,                                   // IN: size of bytes to write
  void          *data                                   // OUT: data buffer
  )
{
  return PlatformTpmLibNvMemoryWrite (
           startOffset,
           size,
           data
           );
}

/**
  Function is used to set a range of NV memory bytes to an implementation-dependent
  value. The value represents the erase state of the memory.

  @param [in] StartOffset   Start offset to clear
  @param [in] SIze          Size to be clear


  @return 0 Failed to clear
  @return 1 Success

**/
LIB_EXPORT int
_plat__NvMemoryClear (
  unsigned int  start,   // IN: clear start
  unsigned int  size     // IN: number of bytes to clear
  )
{
  return PlatformTpmLibNvMemoryClear (
           start,
           size
           );
}

/**
  Move a chunk of NV memory from source to destination
  This function should ensure that if there overlap, the original data is
  copied before it is written

  @param [in] SourceOffset  Source offset to move
  @param [in] DestOffset    Destination offset to move
  @param [in] Size          Size to be moved

  @return 0 Failed to move
  @return 1 Success

**/
LIB_EXPORT int
_plat__NvMemoryMove (
  unsigned int  sourceOffset,   // IN: source offset
  unsigned int  destOffset,     // IN: destination offset
  unsigned int  size            // IN: size of data being moved
  )
{
  return PlatformTpmLibNvMemoryMove (
           sourceOffset,
           destOffset,
           size
           );
}

/**
  This function writes the local copy of NV to NV for permanent store.
  It will write TPM_NV_MEMORY_SIZE bytes to NV.

   @return 0       NV write success
   @return non-0   NV write fail

**/
LIB_EXPORT int
_plat__NvCommit (
  void
  )
{
  return PlatformTpmLibNvCommit ();
}

/**
   Set the current NV state to available.
   This function is for testing purpose only.
   It is not part of the platform NV logic

**/
LIB_EXPORT void
_plat__SetNvAvail (
  void
  )
{
  PlatformTpmLibSetNvAvail ();
}

/**
  Set the current NV state to unavailable.
  This function is for testing purpose only.
  It is not part of the platform NV logic

**/
LIB_EXPORT void
_plat__ClearNvAvail (
  void
  )
{
  PlatformTpmLibClearNvAvail ();
}

/**
  _plat__NVNeedsManufacture()

  This function checks whether TPM's NV state needs to be manufactured

**/
LIB_EXPORT int
_plat__NVNeedsManufacture (
  void
  )
{
  return (int)PlatformTpmLibNVNeedsManufacture ();
}

/**
  Excecute TPM command

  @param[in]    requestSize    command buffer size
  @param[in]    request        command buffer
  @param[inout] responseSize   buffer size
  @param[inout] response       buffer

**/
LIB_EXPORT void
_plat__RunCommand (
  uint32_t       requestSize,
  unsigned char  *request,
  uint32_t       *responseSize,
  unsigned char  **response
  )
{
  ExecuteCommand (
    requestSize,
    request,
    responseSize,
    response
    );
}

/**
  This function is used to access the platform-specific unique value.
  This function places the unique value in the provided buffer ('b')
  and returns the number of bytes transferred. The function will not
  copy more data than 'bSize'.

  NOTE: If a platform unique value has unequal distribution of uniqueness
  and 'bSize' is smaller than the size of the unique value, the 'bSize'
  portion with the most uniqueness should be returned.

  @param [in] Which    if == 0, Copy unique value from start
                       otherwise, copy from end
  @param [in] Size     Size of Buffer
  @param [out] Buffer

  @return Size of unique value.

**/
LIB_EXPORT uint32_t
_plat__GetUnique (
  uint32_t       which,                                // authorities (0) or details
  uint32_t       bSize,                                // size of the buffer
  unsigned char  *b                                    // output buffer
  )
{
  return PlatformTpmLibGetUnique (
           which,
           bSize,
           b
           );
}

/**
  return the 4 character Manufacturer Capability code.
  This should come from the platform library since
  that is provided by the manufacturer.


  @return               Manufacturer capability Code.

**/
LIB_EXPORT uint32_t
_plat__GetManufacturerCapabilityCode (
  void
  )
{
  return PlatformTpmLibGetManufacturerCapabilityCode ();
}

/**
  _plat__GetVendorCapabilityCode()

  return the 4 character VendorStrings for Capabilities.
  Index is ONE-BASED, and may be in the range [1,4] inclusive.
  Any other index returns all zeros. The return value will be interpreted
  as an array of 4 ASCII characters (with no null terminator).

  @param[in] Index      index

  @return               Vendor specific capability code.

**/
LIB_EXPORT uint32_t
_plat__GetVendorCapabilityCode (
  int  index
  )
{
  return PlatformTpmLibGetVendorCapabilityCode (index);
}

/**
  return the most-significant 32-bits of the TPM Firmware Version

  @return               High 32-bits of TPM Firmware Version.

**/
LIB_EXPORT uint32_t
_plat__GetTpmFirmwareVersionHigh (
  void
  )
{
  return PlatformTpmLibGetTpmFirmwareVersionHigh ();
}

/**
  return the least-significant 32-bits of the TPM Firmware Version

  @return               Low 32-bits of TPM Firmware Version.

**/
LIB_EXPORT uint32_t
_plat__GetTpmFirmwareVersionLow (
  void
  )
{
  return PlatformTpmLibGetTpmFirmwareVersionLow ();
}

/**
  return the TPM Firmware current SVN.

  @return               current SVN.

**/
LIB_EXPORT uint16_t
_plat__GetTpmFirmwareSvn (
  void
  )
{
  return PlatformTpmLibGetTpmFirmwareSvn ();
}

/**
  return the TPM Firmware maximum SVN.

  @return               Maximum SVN.

**/
LIB_EXPORT uint16_t
_plat__GetTpmFirmwareMaxSvn (
  void
  )
{
  return PlatformTpmLibGetTpmFirmwareMaxSvn ();
}

/**
  return the TPM Firmware SVN Secret value associated with SVN

  @param[in]            Svn                     Svn
  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
LIB_EXPORT int
_plat__GetTpmFirmwareSvnSecret (
  uint16_t  svn,
  uint16_t  secret_buf_size,
  uint8_t   *secret_buf,
  uint16_t  *secret_size
  )
{
  return PlatformTpmLibGetTpmFirmwareSvnSecret (
           svn,
           secret_buf_size,
           secret_buf,
           secret_size
           );
}

/**
  return the TPM Firmware Secret value associated with SVN

  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
LIB_EXPORT int
_plat__GetTpmFirmwareSecret (
  uint16_t  secret_buf_size,
  uint8_t   *secret_buf,
  uint16_t  *secret_size
  )
{
  return PlatformTpmLibGetTpmFirmwareSecret (
           secret_buf_size,
           secret_buf,
           secret_size
           );
}

/**
  This function allows the platform to provide a small amount of data to be
  stored as part of the TPM's PERSISTENT_DATA structure during manufacture.
  Of course the platform can store data separately as well,
  but this allows a simple platform implementation to store
  a few bytes of data without implementing a multi-layer storage system.
  This function is called on manufacture and CLEAR.
  The buffer will contain the last value provided
  to the Core library.

  @param[out]  PlatformPersistentData          Platform data.
  @param[in]   Size                            Size of PlatformPersistentData.

**/
LIB_EXPORT void
_plat__GetPlatformManufactureData (
  uint8_t   *pPlatformPersistentData,
  uint32_t  bufferSize
  )
{
  return PlatformTpmLibGetPlatformManufactureData (
           pPlatformPersistentData,
           bufferSize
           );
}

/**
  Reset platform specific failure data.

**/
void
_plat_internal_resetFailureData (
  void
  )
{
  return PlatformTpmLibInternalResetFailureData ();
}

/**
  Check whether platform is in the failure mode.

**/
LIB_EXPORT BOOL
_plat__InFailureMode (
  void
  )
{
  return (BOOL)PlatformTpmLibInFailureMode ();
}

/**
  A function for the TPM to call the platform to indicate the
  TPM code has detected a failure.

**/
LIB_EXPORT NORETURN void
_plat__Fail (
  const char  *function,
  int         line,
  uint64_t    locationCode,
  int         failureCode
  )
{
  PlatformTpmLibFail (function, line, locationCode, failureCode);

  /*
   * for NORETURN
   */
  while (1) {
  }
}

/**
  Get last failure code.

  @return Last failure code.

**/
LIB_EXPORT UINT32
_plat__GetFailureCode (
  void
  )
{
  return PlatformTpmLibGetFailureCode ();
}

/**
  _plat__GetFailureLocation()

  Get last failure location.

  @return Last failure location code.

**/
LIB_EXPORT uint64_t
_plat__GetFailureLocation (
  void
  )
{
  return PlatformTpmLibGetFailureLocation ();
}

/**
  Get last function name where failed.

  @return Last function name where failed.

**/
LIB_EXPORT const char *
_plat__GetFailureFunctionName (
  void
  )
{
  return PlatformTpmLibGetFailureFunctionName ();
}

/**
  Get last failure line.

  @return Last line number where failed.

**/
LIB_EXPORT uint32_t
_plat__GetFailureLine (
  void
  )
{
  return PlatformTpmLibGetFailureLine ();
}

/**
  return the Platform TPM type.

  @return               TPM type.

**/
LIB_EXPORT uint32_t
_plat__GetVendorTpmType (
  void
  )
{
  return PlatformTpmLibGetVendorTpmType ();
}

/**
  return TPM spec information.
  This is copied from TPM/TPMCmd/Platform/src/VendorInfo.c

  @param[out] returnData  Spec information.

**/
LIB_EXPORT void
_plat_GetSpecCapabilityValue (
  SPEC_CAPABILITY_VALUE  *returnData
  )
{
  // clang-format off
  // this is on the title page of part1 of the TPM spec
  returnData->tpmSpecLevel = 0;
  // these come from part2 of the TPM spec
  returnData->tpmSpecVersion   = 184;
  returnData->tpmSpecYear      = 2025;
  returnData->tpmSpecDayOfYear = 79;    // March 20
  // these come from the PC Client Platform TPM Profile Specification
  returnData->platformFamily = 1;
  returnData->platfromLevel  = 0;
  // The platform spec version is recorded such that 0x00000101 means version 1.01
  // Note this differs from some TPM/TCG specifications, but matches the behavior of Windows.
  // more recent TCG specs have discontinued using this field, but Windows displays it, so we
  // retain it using the historical encoding.
  returnData->platformRevision  = 0x105;
  returnData->platformYear      = 0;
  returnData->platformDayOfYear = 0;
  // clang-format on
  return;
}

/**
  TpmBuildSwitch.h which includes in tpm_to_platform_interface.h
  defines its own DEBUG macro to compile TPM.
  So, undef DEBUG macro defined by TpmBuildSwitch.h
  and use DEBUG macro defined in DebugLib.h

**/
#undef DEBUG
#include <Library/DebugLib.h>

#if CERTIFYX509_DEBUG

/**
  This function opens the file used to hold the debug data.

  @retval  0             Success
  @retval  != 0          Error
**/
INT32
EFIAPI
DebugFileInit (
  VOID
  )
{
  return 0;
}

/**
  Dump Buffer

  @param[in] size        Size of buf
  @param[in] buf         Buffer
  @param[in] identifier  Print identifier

**/
VOID
EFIAPI
DebugDumpBuffer (
  IN INT32          size,
  IN unsigned char  *buf,
  IN CONST CHAR8    *identifier
  )
{
  INT32  Idx;

  DEBUG ((DEBUG_INFO, "%a\n", identifier));

  for (Idx = 0; Idx < size; Idx++) {
    if (((Idx % 16) == 0) && (Idx != 0)) {
      DEBUG ((DEBUG_INFO, "\n"));
    }

    DEBUG ((DEBUG_INFO, "%02x\n", buf[Idx]));
  }

  if ((size % 16) != 0) {
    DEBUG ((DEBUG_INFO, "\n"));
  }
}

#endif // CERTIFYX509_DEBUG
