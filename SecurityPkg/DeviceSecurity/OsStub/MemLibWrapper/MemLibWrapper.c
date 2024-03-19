/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include "hal/base.h"
#include "hal/library/memlib.h"

/**
 * Copies bytes from a source buffer to a destination buffer.
 *
 * This function copies "src_len" bytes from "src_buf" to "dst_buf".
 *
 * Asserts and returns a non-zero value if any of the following are true:
 *   1) "src_buf" or "dst_buf" are NULL.
 *   2) "src_len" or "dst_len" is greater than (SIZE_MAX >> 1).
 *   3) "src_len" is greater than "dst_len".
 *   4) "src_buf" and "dst_buf" overlap.
 *
 * If any of these cases fail, a non-zero value is returned. Additionally if
 * "dst_buf" points to a non-NULL value and "dst_len" is valid, then "dst_len"
 * bytes of "dst_buf" are zeroed.
 *
 * This function follows the C11 cppreference description of memcpy_s.
 * https://en.cppreference.com/w/c/string/byte/memcpy
 * The cppreferece description does NOT allow the source or destination
 * buffers to be NULL.
 *
 * This function differs from the Microsoft and Safeclib memcpy_s implementations
 * in that the Microsoft and Safeclib implementations allow for NULL source and
 * destinations pointers when the number of bytes to copy (src_len) is zero.
 *
 * In addition the Microsoft and Safeclib memcpy_s functions return different
 * negative values on error. For best support, clients should generally check
 * against zero for success or failure.
 *
 * @param    dst_buf   Destination buffer to copy to.
 * @param    dst_len   Maximum length in bytes of the destination buffer.
 * @param    src_buf   Source buffer to copy from.
 * @param    src_len   The number of bytes to copy from the source buffer.
 *
 * @return   0 on success. non-zero on error.
 *
 **/
void
libspdm_copy_mem (
  void        *dst_buf,
  size_t      dst_len,
  const void  *src_buf,
  size_t      src_len
  )
{
  volatile uint8_t        *dst;
  const volatile uint8_t  *src;

  dst = (volatile uint8_t *)dst_buf;
  src = (const volatile uint8_t *)src_buf;

  /* Check for case where "dst" or "dst_len" may be invalid.
   * Do not zero "dst" in this case. */
  if ((dst == NULL) || (dst_len > (SIZE_MAX >> 1))) {
    ASSERT (0);
  }

  /* Gaurd against invalid source. Zero "dst" in this case. */
  if (src == NULL) {
    ZeroMem (dst_buf, dst_len);
    ASSERT (0);
  }

  /* Guard against overlap case. Zero "dst" in these cases. */
  if (((src < dst) && (src + src_len > dst)) || ((dst < src) && (dst + src_len > src))) {
    ZeroMem (dst_buf, dst_len);
    ASSERT (0);
  }

  /* Guard against invalid lengths. Zero "dst" in these cases. */
  if ((src_len > dst_len) ||
      (src_len > (SIZE_MAX >> 1)))
  {
    ZeroMem (dst_buf, dst_len);
    ASSERT (0);
  }

  while (src_len-- != 0) {
    *(dst++) = *(src++);
  }
}

/**
 * Fills a target buffer with a byte value, and returns the target buffer.
 *
 * This function fills length bytes of buffer with value, and returns buffer.
 *
 * If length is greater than (MAX_ADDRESS - buffer + 1), then ASSERT().
 *
 * @param  buffer    The memory to set.
 * @param  length    The number of bytes to set.
 * @param  value     The value with which to fill length bytes of buffer.
 *
 * @return buffer.
 *
 **/
void
libspdm_set_mem (
  void     *buffer,
  size_t   length,
  uint8_t  value
  )
{
  SetMem (buffer, length, value);
}

/**
 * Fills a target buffer with zeros, and returns the target buffer.
 *
 * This function fills length bytes of buffer with zeros, and returns buffer.
 *
 * If length > 0 and buffer is NULL, then ASSERT().
 * If length is greater than (MAX_ADDRESS - buffer + 1), then ASSERT().
 *
 * @param  buffer      The pointer to the target buffer to fill with zeros.
 * @param  length      The number of bytes in buffer to fill with zeros.
 *
 * @return buffer.
 *
 **/
void
libspdm_zero_mem (
  void    *buffer,
  size_t  length
  )
{
  ZeroMem (buffer, length);
}

/**
 * Compares the contents of two buffers in const time.
 *
 * This function compares length bytes of source_buffer to length bytes of destination_buffer.
 * If all length bytes of the two buffers are identical, then 0 is returned.  Otherwise, the
 * value returned is the first mismatched byte in source_buffer subtracted from the first
 * mismatched byte in destination_buffer.
 *
 * If length > 0 and destination_buffer is NULL, then ASSERT().
 * If length > 0 and source_buffer is NULL, then ASSERT().
 * If length is greater than (MAX_ADDRESS - destination_buffer + 1), then ASSERT().
 * If length is greater than (MAX_ADDRESS - source_buffer + 1), then ASSERT().
 *
 * @param  destination_buffer A pointer to the destination buffer to compare.
 * @param  source_buffer      A pointer to the source buffer to compare.
 * @param  length            The number of bytes to compare.
 *
 * @return 0                 All length bytes of the two buffers are identical.
 * @retval Non-zero          There is mismatched between source_buffer and destination_buffer.
 *
 **/
bool
libspdm_consttime_is_mem_equal (
  const void  *destination_buffer,
  const void  *source_buffer,
  size_t      length
  )
{
  if (CompareMem (destination_buffer, source_buffer, length) == 0) {
    return true;
  } else {
    return false;
  }
}
