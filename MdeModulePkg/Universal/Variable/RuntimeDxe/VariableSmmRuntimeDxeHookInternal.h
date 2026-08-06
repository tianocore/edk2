/** @file
  Internal support for the optional VariableSmmRuntimeDxe hook protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Protocol/VariableSmmRuntimeDxeHook.h>

/**
  Discovers the first valid variable runtime hook provider and registers for
  notification if a provider is not yet available.

  @retval EFI_SUCCESS  Discovery was initialized or a provider was cached.
  @retval Others       Event or protocol notification registration failed.
**/
EFI_STATUS
VariableRuntimeHookInitialize (
  VOID
  );

/**
  Stops variable runtime hook provider discovery and closes discovery events.
**/
VOID
VariableRuntimeHookStopDiscovery (
  VOID
  );

/**
  Converts cached variable runtime hook callback pointers to virtual addresses.
**/
VOID
VariableRuntimeHookConvertPointers (
  VOID
  );

/**
  Invokes the cached pre-hook for an OS runtime SetVariable() request.

  @param[in]  VariableName  Name of the variable.
  @param[in]  VendorGuid    Variable vendor GUID.
  @param[in]  Attributes    Variable attributes.
  @param[in]  DataSize      Size of Data in bytes.
  @param[in]  Data          Variable data.
  @param[out] HookInvoked   TRUE if the pre-hook returned success.

  @retval EFI_SUCCESS  Continue with MM communication.
  @retval Others       The pre-hook rejected or deferred the operation.
**/
EFI_STATUS
VariableRuntimeHookPreSetVariable (
  IN  CONST CHAR16    *VariableName,
  IN  CONST EFI_GUID  *VendorGuid,
  IN        UINT32    Attributes,
  IN        UINTN     DataSize,
  IN  CONST VOID      *Data,
  OUT       BOOLEAN   *HookInvoked
  );

/**
  Invokes the cached post-hook after MM communication completes.

  @param[in] HookInvoked        TRUE if the pre-hook returned success.
  @param[in] SetVariableStatus  Authoritative MM variable service result.
**/
VOID
VariableRuntimeHookPostSetVariable (
  IN BOOLEAN     HookInvoked,
  IN EFI_STATUS  SetVariableStatus
  );
