/** @file
The EFI_SWAP_ADDRESS_RANGE_PROTOCOL is used to abstract the swap operation of boot block
and backup block of FV. This swap is especially needed when updating the boot block of FV. If a
power failure happens during the boot block update, the swapped backup block (now the boot block)
can boot the machine with the old boot block backed up in it. The swap operation is platform dependent, so
other protocols such as FTW (Fault Tolerant Write) should use this protocol instead of handling hardware directly.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_SWAP_ADDRESS_RANGE_PROTOCOL_H_
#define _EFI_SWAP_ADDRESS_RANGE_PROTOCOL_H_

#define EFI_SWAP_ADDRESS_RANGE_PROTOCOL_GUID \
  { \
    0x1259f60d, 0xb754, 0x468e, {0xa7, 0x89, 0x4d, 0xb8, 0x5d, 0x55, 0xe8, 0x7e } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_SWAP_ADDRESS_RANGE_PROTOCOL  EFI_SWAP_ADDRESS_RANGE_PROTOCOL;

#define EFI_UNSUPPORT_LOCK  0
#define EFI_SOFTWARE_LOCK   1
#define EFI_HARDWARE_LOCK   2

typedef UINT8 EFI_SWAP_LOCK_CAPABILITY;

//
// Protocol APIs
//

/**
  This function gets the address range location of
  boot block and backup block.

  @param This             Indicates the calling context.
  @param BootBlockBase    The base address of current boot block.
  @param BootBlockSize    The size (in bytes) of current boot block.
  @param BackupBlockBase  The base address of current backup block.
  @param BackupBlockSize  The size (in bytes) of current backup block.

  @retval EFI_SUCCESS  The call was successful.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_RANGE_LOCATION)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  OUT EFI_PHYSICAL_ADDRESS                      *BootBlockBase,
  OUT UINTN                                     *BootBlockSize,
  OUT EFI_PHYSICAL_ADDRESS                      *BackupBlockBase,
  OUT UINTN                                     *BackupBlockSize
  );

/**
  This service checks if the boot block and backup block has been swapped.

  @param This          Indicates the calling context.
  @param SwapState     True if the boot block and backup block has been swapped.
                       False if the boot block and backup block has not been swapped.

  @retval EFI_SUCCESS  The call was successful.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_SWAP_STATE)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  OUT BOOLEAN                                   *SwapState
  );

/**
  This service swaps the boot block and backup block, or swaps them back.

  It also acquires and releases software swap lock during operation. The setting of the new swap state
  is not affected by the old swap state.

  @param This            Indicates the calling context.
  @param NewSwapState    True to swap real boot block and backup block, False to swap them back.

  @retval EFI_SUCCESS  The call was successful.
  @retval EFI_ABORTED  Set swap state error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SET_SWAP_STATE)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  IN BOOLEAN                                    NewSwapState
  );



/**
  This service checks if a Real Time Clock (RTC) power failure happened.

  If parameter RtcPowerFailed is true after the function returns, RTC power supply failed or was removed.
  It is recommended to check RTC power status before calling GetSwapState().

  @param This             Indicates the calling context.
  @param RtcPowerFailed   True if the RTC (Real Time Clock) power failed or was removed.

  @retval EFI_SUCCESS The call was successful.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_RTC_POWER_STATUS)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  OUT BOOLEAN                                   *RtcPowerFailed
  );

/**
  This service returns all lock methods for swap operations that the current platform
  supports. Could be software lock, hardware lock, or unsupport lock.
  Note that software and hardware lock methods can be used simultaneously.

  @param This             Indicates the calling context.
  @param LockCapability   The current lock method for swap operations.

  @retval EFI_SUCCESS The call was successful.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_SWAP_LOCK_CAPABILITY)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  OUT EFI_SWAP_LOCK_CAPABILITY                  *LockCapability
  );



/**
  This service is used to acquire or release appointed kind of lock for Swap Address Range operations.

  Note that software and hardware lock mothod can be used simultaneously.

  @param This              Indicates the calling context.
  @param LockCapability    Indicates which lock to acquire or release.
  @param NewLockState      True to acquire lock; False to release lock.

  @retval EFI_SUCCESS The call was successful.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SET_SWAP_LOCK)(
  IN EFI_SWAP_ADDRESS_RANGE_PROTOCOL            *This,
  IN EFI_SWAP_LOCK_CAPABILITY                   LockCapability,
  IN BOOLEAN                                    NewLockState
  );

struct _EFI_SWAP_ADDRESS_RANGE_PROTOCOL {
  EFI_GET_RANGE_LOCATION        GetRangeLocation;       // has output parameters for base and length
  EFI_GET_SWAP_STATE            GetSwapState;           // are ranges swapped or not
  EFI_SET_SWAP_STATE            SetSwapState;           // swap or unswap ranges
  EFI_GET_RTC_POWER_STATUS      GetRtcPowerStatus;      // checks RTC battery, or whatever...
  EFI_GET_SWAP_LOCK_CAPABILITY  GetSwapLockCapability;  // Get TOP_SWAP lock capability,
  EFI_SET_SWAP_LOCK             SetSwapLock;            // Set TOP_SWAP lock state
};

extern EFI_GUID gEfiSwapAddressRangeProtocolGuid;

#endif
