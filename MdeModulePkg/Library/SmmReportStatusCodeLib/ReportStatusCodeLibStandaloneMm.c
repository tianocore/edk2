/** @file
  Abstraction layer for MM service table used by MM ReportStatusCodeLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/MmServicesTableLib.h>

/**
  Returns the first protocol instance that matches the given protocol.

  @param[in]  Protocol          Provides the protocol to search for.
  @param[in]  Registration      Optional registration key returned from
                                RegisterProtocolNotify().
  @param[out]  Interface        On return, a pointer to the first interface that matches Protocol and
                                Registration.

  @retval EFI_SUCCESS           A protocol instance matching Protocol was found and returned in
                                Interface.
  @retval EFI_NOT_FOUND         No protocol instances were found that match Protocol and
                                Registration.
  @retval EFI_INVALID_PARAMETER Interface is NULL.
                                Protocol is NULL.

**/
EFI_STATUS
InternalLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration  OPTIONAL,
  OUT VOID      **Interface
  )
{
  if (gMmst == NULL) {
    return EFI_NOT_FOUND;
  }

  return gMmst->MmLocateProtocol (Protocol, Registration, Interface);
}
