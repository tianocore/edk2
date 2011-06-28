/** @file
    Extended multibyte and wide character utilities.

    Within this implementation, multibyte characters are represented using the
    Unicode UTF-8 encoding and wide characters are represented using the
    16-bit UCS-2 encoding.

    Unless explicitly stated otherwise, if the execution of a function declared
    in this file causes copying to take place between objects that overlap, the
    behavior is undefined.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _WCHAR_H
#define _WCHAR_H
#include  <sys/EfiCdefs.h>
#include  <machine/ansi.h>
#include  <machine/limits.h>
#include  <stdarg.h>
#include  <stdio.h>

#if defined(_MSC_VER)
  #pragma warning ( disable : 4142 )
#endif

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _BSD_SIZE_T_
  #undef _EFI_SIZE_T_
#endif

#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    typedef _EFI_WCHAR_T wchar_t;
    #undef _BSD_WCHAR_T_
    #undef  _EFI_WCHAR_T
  #endif
#endif

/*  mbstate_t is an opaque object, that must not be an array type, used to keep
    conversion state during multibyte stream conversions.
 */
#ifdef _BSD_MBSTATE_T_
  typedef _BSD_MBSTATE_T_ mbstate_t;
  #undef _BSD_MBSTATE_T_
#endif

/*  wint_t is an integer type unchanged by default argument promotions that can
    hold any value corresponding to members of the extended character set, as
    well as at least one value that does not correspond to any member of the
    extended character set: WEOF.
*/
#ifdef _EFI_WINT_T
  typedef _EFI_WINT_T  wint_t;
  #undef _BSD_WINT_T_
  #undef _EFI_WINT_T
#endif

/*  Since wchar_t is an unsigned 16-bit value, it has a minimum value of 0, and
    a maximum value defined by __USHRT_MAX (65535 on IA processors).
*/
#ifndef WCHAR_MIN
  #define WCHAR_MIN 0
  #define WCHAR_MAX __USHRT_MAX
#endif

/* limits of wint_t */
#ifndef WINT_MIN
  #define WINT_MIN  _EFI_WINT_MIN       /* wint_t   */
  #define WINT_MAX  _EFI_WINT_MAX       /* wint_t   */
#endif

/*  WEOF expands to a constant expression of type wint_t whose value does not
    correspond to any member of the extended character set. It is accepted
    (and returned) by several functions, declared in this file, to indicate
    end-of-file, that is, no more input from a stream. It is also used as a
    wide character value that does not correspond to any member of the
    extended character set.
*/
#ifndef WEOF
  #define WEOF  ((wint_t)-1)
#endif

/*  tm is declared here as an incomplete structure type.  The full structure
    declaration is in <time.h>.
*/
struct  tm;

/* ###############  Formatted Input/Output Functions  ##################### */

/**
The fwprintf function writes output to the stream pointed to by stream, under
control of the wide string pointed to by format that specifies how subsequent arguments
are converted for output. If there are insufficient arguments for the format, the behavior
is undefined. If the format is exhausted while arguments remain, the excess arguments
are evaluated (as always) but are otherwise ignored. The fwprintf function returns
when the end of the format string is encountered.

The fwprintf function returns the number of wide characters transmitted, or a negative
value if an output or encoding error occurred.
**/
int fwprintf(FILE * __restrict stream, const wchar_t * __restrict format, ...);

/**
The fwscanf function reads input from the stream pointed to by stream, under
control of the wide string pointed to by format that specifies the admissible input
sequences and how they are to be converted for assignment, using subsequent arguments
as pointers to the objects to receive the converted input. If there are insufficient
arguments for the format, the behavior is undefined. If the format is exhausted while
arguments remain, the excess arguments are evaluated (as always) but are otherwise
ignored.

The fwscanf function returns the value of the macro EOF if an input failure occurs
before any conversion. Otherwise, the function returns the number of input items
assigned, which can be fewer than provided for, or even zero, in the event of an early
matching failure.
**/
int fwscanf(FILE * __restrict stream, const wchar_t * __restrict format, ...);

/**
The swprintf function is equivalent to fwprintf, except that the argument s
specifies an array of wide characters into which the generated output is to be written,
rather than written to a stream. No more than n wide characters are written, including a
terminating null wide character, which is always added (unless n is zero).

The swprintf function returns the number of wide characters written in the array, not
counting the terminating null wide character, or a neg ative value if an encoding error
occurred or if n or more wide characters were requested to be written.
**/
int swprintf(wchar_t * __restrict s, size_t n, const wchar_t * __restrict format, ...);

/**
**/
int swscanf(const wchar_t * __restrict s, const wchar_t * __restrict format, ...);

/**
**/
int vfwprintf(FILE * __restrict stream, const wchar_t * __restrict format, va_list arg);

/**
**/
int vfwscanf(FILE * __restrict stream, const wchar_t * __restrict format, va_list arg);

/**
**/
int vswprintf(wchar_t * __restrict s, size_t n, const wchar_t * __restrict format, va_list arg);

/**
**/
int vswscanf(const wchar_t * __restrict s, const wchar_t * __restrict format, va_list arg);

/**
**/
int vwprintf(const wchar_t * __restrict format, va_list arg);

/**
**/
int vwscanf(const wchar_t * __restrict format, va_list arg);

/**
**/
int wprintf(const wchar_t * __restrict format, ...);

/**
**/
int wscanf(const wchar_t * __restrict format, ...);

/* ###################  Input/Output Functions  ########################### */


/**
**/
wint_t fgetwc(FILE *stream);

/**
**/
wchar_t *fgetws(wchar_t * __restrict s, int n, FILE * __restrict stream);

/**
**/
wint_t fputwc(wchar_t c, FILE *stream);

/**
**/
int fputws(const wchar_t * __restrict s, FILE * __restrict stream);

/**
**/
int fwide(FILE *stream, int mode);

/**
**/
wint_t getwc(FILE *stream);

/**
**/
wint_t getwchar(void);

/**
**/
wint_t putwc(wchar_t c, FILE *stream);

/**
**/
wint_t putwchar(wchar_t c);

/**
**/
wint_t ungetwc(wint_t c, FILE *stream);

/* ###################  Numeric Conversions     ########################### */

/**
**/
double wcstod(const wchar_t * __restrict nptr, wchar_t ** __restrict endptr);

/**
**/
float wcstof(const wchar_t * __restrict nptr, wchar_t ** __restrict endptr);

/**
**/
long double wcstold(const wchar_t * __restrict nptr, wchar_t ** __restrict endptr);

/**
**/
long int wcstol( const wchar_t * __restrict nptr, wchar_t ** __restrict endptr, int base);

/**
**/
long long int wcstoll( const wchar_t * __restrict nptr, wchar_t ** __restrict endptr, int base);

/**
**/
unsigned long int wcstoul( const wchar_t * __restrict nptr, wchar_t ** __restrict endptr, int base);

/**
**/
unsigned long long int wcstoull( const wchar_t * __restrict nptr, wchar_t ** __restrict endptr, int base);

/* #######################  String Copying  ############################### */

/** The wcscpy function copies the wide string pointed to by s2 (including the
    terminating null wide character) into the array pointed to by s1.

    @return   The wcscpy function returns the value of s1.
**/
wchar_t *wcscpy(wchar_t * __restrict s1, const wchar_t * __restrict s2);

/** The wcsncpy function copies not more than n wide characters (those that
    follow a null wide character are not copied) from the array pointed to by
    s2 to the array pointed to by s1.

    If the array pointed to by s2 is a wide string that is shorter than n wide
    characters, null wide characters are appended to the copy in the array
    pointed to by s1, until n wide characters in all have been written.

    @return   The wcsncpy function returns the value of s1.
**/
wchar_t *wcsncpy(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n);

/** The wmemcpy function copies n wide characters from the object pointed to by
    s2 to the object pointed to by s1.

    Use this function if you know that s1 and s2 DO NOT Overlap.  Otherwise,
    use wmemmove.

    @return   The wmemcpy function returns the value of s1.
**/
wchar_t *wmemcpy(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n);

/** The wmemmove function copies n wide characters from the object pointed to by
    s2 to the object pointed to by s1. The objects pointed to by s1 and s2 are
    allowed to overlap.

    Because the UEFI BaseMemoryLib function CopyMem explicitly handles
    overlapping source and destination objects, this function and wmemcpy are
    implemented identically.

    For programming clarity, it is recommended that you use wmemcpy if you know
    that s1 and s2 DO NOT Overlap.  If s1 and s2 might possibly overlap, then
    use wmemmove.

    @return   The wmemmove function returns the value of s1.
**/
wchar_t *wmemmove(wchar_t *s1, const wchar_t *s2, size_t n);

/* ###################  String Concatenation     ########################## */

/** The wcscat function appends a copy of the wide string pointed to by s2
    (including the terminating null wide character) to the end of the wide
    string pointed to by s1. The initial wide character of s2 overwrites the
    null wide character at the end of s1.

    @return   The wcscat function returns the value of s1.
**/
wchar_t *wcscat(wchar_t * __restrict s1, const wchar_t * __restrict s2);

/** The wcsncat function appends not more than n wide characters (a null wide
    character and those that follow it are not appended) from the array pointed
    to by s2 to the end of the wide string pointed to by s1. The initial wide
    character of s2 overwrites the null wide character at the end of s1.
    A terminating null wide character is always appended to the result.

    @return   The wcsncat function returns the value of s1.
**/
wchar_t *wcsncat(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n);

/* #####################  String Comparison   ############################# */

/** The wcscmp function compares the wide string pointed to by s1 to the wide
    string pointed to by s2.

    @return   The wcscmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the wide string pointed to by s1
              is greater than, equal to, or less than the wide string
              pointed to by s2.
**/
int wcscmp(const wchar_t *s1, const wchar_t *s2);

/** The wcscoll function compares the wide string pointed to by s1 to the wide
    string pointed to by s2, both interpreted as appropriate to the LC_COLLATE
    category of the current locale.

    @return   The wcscoll function returns an integer greater than, equal to,
              or less than zero, accordingly as the wide string pointed to by
              s1 is greater than, equal to, or less than the wide string
              pointed to by s2 when both are interpreted as appropriate to
              the current locale.
**/
int wcscoll(const wchar_t *s1, const wchar_t *s2);

/** The wcsncmp function compares not more than n wide characters (those that
    follow a null wide character are not compared) from the array pointed to by
    s1 to the array pointed to by s2.

    @return   The wcsncmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the possibly null-terminated
              array pointed to by s1 is greater than, equal to, or less than
              the possibly null-terminated array pointed to by s2.
**/
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);

/** The wcsxfrm function transforms the wide string pointed to by s2 and places
    the resulting wide string into the array pointed to by s1. The
    transformation is such that if the wcscmp function is applied to two
    transformed wide strings, it returns a value greater than, equal to, or
    less than zero, corresponding to the result of the wcscoll function applied
    to the same two original wide strings. No more than n wide characters are
    placed into the resulting array pointed to by s1, including the terminating
    null wide character. If n is zero, s1 is permitted to be a null pointer.

    @return   The wcsxfrm function returns the length of the transformed wide
              string (not including the terminating null wide character). If
              the value returned is n or greater, the contents of the array
              pointed to by s1 are indeterminate.
**/
size_t wcsxfrm(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n);

/** The wmemcmp function compares the first n wide characters of the object
    pointed to by s1 to the first n wide characters of the object pointed to
    by s2.

    @return   The wmemcmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the object pointed to by s1 is
              greater than, equal to, or less than the object pointed to by s2.
**/
int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);

/* #####################  String Searching   ############################## */

/** The wcschr function locates the first occurrence of c in the wide string
    pointed to by s.  The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcschr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the wide string.
**/
wchar_t *wcschr(const wchar_t *s, wchar_t c);

/** The wcscspn function computes the length of the maximum initial segment of
    the wide string pointed to by s1 which consists entirely of wide characters
    not from the wide string pointed to by s2.

    @return   The wcscspn function returns the length of the segment.
**/
size_t wcscspn(const wchar_t *s1, const wchar_t *s2);

/** The wcspbrk function locates the first occurrence in the wide string
    pointed to by s1 of any wide character from the wide string
    pointed to by s2.

    @return   The wcspbrk function returns a pointer to the wide character
              in s1, or a null pointer if no wide character from s2 occurs
              in s1.
**/
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2);

/** The wcsrchr function locates the last occurrence of c in the wide string
    pointed to by s. The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcsrchr function returns a pointer to the wide character,
              or a null pointer if c does not occur in the wide string.
**/
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);

/** The wcsspn function computes the length of the maximum initial segment of
    the wide string pointed to by s1 which consists entirely of wide characters
    from the wide string pointed to by s2.

    @return   The wcsspn function returns the length of the segment.
**/
size_t wcsspn(const wchar_t *s1, const wchar_t *s2);

/** The wcsstr function locates the first occurrence in the wide string pointed
    to by s1 of the sequence of wide characters (excluding the terminating null
    wide character) in the wide string pointed to by s2.

    @return   The wcsstr function returns a pointer to the located wide string,
              or a null pointer if the wide string is not found. If s2 points
              to a wide string with zero length, the function returns s1.
**/
wchar_t *wcsstr(const wchar_t *s1, const wchar_t *s2);

/** A sequence of calls to the wcstok function breaks the wide string pointed
    to by s1 into a sequence of tokens, each of which is delimited by a wide
    character from the wide string pointed to by s2. The third argument points
    to a caller-provided wchar_t pointer into which the wcstok function stores
    information necessary for it to continue scanning the same wide string.

    The first call in a sequence has a non-null first argument and stores an
    initial value in the object pointed to by ptr. Subsequent calls in the
    sequence have a null first argument and the object pointed to by ptr is
    required to have the value stored by the previous call in the sequence,
    which is then updated. The separator wide string pointed to by s2 may be
    different from call to call.

    The first call in the sequence searches the wide string pointed to by s1
    for the first wide character that is not contained in the current separator
    wide string pointed to by s2. If no such wide character is found, then
    there are no tokens in the wide string pointed to by s1 and the wcstok
    function returns a null pointer. If such a wide character is found, it is
    the start of the first token.

    The wcstok function then searches from there for a wide character that is
    contained in the current separator wide string. If no such wide character
    is found, the current token extends to the end of the wide string pointed
    to by s1, and subsequent searches in the same wide string for a token
    return a null pointer. If such a wide character is found, it is overwritten
    by a null wide character, which terminates the current token.

    In all cases, the wcstok function stores sufficient information in the
    pointer pointed to by ptr so that subsequent calls, with a null pointer for
    s1 and the unmodified pointer value for ptr, shall start searching just
    past the element overwritten by a null wide character (if any).

    @return   The wcstok function returns a pointer to the first wide character
              of a token, or a null pointer if there is no token.
**/
wchar_t *wcstok(wchar_t * __restrict s1, const wchar_t * __restrict s2, wchar_t ** __restrict ptr);

/** The wmemchr function locates the first occurrence of c in the initial n
    wide characters of the object pointed to by s.

    @return   The wmemchr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the object.
**/
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);

/* ###################  String Manipulation   ############################# */

/** The wcslen function computes the length of the wide string pointed to by s.

    @return   The wcslen function returns the number of wide characters that
              precede the terminating null wide character.
**/
size_t wcslen(const wchar_t *s);

/** The wmemset function copies the value of c into each of the first n wide
    characters of the object pointed to by s.

    @return   The wmemset function returns the value of s.
**/
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);

/* #################  Date and Time Conversion  ########################### */

/**
**/
size_t wcsftime(wchar_t * __restrict s, size_t maxsize, const wchar_t * __restrict format, const struct tm * __restrict timeptr);

/* #############  Multibyte <--> Wide Character Conversion  ############### */

/**
**/
wint_t btowc(int c);

/** The wctob function determines whether c corresponds to a member of the extended
    character set whose multibyte character representation is a single byte when in the initial
    shift state.

    @Returns    The wctob function returns EOF if c does not correspond to a multibyte
                character with length one in the initial shift state. Otherwise, it
                returns the single-byte representation of that character as an
                unsigned char converted to an int.
**/
int wctob(wint_t c);

/** If ps is not a null pointer, the mbsinit function determines whether the
    pointed-to mbstate_t object describes an initial conversion state.

    @Returns    The mbsinit function returns nonzero if ps is a null pointer
                or if the pointed-to object describes an initial conversion
                state; otherwise, it returns zero.
**/
int mbsinit(const mbstate_t *ps);

/* #######  Restartable Multibyte <--> Wide Character Conversion  ######### */

/**
**/
size_t mbrlen(const char * __restrict s, size_t n, mbstate_t * __restrict ps);

/**
**/
size_t mbrtowc(wchar_t * __restrict pwc, const char * __restrict s, size_t n, mbstate_t * __restrict ps);

/**
**/
size_t wcrtomb(char * __restrict s, wchar_t wc, mbstate_t * __restrict ps);

/**
**/
size_t mbsrtowcs(wchar_t * __restrict dst, const char ** __restrict src, size_t len, mbstate_t * __restrict ps);

/** The wcsrtombs function converts a sequence of wide characters from the array
    indirectly pointed to by src into a sequence of corresponding multibyte
    characters that begins in the conversion state described by the object
    pointed to by ps. If dst is not a null pointer, the converted characters
    are then stored into the array pointed to by dst.  Conversion continues
    up to and including a terminating null wide character, which is also
    stored. Conversion stops earlier in two cases: when a wide character is
    reached that does not correspond to a valid multibyte character, or
    (if dst is not a null pointer) when the next multibyte character would
    exceed the limit of len total bytes to be stored into the array pointed
    to by dst. Each conversion takes place as if by a call to the wcrtomb
    function.)

    If dst is not a null pointer, the pointer object pointed to by src is
    assigned either a null pointer (if conversion stopped due to reaching
    a terminating null wide character) or the address just past the last wide
    character converted (if any). If conversion stopped due to reaching a
    terminating null wide character, the resulting state described is the
    initial conversion state.

    @Returns    If conversion stops because a wide character is reached that
                does not correspond to a valid multibyte character, an
                encoding error occurs: the wcsrtombs function stores the
                value of the macro EILSEQ in errno and returns (size_t)(-1);
                the conversion state is unspecified. Otherwise, it returns
                the number of bytes in the resulting multibyte character
                sequence, not including the terminating null character (if any).
**/
size_t wcsrtombs(char * __restrict dst, const wchar_t ** __restrict src, size_t len, mbstate_t * __restrict ps);

#endif  /* _WCHAR_H */
