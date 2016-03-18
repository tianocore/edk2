/*  $NetBSD: s_infinity.c,v 1.5 2003/07/26 19:25:05 salo Exp $  */

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */
#include  <LibConfig.h>

#include <sys/types.h>

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
  // Force 8-byte alignment
  #define ALIGN8  __declspec(align(8))

  // C4742: identifier has different alignment in 'X' and 'Y'
  #pragma warning ( disable : 4742 )
  // C4744: identifier has different type in 'X' and 'Y'
  #pragma warning ( disable : 4744 )
#else
  #define ALIGN8
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
ALIGN8 char __infinity[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f };
#else
ALIGN8 char __infinity[] = { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif
