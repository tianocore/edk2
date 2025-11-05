/** @file
  PC/AT CMOS access routines

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CMOS_H_
#define _CMOS_H_

/**
  Reads 8-bits of CMOS data.

  Reads the 8-bits of CMOS data at the location specified by Index.
  The 8-bit read value is returned.

  @param  Index  The CMOS location to read.

  @return The value read.

**/
UINT8
EFIAPI
CmosRead8 (
  IN      UINTN  Index
  );

/**
  Writes 8-bits of CMOS data.

  Writes 8-bits of CMOS data to the location specified by Index
  with the value specified by Value and returns Value.

  @param  Index  The CMOS location to write.
  @param  Value  The value to write to CMOS.

  @return The value written to CMOS.

**/
UINT8
EFIAPI
CmosWrite8 (
  IN      UINTN  Index,
  IN      UINT8  Value
  );

#endif /* _CMOS_H_ */
