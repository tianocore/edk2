/** @file
  Produces the Random Number Generator protocol in Standalone MM.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/RngLib.h>
#include <Protocol/Rng.h>

/**
  Returns information about the random number generation algorithm supported
  by this driver.

  @param[in]      This                  A pointer to the EFI_RNG_PROTOCOL
                                        instance.
  @param[in, out] RNGAlgorithmListSize  On input, the size in bytes of
                                        RNGAlgorithmList. On output, the size
                                        in bytes of the supported algorithm
                                        list.
  @param[out]     RNGAlgorithmList      A caller-allocated buffer that receives
                                        the supported EFI_RNG_ALGORITHM.

  @retval EFI_SUCCESS            The supported RNG algorithm was returned.
  @retval EFI_BUFFER_TOO_SMALL   RNGAlgorithmList is too small. The required
                                 size is returned in RNGAlgorithmListSize.
  @retval EFI_INVALID_PARAMETER  This or RNGAlgorithmListSize is NULL, or
                                 RNGAlgorithmList is NULL when the supplied
                                 buffer size is sufficient.
  @retval EFI_UNSUPPORTED        The selected RngLib does not report its
                                 algorithm.

**/
static
EFI_STATUS
EFIAPI
RngGetInfo (
  IN EFI_RNG_PROTOCOL    *This,
  IN OUT UINTN           *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM  *RNGAlgorithmList
  )
{
  EFI_STATUS         Status;
  EFI_RNG_ALGORITHM  RngAlgorithm;

  if ((This == NULL) || (RNGAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*RNGAlgorithmListSize < sizeof (RngAlgorithm)) {
    *RNGAlgorithmListSize = sizeof (RngAlgorithm);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RNGAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetRngGuid (&RngAlgorithm);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (RNGAlgorithmList, &RngAlgorithm, sizeof (RngAlgorithm));
  *RNGAlgorithmListSize = sizeof (RngAlgorithm);
  return EFI_SUCCESS;
}

/**
  Produces and returns an RNG value using either the default or specified RNG
  algorithm.

  @param[in]  This            A pointer to the EFI_RNG_PROTOCOL instance.
  @param[in]  RNGAlgorithm    A pointer to the EFI_RNG_ALGORITHM that identifies
                              the RNG algorithm to use. May be NULL to use the
                              driver's default RNG algorithm.
  @param[in]  RNGValueLength  The number of random bytes to return.
  @param[out] RNGValue        A caller-allocated buffer that receives the random
                              bytes.

  @retval EFI_SUCCESS            The RNG value was returned successfully.
  @retval EFI_UNSUPPORTED        RNGAlgorithm is not supported, or the selected
                                 RngLib does not report its algorithm.
  @retval EFI_NOT_READY          The selected RngLib failed to produce a random
                                 value.
  @retval EFI_INVALID_PARAMETER  This or RNGValue is NULL, or RNGValueLength is
                                 zero.

**/
static
EFI_STATUS
EFIAPI
RngGetRNG (
  IN EFI_RNG_PROTOCOL   *This,
  IN EFI_RNG_ALGORITHM  *RNGAlgorithm OPTIONAL,
  IN UINTN              RNGValueLength,
  OUT UINT8             *RNGValue
  )
{
  EFI_STATUS         Status;
  EFI_RNG_ALGORITHM  RngLibAlgorithm;
  UINT64             RandomValue;
  UINTN              BytesToCopy;

  if ((This == NULL) || (RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetRngGuid (&RngLibAlgorithm);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((RNGAlgorithm != NULL) && !CompareGuid (RNGAlgorithm, &RngLibAlgorithm)) {
    return EFI_UNSUPPORTED;
  }

  while (RNGValueLength > 0) {
    if (!GetRandomNumber64 (&RandomValue)) {
      return EFI_NOT_READY;
    }

    BytesToCopy = MIN (RNGValueLength, sizeof (RandomValue));
    CopyMem (RNGValue, &RandomValue, BytesToCopy);
    RNGValue       += BytesToCopy;
    RNGValueLength -= BytesToCopy;
  }

  return EFI_SUCCESS;
}

static EFI_RNG_PROTOCOL  mRngProtocol = {
  RngGetInfo,
  RngGetRNG
};

/**
  Initializes the Standalone MM RNG driver and installs EFI_RNG_PROTOCOL.

  The driver publishes the protocol only after the selected RngLib reports its
  algorithm and successfully produces an initial random value.

  @param[in] ImageHandle    The firmware-allocated handle for this driver.
  @param[in] MmSystemTable  A pointer to the MM System Table.

  @retval EFI_SUCCESS      EFI_RNG_PROTOCOL was installed successfully.
  @retval EFI_UNSUPPORTED  The selected RngLib does not report an algorithm or
                           failed to produce an initial random value.
  @retval Others           An error returned by MmInstallProtocolInterface().

**/
EFI_STATUS
EFIAPI
RngStandaloneMmEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS         Status;
  EFI_HANDLE         Handle;
  EFI_RNG_ALGORITHM  RngAlgorithm;
  UINT64             RandomValue;

  Status = GetRngGuid (&RngAlgorithm);
  if (EFI_ERROR (Status) || !GetRandomNumber64 (&RandomValue)) {
    return EFI_UNSUPPORTED;
  }

  Handle = NULL;
  return gMmst->MmInstallProtocolInterface (
                  &Handle,
                  &gEfiRngProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mRngProtocol
                  );
}
