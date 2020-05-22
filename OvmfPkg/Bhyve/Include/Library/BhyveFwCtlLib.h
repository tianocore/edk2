/** @file
  bhyve firmware configuration access

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2015 Nahanni Systems

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BHYVE_FW_CTL_LIB__
#define __BHYVE_FW_CTL_LIB__

/**
  Sysctl-like interface to read host information via a dot-separated
  ASCII OID.

  The information is copied into the buffer specified by Item. The
  size of the buffer is given by the location specified by Size
  before the call, and that location gives the amount of data copied
  after a successfull call, and after a call that returns a truncated
  value with an error return of RETURN_BUFFER_TOO_SMALL.

  The size of the available data can be determined by passing a NULL
  argument for Item. The size will be returned in the location pointed
  to by Size.

  @param[]  Name    - ASCII OID name
  @param[]  Data    - return buffer pointer
  @param[]  Size    - pointer to length

  @return    RETURN_SUCCESS          Valid data/len returned.
          RETURN_UNSUPPORTED        f/w interface not present.
          RETURN_NOT_FOUND      OID not found.
        RETURN_BUFFER_TOO_SMALL      Return message truncated.
        RETURN_INVALID_PARAMETER  Buffer too large
        RETURN_PROTOCOL_ERROR     Unknown error from host
 **/
RETURN_STATUS
EFIAPI
BhyveFwCtlGet (
  IN   CONST CHAR8    *Name,
  OUT  VOID        *Item,
  IN OUT  UINTN        *Size
  );

#endif
