/** @file
  This file connects the platform specific functions of
  the TCG TPM reference code to the corresponding functions in PlatformTpmLib.

  To use TPM reference code as software based TPM, each Platform should
  implements TpmPlatformLib.

  For the __plat_XXX interface, Please see the:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/tpm/include/platform_interface

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// ClockRateAdjust uses predefined signal values and encapsulates the platform
// specifics regarding the number of ticks the underlying clock is running at.
//
// The adjustment must be one of these values. A COARSE adjustment is 1%, MEDIUM
// is 0.1%, and FINE is the smallest amount supported by the platform.  The
// total (cumulative) adjustment is limited to ~15% total.  Attempts to adjust
// the clock further are silently ignored as are any invalid values.  These
// values are defined here to insulate them from spec changes and to avoid
// needing visibility to the doc-generated structure headers.
typedef enum {
  TpmClockAdjustCoarseSlower = -3,
  TpmClockAdjustMediumSlower = -2,
  TpmClockAdjustFineSlower   = -1,
  TpmClockAdjustFineFaster   = 1,
  TpmClockAdjustMediumFaster = 2,
  TpmClockAdjustCoarseFaster = 3
} PLATFORM_TPM_CLOCK_ADJUST_STEP;

#define PLATFORM_TPM_TYPE_UNKNOWN        0x00
#define PLATFORM_TPM_TYPE_DISCRETE_CHIP  0x01
#define PLATFORM_TPM_TYPE_INTEGRATE_SOC  0x02
#define PLATFORM_TPM_TYPE_FTPM           0x03
#define PLATFORM_TPM_TYPE_VTPM           0x04

// ** From EPS.c

/**
  _plat__GetEPS()

  This function used to generate Endorsement Seed when
  first initailization of TPM.

   EPS can be generated, for example, as follows:

    EPS = SHA-512(TRNG_output || nonce || optional_mixing || DeviceUnique)

  Alternatively, EPS can be generated using DRBG_Generate(),
  as done in the TCG TPM 2.0 reference implementation.

  This function is not expected to fail; however,
  if it does have the potential to fail,
  the platform should handle the failure appropriately,
  for example by disabling TPM functionality or
  retrying the manufacturing process.

  @param [in]   Size              Size of endorsement seed
  @param [out]  EndorsementSeed   Endorsement Seed.

**/
VOID
EFIAPI
PlatformTpmLibGetEPS (
  IN  UINT16  Size,
  OUT UINT8   *EndorsementSeed
  );

// ** From Clock.c

/**
  _plat__TimerReset()

  This function sets current system clock time as t0 for counting TPM time.
  This function is called at a power on event to reset the clock. When the clock
  is reset, the indication that the clock was stopped is also set.

**/
VOID
EFIAPI
PlatformTpmLibTimerReset (
  VOID
  );

/**
  _plat__TimerRestart()

  This function should be called in order to simulate the restart of the timer
  should it be stopped while power is still applied.

**/
VOID
EFIAPI
PlatformTpmLibTimerRestart (
  VOID
  );

/**
  _plat__RealTime()

  This is another, probably futile, attempt to define a portable function
  that will return a 64-bit clock value that has mSec resolution.

  @return   value   realtime

**/
UINT64
EFIAPI
PlatformTpmLibRealTime (
  VOID
  );

/**
  _plat__TimerRead()

  This function provides access to the tick timer of the platform. The TPM code
  uses this value to drive the TPM Clock.

  The tick timer is supposed to run when power is applied to the device. This timer
  should not be reset by time events including _TPM_Init. It should only be reset
  when TPM power is re-applied.

  If the TPM is run in a protected environment, that environment may provide the
  tick time to the TPM as long as the time provided by the environment is not
  allowed to go backwards. If the time provided by the system can go backwards
  during a power discontinuity, then the _plat__Signal_PowerOn should call
  _plat__TimerReset().

  @return value timeval

**/
UINT64
EFIAPI
PlatformTpmLibTimerRead (
  VOID
  );

/**
  _plat__TimerWasReset()

  This function is used to interrogate the flag indicating if the tick timer has
  been reset.

  If the resetFlag parameter is SET, then the flag will be CLEAR before the
  function returns.

  @return TRUE    Timer was reset.
  @return FALSE   Timer wasn't reset.

**/
INT32
EFIAPI
PlatformTpmLibTimerWasReset (
  VOID
  );

/**
  _plat__TimerWasStopped()

  This function is used to interrogate the flag indicating if the tick timer has
  been stopped. If so, this is typically a reason to roll the nonce.

  This function will CLEAR the s_timerStopped flag before returning. This provides
  functionality that is similar to status register that is cleared when read. This
  is the model used here because it is the one that has the most impact on the TPM
  code as the flag can only be accessed by one entity in the TPM. Any other
  implementation of the hardware can be made to look like a read-once register.

  @return TRUE    timer was stopped
  @return FALSE   timer wasn't stopped

**/
BOOLEAN
EFIAPI
PlatformTpmLibTimerWasStopped (
  VOID
  );

/**
  _plat__ClockAdjustRate()

  Adjust the clock rate.

  @param [in] Adjust  The adjust number. It could be positive

**/
VOID
EFIAPI
PlatformTpmLibClockAdjustRate (
  IN INT32  Adjust
  );

/**
  _plat__GetEntropy()

  This function is used to get available hardware entropy. In a hardware
  implementation of this function, there would be no call to the system
  to get entropy.

  @param [out] Entropy   output buffer
  @param [in]  Amount    amount reuqested.

  @return < 0   Failed to generate entropy
  @return >= 0  The returned amount of entropy (bytes)

**/
INT32
EFIAPI
PlatformTpmLibGetEntropy (
  OUT UINT8   *Entropy,
  IN  UINT32  Amount
  );

// ** From NVMem.c

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
  );

/**
  _plat__NVDisable()

  Disable NV memory.

**/
VOID
EFIAPI
PlatformTpmLibNVDisable (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  _plat__SetNvAvail()

   Set the current NV state to available.
   This function is for testing purpose only.
   It is not part of the platform NV logic

**/
VOID
EFIAPI
PlatformTpmLibSetNvAvail (
  VOID
  );

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
  );

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
  );

// ** From Unique.c

/**
  _plat__GetUnique()

  This function is used to access the platform-specific unique value.
  This function places the unique value in the provided Buffer ('b')
  and returns the number of bytes transferred. The function will not
  copy more data than 'Size'.

  NOTE: If a platform unique value has unequal distribution of uniqueness
  and 'Size' is smaller than the size of the unique value, the 'Size'
  portion with the most uniqueness should be returned.

  @param [in]  Which    if == 0, Copy unique value from start
                        otherwise, copy from end
  @param [in]  Size     Size of Buffer
  @param [out] Buffer   Output Buffer

  @return Size of unique value.

**/
UINT32
EFIAPI
PlatformTpmLibGetUnique (
  IN  UINT32  Which,
  IN  UINT32  Size,
  OUT UINT8   *Buffer
  );

/**
  _plat__GetManufacturerCapabilityCode()

  return the 4 character Manufacturer Capability code.
  This should come from the platform library since
  that is provided by the manufacturer.


  @return               Manufacturer capability Code.

**/
UINT32
EFIAPI
PlatformTpmLibGetManufacturerCapabilityCode (
  VOID
  );

/**
  _plat__GetVendorCapabilityCode()

  return the 4 character VendorStrings for Capabilities.
  Index is ONE-BASED, and may be in the range [1,4] inclusive.
  Any other index returns all zeros. The return value will be interpreted
  as an array of 4 ASCII characters (with no null terminator).

  @param[in] Index      index

  @return               Vendor specific capability code.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorCapabilityCode (
  IN INT32  Index
  );

/**
  _plat__GetTpmFirmwareVersionHigh()

  return the most-significant 32-bits of the TPM Firmware Version

  @return               High 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionHigh (
  VOID
  );

/**
  _plat__GetTpmFirmwareVersionLow()

  return the least-significant 32-bits of the TPM Firmware Version

  @return               Low 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionLow (
  VOID
  );

/**
  _plat__GetTpmFirmwareSvn()

  return the TPM Firmware current SVN.

  @return               current SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareSvn (
  VOID
  );

/**
  _plat__GetTpmFirmwareMaxSvn()

  return the TPM Firmware maximum SVN.

  @return               Maximum SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareMaxSvn (
  VOID
  );

/**
  _plat__GetTpmFirmwareSvnSecret()

  return the TPM Firmware SVN Secret value associated with SVN

  @param[in]            Svn                     Svn
  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSvnSecret (
  IN  UINT16  Svn,
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  );

/**
  _plat__GetTpmFirmwareSecret()

  return the TPM Firmware Secret value associated with SVN

  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSecret (
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  );

/**
  _plat__GetVendorTpmType()

  return the Platform TPM type.

  @return               TPM type.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorTpmType (
  VOID
  );

/**
  _plat__GetPlatformManufactureData

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
VOID
EFIAPI
PlatformTpmLibGetPlatformManufactureData (
  UINT8   *PlatformPersistentData,
  UINT32  Size
  );

/**
  _plat_internal_resetFailureData()

  Reset platform specific failure data.

**/
VOID
EFIAPI
PlatformTpmLibInternalResetFailureData (
  VOID
  );

/**
  _plat__InFailureMode()

  Check whether platform is in the failure mode.

  @return whether TPM is in failure mode or not

**/
BOOLEAN
EFIAPI
PlatformTpmLibInFailureMode (
  VOID
  );

/**
  _plat__Fail()

  This is the platform depended failure exit for the TPM.

  @param[in]  Function       Function name where failure happens.
  @param[in]  Line           line number where failure happens.
  @param[in]  LocationCode   Location code where failure happens.
  @param[in]  FailureCode    Fail reson.

**/
VOID
EFIAPI
PlatformTpmLibFail (
  IN CONST CHAR8  *Function,
  IN INT32        Line,
  IN UINT64       LocationCode,
  IN INT32        FailureCode
  );

/**
  _plat__GetFailureCode()

  Get last failure code.

  @return Last failure code.

**/
UINT32
EFIAPI
PlatformTpmLibGetFailureCode (
  VOID
  );

/**
  _plat__GetFailureLocation()

  Get last failure location.

  @return Last failure location code.

**/
UINT64
EFIAPI
PlatformTpmLibGetFailureLocation (
  VOID
  );

/**
  _plat__GetFailureFunctionName()

  Get last function name where failed.

  @return Last function name where failed.

**/
CONST CHAR8 *
EFIAPI
PlatformTpmLibGetFailureFunctionName (
  VOID
  );

/**
  _plat__GetFailureLine()

  Get last failure line.

  @return Last line number where failed.

**/
UINT32
EFIAPI
PlatformTpmLibGetFailureLine (
  VOID
  );

/**
  This function is called in TpmLibConstructor() to
  initialise PlatformTpmLib once.

  @return EFI_SUCCESS   Success
  @return Others        Errors

**/
EFI_STATUS
EFIAPI
PlatformTpmLibInit (
  VOID
  );
