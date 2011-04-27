/** @file
  Character classification and case conversion functions for <ctype.h>.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <LibConfig.h>

#define NO_CTYPE_MACROS            // So that we don't define the classification macros
#include  <ctype.h>

int
__isCClass( int _c, unsigned int mask)
{
  return ((_c < 0 || _c > 127) ? 0 : (_cClass[_c] & mask));
}

/**

    @return
**/
int isalnum(int c)
{
  return (__isCClass( c, (_CD | _CU | _CL | _XA)));
}

/**

    @return
**/
int isalpha(int c)
{
  return (__isCClass( c, (_CU | _CL | _XA)));
}

/**

    @return
**/
int iscntrl(int c)
{
  return (__isCClass( c, (_CC)));
}

/**

    @return
**/
int isdigit(int c)
{
  return (__isCClass( c, (_CD)));
}

/**

    @return
**/
int isgraph(int c)
{
  return (__isCClass( c, (_CG)));
}

/**

    @return
**/
int islower(int c)
{
  return (__isCClass( c, (_CL)));
}

/**

    @return
**/
int isprint(int c)
{
  return (__isCClass( c, (_CS | _CG)));
}

/**

    @return
**/
int ispunct(int c)
{
  return (__isCClass( c, (_CP)));
}

/**

    @return
**/
int isspace(int c)
{
  return (__isCClass( c, (_CW)));
}

/**

    @return
**/
int isupper(int c)
{
  return (__isCClass( c, (_CU)));
}

/**

    @return
**/
int isxdigit(int c)
{
  return (__isCClass( c, (_CD | _CX)));
}

#if defined(_NETBSD_SOURCE)
int
isblank(int c)
{
  return (__isCClass( c, _CB));
}
#endif

/** The isascii function tests that a character is one of the 128 ASCII characters.

  @param[in]  c   The character to test.
  @return     Returns nonzero (true) if c is a valid ASCII character.  Otherwize,
              zero (false) is returned.
**/
int isascii(int c){
  return ((c >= 0) && (c < 128));
}
