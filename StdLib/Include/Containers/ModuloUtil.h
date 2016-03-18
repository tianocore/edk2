/** @file
  Utility functions for performing basic math operations constrained within a
  modulus.

  These functions are intended to simplify small changes to a value which much
  remain within a specified modulus.  Changes must be less than or equal to
  the modulus specified by MaxVal.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _MODULO_UTIL_H
#define _MODULO_UTIL_H
#include  <Uefi.h>
#include  <sys/EfiCdefs.h>

__BEGIN_DECLS

/** Counter = (Counter + 1) % MaxVal;

    Counter is always expected to be LESS THAN MaxVal.
        0 <= Counter < MaxVal

    @param[in]    Counter   The value to be incremented.
    @param[in]    MaxVal    Modulus of the operation.

    @return   Returns the result of incrementing Counter, modulus MaxVal.
              If Counter >= MaxVal, returns -1.
**/
INT32
EFIAPI
ModuloIncrement(
  UINT32  Counter,
  UINT32  MaxVal
  );

/** Counter = (Counter - 1) % MaxVal;

    Counter is always expected to be LESS THAN MaxVal.
        0 <= Counter < MaxVal

    @param[in]    Counter   The value to be decremented.
    @param[in]    MaxVal    Modulus of the operation.

    @return   Returns the result of decrementing Counter, modulus MaxVal.
              If Counter >= MaxVal, returns -1.
**/
INT32
EFIAPI
ModuloDecrement(
  UINT32  Counter,
  UINT32  MaxVal
  );

/** Counter = (Counter + Increment) % MaxVal;

    @param[in]    Counter   The value to be incremented.
    @param[in]    Increment The value to add to Counter.
    @param[in]    MaxVal    Modulus of the operation.

    @return   Returns the result of adding Increment to Counter, modulus MaxVal,
              or -1 if Increment is larger than MaxVal.
**/
INT32
EFIAPI
ModuloAdd (
  UINT32  Counter,
  UINT32  Increment,
  UINT32  MaxVal
  );

/** Increment Counter but don't increment past MaxVal.

    @param[in]    Counter   The value to be decremented.
    @param[in]    MaxVal    The upper bound for Counter.  Counter < MaxVal.

    @return   Returns the result of incrementing Counter.
**/
UINT32
EFIAPI
BoundIncrement(
  UINT32  Counter,
  UINT32  MaxVal
  );

/** Decrement Counter but don't decrement past zero.

    @param[in]    Counter   The value to be decremented.

    @return   Returns the result of decrementing Counter.
**/
UINT32
EFIAPI
BoundDecrement(
  UINT32  Counter
  );

__END_DECLS
#endif  /* _MODULO_UTIL_H */
