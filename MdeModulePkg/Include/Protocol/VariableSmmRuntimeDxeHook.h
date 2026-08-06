/** @file
  Optional hooks around VariableSmmRuntimeDxe SetVariable() MM communication.

  A provider must be a DXE_RUNTIME_DRIVER, or otherwise guarantee that its code
  and data remain runtime-resident. It must not use boot services or allocate
  boot-services memory after ExitBootServices().

  Hook inputs are untrusted runtime-service inputs. Providers must validate all
  inputs they consume, bound all waits, and must not call SetVariable() from
  either hook. These hooks do not replace SMM validation, VarCheck, variable
  policy, Secure Boot authentication, MOR handling, or flash-write
  authorization.

  A provider MAY access MMIO, PIO, mailbox, or device resources reserved
  exclusively for firmware and not exposed to or accessed by the OS. If a
  resource may also be accessed by the OS, the provider MUST use a
  platform-defined ownership or synchronization mechanism shared with the OS.
  UEFI locks and TPL do not synchronize with OS drivers. For this contract,
  "not exposed to the OS" means absent from OS discovery and description
  mechanisms such as ACPI, PCI BAR assignment, and device tree. It is not
  sufficient that the currently loaded OS driver does not use the resource.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define EDKII_VARIABLE_RUNTIME_HOOK_PROTOCOL_GUID \
  { \
    0xdeddcd7a, 0xd76d, 0x46a3, { 0xba, 0xa5, 0xbf, 0x25, 0x50, 0x77, 0xdc, 0x10 } \
  }

/**
  Invoked immediately before runtime SetVariable() communication is sent to MM.

  The provider owns its runtime-safe synchronization state. The interface
  intentionally carries no opaque per-operation context.

  @param[in] VariableName  Name of the variable.
  @param[in] VendorGuid    Variable vendor GUID.
  @param[in] Attributes    Variable attributes.
  @param[in] DataSize      Size of Data in bytes.
  @param[in] Data          Variable data.

  @retval EFI_SUCCESS  Continue with the existing MM variable service.
  @retval Others       Reject or defer the operation without entering MM.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_PRE_SET_VARIABLE)(
  IN CONST CHAR16    *VariableName,
  IN CONST EFI_GUID  *VendorGuid,
  IN       UINT32    Attributes,
  IN       UINTN     DataSize,
  IN CONST VOID      *Data
  );

/**
  Invoked after runtime SetVariable() communication completes.

  This notification is called only when PreSetVariable() returned success.
  SetVariableStatus is authoritative and is returned unchanged by the existing
  variable service.

  @param[in] SetVariableStatus  Result from the existing MM variable service.
**/
typedef
VOID
(EFIAPI *EDKII_VARIABLE_POST_SET_VARIABLE)(
  IN EFI_STATUS  SetVariableStatus
  );

///
/// This protocol is optional. Both callbacks are required. The first valid
/// provider discovered before EndOfDxe is used.
///
typedef struct {
  EDKII_VARIABLE_PRE_SET_VARIABLE     PreSetVariable;
  EDKII_VARIABLE_POST_SET_VARIABLE    PostSetVariable;
} EDKII_VARIABLE_RUNTIME_HOOK_PROTOCOL;

extern EFI_GUID  gEdkiiVariableSmmRuntimeDxeHookProtocolGuid;
