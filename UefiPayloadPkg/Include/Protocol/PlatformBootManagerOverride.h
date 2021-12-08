/** @file
  This file defines the Univeral Payload Platform BootManager Protocol.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PLATFORM_BOOT_MANAGER_OVERRIDE_H__
#define __PLATFORM_BOOT_MANAGER_OVERRIDE_H__

/**
  Do the platform specific action before the console is connected.

  Such as:
    Update console variable;
    Register new Driver#### or Boot####;
    Signal ReadyToLock event.

  This function will override the default behavior in PlatformBootManagerLib
**/
typedef
VOID
(EFIAPI *UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_BEFORE_CONSOLE)(
  VOID
  );

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch aditional option roms.

  This function will override the default behavior in PlatformBootManagerLib
**/
typedef
VOID
(EFIAPI *UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_AFTER_CONSOLE)(
  VOID
  );

/**
  This function is called each second during the boot manager waits the timeout.
  This function will override the default behavior in PlatformBootManagerLib

  @param TimeoutRemain  The remaining timeout.
**/
typedef
VOID
(EFIAPI *UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_WAIT_CALLBACK)(
  UINT16          TimeoutRemain
  );

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
  This function will override the default behavior in PlatformBootManagerLib
**/
typedef
VOID
(EFIAPI *UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_UNABLE_TO_BOOT)(
  VOID
  );

///
/// Provides an interface to override the default behavior in PlatformBootManagerLib,
/// so platform can provide its own platform specific logic through this protocol
///
typedef struct {
  UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_BEFORE_CONSOLE    BeforeConsole;
  UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_AFTER_CONSOLE     AfterConsole;
  UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_WAIT_CALLBACK     WaitCallback;
  UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_UNABLE_TO_BOOT    UnableToBoot;
} UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_PROTOCOL;

extern GUID  gUniversalPayloadPlatformBootManagerOverrideProtocolGuid;

#endif
