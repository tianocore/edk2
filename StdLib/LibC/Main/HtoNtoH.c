/** @File
    Routines for translating between host and network byte-order.

    Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Library/BaseLib.h>
#include  <LibConfig.h>
#include  <sys/endian.h>

// Undefine macro versions of the functions to be defined below.
#undef  htonl
#undef  htons
#undef  ntohl
#undef  ntohs

/** 32-bit Host to Network byte order conversion.

  @param[in]  Datum   The 32-bit value to be converted.
  @return     Datum, converted to network byte order.
**/
uint32_t
htonl(
  IN  uint32_t Datum
  )
{
#if BYTE_ORDER == LITTLE_ENDIAN
  return SwapBytes32(Datum);
#else
  return Datum;
#endif
}

/** 16-bit Host to Network byte order conversion.

  @param[in]  Datum   The 16-bit value to be converted.
  @return     Datum, converted to network byte order.
**/
uint16_t
htons(
  IN  uint16_t Datum
  )
{
#if BYTE_ORDER == LITTLE_ENDIAN
  return SwapBytes16(Datum);
#else
  return Datum;
#endif
}

/** 32-bit Network to Host byte order conversion.

  @param[in]  Datum   The 16-bit value to be converted.
  @return     Datum, converted to host byte order.
**/
uint32_t
ntohl(
  IN  uint32_t Datum
  )
{
#if BYTE_ORDER == LITTLE_ENDIAN
  return SwapBytes32(Datum);
#else
  return Datum;
#endif
}

/** 16-bit Network to Host byte order conversion.

  @param[in]  Datum   The 16-bit value to be converted.
  @return     Datum, converted to host byte order.
**/
uint16_t
ntohs(
  IN uint16_t Datum
  )
{
#if BYTE_ORDER == LITTLE_ENDIAN
  return SwapBytes16(Datum);
#else
  return Datum;
#endif
}
