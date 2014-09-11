/** @file
    Extended multibyte and wide character utilities.

    Within this implementation, multibyte characters are represented using the
    Unicode UTF-8 encoding and wide characters are represented using the
    16-bit UCS-2 encoding.

    Unless explicitly stated otherwise, if the execution of a function declared
    in this file causes copying to take place between objects that overlap, the
    behavior is undefined.

    The following macros are defined in this file:<BR>
    @verbatim
      NULL        Actually defined in <sys/EfiCdefs.h>
      WCHAR_MIN   Minimum value of a wide char.
      WCHAR_MAX   Maximum value of a wide char.
      WEOF        Wide char version of end-of-file.
    @endverbatim

    The following types are defined in this file:<BR>
    @verbatim
      size_t      Unsigned integer type of the result of the sizeof operator.
      wchar_t     Type of wide characters.
      wint_t      Type capable of holding all wchar_t values and WEOF.
      mbstate_t   Type of object holding multibyte conversion state.
      struct tm   Incomplete declaration of the broken-down time structure.
    @endverbatim

    The following functions are declared in this file:<BR>
@verbatim
      ###############  Formatted Input/Output Functions
      int       fwprintf  (FILE * __restrict stream,
                           const wchar_t * __restrict format, ...);
      int       fwscanf   (FILE * __restrict stream,
                           const wchar_t * __restrict format, ...);
      int       swprintf  (wchar_t * __restrict s,  size_t n,
                           const wchar_t * __restrict format, ...);
      int       swscanf   (const wchar_t * __restrict s,
                           const wchar_t * __restrict format, ...);
      int       vfwprintf (FILE * __restrict stream,
                           const wchar_t * __restrict format,   va_list arg);
      int       vfwscanf  (FILE * __restrict stream,
                           const wchar_t * __restrict format,   va_list arg);
      int       vswprintf (wchar_t * __restrict s,  size_t n,
                           const wchar_t * __restrict format,   va_list arg);
      int       vswscanf  (const wchar_t * __restrict s,
                           const wchar_t * __restrict format,   va_list arg);
      int       vwprintf  (const wchar_t * __restrict format,   va_list arg);
      int       vwscanf   (const wchar_t * __restrict format,   va_list arg);
      int       wprintf   (const wchar_t * __restrict format, ...);
      int       wscanf    (const wchar_t * __restrict format, ...);

      ###################  Input/Output Functions
      wint_t    fgetwc    (FILE *stream);
      wchar_t  *fgetws    (wchar_t * __restrict S,  int n,
                           FILE * __restrict stream);
      wint_t    fputwc    (wchar_t c, FILE *stream);
      int       fputws    (const wchar_t * __restrict S,
                           FILE * __restrict stream);
      int       fwide     (FILE *stream, int mode);
      wint_t    getwc     (FILE *stream);
      wint_t    getwchar  (void);
      wint_t    putwc     (wchar_t c, FILE *stream);
      wint_t    putwchar  (wchar_t c);
      wint_t    ungetwc   (wint_t c, FILE *stream);

      ###################  Numeric Conversions
      double                  wcstod    (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr);
      float                   wcstof    (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr);
      long double             wcstold   (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr);
      long int                wcstol    (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr, int base);
      long long int           wcstoll   (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr, int base);
      unsigned long int       wcstoul   (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr, int base);
      unsigned long long int  wcstoull  (const wchar_t * __restrict nptr,
                                         wchar_t ** __restrict endptr, int base);

      #######################  String Copying
      wchar_t  *wcscpy    (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2);
      wchar_t  *wcsncpy   (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2,   size_t n);
      wchar_t  *wmemcpy   (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2,   size_t n);
      wchar_t  *wmemmove  (wchar_t *s1, const wchar_t *s2,  size_t n);

      ###################  String Concatenation
      wchar_t  *wcscat    (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2);
      wchar_t  *wcsncat   (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2,   size_t n);

      #####################  String Comparison
      int       wcscmp    (const wchar_t *s1, const wchar_t *s2);
      int       wcscoll   (const wchar_t *s1, const wchar_t *s2);
      int       wcsncmp   (const wchar_t *s1, const wchar_t *s2,  size_t n);
      size_t    wcsxfrm   (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2,   size_t n);
      int       wmemcmp   (const wchar_t *s1,  const wchar_t *s2,  size_t n);

      #####################  String Searching
      wchar_t  *wcschr    (const wchar_t *S, wchar_t c);
      size_t    wcscspn   (const wchar_t *s1, const wchar_t *s2);
      wchar_t  *wcspbrk   (const wchar_t *s1, const wchar_t *s2);
      wchar_t  *wcsrchr   (const wchar_t *S, wchar_t c);
      size_t    wcsspn    (const wchar_t *s1, const wchar_t *s2);
      wchar_t  *wcsstr    (const wchar_t *s1, const wchar_t *s2);
      wchar_t  *wcstok    (wchar_t * __restrict s1,
                           const wchar_t * __restrict s2,
                           wchar_t ** __restrict ptr);
      wchar_t  *wmemchr   (const wchar_t *S,  wchar_t c,  size_t n);

      ###################  String Manipulation
      size_t    wcslen    (const wchar_t *S);
      wchar_t  *wmemset   (wchar_t *S,  wchar_t c,  size_t n);

      #################  Date and Time Conversion
      size_t    wcsftime  (wchar_t * __restrict S,  size_t maxsize,
                           const wchar_t * __restrict format,
                           const struct tm * __restrict timeptr);

      #############  Multibyte <--> Wide Character Conversion
      wint_t    btowc     (int c);
      int       wctob     (wint_t c);
      int       mbsinit   (const mbstate_t *ps);

      #######  Restartable Multibyte <--> Wide Character Conversion
      size_t    mbrlen    (const char * __restrict S,   size_t n,
                           mbstate_t * __restrict ps);
      size_t    mbrtowc   (wchar_t * __restrict pwc,  const char * __restrict S,
                           size_t n, mbstate_t * __restrict ps);
      size_t    wcrtomb   (char * __restrict S,   wchar_t wc,
                           mbstate_t * __restrict ps);
      size_t    mbsrtowcs (wchar_t * __restrict dst,
                           const char ** __restrict src,  size_t len,
                           mbstate_t * __restrict ps);
      size_t    wcsrtombs (char * __restrict dst,
                           const wchar_t ** __restrict src,
                           size_t len,  mbstate_t * __restrict ps);
@endverbatim

    @note   Properly constructed programs will take the following into consideration:
              - wchar_t and wint_t may be the same integer type.
              - WEOF might be a different value than that of EOF.
              - WEOF might not be negative.
              - mbstate_t objects are not intended to be inspected by programs.

    Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

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
  typedef _EFI_SIZE_T_    size_t;   /**< Unsigned integer type of the result of the sizeof operator. */
  #undef _BSD_SIZE_T_
  #undef _EFI_SIZE_T_
#endif

#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    /** An integer type capable of representing all distinct codes in the
        UCS-2 encoding supported by UEFI.
    **/
    typedef _EFI_WCHAR_T  wchar_t;
    #undef _BSD_WCHAR_T_
    #undef  _EFI_WCHAR_T
  #endif
#endif

#ifdef _BSD_MBSTATE_T_
  /** mbstate_t is an opaque object, that is not an array type, used to keep
      conversion state during multibyte stream conversions.
   */
  typedef _BSD_MBSTATE_T_ mbstate_t;
  #undef _BSD_MBSTATE_T_
#endif

#ifdef _EFI_WINT_T
  /** wint_t is an integer type unchanged by default argument promotions that can
      hold any value corresponding to members of the extended character set, as
      well as at least one value that does not correspond to any member of the
      extended character set: WEOF.
  */
  typedef _EFI_WINT_T     wint_t;
  #undef _BSD_WINT_T_
  #undef _EFI_WINT_T
#endif

#ifndef WCHAR_MIN
  /** @{
      Since wchar_t is an unsigned 16-bit value, it has a minimum value of 0, and
      a maximum value defined by __USHRT_MAX (65535 on IA processors).
  */
  #define WCHAR_MIN       0
  #define WCHAR_MAX       __USHRT_MAX
  /*@}*/
#endif

#ifndef WEOF
  /** WEOF expands to a constant expression of type wint_t whose value does not
      correspond to any member of the extended character set. It is accepted
      (and returned) by several functions, declared in this file, to indicate
      end-of-file, that is, no more input from a stream. It is also used as a
      wide character value that does not correspond to any member of the
      extended character set.
  */
  #define WEOF  ((wint_t)-1)
#endif

/* limits of wint_t -- These are NOT specified by ISO/IEC 9899 */
#ifndef WINT_MIN
  #define WINT_MIN        _EFI_WINT_MIN       /* wint_t   */
  #define WINT_MAX        _EFI_WINT_MAX       /* wint_t   */
#endif

/** Type struct tm is declared here as an incomplete structure type for use as an argument
    type by the wcsftime function.  The full structure declaration is in <time.h>.
*/
struct  tm;

/* ###############  Formatted Input/Output Functions  ##################### */

/** The fwprintf function writes output to the stream pointed to by stream,
    under control of the wide string pointed to by format that specifies how
    subsequent arguments are converted for output. If there are insufficient
    arguments for the format, the behavior is undefined. If the format is
    exhausted while arguments remain, the excess arguments are evaluated
    (as always) but are otherwise ignored. The fwprintf function returns
    when the end of the format string is encountered.

    The format is composed of zero or more directives: ordinary wide characters
    (not %), which are copied unchanged to the output stream; and conversion
    specifications, each of which results in fetching zero or more subsequent
    arguments, converting them, if applicable, according to the corresponding
    conversion specifier, and then writing the result to the output stream.

    Each conversion specification is introduced by the wide character %. After
    the %, the following appear in sequence:
      * Zero or more flags (in any order) that modify the meaning of the
        conversion specification.
      * An optional minimum field width. If the converted value has fewer wide
        characters than the field width, it is padded with spaces (by default)
        on the left (or right, if the left adjustment flag, described later,
        has been given) to the field width. The field width takes the form of
        an asterisk * (described later) or a nonnegative decimal integer.
      * An optional precision that gives the minimum number of digits to appear
        for the d, i, o, u, x, and X conversions, the number of digits to
        appear after the decimal-point wide character for e, E, f, and F
        conversions, the maximum number of significant digits for the g and G
        conversions, or the maximum number of wide characters to be written
        for s conversions. The precision takes the form of a period (.)
        followed either by an asterisk * (described later) or by an optional
        decimal integer; if only the period is specified, the precision is
        taken as zero. If a precision appears with any other conversion
        specifier, the behavior is undefined.
      * An optional length modifier that specifies the size of the argument.
      * A conversion specifier wide character that specifies the type of
        conversion to be applied.

    As noted above, a field width, or precision, or both, may be indicated by
    an asterisk. In this case, an int argument supplies the field width or
    precision. The arguments specifying field width, or precision, or both,
    must appear (in that order) before the argument (if any) to be converted.
    A negative field width argument is taken as a - flag followed by a positive
    field width. A negative precision argument is taken as if the precision
    were omitted.

    The flag wide characters and their meanings are:<BR>
    -     The result of the conversion is left-justified within the field.
          (It is right-justified if this flag is not specified.)
    +     The result of a signed conversion always begins with a plus or minus
          sign. (It begins with a sign only when a negative value is converted
          if this flag is not specified.)
    space If the first wide character of a signed conversion is not a sign, or
          if a signed conversion results in no wide characters, a space is
          prefixed to the result. If the space and + flags both appear, the
          space flag is ignored.
    #     The result is converted to an "alternative form". For o conversion,
          it increases the precision, if and only if necessary, to force the
          first digit of the result to be a zero (if the value and precision
          are both 0, a single 0 is printed). For x (or X) conversion, a
          nonzero result has 0x (or 0X) prefixed to it. For e, E, f, F, g,
          and G conversions, the result of converting a floating-point number
          always contains a decimal-point wide character, even if no digits
          follow it. (Normally, a decimal-point wide character appears in the
          result of these conversions only if a digit follows it.) For g and G
          conversions, trailing zeros are not removed from the result. For
          other conversions, the behavior is undefined.
    0     For d, i, o, u, x, X, e, E, f, F, g, and G conversions, leading zeros
          (following any indication of sign or base) are used to pad to the
          field width rather than performing space padding, except when
          converting an infinity or NaN. If the 0 and - flags both appear,
          the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a
          precision is specified, the 0 flag is ignored. For other conversions,
          the behavior is undefined.

    The length modifiers and their meanings are:<BR>
    hh    Specifies that a following d, i, o, u, x, or X conversion specifier
          applies to a signed char or unsigned char argument (the argument
          will have been promoted according to the integer promotions, but its
          value shall be converted to signed char or unsigned char before
          printing); or that a following n conversion specifier applies to a
          pointer to a signed char argument.
    h     Specifies that a following d, i, o, u, x, or X conversion specifier
          applies to a short int or unsigned short int argument (the argument
          will have been promoted according to the integer promotions, but its
          value shall be converted to short int or unsigned short int before
          printing); or that a following n conversion specifier applies to a
          pointer to a short int argument.
    l (ell)   Specifies that a following d, i, o, u, x, or X conversion
              specifier applies to a long int or unsigned long int argument;
              that a following n conversion specifier applies to a pointer to a
              long int argument; that a following c conversion specifier
              applies to a wint_t argument; that a following s conversion
              specifier applies to a pointer to a wchar_t argument; or has no
              effect on a following e, E, f, F, g, or G conversion specifier.
    ll (ell-ell)  Specifies that a following d, i, o, u, x, or X conversion
                  specifier applies to a long long int or unsigned long long int
                  argument; or that a following n conversion specifier applies
                  to a pointer to a long long int argument.
    j     Specifies that a following d, i, o, u, x, or X conversion specifier
          applies to an intmax_t or uintmax_t argument; or that a following
          n conversion specifier applies to a pointer to an intmax_t argument.
    z     Specifies that a following d, i, o, u, x, or X conversion specifier
          applies to a size_t or the corresponding signed integer type
          argument; or that a following n conversion specifier applies to a
          pointer to a signed integer type corresponding to size_t argument.
    t     Specifies that a following d, i, o, u, x, or X conversion specifier
          applies to a ptrdiff_t or the corresponding unsigned integer type
          argument; or that a following n conversion specifier applies to a
          pointer to a ptrdiff_t argument.
    L     Specifies that a following a, A, e, E, f, F, g, or G conversion
          specifier applies to a long double argument.

    If a length modifier appears with any conversion specifier other than as
    specified above, the behavior is undefined.

    The conversion specifiers and their meanings are:<BR>
    d,i     The int argument is converted to signed decimal in the
            style [-]dddd. The precision specifies the minimum number of digits
            to appear; if the value being converted can be represented in fewer
            digits, it is expanded with leading zeros. The default precision
            is 1. The result of converting a zero value with a precision of
            zero is no wide characters.
    o,u,x,X The unsigned int argument is converted to unsigned octal (o),
            unsigned decimal (u), or unsigned hexadecimal notation (x or X) in
            the style dddd; the letters abcdef are used for x conversion and
            the letters ABCDEF for X conversion. The precision specifies the
            minimum number of digits to appear; if the value being converted
            can be represented in fewer digits, it is expanded with leading
            zeros. The default precision is 1. The result of converting a zero
            value with a precision of zero is no wide characters.
    f,F     A double argument representing a floating-point number is converted
            to decimal notation in the style [-]ddd.ddd, where the number of
            digits after the decimal-point wide character is equal to the
            precision specification. If the precision is missing, it is taken
            as 6; if the precision is zero and the # flag is not specified, no
            decimal-point wide character appears. If a decimal-point wide
            character appears, at least one digit appears before it. The value
            is rounded to the appropriate number of digits.<BR>
            A double argument representing an infinity is converted to [-]inf.
            A double argument representing a NaN is converted to [-]nan.
            The F conversion specifier produces INF or NAN instead
            of inf or nan, respectively.
    e,E     A double argument representing a floating-point number is converted
            in the style [-]d.ddd e +/- dd, where there is one digit (which is
            nonzero if the argument is nonzero) before the decimal-point wide
            character and the number of digits after it is equal to the
            precision; if the precision is missing, it is taken as 6; if the
            precision is zero and the # flag is not specified, no decimal-point
            wide character appears. The value is rounded to the appropriate
            number of digits. The E conversion specifier produces a number with
            E instead of e introducing the exponent. The exponent always
            contains at least two digits, and only as many more digits as
            necessary to represent the exponent. If the value is zero, the
            exponent is zero. A double argument representing an infinity or NaN
            is converted in the style of an f or F conversion specifier.
    g,G     A double argument representing a floating-point number is converted
            in style f or e (or in style F or E in the case of a G conversion
            specifier), depending on the value converted and the precision.
            Let P equal the precision if nonzero, 6 if the precision is
            omitted, or 1 if the precision is zero. Then, if a conversion with
            style E would have an exponent of X:
              - if P > X = -4, the conversion is with style f (or F) and
                precision P - (X + 1).
              - otherwise, the conversion is with style e (or E) and
                precision P - 1.
            Finally, unless the # flag is used, any trailing zeros are removed
            from the fractional portion of the result and the decimal-point
            wide character is removed if there is no fractional portion
            remaining.  A double argument representing an infinity or NaN is
            converted in the style of an f or F conversion specifier.
    c       If no l length modifier is present, the int argument is converted
            to a wide character as if by calling btowc and the resulting wide
            character is written.  If an l length modifier is present, the
            wint_t argument is converted to wchar_t and written.
    s       If no l length modifier is present, the argument shall be a pointer
            to the initial element of a character array containing a multibyte
            character sequence beginning in the initial shift state. Characters
            from the array are converted as if by repeated calls to the mbrtowc
            function, with the conversion state described by an mbstate_t
            object initialized to zero before the first multibyte character is
            converted, and written up to (but not including) the terminating
            null wide character. If the precision is specified, no more than
            that many wide characters are written. If the precision is not
            specified or is greater than the size of the converted array, the
            converted array shall contain a null wide character.<BR>
            If an l length modifier is present, the argument shall be a pointer
            to the initial element of an array of wchar_t type. Wide characters
            from the array are written up to (but not including) a terminating
            null wide character. If the precision is specified, no more than
            that many wide characters are written. If the precision is not
            specified or is greater than the size of the array, the array
            shall contain a null wide character.
    p       The argument shall be a pointer to void. The value of the pointer
            is converted to a sequence of printing wide characters, in an
            implementation-defined manner.
    n       The argument shall be a pointer to signed integer into which is
            written the number of wide characters written to the output stream
            so far by this call to fwprintf. No argument is converted, but one
            is consumed. If the conversion specification includes any flags, a
            field width, or a precision, the behavior is undefined.
    %       A % wide character is written. No argument is converted. The
            complete conversion specification is %%.


    @param[in]  stream    An open File specifier to which the output is sent.
    @param[in]  format    A wide character sequence containing characters
                          to be copied unchanged, and conversion specifiers
                          which convert their associated arguments.
    @param      ...       Variable number of parameters as required by format.

    @return   The fwprintf function returns the number of wide characters
              transmitted, or a negative value if an output or encoding error
              occurred.
**/
int fwprintf(FILE * __restrict stream, const wchar_t * __restrict format, ...);

/** The fwscanf function reads input from the stream pointed to by stream,
    under control of the wide string pointed to by format that specifies
    the admissible input sequences and how they are to be converted for
    assignment, using subsequent arguments as pointers to the objects to
    receive the converted input. If there are insufficient arguments for
    the format, the behavior is undefined. If the format is exhausted while
    arguments remain, the excess arguments are evaluated (as always) but are
    otherwise ignored.

    The format is composed of zero or more directives: one or more white-space
    wide characters, an ordinary wide character (neither % nor a white-space
    wide character), or a conversion specification. Each conversion
    specification is introduced by the wide character %. After the %, the
    following appear in sequence:
      - An optional assignment-suppressing wide character *.
      - An optional decimal integer greater than zero that specifies the
        maximum field width (in wide characters).
      - An optional length modifier that specifies the size of the receiving object.
      - A conversion specifier wide character that specifies the type of
        conversion to be applied.

    The fwscanf function executes each directive of the format in turn. If a
    directive fails, as detailed below, the function returns. Failures are
    described as input failures (due to the occurrence of an encoding error
    or the unavailability of input characters), or matching failures
    (due to inappropriate input).

    A directive composed of white-space wide character(s) is executed by
    reading input up to the first non-white-space wide character (which remains
    unread), or until no more wide characters can be read.

    A directive that is an ordinary wide character is executed by reading the
    next wide character of the stream. If that wide character differs from the
    directive, the directive fails and the differing and subsequent wide
    characters remain unread. Similarly, if end-of-file, an encoding error, or
    a read error prevents a wide character from being read, the directive fails.

    A directive that is a conversion specification defines a set of matching
    input sequences, as described below for each specifier. A conversion
    specification is executed in the following steps:
      - Input white-space wide characters (as specified by the iswspace
        function) are skipped, unless the specification includes
        a [, c, or n specifier.
      - An input item is read from the stream, unless the specification
        includes an n specifier. An input item is defined as the longest
        sequence of input wide characters which does not exceed any specified
        field width and which is, or is a prefix of, a matching input sequence.
        The first wide character, if any, after the input item remains unread.
        If the length of the input item is zero, the execution of the directive
        fails; this condition is a matching failure unless end-of-file, an
        encoding error, or a read error prevented input from the stream, in
        which case it is an input failure.
      - Except in the case of a % specifier, the input item (or, in the case of
        a %n directive, the count of input wide characters) is converted to a
        type appropriate to the conversion specifier. If the input item is not
        a matching sequence, the execution of the directive fails: this
        condition is a matching failure. Unless assignment suppression was
        indicated by a *, the result of the conversion is placed in the object
        pointed to by the first argument following the format argument that has
        not already received a conversion result. If this object does not have
        an appropriate type, or if the result of the conversion cannot be
        represented in the object, the behavior is undefined.

    The length modifiers and their meanings are:<BR>
    hh      Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to signed char
            or unsigned char.
    h       Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to short int
            or unsigned short int.
    l (ell) Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to long int or
            unsigned long int; that a following e, E, f, F, g, or G conversion
            specifier applies to an argument with type pointer to double; or
            that a following c, s, or [ conversion specifier applies to an
            argument with type pointer to wchar_t.
    ll (ell-ell)  Specifies that a following d, i, o, u, x, X, or n conversion
                  specifier applies to an argument with type
                  pointer to long long int or unsigned long long int.
    j       Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to intmax_t
            or uintmax_t.
    z       Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to size_t or the
            corresponding signed integer type.
    t       Specifies that a following d, i, o, u, x, X, or n conversion
            specifier applies to an argument with type pointer to ptrdiff_t or
            the corresponding unsigned integer type.
    L       Specifies that a following e, E, f, F, g, or G conversion specifier
            applies to an argument with type pointer to long double.

    If a length modifier appears with any conversion specifier other than as
    specified above, the behavior is undefined.

    The conversion specifiers and their meanings are:<BR>
    d       Matches an optionally signed decimal integer, whose format is the
            same as expected for the subject sequence of the wcstol function
            with the value 10 for the base argument. The corresponding argument
            shall be a pointer to signed integer.
    i       Matches an optionally signed integer, whose format is the same as
            expected for the subject sequence of the wcstol function with the
            value 0 for the base argument. The corresponding argument shall be
            a pointer to signed integer.
    o       Matches an optionally signed octal integer, whose format is the
            same as expected for the subject sequence of the wcstoul function
            with the value 8 for the base argument. The corresponding argument
            shall be a pointer to unsigned integer.
    u       Matches an optionally signed decimal integer, whose format is the
            same as expected for the subject sequence of the wcstoul function
            with the value 10 for the base argument. The corresponding argument
            shall be a pointer to unsigned integer.
    x       Matches an optionally signed hexadecimal integer, whose format is
            the same as expected for the subject sequence of the wcstoul
            function with the value 16 for the base argument. The corresponding
            argument shall be a pointer to unsigned integer.
    e,f,g   Matches an optionally signed floating-point number, infinity, or
            NaN, whose format is the same as expected for the subject sequence
            of the wcstod function. The corresponding argument shall be a
            pointer to float.
    c       Matches a sequence of wide characters of exactly the number
            specified by the field width (1 if no field width is present in the
            directive).<BR>
            If no l length modifier is present, characters from the input field
            are converted as if by repeated calls to the wcrtomb function, with
            the conversion state described by an mbstate_t object initialized
            to zero before the first wide character is converted. The
            corresponding argument shall be a pointer to the initial element of
            a character array large enough to accept the sequence. No null
            character is added.<BR>
            If an l length modifier is present, the corresponding argument
            shall be a pointer to the initial element of an array of
            wchar_t large enough to accept the sequence.
            No null wide character is added.
    s       Matches a sequence of non-white-space wide characters.
            If no l length modifier is present, characters from the input field
            are converted as if by repeated calls to the wcrtomb function, with
            the conversion state described by an mbstate_t object initialized
            to zero before the first wide character is converted. The
            corresponding argument shall be a pointer to the initial element of
            a character array large enough to accept the sequence and a
            terminating null character, which will be added automatically.<BR>
            If an l length modifier is present, the corresponding argument
            shall be a pointer to the initial element of an array of wchar_t
            large enough to accept the sequence and the terminating null wide
            character, which will be added automatically.
    [       Matches a nonempty sequence of wide characters from a set of
            expected characters (the scanset).<BR>
            If no l length modifier is present, characters from the input field
            are converted as if by repeated calls to the wcrtomb function, with
            the conversion state described by an mbstate_t object initialized
            to zero before the first wide character is converted. The
            corresponding argument shall be a pointer to the initial element of
            a character array large enough to accept the sequence and a
            terminating null character, which will be added automatically.<BR>
            If an l length modifier is present, the corresponding argument
            shall be a pointer to the initial element of an array of wchar_t
            large enough to accept the sequence and the terminating null wide
            character, which will be added automatically.<BR>
            The conversion specifier includes all subsequent wide characters
            in the format string, up to and including the matching right
            bracket (]). The wide characters between the brackets
            (the scanlist) compose the scanset, unless the wide character after
            the left bracket is a circumflex (^), in which case the scanset
            contains all wide characters that do not appear in the scanlist
            between the circumflex and the right bracket. If the conversion
            specifier begins with [] or [^], the right bracket wide character
            is in the scanlist and the next following right bracket wide
            character is the matching right bracket that ends the specification;
            otherwise the first following right bracket wide character is the
            one that ends the specification. If a - wide character is in the
            scanlist and is not the first, nor the second where the first wide
            character is a ^, nor the last character,
            the - is added to the scanset.
    p       Matches the set of sequences produced by the %p conversion of the
            fwprintf function. The corresponding argument is a pointer to a
            pointer to void. The input item is converted to a pointer value. If
            the input item is a value converted earlier during the same program
            execution, the pointer that results will compare equal to that
            value.
    n       No input is consumed. The corresponding argument is a pointer to
            signed integer into which is to be written the number of wide
            characters read from the input stream so far by this call to the
            fwscanf function. Execution of a %n directive does not increment
            the assignment count returned at the completion of execution of the
            fwscanf function. No argument is converted, but one is consumed.
    %       Matches a single % wide character; no conversion or assignment
            occurs. The complete conversion specification shall be %%.

    The conversion specifiers E, F, G, and X are also valid and behave the same
    as, respectively, e, f, g, and x.

    Trailing white space (including new-line wide characters) is left unread
    unless matched by a directive. The success of literal matches and
    suppressed assignments is not directly determinable other than via
    the %n directive.

    @param[in]  stream    An open File specifier from which the input is read.
    @param[in]  format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param      ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return   The fwscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              function returns the number of input items assigned, which can be
              fewer than provided for, or even zero, in the event of an early
              matching failure.
**/
int fwscanf(FILE * __restrict stream, const wchar_t * __restrict format, ...);

/** Formatted wide-character output to a buffer.

    The swprintf function is equivalent to fwprintf, except that the argument s
    specifies an array of wide characters into which the generated output is to
    be written, rather than written to a stream. No more than n wide characters
    are written, including a terminating null wide character, which is always
    added (unless n is zero).

    @param[out]   s         A pointer to the array to receive the formatted output.
    @param[in]    n         Maximum number of characters to write into buffer s.
    @param[in]    format    A wide character sequence containing characters
                            to be copied unchanged, and conversion specifiers
                            which convert their associated arguments.  Copied and
                            converted characters are written to the array pointed
                            to by s.
    @param        ...       Variable number of parameters as required by format.

    @return   The swprintf function returns the number of wide characters
              written in the array, not counting the terminating null wide
              character, or a negative value if an encoding error occurred or
              if n or more wide characters were requested to be written.
**/
int swprintf(wchar_t * __restrict s, size_t n, const wchar_t * __restrict format, ...);

/** Formatted wide input from a string.

    The swscanf function is equivalent to fwscanf, except that the argument
    Buff specifies a wide string from which the input is to be obtained, rather
    than from a stream. Reaching the end of the wide string is equivalent to
    encountering end-of-file for the fwscanf function.

    @param[in]  Buff      Pointer to the string from which to obtain input.
    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[out] ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return   The swscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              swscanf function returns the number of input items assigned,
              which can be fewer than provided for, or even zero, in the event
              of an early matching failure.
**/
int swscanf(const wchar_t * __restrict Buff, const wchar_t * __restrict Format, ...);

/** Print formatted values from an argument list.

The vfwprintf function is equivalent to fwprintf, with the variable argument list
replaced by Args, which shall have been initialized by the va_start macro (and
possibly subsequent va_arg calls). The vfwprintf function does not invoke the
va_end macro.

    @param[in]  Stream    The output stream to receive the formatted output.
    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[in]  Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vfwprintf function returns the number of wide characters
              transmitted, or a negative value if an output or encoding
              error occurred.
**/
int vfwprintf(FILE * __restrict Stream, const wchar_t * __restrict Format, va_list Args);

/** Formatted input from a stream.

    The vfwscanf function is equivalent to fwscanf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start
    macro (and possibly subsequent va_arg calls). The vfwscanf function does
    not invoke the va_end macro.

    @param[in]  Stream    The input stream.
    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[in]  Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vfwscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              vfwscanf function returns the number of input items assigned,
              which can be fewer than provided for, or even zero, in the event
              of an early matching failure.
**/
int vfwscanf(FILE * __restrict Stream, const wchar_t * __restrict Format, va_list Args);

/** Formatted print, to a buffer, from an argument list.

    The vswprintf function is equivalent to swprintf, with the variable
    argument list replaced by Args, which must have been initialized by the
    va_start macro (and possibly subsequent va_arg calls). The vswprintf
    function does not invoke the va_end macro.

    @param[in]  S         A pointer to the array to receive the formatted output.
    @param[in]  N         Maximum number of characters to write into array S.
    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[in]  Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vswprintf function returns the number of wide characters
              written in the array, not counting the terminating null wide
              character, or a neg ative value if an encoding error occurred or
              if n or more wide characters were requested to be generated.
**/
int vswprintf(wchar_t * __restrict S, size_t N, const wchar_t * __restrict Format, va_list Args);

/** Formatted input from a string, using an argument list.

    The vswscanf function is equivalent to swscanf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start
    macro. The vswscanf function does not invoke the va_end macro.

    @param[in]  S         Pointer to the string from which to obtain input.
    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[out] Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vswscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              vswscanf function returns the number of input items assigned,
              which can be fewer than provided for, or even zero, in the event
              of an early matching failure.
**/
int vswscanf(const wchar_t * __restrict S, const wchar_t * __restrict Format, va_list Args);

/** Formatted print, to stdout, from an argument list.

    The vwprintf function is equivalent to wprintf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start
    macro. The vwprintf function does not invoke the va_end macro.

    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[out] Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vwprintf function returns the number of wide characters
              transmitted, or a negative value if an output or encoding error
              occurred.
**/
int vwprintf(const wchar_t * __restrict Format, va_list Args);

/** Formatted input, from stdin, to an argument list.

    The vwscanf function is equivalent to wscanf, with the variable argument
    list replaced by arg, which shall have been initialized by the va_start
    macro. The vwscanf function does not invoke the va_end macro.

    @param[in]  Format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.
    @param[out] Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vwscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              vwscanf function returns the number of input items assigned,
              which can be fewer than provided for, or even zero, in the event
              of an early matching failure.
**/
int vwscanf(const wchar_t * __restrict Format, va_list Args);

/** Formatted print to stdout.

    The wprintf function is equivalent to fwprintf with the argument stdout
    specifying the output stream.

    @param[in]  format    A wide character sequence containing characters
                          to be copied unchanged, and conversion specifiers
                          which convert their associated arguments.
    @param      ...       Variable number of parameters as required by format.

    @return   The wprintf function returns the number of wide characters
              transmitted, or a negative value if an output or encoding error
              occurred.
**/
int wprintf(const wchar_t * __restrict Format, ...);

/** Formatted input from stdin.

    The wscanf function is equivalent to fwscanf with the argument stdin
    specifying the input stream.

    @param[in]  format    A wide character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param      ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return   The wscanf function returns the value of the macro EOF if an
              input failure occurs before any conversion. Otherwise, the
              wscanf function returns the number of input items assigned,
              which can be fewer than provided for, or even zero, in the event
              of an early matching failure.
**/
int wscanf(const wchar_t * __restrict format, ...);

/* ###################  Input/Output Functions  ########################### */


/** Get a character from an input Stream.

If the end-of-file indicator for the input stream pointed to by stream is not set and a
next wide character is present, the fgetwc function obtains that wide character as a
wchar_t converted to a wint_t and advances the associated file position indicator for
the stream (if defined).

    @param[in]  Stream    An input stream from which to obtain a character.

    @return   If the end-of-file indicator for the stream is set, or if the stream is at end-of-file, the endof-
file indicator for the stream is set and the fgetwc function returns WEOF. Otherwise,
the fgetwc function returns the next wide character from the input stream pointed to by
stream. If a read error occurs, the error indicator for the stream is set and the fgetwc
function returns WEOF. If an encoding error occurs (including too few bytes), the value of
the macro EILSEQ is stored in errno and the fgetwc function returns WEOF.
**/
wint_t fgetwc(FILE *Stream);

/** Read a string from an input stream into a buffer.

    The fgetws function reads at most one less than the number of
    wide characters specified by n from the stream pointed to by
    stream into the array pointed to by s. No additional wide
    characters are read after a new-line wide character (which is
    retained) or after end-of-file. A null wide character is written
    immediately after the last wide character read into the array.

    @param[out] S         A pointer to the array to receive the input string.
    @param[in]  Limit     The maximum number of characters to put into Buff,
                          including the terminating null character.
    @param[in]  Stream    An input stream from which to obtain the string.

    @return   The fgetws function returns S if successful. If end-of-file is
              encountered and no characters have been read into the array, the
              contents of the array remain unchanged and a null pointer is
              returned. If a read or encoding error occurs during the
              operation, the array contents are indeterminate and a
              null pointer is returned.
**/
wchar_t *fgetws(wchar_t * __restrict S, int Limit, FILE * __restrict Stream);

/** Write a character to an output stream.

The fputwc function writes the wide character specified by c to the output stream
pointed to by stream, at the position indicated by the associated file position indicator
for the stream (if defined), and advances the indicator appropriately. If the file cannot
support positioning requests, or if the stream was opened with append mode, the
character is appended to the output stream.

    @param[in]  C       The character to be written to Stream.
    @param[in]  Stream  The output stream that C is to be written to.

    @return   The fputwc function returns the wide character written. If a write error occurs, the
error indicator for the stream is set and fputwc returns WEOF. If an encoding error
occurs, the value of the macro EILSEQ is stored in errno and fputwc returns WEOF.
**/
wint_t fputwc(wchar_t C, FILE *Stream);

/** Write a string to an output stream.

The fputws function writes the wide string pointed to by S to the stream pointed to by
Stream. The terminating null wide character is not written.

    @param[in]  String  The character string to be written to Stream.
    @param[in]  Stream  The output stream that String is to be written to.

    @return   The fputws function returns EOF if a write or encoding error occurs; otherwise, it
returns a nonnegative value.
**/
int fputws(const wchar_t * __restrict S, FILE * __restrict Stream);

/** Query or set a stream's orientation.

The fwide function determines the orientation of the stream pointed to by stream. If
Mode is greater than zero, the function first attempts to make the stream wide oriented. If
Mode is less than zero, the function first attempts to make the stream byte oriented.
Otherwise, Mode is zero and the function does not alter the orientation of the stream.

    @param[in]  Stream    The stream to be queried.
    @param[in]  Mode      Control value selecting between quering or setting
                          the Stream's orientation.
    @return   The fwide function returns a value greater than zero if, after the call, the stream has
wide orientation, a value less than zero if the stream has byte orientation, or zero if the
stream has no orientation.
**/
int fwide(FILE *Stream, int Mode);

/** Get a character from an input stream.

The getwc function is equivalent to fgetwc, except that if it is implemented as a
macro, it may evaluate Stream more than once, so the argument should never be an
expression with side effects.

    @param[in]  Stream    The stream to be read.

    @return   The getwc function returns the next wide character from the input stream pointed to by
stream, or WEOF.
**/
wint_t getwc(FILE *Stream);

/** Get a character from stdin.

    The getwchar function is equivalent to getwc with the argument stdin.

    @return   The getwchar function returns the next wide character from the
              input stream pointed to by stdin, or WEOF.
**/
wint_t getwchar(void);

/** Write a character to an output stream.

The putwc function is equivalent to fputwc, except that if it is implemented as a
macro, it may evaluate Stream more than once, so the Stream argument should never be an
expression with side effects.

    @param[in]  C       The wide character to be written to Stream.
    @param[in]  Stream  The output stream that C is to be written to.

    @return   The putwc function returns the wide character written, or WEOF.
**/
wint_t putwc(wchar_t C, FILE *Stream);

/** Write a character to stdout.

The putwchar function is equivalent to putwc with the second argument stdout.

    @param[in]  C       The wide character to be written to stdout.

    @return   The putwchar function returns the character written, or WEOF.
**/
wint_t putwchar(wchar_t C);

/** Return a character to the input Stream as if it had not been read.

The ungetwc function pushes the wide character specified by C back onto the input
stream pointed to by Stream. Pushed-back wide characters will be returned by
subsequent reads on that stream in the reverse order of their pushing. A successful
intervening call (with the stream pointed to by Stream) to a file positioning function
(fseek, fsetpos, or rewind) discards any pushed-back wide characters for the
stream. The external storage corresponding to the stream is unchanged.

One wide character of pushback is guaranteed, even if the call to the ungetwc function
follows just after a call to a formatted wide character input function fwscanf,
vfwscanf, vwscanf, or wscanf. If the ungetwc function is called too many times
on the same stream without an intervening read or file positioning operation on that
stream, the operation may fail.

If the value of C equals that of the macro WEOF, the operation fails and the input stream is
unchanged.

A successful call to the ungetwc function clears the end-of-file indicator for the stream.
The value of the file position indicator for the stream after reading or discarding all
pushed-back wide characters is the same as it was before the wide characters were pushed
back. For a text or binary stream, the value of its file position indicator after a successful
call to the ungetwc function is unspecified until all pushed-back wide characters are
read or discarded.

    @param[in]  C       The wide character to push back onto the Stream.
    @param[in]  Stream  The output stream that C is to be pushed back onto.

    @return   The ungetwc function returns the character pushed back,
              or WEOF if the operation fails.
**/
wint_t ungetwc(wint_t C, FILE *Stream);

/* ###################  Numeric Conversions     ########################### */

/** @{
The wcstod, wcstof, and wcstold functions convert the initial portion of the wide
string pointed to by nptr to double, float, and long double representation,
respectively. First, they decompose the input string into three parts: an initial, possibly
empty, sequence of white-space wide characters (as specified by the iswspace
function), a subject sequence resembling a floating-point constant or representing an
infinity or NaN; and a final wide string of one or more unrecognized wide characters,
including the terminating null wide character of the input wide string. Then, they attempt
to convert the subject sequence to a floating-point number, and return the result.

    @param[in]  Nptr    Pointer to the string to convert to a floating-point value.
    @param[in]  EndPtr  Optional pointer to an object in which to store a pointer
                        to the final wide string.

The functions return the converted value, if any. If no conversion could be performed,
zero is returned. If the correct value is outside the range of representable values, plus or
minus HUGE_VAL, HUGE_VALF, or HUGE_VALL is returned (according to the return
type and sign of the value), and the value of the macro ERANGE is stored in errno. If
the result underflows (7.12.1), the functions return a value whose magnitude is no greater
than the smallest normalized positive number in the return type. A pointer to the
final wide string is stored in the object pointed to by endptr, provided that endptr is
not a null pointer.
**/
double      wcstod  (const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr);
float       wcstof  (const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr);
long double wcstold (const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr);
/*@}*/

/** @{
The wcstol, wcstoll, wcstoul, and wcstoull functions convert the initial
portion of the wide string pointed to by nptr to long int, long long int,
unsigned long int, and unsigned long long int representation,
respectively. First, they decompose the input string into three parts: an initial, possibly
empty, sequence of white-space wide characters (as specified by the iswspace
function), a subject sequence resembling an integer represented in some radix determined
by the value of base, and a final wide string of one or more unrecognized wide
characters, including the terminating null wide character of the input wide string. Then,
they attempt to convert the subject sequence to an integer, and return the result.

    @param[in]  Nptr    Pointer to the string to convert.
    @param[in]  EndPtr  Optional pointer to an object in which to store a pointer
                        to the final wide string.
    @param[in]  Base    Base, 0 to 36, of the value represented by the string
                        pointed to by Nptr.

    @return   The wcstol, wcstoll, wcstoul, and wcstoull functions return the converted
value, if any. If no conversion could be performed, zero is returned. If the correct value
is outside the range of representable values, LONG_MIN, LONG_MAX, LLONG_MIN,
LLONG_MAX, ULONG_MAX, or ULLONG_MAX is returned (according to the return type
sign of the value, if any), and the value of the macro ERANGE is stored in errno.
**/
long int                wcstol  ( const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr, int Base);
long long int           wcstoll ( const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr, int Base);
unsigned long int       wcstoul ( const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr, int Base);
unsigned long long int  wcstoull( const wchar_t * __restrict Nptr, wchar_t ** __restrict EndPtr, int Base);
/*@}*/

/* #######################  String Copying  ############################### */

/** The wcscpy function copies the wide string pointed to by Src (including the
    terminating null wide character) into the array pointed to by Dest.

    @return   The wcscpy function returns the value of Dest.
**/
wchar_t *wcscpy(wchar_t * __restrict Dest, const wchar_t * __restrict Src);

/** The wcsncpy function copies not more than n wide characters (those that
    follow a null wide character are not copied) from the array pointed to by
    Src to the array pointed to by Dest.

    If the array pointed to by Src is a wide string that is shorter than n wide
    characters, null wide characters are appended to the copy in the array
    pointed to by Dest, until n wide characters in all have been written.

    @return   The wcsncpy function returns the value of Dest.
**/
wchar_t *wcsncpy(wchar_t * __restrict Dest, const wchar_t * __restrict Src, size_t n);

/** The wmemcpy function copies n wide characters from the object pointed to by
    Src to the object pointed to by Dest.

    Use this function if you know that Dest and Src DO NOT Overlap.  Otherwise,
    use wmemmove.

    @return   The wmemcpy function returns the value of Dest.
**/
wchar_t *wmemcpy(wchar_t * __restrict Dest, const wchar_t * __restrict Src, size_t n);

/** The wmemmove function copies n wide characters from the object pointed to by
    Src to the object pointed to by Dest. The objects pointed to by Dest and Src are
    allowed to overlap.

    Because the UEFI BaseMemoryLib function CopyMem explicitly handles
    overlapping source and destination objects, this function and wmemcpy are
    implemented identically.

    For programming clarity, it is recommended that you use wmemcpy if you know
    that Dest and Src DO NOT Overlap.  If Dest and Src might possibly overlap, then
    use wmemmove.

    @return   The wmemmove function returns the value of Dest.
**/
wchar_t *wmemmove(wchar_t *Dest, const wchar_t *Src, size_t n);

/* ###################  String Concatenation     ########################## */

/** The wcscat function appends a copy of the wide string pointed to by Src
    (including the terminating null wide character) to the end of the wide
    string pointed to by Dest. The initial wide character of Src overwrites the
    null wide character at the end of Dest.

    @return   The wcscat function returns the value of Dest.
**/
wchar_t *wcscat(wchar_t * __restrict Dest, const wchar_t * __restrict Src);

/** The wcsncat function appends not more than n wide characters (a null wide
    character and those that follow it are not appended) from the array pointed
    to by Src to the end of the wide string pointed to by Dest. The initial wide
    character of Src overwrites the null wide character at the end of Dest.
    A terminating null wide character is always appended to the result.

    @return   The wcsncat function returns the value of Dest.
**/
wchar_t *wcsncat(wchar_t * __restrict Dest, const wchar_t * __restrict Src, size_t n);

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

/** The wcschr function locates the first occurrence of C in the wide string
    pointed to by S.  The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcschr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the wide string.
**/
wchar_t *wcschr(const wchar_t *S, wchar_t C);

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

/** The wcsrchr function locates the last occurrence of C in the wide string
    pointed to by S. The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcsrchr function returns a pointer to the wide character,
              or a null pointer if C does not occur in the wide string.
**/
wchar_t *wcsrchr(const wchar_t *S, wchar_t C);

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

/** The wmemchr function locates the first occurrence of C in the initial n
    wide characters of the object pointed to by S.

    @return   The wmemchr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the object.
**/
wchar_t *wmemchr(const wchar_t *S, wchar_t C, size_t n);

/* ###################  String Manipulation   ############################# */

/** The wcslen function computes the length of the wide string pointed to by S.

    @return   The wcslen function returns the number of wide characters that
              precede the terminating null wide character.
**/
size_t wcslen(const wchar_t *S);

/** The wmemset function copies the value of C into each of the first n wide
    characters of the object pointed to by S.

    @return   The wmemset function returns the value of S.
**/
wchar_t *wmemset(wchar_t *S, wchar_t C, size_t n);

/* #################  Date and Time Conversion  ########################### */

/**
The wcsftime function is equivalent to the strftime function, except that:
  - The argument s points to the initial element of an array of wide characters into which
the generated output is to be placed.
  - The argument maxsize indicates the limiting number of wide characters.
  - The argument format is a wide string and the conversion specifiers are replaced by
corresponding sequences of wide characters.
  - The return value indicates the number of wide characters.

If the total number of resulting wide characters including the terminating null wide
character is not more than maxsize, the wcsftime function returns the number of
wide characters placed into the array pointed to by s not including the terminating null
wide character. Otherwise, zero is returned and the contents of the array are
indeterminate.
**/
size_t wcsftime(wchar_t * __restrict S, size_t maxsize, const wchar_t * __restrict format, const struct tm * __restrict timeptr);

/* #############  Multibyte <--> Wide Character Conversion  ############### */

/** The btowc function determines whether C constitutes a valid single-byte
    character in the initial shift state.

    @return   The btowc function returns WEOF if c has the value EOF or if
              (unsigned char)C does not constitute a valid single-byte
              character in the initial shift state. Otherwise, it returns the
              wide character representation of that character.
**/
wint_t btowc(int C);

/** The wctob function determines whether C corresponds to a member of the extended
    character set whose multibyte character representation is a single byte when in the initial
    shift state.

    @return     The wctob function returns EOF if C does not correspond to a multibyte
                character with length one in the initial shift state. Otherwise, it
                returns the single-byte representation of that character as an
                unsigned char converted to an int.
**/
int wctob(wint_t C);

/** If ps is not a null pointer, the mbsinit function determines whether the
    pointed-to mbstate_t object describes an initial conversion state.

    @return     The mbsinit function returns nonzero if ps is a null pointer
                or if the pointed-to object describes an initial conversion
                state; otherwise, it returns zero.
**/
int mbsinit(const mbstate_t *ps);

/* #######  Restartable Multibyte <--> Wide Character Conversion  ######### */

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
**/
size_t mbrlen(const char * __restrict S, size_t n, mbstate_t * __restrict pS);

/** Restartable Multibyte to Wide character conversion.
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
**/
size_t mbrtowc(wchar_t * __restrict pwc, const char * __restrict S, size_t n, mbstate_t * __restrict ps);

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

    @return   The wcrtomb function returns the number of bytes stored in the
              array object (including any shift sequences). When wc is not a
              valid wide character, an encoding error occurs: the function
              stores the value of the macro EILSEQ in errno and
              returns (size_t)(-1); the conversion state is unspecified.
**/
size_t wcrtomb(char * __restrict S, wchar_t wc, mbstate_t * __restrict ps);

/** Convert a sequence of multibyte characters into a sequence of wide characters.
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

    @param[in]    dst   Destination for the Wide character sequence.
    @param[in]    src   Pointer to Pointer to MBCS char. sequence to convert.
    @param[in]    len   Length of dest, in WIDE characters.
    @param[in]    ps    Pointer to the conversion state object to be used for this conversion.

    @return   If the input conversion encounters a sequence of bytes that do
              not form a valid multibyte character, an encoding error occurs:
              the mbsrtowcs function stores the value of the macro EILSEQ in
              errno and returns (size_t)(-1); the conversion state is
              unspecified. Otherwise, it returns the number of multibyte
              characters successfully converted, not including the terminating
              null character (if any).
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

    @param[in]    dst   Destination for the MBCS sequence.
    @param[in]    src   Pointer to Pointer to wide char. sequence to convert.
    @param[in]    len   Length of dest, in bytes.
    @param[in]    ps    Pointer to the conversion state object to be used for this conversion.

    @return     If conversion stops because a wide character is reached that
                does not correspond to a valid multibyte character, an
                encoding error occurs: the wcsrtombs function stores the
                value of the macro EILSEQ in errno and returns (size_t)(-1);
                the conversion state is unspecified. Otherwise, it returns
                the number of bytes in the resulting multibyte character
                sequence, not including the terminating null character (if any).
**/
size_t wcsrtombs(char * __restrict dst, const wchar_t ** __restrict src, size_t len, mbstate_t * __restrict ps);

#endif  /* _WCHAR_H */
