/** @file
  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <assert.h>
#include  <string.h>
#include  <errno.h>
#include  <stdlib.h>
#include  <wchar.h>
#include  <sys/types.h>
#include  <limits.h>

typedef      int      ch_UCS4;

static  mbstate_t     LocalConvState = {0};

/** Map a UTF-8 encoded prefix byte to a sequence length.
    Zero means illegal prefix, but valid surrogate if < 0xC0.
    One indicates an ASCII-7 equivalent character.
    Two, three, and four are the first byte for 2, 3, and 4 byte sequences, respectively.
    See RFC 3629 for details.

  TABLE ENCODING:
    Low Nibble decodes the first byte into the number of bytes in the sequence.
      A value of zero indicates an invalid byte.
    The High Nibble encodes a bit mask to be used to match against the high nibble of the second byte.

    example:
      SequenceLength = code[c0] & 0x0F;
      Mask           = 0x80 | code[c0];

      Surrogate bytes are valid if: code[cX] & Mask > 0x80;

*/
static
UINT8 utf8_code_length[256] = {
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* 00-0F */
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* 70-7F */
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, /* 80-8F */
  0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, /* 90-9F */
  0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, /* A0-AF */
  0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, /* B0-BF */
  0x00, 0x00, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, /* C0-C1 + C2-CF */
  0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, /* D0-DF */
  0x43, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x33, 0x73, 0x73, /* E0-EF */
  0x64, 0x74, 0x74, 0x74, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* F0-F4 + F5-FF */
};

/** Process one byte of a multibyte character.

    @param[in]      ch    One byte of a multibyte character.
    @param[in,out]  ps    Pointer to a conversion state object.

    @retval   -2      ch is an incomplete but potentially valid character.
    @retval   -1      ch is not valid in this context.
    @retval   1:4     The length, in bytes, of the character ch just completed.
**/
static
int
ProcessOneByte(unsigned char ch, mbstate_t *ps)
{
  UINT32    Mask;
  UINT32    Length;
  int       RetVal = 0;

  if(ps->A > 3) {
    // We are in an invalid state
    ps->A = 0;    // Initial State
  }
  ps->C[ps->A] = ch;  // Save the current character
  Mask = utf8_code_length[ch];

  if(ps->A == 0) {    // Initial State.  First byte of sequence.
    ps->E   = Mask | 0x80;
    Length  = Mask & 0xF;
    switch(Length) {
      case 0:                       // State 0, Code 0
        errno = EILSEQ;
        RetVal = -1;
        ps->E = 1;        // Consume this character
        break;
      case 1:                       // State 0, Code 1
        // ASCII-7 Character
        ps->B = ps->D[0] = ch;
        RetVal = 1;
        break;
      default:                      // State 0, Code 2, 3, 4
        ps->A = 1;    // Next state is State-1
        RetVal = -2;  // Incomplete but potentially valid character
        break;
    }
  }
  else {
    // We are in state 1, 2, or 3 and processing a surrogate byte
    Length  = ps->E & 0xF;
    if((Mask & ps->E) > 0x80) {
      // This byte is valid
      switch(ps->A) {   // Process based upon our current state
        case 1:             // Second byte of the sequence.
          if(Length == 2) {         // State 1, Code 2
            Length = ((ps->C[0] & 0x1f) << 6) + (ps->C[1] & 0x3f);
            assert ((Length > 0x007F) && (Length <= 0x07FF));
            ps->B = ps->D[0] = (UINT16)Length;
            ps->A = 0;      // Next state is State-0
            RetVal = 2;
          }
          else {    // This isn't the last character, get more.  State 1, Code 3 or 4
            ps->A = 2;
            RetVal = -2;
          }
          break;
        case 2:             // Third byte of the sequence
          if(Length == 3) {
            Length = ((ps->C[0] & 0x0f) << 12) + ((ps->C[1] & 0x3f) << 6) + (ps->C[2] & 0x3f);
            assert ((Length > 0x07FF) && (Length <= 0xFFFF));
            ps->B = ps->D[0] = (UINT16)Length;
            ps->A = 0;      // Next state is State-0
            RetVal = 3;
          }
          else {
            ps->A = 3;
            RetVal = -2;
          }
          break;
        case 3:             // Fourth byte of the sequence
          if(Length == 4) {
            Length = ((ps->C[0] & 0x7) << 18) + ((ps->C[1] & 0x3f) << 12) +
                     ((ps->C[2] & 0x3f) << 6) + (ps->C[3] & 0x3f);
            ps->B = Length;
            assert ((Length > 0xFFFF) && (Length <= 0x10ffff));

            /*  compute and append the two surrogates: */

            /*  translate from 10000..10FFFF to 0..FFFF */
            Length -= 0x10000;

            /*  high surrogate = top 10 bits added to D800 */
            ps->D[0] = (UINT16)(0xD800 + (Length >> 10));

            /*  low surrogate = bottom 10 bits added to DC00 */
            ps->D[1] = (UINT16)(0xDC00 + (Length & 0x03FF));
            ps->A = 0;      // Next state is State-0
            RetVal = 4;
          }
          else {
            errno = EILSEQ;
            ps->A = 0;
            RetVal = -1;
            ps->E = 4;      // Can't happen, but consume this character anyway
          }
          break;
      }
    }
    else {                // Invalid surrogate character
      errno = EILSEQ;
      ps->A = 0;          // Next is State-0
      RetVal = -1;
      ps->E = 0;            // Don't Consume, it may be an initial byte
    }
  }
  return RetVal;
}

/** Convert one Multibyte sequence.

    @param[out]   Dest      Pointer to output location, or NULL
    @param[in]    Src       Multibyte Source (UTF8)
    @param[in]    Len       Max Number of bytes to convert
    @param[in]    pS        Pointer to State struct., or NULL

    @retval   -2      Bytes processed comprise an incomplete, but potentially valid, character.
    @retval   -1      An encoding error was encountered.  ps->E indicates the number of bytes consumed.
    @retval   0       Either Src is NULL or it points to a NUL character.
    @retval   1:N     N bytes were consumed producing a valid wide character.
**/
int
DecodeOneStateful(
  wchar_t    *Dest,       // Pointer to output location, or NULL
  const char *Src,        // Multibyte Source (UTF8)
  ssize_t     Len,        // Max Number of bytes to convert
  mbstate_t  *pS          // Pointer to State struct., or NULL
  )
{
  const char   *SrcEnd;
  int           NumConv;
  unsigned char ch;

  if(pS == NULL) {
    pS = &LocalConvState;
  }
  NumConv = 0;
  if(Src != NULL) {
    if(*Src != 0) {
      SrcEnd  = Src + Len;
      while(Src < SrcEnd) {
        ch = (unsigned char)*Src++;
        NumConv = ProcessOneByte(ch, pS);
        if(NumConv != -2) {
          break;
        }
      }
    }
    else if(Dest != NULL) {
      *Dest = 0;
    }
  }
  if((NumConv > 0) && (Dest != NULL)) {
    Dest[0] = pS->D[0];
    if(NumConv == 4) {
      Dest[1] = pS->D[1];
    }
  }
  return NumConv;
}

/*  Determine the number of bytes needed to represent a Wide character
    as a MBCS character.

    A single wide character may convert into a one, two, three, or four byte
    narrow (MBCS or UTF-8) character.  The number of MBCS bytes can be determined
    as follows.

    If WCS char      < 0x00000080      One Byte
    Else if WCS char < 0x0000D800      Two Bytes
    Else                               Three Bytes

    Since UEFI only supports the Unicode Base Multilingual Plane (BMP),
    Four-byte characters are not supported.

    @param[in]    InCh      Wide character to test.

    @retval     -1      Improperly formed character
    @retval      0      InCh is 0x0000
    @retval     >0      Number of bytes needed for the MBCS character
*/
int
EFIAPI
OneWcToMcLen(const wchar_t InCh)
{
  ssize_t   NumBytes;

  if(InCh == 0) {             //    Is this a NUL, 0x0000 ?
    NumBytes = 0;
  }
  else if(InCh < 0x0080) {    //    Is this a 1-byte character?
    NumBytes = 1;
  }
  else if(InCh < 0x0800) {    //    Is this a 2-byte character?
    NumBytes = 2;
  }
  else if((InCh >= 0xD800) && (InCh < 0xE000)) {    //    Is this a surrogate?
    NumBytes = -1;
  }
  else {
    NumBytes = 3;             //    Otherwise, it must be a 3-byte character.
  }
  return (int)NumBytes;      // Return extimate of required bytes.
}

/*  Determine the number of bytes needed to represent a Wide character string
    as a MBCS string of given maximum length.  Will optionally return the number
    of wide characters that would be consumed.

    A single wide character may convert into a one, two, three, or four byte
    narrow (MBCS or UTF-8) character.  The number of MBCS bytes can be determined
    as follows.

    If WCS char      < 0x00000080      One Byte
    Else if WCS char < 0x00000800      Two Bytes
    Else if WCS char < 0x00010000      Three Bytes
    Else                               Four Bytes

    Since UEFI only supports the Unicode Base Multilingual Plane (BMP),
    Four-byte characters should not be encountered.

    @param[in]    Src       Pointer to a wide character string.
    @param[in]    Limit     Maximum number of bytes the converted string may occupy.
    @param[out]   NumChar   Pointer to where to store the number of wide characters, or NULL.

    @return     The number of bytes required to convert Src to MBCS,
                not including the terminating NUL.  If NumChar is not NULL, the number
                of characters represented by the return value will be written to
                where it points.
*/
size_t
EFIAPI
EstimateWtoM(const wchar_t * Src, size_t Limit, size_t *NumChar)
{
  ssize_t    Estimate;
  size_t    CharCount;
  ssize_t   NumBytes;
  wchar_t   EChar;

  Estimate  = 0;
  CharCount = 0;
  EChar = *Src++;               // Get the initial character and point to next
  while(((NumBytes = OneWcToMcLen(EChar)) > 0)  &&
        ((size_t)(Estimate + NumBytes) < Limit))
  {                             // Until one of the source characters is NUL
    ++CharCount;                //    Count this character.
    Estimate += NumBytes;       //    Count the Bytes for this character
    EChar = *Src++;             //    Get the next source character and point to the next.
  }
  if(NumChar != NULL) {
    *NumChar = CharCount;
  }
  return (size_t)Estimate;      // Return esimate of required bytes.
}

/*  Determine the number of characters in a MBCS string.
    MBCS characters are one to four bytes long.  By examining the first byte
    of a MBCS character, one can determine the number of bytes comprising the
    character.

    0x00 - 0x7F     One
    0xC0 - 0xDF     Two
    0xE0 - 0xEF     Three
    0xF0 - 0xF7     Four

    Since UEFI only supports the Unicode Base Multilingual Plane (BMP),
    Four-byte characters should not be encountered.

    @param[in]    Src     The string to examine

    @return   The number of characters represented by the MBCS string.
**/
size_t
EFIAPI
CountMbcsChars(const char *Src)
{
  size_t      Count;
  char        EChar;

  Count = 0;
  EChar = *Src++;
  while(EChar != 0) {
    if(EChar < 0x80) {
      ++Count;
    }
    else if(EChar < 0xE0) {
      Count += 2;
      ++Src;
    }
    else if(EChar < 0xF0) {
      Count += 3;
      Src += 2;
    }
    else {
      // Ill-formed character
      break;
    }
  }
  return Count;
}

/** Convert a wide character (UTF16) into a multibyte character (UTF8)

    Converts a wide character into a corresponding multibyte character that
    begins in the conversion state described by the object pointed to by ps.
    If dst is not a null pointer, the converted character is then stored into
    the array pointed to by dst.

    It is the caller's responsibility to ensure that Dest is large enough to
    hold the resulting MBCS sequence.

    @param  s       Pointer to the wide-character string to convert
    @param  Dest    Pointer to the buffer in which to place the converted sequence, or NULL.

    @retval   -1    An error occurred.  The error reason is in errno.
    @retval   >=0   The number of bytes stored into Dest.
**/
ssize_t
EncodeUtf8(char *Dest, wchar_t ch)
{
  char       *p;              /* next free byte in build buffer */
  int         NumInBuff;      // number of bytes in Buff
  char        Buff[4];        // Buffer into which each character is built

    p = Buff;

  NumInBuff = 0;
  if (ch < 0x80) {
    /* Encode ASCII -- One Byte */
    *p++ = (char) ch;
    NumInBuff = 1;
  }
  else if (ch < 0x0800) {
    /* Encode Latin-1 -- Two Byte */
    *p++ = (char)(0xc0 | (ch >> 6));
    *p++ = (char)(0x80 | (ch & 0x3f));
    NumInBuff = 2;
  }
  else {
      /* Encode UCS2 Unicode ordinals -- Three Byte */
    /* Special case: check for surrogate -- Shouldn't happen in UEFI */
    if (0xD800 <= ch && ch < 0xE000) {
      errno = EILSEQ;
      return -1;
      }
    else {
      *p++ = (char)(0xe0 | (ch >> 12));
      *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
      *p++ = (char)(0x80 | (ch & 0x3f));
      NumInBuff = 3;
    }
  }
  /*  At this point, Buff holds the converted character which is NumInBuff bytes long.
      NumInBuff is the value 1, 2, 3, or 4
  */
  if(Dest != NULL) {        // Save character if Dest is not NULL
    memcpy(Dest, Buff, NumInBuff);
  }
  return NumInBuff;             // Tell the caller
}

// ########################  Narrow to Wide Conversions #######################

/** If ps is not a null pointer, the mbsinit function determines whether the
    pointed-to mbstate_t object describes an initial conversion state.

    @param[in]  ps    Pointer to the conversion state object to test.

    @return     The mbsinit function returns nonzero if ps is a null pointer
                or if the pointed-to object describes an initial conversion
                state; otherwise, it returns zero.

    Declared in: wchar.h
**/
int
mbsinit(const mbstate_t *ps)
{
  if((ps == NULL) || (ps->A == 0)) {
    return 1;
  }
  return 0;
}

/** The mbrlen function is equivalent to the call:<BR>
@verbatim
    mbrtowc(NULL, s, n, ps != NULL ? ps : &internal)
@endverbatim
    where internal is the mbstate_t object for the mbrlen function, except that
    the expression designated by ps is evaluated only once.

    @param[in]  s     Pointer to a multibyte character sequence.
    @param[in]  n     Maximum number of bytes to examine.
    @param[in]  pS    Pointer to the conversion state object.

    @retval   0       The next n or fewer characters complete a NUL.
    @retval   1..n    The number of bytes that complete the multibyte character.
    @retval   -2      The next n bytes contribute to an incomplete (but potentially valid) multibyte character.
    @retval   -1      An encoding error occurred.

    Declared in: wchar.h
**/
size_t
mbrlen(
  const char *s,
  size_t n,
  mbstate_t *pS
  )
{
  return mbrtowc(NULL, s, n, pS);
}

/** Determine the number of bytes comprising a multibyte character.

  If S is not a null pointer, the mblen function determines the number of bytes
  contained in the multibyte character pointed to by S. Except that the
  conversion state of the mbtowc function is not affected, it is equivalent to
    mbtowc((wchar_t *)0, S, N);

  @param[in]  S   NULL to query whether multibyte characters have
                  state-dependent encodings.  Otherwise, points to a
                  multibyte character.
  @param[in]  N   The maximum number of bytes in a multibyte character.

  @return   If S is a null pointer, the mblen function returns a nonzero or
            zero value, if multibyte character encodings, respectively, do
            or do not have state-dependent encodings. If S is not a null
            pointer, the mblen function either returns 0 (if S points to the
            null character), or returns the number of bytes that are contained
            in the multibyte character (if the next N or fewer bytes form a
            valid multibyte character), or returns -1 (if they do not form a
            valid multibyte character).

    Declared in: stdlib.h
**/
int
mblen(
  const char *s,
  size_t n
  )
{
  return (int)mbrlen(s, n, NULL);
}

/**
If S is a null pointer, the mbrtowc function is equivalent to the call:<BR>
@verbatim
        mbrtowc(NULL, "", 1, ps)
@endverbatim

In this case, the values of the parameters pwc and n are ignored.

If S is not a null pointer, the mbrtowc function inspects at most n bytes beginning with
the byte pointed to by S to determine the number of bytes needed to complete the next
multibyte character (including any shift sequences). If the function determines that the
next multibyte character is complete and valid, it determines the value of the
corresponding wide character and then, if pwc is not a null pointer, stores that value in
the object pointed to by pwc. If the corresponding wide character is the null wide
character, the resulting state described is the initial conversion state.

    @param[out]   pwc   Pointer to where the resulting wide character is to be stored.
    @param[in]     s    Pointer to a multibyte character "string".
    @param[in]     n    The maximum number of bytes to inspect.
    @param[in]     ps   Pointer to a conversion state object.

    @retval   0             if the next n or fewer bytes complete the multibyte
                            character that corresponds to the null wide
                            character (which is the value stored).
    @retval   between_1_and_n_inclusive   if the next n or fewer bytes complete
                            a valid multibyte character (which is the value
                            stored); the value returned is the number of bytes
                            that complete the multibyte character.
    @retval   (size_t)(-2)  if the next n bytes contribute to an incomplete
                            (but potentially valid) multibyte character, and
                            all n bytes have been processed (no value is stored).
    @retval   (size_t)(-1)  if an encoding error occurs, in which case the next
                            n or fewer bytes do not contribute to a complete and
                            valid multibyte character (no value is stored); the
                            value of the macro EILSEQ is stored in errno, and
                            the conversion state is unspecified.

    Declared in: wchar.h
**/
size_t
mbrtowc(
  wchar_t *pwc,
  const char *s,
  size_t n,
  mbstate_t *ps
  )
{
  int     RetVal;

  RetVal = DecodeOneStateful(pwc, s, (ssize_t)n, ps);
  return (size_t)RetVal;
}

/** Convert a multibyte character into a wide character.

    If S is not a null pointer, the mbtowc function inspects at most N bytes
    beginning with the byte pointed to by S to determine the number of bytes
    needed to complete the next multibyte character (including any shift
    sequences). If the function determines that the next multibyte character
    is complete and valid, it determines the value of the corresponding wide
    character and then, if Pwc is not a null pointer, stores that value in
    the object pointed to by Pwc. If the corresponding wide character is the
    null wide character, the function is left in the initial conversion state.

    @param[out]   Pwc Pointer to a wide-character object to receive the converted character.
    @param[in]    S   Pointer to a multibyte character to convert.
    @param[in]    N   Maximum number of bytes in a multibyte character.

    @return   If S is a null pointer, the mbtowc function returns a nonzero or
              zero value, if multibyte character encodings, respectively, do
              or do not have state-dependent encodings. If S is not a null
              pointer, the mbtowc function either returns 0 (if S points to
              the null character), or returns the number of bytes that are
              contained in the converted multibyte character (if the next N or
              fewer bytes form a valid multibyte character), or returns -1
              (if they do not form a valid multibyte character).

              In no case will the value returned be greater than N or the value
              of the MB_CUR_MAX macro.

    Declared in: stdlib.h
**/
int
mbtowc(
  wchar_t *pwc,
  const char *s,
  size_t n
  )
{
  return (int)mbrtowc(pwc, s, n, NULL);
}

/**
The mbsrtowcs function converts a sequence of multibyte characters that begins in the
conversion state described by the object pointed to by ps, from the array indirectly
pointed to by src into a sequence of corresponding wide characters. If dst is not a null
pointer, the converted characters are stored into the array pointed to by dst. Conversion
continues up to and including a terminating null character, which is also stored.
Conversion stops earlier in two cases: when a sequence of bytes is encountered that does
not form a valid multibyte character, or (if dst is not a null pointer) when len wide
characters have been stored into the array pointed to by dst. Each conversion takes
place as if by a call to the mbrtowc function.

If dst is not a null pointer, the pointer object pointed to by src is assigned either a null
pointer (if conversion stopped due to reaching a terminating null character) or the address
just past the last multibyte character converted (if any). If conversion stopped due to
reaching a terminating null character and if dst is not a null pointer, the resulting state
described is the initial conversion state.

    @param[out]   dst   Pointer to where the resulting wide character sequence is stored.
    @param[in]    src   Pointer to a pointer to the multibyte character sequence to convert.
    @param[in]    len   Maximum number of wide characters to be stored into dst.
    @param[in]    ps    Pointer to a conversion state object.

    @return   If the input conversion encounters a sequence of bytes that do
              not form a valid multibyte character, an encoding error occurs:
              the mbsrtowcs function stores the value of the macro EILSEQ in
              errno and returns (size_t)(-1); the conversion state is
              unspecified. Otherwise, it returns the number of multibyte
              characters successfully converted, not including the terminating
              null character (if any).

    Declared in: wchar.h
**/
size_t
mbsrtowcs(
  wchar_t      *dst,
  const char  **src,
  size_t        len,
  mbstate_t    *ps
  )
{
  int           x;
  size_t        RetVal = 0;
  const char   *MySrc;

  if((src == NULL) || (*src == NULL)) {
    return 0;
  }

  MySrc = *src;
  for(x = 1 ; (len != 0) && (x > 0); --len) {
    x = DecodeOneStateful(dst, MySrc, MB_LEN_MAX, ps);
    switch(x) {
      case -2:    // Incomplete character
      case -1:    // Encoding error
        RetVal = (size_t)x;
        break;
      case 0:     // Encountered NUL character: done.
        if(dst != NULL) {
          *dst = 0;
          *src = NULL;
        }
        break;
      default:    // Successfully decoded a character, continue with next
        MySrc += x;
        if(dst != NULL) {
          ++dst;
          if(x == 4) {
            ++dst;
          }
          *src = MySrc;
        }
        ++RetVal;
        break;
    }
  }
  return RetVal;
}

/** Convert a multibyte character string into a wide-character string.

    The mbstowcs function converts a sequence of multibyte characters that
    begins in the initial shift state from the array pointed to by Src into
    a sequence of corresponding wide characters and stores not more than limit
    wide characters into the array pointed to by Dest.  No multibyte
    characters that follow a null character (which is converted into a null
    wide character) will be examined or converted. Each multibyte character
    is converted as if by a call to the mbtowc function, except that the
    conversion state of the mbtowc function is not affected.

    No more than Limit elements will be modified in the array pointed to by Dest.
    If copying takes place between objects that overlap,
    the behavior is undefined.

    @param[out]   Dest    Pointer to the array to receive the converted string.
    @param[in]    Src     Pointer to the string to be converted.
    @param[in]    Limit   Maximum number of elements to be written to Dest.

    @return   If an invalid multibyte character is encountered, the mbstowcs
              function returns (size_t)(-1). Otherwise, the mbstowcs function
              returns the number of array elements modified, not including a
              terminating null wide character, if any.

    Declared in: stdlib.h
**/
size_t
mbstowcs(
  wchar_t *Dest,
  const char *Src,
  size_t Limit
  )
{

  /* Dest may be NULL */
  /* Src may be NULL */

  return mbsrtowcs(Dest, &Src, Limit, NULL);
}

/** The btowc function determines whether C constitutes a valid single-byte
    character in the initial shift state.

    @param[in]    C   A narrow character to test or convert to wide.

    @return   The btowc function returns WEOF if c has the value EOF or if
              (unsigned char)C does not constitute a valid single-byte
              character in the initial shift state. Otherwise, it returns the
              wide character representation of that character.

    Declared in: wchar.h
**/
wint_t
btowc(int c)
{
  int       x;
  wchar_t   Dest;
  wint_t    RetVal = WEOF;

  if (c == EOF)
    return WEOF;
  x = DecodeOneStateful(&Dest, (const char *)&c, 1, NULL);
  if(x == 0) {
    RetVal = 0;
  }
  else if(x == 1) {
    RetVal = (wint_t)Dest;
  }
  return RetVal;
}

// ########################  Wide to Narrow Conversions #######################

/**
If S is a null pointer, the wcrtomb function is equivalent to the call:<BR>
@verbatim
        wcrtomb(buf, L'\0', ps)
@endverbatim
where buf is an internal buffer.

If S is not a null pointer, the wcrtomb function determines the number of bytes needed
to represent the multibyte character that corresponds to the wide character given by wc
(including any shift sequences), and stores the multibyte character representation in the
array whose first element is pointed to by S. At most MB_CUR_MAX bytes are stored. If
wc is a null wide character, a null byte is stored, preceded by any shift sequence needed
to restore the initial shift state; the resulting state described is the initial conversion state.

    @param[out]     Dest    Pointer to the location in which to store the resulting
                            multibyte character.  Otherwise, NULL to reset the
                            conversion state.
    @param[in]      wchar   The wide character to convert.
    @param[in,out]  pS      Pointer to a conversion state object, or NULL.

    @return   The wcrtomb function returns the number of bytes stored in the
              array object (including any shift sequences). When wc is not a
              valid wide character, an encoding error occurs: the function
              stores the value of the macro EILSEQ in errno and
              returns (size_t)(-1); the conversion state is unspecified.

    Declared in: wchar.h
**/
size_t
wcrtomb(
  char *Dest,
  wchar_t wchar,
  mbstate_t *pS
  )
{
  size_t    RetVal;

  /* Dest may be NULL */
  if (Dest == NULL) {
    RetVal = 1;
  }
  else {
    if (wchar == L'\0') {
      *Dest = '\0';
      RetVal = 1;
    }
    else {
      RetVal = EncodeUtf8(Dest, wchar);
    }
  }
  if(pS == NULL) {
    pS = &LocalConvState;
  }
  pS->A = 0;      // Set ps to the initial conversion state

  return RetVal;
}

/** Convert a wide character into a multibyte character.

    The wctomb function determines the number of bytes needed to represent the
    multibyte character corresponding to the wide character given by WC
    (including any shift sequences), and stores the multibyte character
    representation in the array whose first element is pointed to by S (if S is
    not a null pointer). At most MB_CUR_MAX characters are stored. If WC is a
    null wide character, a null byte is stored, preceded by any shift sequence
    needed to restore the initial shift state, and the function is left in the
    initial conversion state.

    @param[out]   S   Pointer to the object to receive the converted multibyte character.
    @param[in]    WC  Wide character to be converted.

    @return   If S is a null pointer, the wctomb function returns a nonzero or
              zero value, if multibyte character encodings, respectively, do or
              do not have state-dependent encodings. If S is not a null pointer,
              the wctomb function returns -1 if the value of WC does not
              correspond to a valid multibyte character, or returns the number
              of bytes that are contained in the multibyte character
              corresponding to the value of WC.

              In no case will the value returned be greater than the value of
              the MB_CUR_MAX macro.

    Declared in: stdlib.h
**/
int
wctomb(
  char *s,
  wchar_t wchar
  )
{
  /*
    If s is NULL just return whether MB Characters have state
    dependent encodings -- they don't.
  */
  if (s == NULL)
    return 0;

  return (int)wcrtomb(s, wchar, NULL);
}

/** The wcsrtombs function converts a sequence of wide characters from the array
    indirectly pointed to by Src into a sequence of corresponding multibyte
    characters that begins in the conversion state described by the object
    pointed to by ps.

    If Dest is not a null pointer, the converted characters are stored into the
    array pointed to by Dest.  Conversion continues up to and including a
    terminating null wide character, which is also stored. Conversion stops
    earlier in two cases: when a wide character is reached that does not
    correspond to a valid multibyte character, or (if Dest is not a null
    pointer) when the next multibyte character would exceed the limit of Limit
    total bytes to be stored into the array pointed to by Dest. Each conversion
    takes place as if by a call to the wcrtomb function.)

    If Dest is not a null pointer, the pointer object pointed to by Src is
    assigned either a null pointer (if conversion stopped due to reaching
    a terminating null wide character) or the address just past the last wide
    character converted (if any). If conversion stopped due to reaching a
    terminating null wide character, the resulting state described is the
    initial conversion state.

    @param[in]      Dest
    @param[in,out]  Src
    @param[in]      Limit   Max number of bytes to store in Dest.
    @param[in,out]  ps

    @return     If conversion stops because a wide character is reached that
                does not correspond to a valid multibyte character, an
                encoding error occurs: the wcsrtombs function stores the
                value of the macro EILSEQ in errno and returns (size_t)(-1);
                the conversion state is unspecified. Otherwise, it returns
                the number of bytes in the resulting multibyte character
                sequence, not including the terminating null character (if any).

    Declared in: wchar.h
**/
size_t
wcsrtombs(
  char           *Dest,
  const wchar_t **Src,
  size_t          Limit,
  mbstate_t      *ps
)
{
  size_t  NumStored;
  ssize_t MaxBytes;
  int     count;
  wchar_t InCh;

  NumStored = 0;
  MaxBytes  = (ssize_t)Limit;

  /* Dest may be NULL */
  /* Src may be NULL */
  /* ps appears to be unused */

  if (Src == NULL || *Src == NULL)
    return (0);

  if (Dest == NULL) {
    NumStored = EstimateWtoM(*Src, ASCII_STRING_MAX, NULL);
  }
  else {
    if((MaxBytes < 0) || (MaxBytes > ASCII_STRING_MAX)) {
      MaxBytes = ASCII_STRING_MAX;
    }
    while ((MaxBytes > 0) && (OneWcToMcLen(InCh = *(*Src)++) <= MaxBytes)) {
      if(InCh == 0) {
        *Src = NULL;
        *Dest = 0;      // NUL terminate Dest string, but don't count the NUL
        break;
      }
      count = (int)wcrtomb(Dest, InCh, NULL);
      if(count >= 0) {
        Dest += count;
        MaxBytes -= count;
        NumStored += count;
      }
      else {
        NumStored = (size_t)(-1);
      }
    }
  }


  return NumStored;
}

/** Convert a wide-character string into a multibyte character string.

    The wcstombs function converts a sequence of wide characters from the
    array pointed to by Src into a sequence of corresponding multibyte
    characters that begins in the initial shift state, and stores these
    multibyte characters into the array pointed to by Dest, stopping if a
    multibyte character would exceed the limit of Limit total bytes or if a
    null character is stored. Each wide character is converted as if by
    a call to the wctomb function, except that the conversion state of
    the wctomb function is not affected.

    No more than Limit bytes will be modified in the array pointed to by Dest.
    If copying takes place between objects that overlap,
    the behavior is undefined.

    @param[out]   Dest    Pointer to the array to receive the converted string.
    @param[in]    Src     Pointer to the string to be converted.
    @param[in]    Limit   Maximum number of elements to be written to Dest.

    @return   If a wide character is encountered that does not correspond to a
              valid multibyte character, the wcstombs function returns
              (size_t)(-1). Otherwise, the wcstombs function returns the number
              of bytes modified, not including a terminating null character,
              if any.

    Declared in: stdlib.h
**/
size_t
wcstombs(
  char           *Dest,
  const wchar_t  *Src,
  size_t          Limit
)
{
  /* Dest may be NULL */
  return wcsrtombs(Dest, &Src, Limit, NULL);
}

/** The wctob function determines whether C corresponds to a member of the extended
    character set whose multibyte character representation is a single byte when in the initial
    shift state.

    wctob needs to be consistent with wcrtomb.
    If wcrtomb says that a character is representable in 1 byte,
    then wctob needs to also represent the character as 1 byte.

    @return     The wctob function returns EOF if C does not correspond to a multibyte
                character with length one in the initial shift state. Otherwise, it
                returns the single-byte representation of that character as an
                unsigned char converted to an int.

    Declared in: wchar.h
**/
int
wctob(wint_t c)
{
  int   RetVal;

  RetVal = EOF;
  if(c == 0) {
    RetVal = 0;
  }
  else if (OneWcToMcLen((const wchar_t)c) == 1) {
    RetVal = (int)(c & 0xFF);
  }
  return RetVal;
}
