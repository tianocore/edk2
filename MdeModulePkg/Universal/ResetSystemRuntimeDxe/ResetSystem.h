/** @file

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RESET_SYSTEM_H_
#define _RESET_SYSTEM_H_


#include <PiDxe.h>

#include <Protocol/Reset.h>
#include <Protocol/ResetNotification.h>
#include <Protocol/PlatformSpecificResetFilter.h>
#include <Protocol/PlatformSpecificResetHandler.h>
#include <Guid/CapsuleVendor.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>

//
// The maximum recurstion depth to ResetSystem() by reset notification handlers
//
#define MAX_RESET_NOTIFY_DEPTH 10

typedef struct {
  UINT32                   Signature;
  LIST_ENTRY               Link;
  EFI_RESET_SYSTEM         ResetNotify;
} RESET_NOTIFY_ENTRY;
#define RESET_NOTIFY_ENTRY_SIGNATURE    SIGNATURE_32('r', 's', 't', 'n')
#define RESET_NOTIFY_ENTRY_FROM_LINK(a) CR (a, RESET_NOTIFY_ENTRY, Link, RESET_NOTIFY_ENTRY_SIGNATURE)

typedef struct {
  UINT32                          Signature;
  EFI_RESET_NOTIFICATION_PROTOCOL ResetNotification;
  LIST_ENTRY                      ResetNotifies;
} RESET_NOTIFICATION_INSTANCE;
#define RESET_NOTIFICATION_INSTANCE_SIGNATURE    SIGNATURE_32('r', 's', 't', 'i')
#define RESET_NOTIFICATION_INSTANCE_FROM_THIS(a) \
  CR (a, RESET_NOTIFICATION_INSTANCE, ResetNotification, RESET_NOTIFICATION_INSTANCE_SIGNATURE)

/**
  The driver's entry point.

  It initializes the Reset Architectural Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Cannot install ResetArch protocol.

**/
EFI_STATUS
EFIAPI
InitializeResetSystem (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Resets the entire platform.

  @param[in] ResetType          The type of reset to perform.
  @param[in] ResetStatus        The status code for the reset.
  @param[in] DataSize           The size, in bytes, of ResetData.
  @param[in] ResetData          For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.

**/
VOID
EFIAPI
RuntimeServiceResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  );

#endif
