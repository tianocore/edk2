/*++
  SecureZeroMemory.c

  Implementation of SecureZeroMemory() for EDK II SecureZeroMemoryLib.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
--*/

#include <Base.h>
#include <Library/SecureZeroMemoryLib.h>

#ifdef _MSC_VER
#define SZM_NOINLINE  __declspec(noinline)
#elif defined (__GNUC__) || defined (__clang__)
#define SZM_NOINLINE  __attribute__((noinline))
#else
#define SZM_NOINLINE
#endif

#if defined (_MSC_VER)
void
_ReadWriteBarrier (
  void
  );

  #pragma intrinsic(_ReadWriteBarrier)
#endif

/**
  A helper function to emit a compiler barrier and treat Buffer as an input to
  discourage reordering and call-site elimination.

  @param  Buffer  Pointer to the buffer being zeroed.

**/
STATIC
VOID
SecureZeroMemoryBarrier (
  IN VOID  *Buffer
  )
{
 #if defined (_MSC_VER)
  _ReadWriteBarrier ();
  (VOID)Buffer;
 #elif defined (__GNUC__) || defined (__clang__)
  __asm__ __volatile__ ("" : : "r"(Buffer) : "memory");
 #else
  (VOID)Buffer;
 #endif
}

/**
  Internal worker function for SecureZeroMemory().

  This function is marked noinline to prevent the compiler from optimizing away the zeroing loop
  when inlining it into SecureZeroMemory().

  @param  Buffer  Pointer to the buffer to clear.
  @param  Length  Number of bytes to clear.

**/
STATIC
SZM_NOINLINE
VOID
SecureZeroMemoryInternal (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
  volatile UINT8  *Pointer;

  Pointer = (volatile UINT8 *)Buffer;
  while (Length-- != 0) {
    *Pointer++ = 0;
  }

  //
  // Compiler barrier + also treat Buffer as used.
  //
  SecureZeroMemoryBarrier (Buffer);
}

/**
  Securely zero a buffer.

  This function attempts to ensure the buffer is actually cleared and that the
  compiler does not optimize away the writes.

  @param  Buffer  Pointer to the buffer to clear.
  @param  Length  Number of bytes to clear.

  @return Buffer (same pointer passed in).
**/
VOID *
EFIAPI
SecureZeroMemory (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  )
{
  if ((Buffer == NULL) || (Length == 0)) {
    return Buffer;
  }

  SecureZeroMemoryInternal (Buffer, Length);

  //
  // A second barrier to discourage call-site elimination.
  //
  SecureZeroMemoryBarrier (Buffer);

  return Buffer;
}
