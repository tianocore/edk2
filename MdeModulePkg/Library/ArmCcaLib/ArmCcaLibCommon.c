/** @file
  Library that implements the Arm CCA helper functions.

  Copyright (c) 2022 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/
#include <Base.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Queries to see if running in a Realm.

    @retval TRUE    The execution is within the context of a Realm.
    @retval FALSE   The execution is not within the context of a Realm.
**/
BOOLEAN
EFIAPI
ArmCcaReadIsRealm (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINT32         UefiImpl;
  UINT32         RmmImplLow;
  UINT32         RmmImplHigh;

  // Query the RSI_GET_VERSION. If it is implemented then
  // RMM is present and we must be running in a Realm.
  Status = ArmCcaRsiGetVersion (
             &UefiImpl,
             &RmmImplLow,
             &RmmImplHigh
             );
  if (!RETURN_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Read the IPA width of the Realm.

  The IPA width of the Realm is used to configure the protection attribute
  for memory regions, see ArmCcaSetMemoryProtectionAttribute().

    @param [out] IpaWidth  IPA width of the Realm.

    @retval RETURN_SUCCESS            Success.
    @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
    @retval RETURN_OUT_OF_RESOURCES   Out of resources.

**/
RETURN_STATUS
EFIAPI
ArmCcaReadIpaWidth (
  OUT UINT64  *IpaWidth
  )
{
  RETURN_STATUS         Status;
  ARM_CCA_REALM_CONFIG  *Config;

  if (IpaWidth == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)),
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (ARM_CCA_REALM_CONFIG));

  Status = ArmCcaRsiGetRealmConfig (Config);
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    goto ExitHandler;
  }

  if (Config->IpaWidth > ArmGetPhysicalAddressBits ()) {
    ASSERT (0);
    Status = RETURN_INVALID_PARAMETER;
    goto ExitHandler;
  }

  *IpaWidth = Config->IpaWidth;

ExitHandler:
  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)));
  return Status;
}

/** Check if the address range is protected MMIO

  @param [in]   BaseAddress      Base address of the device MMIO region.
  @param [in]   Length           Length of the device MMIO region.
  @param [out]  IsProtectedMmio  TRUE - if the RIPAS for the address range is
                                        protected MMIO.
                                 FALSE - if the RIPAS for the address range is
                                         not protected MMIO.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_UNSUPPORTED        The request is not initiated in a
                                    Realm.
**/
RETURN_STATUS
EFIAPI
ArmCcaMemRangeIsProtectedMmio (
  IN  UINT64   BaseAddress,
  IN  UINT64   Length,
  OUT BOOLEAN  *IsProtectedMmio
  )
{
  RETURN_STATUS  RetStatus;
  UINT64         *Base;
  UINT64         *Top;
  UINT64         *End;
  ARM_CCA_RIPAS  State;

  if ((Length == 0) || (IsProtectedMmio == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (!ArmCcaIsRealm ()) {
    *IsProtectedMmio = FALSE;
    return RETURN_UNSUPPORTED;
  }

  Base = (UINT64 *)BaseAddress;
  End  = (UINT64 *)(BaseAddress + Length);

  while (Base < End) {
    Top       = End;
    RetStatus = ArmCcaRsiGetIpaState (
                  Base,
                  &Top,
                  &State
                  );
    if (RETURN_ERROR (RetStatus)) {
      // An error occurred.
      return RetStatus;
    }

    if (State != RipasDev) {
      // The address range is not protected MMIO.
      *IsProtectedMmio = FALSE;
      return RetStatus;
    }

    Base = Top;
  } // while

  *IsProtectedMmio = TRUE;
  return RETURN_SUCCESS;
}
