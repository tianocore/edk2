/** @file
    Utility functions for performing basic math operations constrained within a
    modulus.

    These functions are intended to simplify small changes to a value which much
    remain within a specified modulus.

  NOTE: Changes must be less than or equal to the modulus specified by MaxVal.

    Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License which accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <LibConfig.h>
#include  <assert.h>

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
  )
{
  INT32  Temp;

  if(Counter < MaxVal) {
    Temp = (INT32)(Counter + 1);
    if(Temp >= (INT32)MaxVal) {
      Temp = 0;
    }
  }
  else {
    Temp = -1;
  }
  return Temp;
}

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
  )
{
  INT32  Temp;

  if(Counter < MaxVal) {
    Temp = (INT32)Counter - 1;
    // If Counter is zero, Temp will become -1.
    if(Temp < 0) {
      Temp = (INT32)MaxVal - 1;
    }
  }
  else {
    Temp = -1;
  }

  return Temp;
}

/** Decrement Counter but don't decrement past zero.

    @param[in]    Counter   The value to be decremented.

    @return   Returns the result of decrementing Counter.
**/
UINT32
EFIAPI
BoundDecrement(
  UINT32  Counter
  )
{
  return ((Counter > 0) ? (Counter - 1) : 0);
}

/** Increment Counter but don't increment past MaxVal.
    Counter should be maintained in the range (0 <= Counter < MaxVal).

    @param[in]    Counter   The value to be decremented.
    @param[in]    MaxVal    The upper bound for Counter.

    @return   Returns the result of incrementing Counter.
**/
UINT32
EFIAPI
BoundIncrement(
  UINT32  Counter,
  UINT32  MaxVal
  )
{
  return ((Counter < (MaxVal - 1)) ? (Counter + 1) : (MaxVal - 1));
}

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
  )
{
  UINT32   Temp;

  if(Increment > MaxVal) {
    return -1;
  }
  Temp = (Counter + Increment);
  while(Temp >= MaxVal) {
    Temp -= MaxVal;
  }
  return Temp;
}
