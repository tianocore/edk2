/*-
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)stdio.h 8.5 (Berkeley) 4/29/95
 */
/*  $NetBSD: stdio.h,v 1.66.2.3 2007/08/24 20:07:38 liamjfoy Exp $  */

#ifndef _STDIO_H_
#define _STDIO_H_

#include  <stdarg.h>
#include  <limits.h>
#include  <sys/ansi.h>
#include  <machine/ansi.h>

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

/*
 * This is fairly grotesque, but pure ANSI code must not inspect the
 * innards of an fpos_t anyway.  The library internally uses off_t,
 * which we assume is exactly as big as eight chars.
 */
#if (!defined(_ANSI_SOURCE) && !defined(__STRICT_ANSI__)) || defined(_LIBC)
typedef __off_t fpos_t;
#else
typedef struct __sfpos {
  __off_t _pos;
} fpos_t;
#endif

#define _FSTDIO     /* Define for new stdio with functions. */

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
struct __sbuf {
  unsigned char *_base;
  int _size;
};

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *  if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *    _lbfsize is -_bf._size, else _lbfsize is 0
 *  if _flags&__SRD, _w is 0
 *  if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NB: see WARNING above before changing the layout of this structure!
 */
typedef struct __sFILE {
  unsigned char  *_p;         /* current position in (some) buffer */
  int             _r;         /* read space left for getc() */
  int             _w;         /* write space left for putc() */
  unsigned short  _flags;     /* flags, below; this FILE is free if 0 */
  short           _file;      /* fileno, if Unix descriptor, else -1 */
  struct  __sbuf  _bf;        /* the buffer (at least 1 byte, if !NULL) */
  int             _lbfsize;   /* 0 or -_bf._size, for inline putc */

  /* operations */
  void           *_cookie;    /* cookie passed to io functions */
  int           (*_close)(void *);
  int           (*_read) (void *, char *, int);
  fpos_t        (*_seek) (void *, fpos_t, int);
  int           (*_write)(void *, const char *, int);

  /* file extension */
  struct  __sbuf  _ext;

  /* separate buffer for long sequences of ungetc() */
  unsigned char  *_up;        /* saved _p when _p is doing ungetc data */
  int             _ur;        /* saved _r when _r is counting ungetc data */

  /* tricks to meet minimum requirements even when malloc() fails */
  unsigned char   _ubuf[3];   /* guarantee an ungetc() buffer */
  unsigned char   _nbuf[1];   /* guarantee a getc() buffer */

  /* separate buffer for fgetln() when line crosses buffer boundary */
  struct  __sbuf  _lb;        /* buffer for fgetln() */

  /* Unix stdio files get aligned to block boundaries on fseek() */
  int             _blksize;   /* stat.st_blksize (may be != _bf._size) */
  fpos_t          _offset;    /* current lseek offset */
} FILE;

__BEGIN_DECLS
extern FILE   __sF[];
__END_DECLS

#define __SLBF  0x0001    /* line buffered */
#define __SNBF  0x0002    /* unbuffered */
#define __SRD   0x0004    /* OK to read */
#define __SWR   0x0008    /* OK to write */
  /* RD and WR are never simultaneously asserted */
#define __SRW   0x0010    /* open for reading & writing */
#define __SEOF  0x0020    /* found EOF */
#define __SERR  0x0040    /* found error */
#define __SMBF  0x0080    /* _buf is from malloc */
#define __SAPP  0x0100    /* fdopen()ed in append mode */
#define __SSTR  0x0200    /* this is an sprintf/snprintf string */
#define __SOPT  0x0400    /* do fseek() optimization */
#define __SNPT  0x0800    /* do not do fseek() optimization */
#define __SOFF  0x1000    /* set iff _offset is in fact correct */
#define __SMOD  0x2000    /* true => fgetln modified _p text */
#define __SALC  0x4000    /* allocate string space dynamically */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although numbered as their counterparts above, the implementation
 * does not rely on this.
 */
#define _IOFBF  0   /* setvbuf should set fully buffered */
#define _IOLBF  1   /* setvbuf should set line buffered */
#define _IONBF  2   /* setvbuf should set unbuffered */

#define BUFSIZ  1024    /* size of buffer used by setbuf */
#define EOF     (-1)

/*
 * FOPEN_MAX is a minimum maximum, and is the number of streams that
 * stdio can provide without attempting to allocate further resources
 * (which could fail).  Do not use this for anything.
 */
#define FOPEN_MAX     OPEN_MAX    /* must be <= OPEN_MAX <sys/syslimits.h> */
#define FILENAME_MAX  PATH_MAX    /* must be <= PATH_MAX <sys/syslimits.h> */

#define L_tmpnam      PATH_MAX    /* must be == PATH_MAX */

#ifndef TMP_MAX
#define TMP_MAX     308915776 /* Legacy */
#endif

/* Always ensure that these are consistent with <fcntl.h>! */
#ifndef SEEK_SET
#define SEEK_SET  0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR  1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END  2 /* set file offset to EOF plus offset */
#endif

#define stdin   (&__sF[0])
#define stdout  (&__sF[1])
#define stderr  (&__sF[2])

/*
 * Functions defined in ANSI C standard.
 */
__BEGIN_DECLS
void      clearerr(FILE *);
int       fclose  (FILE *);
int       feof    (FILE *);
int       ferror  (FILE *);
int       fflush  (FILE *);
int       fgetc   (FILE *);
int       fgetpos (FILE * __restrict, fpos_t * __restrict);
char     *fgets   (char * __restrict, int, FILE * __restrict);
FILE     *fopen   (const char * __restrict , const char * __restrict);

/** The fprintf function writes output to the stream pointed to by stream,
    under control of the string pointed to by format that specifies how
    subsequent arguments are converted for output. If there are insufficient
    arguments for the format, the behavior is undefined. If the format is
    exhausted while arguments remain, the excess arguments are evaluated
    (as always) but are otherwise ignored. The fprintf function returns when
    the end of the format string is encountered.

    The format shall be a multibyte character sequence, beginning and ending in
    its initial shift state. The format is composed of zero or more directives:
    ordinary multibyte characters (not %), which are copied unchanged to the
    output stream; and conversion specifications, each of which results in
    fetching zero or more subsequent arguments, converting them, if applicable,
    according to the corresponding conversion specifier, and then writing the
    result to the output stream.

    Each conversion specification is introduced by the character %. After
    the %, the following appear in sequence:
      - Zero or more flags (in any order) that modify the meaning of the
        conversion specification.
      - An optional minimum field width. If the converted value has fewer
        characters than the field width, it is padded with spaces (by default)
        on the left (or right, if the left adjustment flag, described later,
        has been given) to the field width. The field width takes the form of
        an asterisk * (described later) or a nonnegative decimal integer.
      - An optional precision that gives the minimum number of digits to appear
        for the d, i, o, u, x, and X conversions, the number of digits to
        appear after the decimal-point character for e, E, f, and F
        conversions, the maximum number of significant digits for the g and G
        conversions, or the maximum number of bytes to be written for s
        conversions. The precision takes the form of a period (.) followed
        either by an asterisk * (described later) or by an optional decimal
        integer; if only the period is specified, the precision is taken as
        zero. If a precision appears with any other conversion specifier, the
        behavior is undefined.
      - An optional length modifier that specifies the size of the argument.
      - A conversion specifier character that specifies the type of conversion
        to be applied.

    As noted above, a field width, or precision, or both, may be indicated by
    an asterisk. In this case, an int argument supplies the field width or
    precision. The arguments specifying field width, or precision, or both, shall
    appear (in that order) before the argument (if any) to be converted. A negative
    field width argument is taken as a - flag followed by a positive field width.
    A negative precision argument is taken as if the precision were omitted.

    The flag characters and their meanings are:
      -     The result of the conversion is left-justified within the field.
            (It is right-justified if this flag is not specified.)
      +     The result of a signed conversion always begins with a plus or
            minus sign. (It begins with a sign only when a negative value is
            converted if this flag is not specified.)
      space If the first character of a signed conversion is not a sign, or
            if a signed conversion results in no characters, a space is
            prefixed to the result. If the space and + flags both appear, the
            space flag is ignored.
      #     The result is converted to an "alternative form". For o
            conversion, it increases the precision, if and only if necessary,
            to force the first digit of the result to be a zero (if the value
            and precision are both 0, a single 0 is printed). For x (or X)
            conversion, a nonzero result has 0x (or 0X) prefixed to it. For e,
            E, f, F, g, and G conversions, the result of converting a
            floating-point number always contains a decimal-point character,
            even if no digits follow it. (Normally, a decimal-point character
            appears in the result of these conversions only if a digit follows
            it.) For g and G conversions, trailing zeros are not removed from
            the result. For other conversions, the behavior is undefined.
      0     For d, i, o, u, x, X, e, E, f, F, g, and G conversions, leading
            zeros (following any indication of sign or base) are used to pad to
            the field width rather than performing space padding, except when
            converting an infinity or NaN. If the 0 and - flags both appear,
            the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a
            precision is specified, the 0 flag is ignored. For other
            conversions, the behavior is undefined.

    The length modifiers and their meanings are:
      hh    Specifies that a following d, i, o, u, x, or X conversion specifier
            applies to a signed char or unsigned char argument (the argument
            will have been promoted according to the integer promotions, but
            its value shall be converted to signed char or unsigned char before
            printing); or that a following n conversion specifier applies to a
            pointer to a signed char argument.
      h     Specifies that a following d, i, o, u, x, or X conversion specifier
            applies to a short int or unsigned short int argument (the argument
            will have been promoted according to the integer promotions, but
            its value shall be converted to short int or unsigned short int
            before printing); or that a following n conversion specifier
            applies to a pointer to a short int argument.
      l (ell)   Specifies that a following d, i, o, u, x, or X conversion
            specifier applies to a long int or unsigned long int argument; that
            a following n conversion specifier applies to a pointer to a long
            int argument; that a following c conversion specifier applies to a
            wint_t argument; that a following s conversion specifier applies to
            a pointer to a wchar_t argument; or has no effect on a following e,
            E, f, F, g, or G conversion specifier.
      ll (ell-ell)  Specifies that a following d, i, o, u, x, or X conversion
            specifier applies to a long long int or unsigned long long int
            argument; or that a following n conversion specifier applies to a
            pointer to a long long int argument.
      j     Specifies that a following d, i, o, u, x, or X conversion specifier
            applies to an intmax_t or uintmax_t argument; or that a following n
            conversion specifier applies to a pointer to an intmax_t argument.
      z     Specifies that a following d, i, o, u, x, or X conversion specifier
            applies to a size_t or the corresponding signed integer type
            argument; or that a following n conversion specifier applies to a
            pointer to a signed integer type corresponding to size_t argument.
      t     Specifies that a following d, i, o, u, x, or X conversion specifier
            applies to a ptrdiff_t or the corresponding unsigned integer type
            argument; or that a following n conversion specifier applies to a
            pointer to a ptrdiff_t argument.
      L     Specifies that a following e, E, f, F, g, or G conversion specifier
            applies to a long double argument.

    If a length modifier appears with any conversion specifier other than as
    specified above, the behavior is undefined.

    The conversion specifiers and their meanings are:
      d,i       The int argument is converted to signed decimal in the style
            [-]dddd. The precision specifies the minimum number of digits to
            appear; if the value being converted can be represented in fewer
            digits, it is expanded with leading zeros. The default precision
            is 1. The result of converting a zero value with a precision of
            zero is no characters.
      o,u,x,X   The unsigned int argument is converted to unsigned octal (o),
            unsigned decimal (u), or unsigned hexadecimal notation (x or X) in
            the style dddd; the letters abcdef are used for x conversion and
            the letters ABCDEF for X conversion. The precision specifies the
            minimum number of digits to appear; if the value being converted
            can be represented in fewer digits, it is expanded with leading
            zeros. The default precision is 1. The result of converting a zero
            value with a precision of zero is no characters.
      f,F       A double argument representing a floating-point number is
            converted to decimal notation in the style [-]ddd.ddd, where the
            number of digits after the decimal-point character is equal to the
            precision specification. If the precision is missing, it is taken
            as 6; if the precision is zero and the # flag is not specified, no
            decimal-point character appears. If a decimal-point character
            appears, at least one digit appears before it. The value is rounded
            to the appropriate number of digits.
                A double argument representing an infinity is converted in one
            of the styles [-]inf or [-]infinity - which style is
            implementation-defined. A double argument representing a NaN is
            converted in one of the styles [-]nan or [-]nan(n-char-sequence)
            - which style, and the meaning of any n-char-sequence, is
            implementation-defined. The F conversion specifier produces INF,
            INFINITY, or NAN instead of inf, infinity, or nan, respectively.
      e,E       A double argument representing a floating-point number is
            converted in the style [-]d.ddd e[+-]dd, where there is one digit
            (which is nonzero if the argument is nonzero) before the
            decimal-point character and the number of digits after it is equal
            to the precision; if the precision is missing, it is taken as 6; if
            the precision is zero and the # flag is not specified, no
            decimal-point character appears. The value is rounded to the
            appropriate number of digits. The E conversion specifier produces a
            number with E instead of e introducing the exponent. The exponent
            always contains at least two digits, and only as many more digits
            as necessary to represent the exponent. If the value is zero, the
            exponent is zero.
                A double argument representing an infinity or NaN is converted
            in the style of an f or F conversion specifier.
      g,G       A double argument representing a floating-point number is
            converted in style f or e (or in style F or E in the case of a G
            conversion specifier), depending on the value converted and the
            precision. Let P equal the precision if nonzero, 6 if the precision
            is omitted, or 1 if the precision is zero. Then, if a conversion
            with style E would have an exponent of X:
              - if P > X = -4, the conversion is with style f (or F) and
                precision P - (X + 1).
              - otherwise, the conversion is with style e (or E) and
                precision P - 1.

            Finally, unless the # flag is used, any trailing zeros are removed
            from the fractional portion of the result and the decimal-point
            character is removed if there is no fractional portion remaining.
            A double argument representing an infinity or NaN is converted in
            the style of an f or F conversion specifier.
      c         If no l length modifier is present, the int argument is
            converted to an unsigned char, and the resulting character is
            written. If an l length modifier is present, the wint_t argument is
            converted as if by an ls conversion specification with no precision
            and an argument that points to the initial element of a two-element
            array of wchar_t, the first element containing the wint_t argument
            to the lc conversion specification and the second a null wide
            character.
      s         If no l length modifier is present, the argument is a pointer
            to the initial element of an array of character type. Characters
            from the array are written up to (but not including) the
            terminating null character. If the precision is specified, no more
            than that many bytes are written. If the precision is not specified
            or is greater than the size of the array, the array shall contain a
            null character.
                If an l length modifier is present, the argument shall be a
            pointer to the initial element of an array of wchar_t type. Wide
            characters from the array are converted to multibyte characters
            (each as if by a call to the wcrtomb function, with the conversion
            state described by an mbstate_t object initialized to zero before
            the first wide character is converted) up to and including a
            terminating null wide character. The resulting multibyte characters
            are written up to (but not including) the terminating null
            character (byte). If no precision is specified, the array shall
            contain a null wide character. If a precision is specified, no more
            than that many bytes are written (including shift sequences, if
            any), and the array shall contain a null wide character if, to
            equal the multibyte character sequence length given by the
            precision, the function would need to access a wide character one
            past the end of the array. In no case is a partial multibyte
            character written.
      p         The argument shall be a pointer to void. The value of the
            pointer is converted to a sequence of printing characters, in an
            implementation-defined manner.
      n         The argument shall be a pointer to signed integer into which is
            written the number of characters written to the output stream so
            far by this call to fprintf. No argument is converted, but one is
            consumed. If the conversion specification includes any flags, a
            field width, or a precision, the behavior is undefined.
      %         A % character is written. No argument is converted. The
            complete conversion specification shall be %%.

    In no case does a nonexistent or small field width cause truncation of a
    field; if the result of a conversion is wider than the field width, the
    field is expanded to contain the conversion result.

    @param[in]  stream    An open File specifier to which the output is sent.
    @param[in]  format    A multi-byte character sequence containing characters
                          to be copied unchanged, and conversion specifiers
                          which convert their associated arguments.  Copied and
                          converted characters are sent to the output stream.
    @param      ...       Variable number of parameters as required by format.

    @return     The fprintf function returns the number of characters
                transmitted, or a negative value if an output or encoding
                error occurred.

**/
int       fprintf (FILE * __restrict stream, const char * __restrict format, ...);

int       fputc   (int, FILE *);
int       fputs   (const char * __restrict, FILE * __restrict);
size_t    fread   (void * __restrict, size_t, size_t, FILE * __restrict);
FILE     *freopen (const char * __restrict, const char * __restrict, FILE * __restrict);
int       fscanf  (FILE * __restrict, const char * __restrict, ...);
int       fseek   (FILE *, long, int);
int       fsetpos (FILE *, const fpos_t *);
long      ftell   (FILE *);
size_t    fwrite  (const void * __restrict, size_t, size_t, FILE * __restrict);
int       getc    (FILE *);
int       getchar (void);
void      perror  (const char *);
int       printf  (const char * __restrict, ...);
int       putc    (int, FILE *);
int       putchar (int);
int       puts    (const char *);
int       remove  (const char *);
void      rewind  (FILE *);
int       scanf   (const char * __restrict, ...);
void      setbuf  (FILE * __restrict, char * __restrict);
int       setvbuf (FILE * __restrict, char * __restrict, int, size_t);
int       sscanf  (const char * __restrict, const char * __restrict, ...);
FILE     *tmpfile (void);
int       ungetc  (int, FILE *);
int       vfprintf(FILE * __restrict, const char * __restrict, va_list);
int       vprintf (const char * __restrict, va_list);

#ifndef __AUDIT__
char     *gets    (char *);
int       sprintf (char * __restrict, const char * __restrict, ...);
char     *tmpnam  (char *);
int       vsprintf(char * __restrict, const char * __restrict, va_list);
#endif

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)
int       rename  (const char *, const char *) __RENAME(__posix_rename);
#else
int       rename  (const char *, const char *);
#endif
__END_DECLS

/*
 * IEEE Std 1003.1-90
 */
#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
  #define L_ctermid 1024  /* size for ctermid(); PATH_MAX */
  #define L_cuserid 9     /* size for cuserid(); UT_NAMESIZE + 1 */

  __BEGIN_DECLS
  char  *ctermid(char *);
  #ifndef __CUSERID_DECLARED
    #define __CUSERID_DECLARED
    /* also declared in unistd.h */
    char  *cuserid(char *);
  #endif /* __CUSERID_DECLARED */
  FILE  *fdopen(int, const char *);
  int  fileno(FILE *);
  __END_DECLS
#endif /* not ANSI */

/*
 * IEEE Std 1003.1c-95, also adopted by X/Open CAE Spec Issue 5 Version 2
 */
#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
  __BEGIN_DECLS
  void    flockfile       (FILE *);
  int     ftrylockfile    (FILE *);
  void    funlockfile     (FILE *);
  int     getc_unlocked   (FILE *);
  int     getchar_unlocked(void);
  int     putc_unlocked   (int, FILE *);
  int     putchar_unlocked(int);
  __END_DECLS
#endif /* _POSIX_C_SOURCE >= 1995056 || _XOPEN_SOURCE >= 500 || ... */

/*
 * Functions defined in POSIX 1003.2 and XPG2 or later.
 */
__BEGIN_DECLS
  int     pclose  (FILE *);
  FILE   *popen   (const char *, const char *);
__END_DECLS

/*
 * Functions defined in ISO XPG4.2, ISO C99, POSIX 1003.1-2001 or later.
 */
__BEGIN_DECLS
  int     snprintf (char * __restrict, size_t, const char * __restrict, ...)
          __attribute__((__format__(__printf__, 3, 4)));
  int     vsnprintf(char * __restrict, size_t, const char * __restrict, va_list)
          __attribute__((__format__(__printf__, 3, 0)));
__END_DECLS

/*
 * Functions defined in XPG4.2.
 */
__BEGIN_DECLS
  int   getw(FILE *);
  int   putw(int, FILE *);
  char *mkdtemp(char *);
  int   mkstemp(char *);
  char *mktemp(char *);

    char *tempnam(const char *, const char *);
__END_DECLS

/*
 * X/Open CAE Specification Issue 5 Version 2
 */
#ifndef off_t
  typedef __off_t   off_t;
  #define off_t   __off_t
#endif /* off_t */

__BEGIN_DECLS
int  fseeko(FILE *, off_t, int);
off_t  ftello(FILE *);
__END_DECLS

/*
 * Routines that are purely local.
 */
#define FPARSELN_UNESCESC 0x01
#define FPARSELN_UNESCCONT  0x02
#define FPARSELN_UNESCCOMM  0x04
#define FPARSELN_UNESCREST  0x08
#define FPARSELN_UNESCALL 0x0f

__BEGIN_DECLS
  //int     asprintf(char ** __restrict, const char * __restrict, ...)
  //      __attribute__((__format__(__printf__, 2, 3)));
  char   *fgetln(FILE * __restrict, size_t * __restrict);
  char   *fparseln(FILE *, size_t *, size_t *, const char[3], int);
  int     fpurge(FILE *);
  void    setbuffer(FILE *, char *, int);
  int     setlinebuf(FILE *);
  int     vasprintf(char ** __restrict, const char * __restrict,
        va_list)
        __attribute__((__format__(__printf__, 2, 0)));
  int     vscanf(const char * __restrict, va_list)
        __attribute__((__format__(__scanf__, 1, 0)));
  int     vfscanf(FILE * __restrict, const char * __restrict,
        va_list)
        __attribute__((__format__(__scanf__, 2, 0)));
  int     vsscanf(const char * __restrict, const char * __restrict,
        va_list)
        __attribute__((__format__(__scanf__, 2, 0)));
  const char *fmtcheck(const char *, const char *)
        __attribute__((__format_arg__(2)));
__END_DECLS

  /*
   * Stdio function-access interface.
   */
__BEGIN_DECLS
  FILE  *funopen(const void *,
      int (*)(void *, char *, int),
      int (*)(void *, const char *, int),
      fpos_t (*)(void *, fpos_t, int),
      int (*)(void *));
__END_DECLS
  //#define fropen(cookie, fn) funopen(cookie, fn, 0, 0, 0)
  //#define fwopen(cookie, fn) funopen(cookie, 0, fn, 0, 0)

/*
 * Functions internal to the implementation.
 */
__BEGIN_DECLS
int __srget(FILE *);
int __swbuf(int, FILE *);
__END_DECLS

/*
 * The __sfoo macros are here so that we can
 * define function versions in the C library.
 */
#define __sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))

#if defined(__GNUC__) && defined(__STDC__)
  static __inline int __sputc(int _c, FILE *_p) {
    if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
      return (*_p->_p++ = _c);
    else
      return (__swbuf(_c, _p));
  }
#else
  /*
   * This has been tuned to generate reasonable code on the vax using pcc.
   */
  #define __sputc(c, p) \
    (--(p)->_w < 0 ? \
      (p)->_w >= (p)->_lbfsize ? \
        (*(p)->_p = (unsigned char)(c)), *(p)->_p != '\n' ? \
          (int)*(p)->_p++ : \
          __swbuf('\n', p) : \
        __swbuf((int)(c), p) : \
      (*(p)->_p = (unsigned char)(c), (int)*(p)->_p++))
#endif

#define __sfeof(p)      (((p)->_flags & __SEOF) != 0)
#define __sferror(p)    (((p)->_flags & __SERR) != 0)
#define __sclearerr(p)  ((void)((p)->_flags &= ~(__SERR|__SEOF)))
#define __sfileno(p)    ((p)->_file)

#ifndef __lint__
    #define feof(p)     __sfeof(p)
    #define ferror(p)   __sferror(p)
    #define clearerr(p) __sclearerr(p)

    #define getc(fp)    __sgetc(fp)
    #define putc(x, fp) __sputc(x, fp)
#endif /* __lint__ */

#define getchar()   getc(stdin)
#define putchar(x)  putc(x, stdout)

#define fileno(p) __sfileno(p)

#define getc_unlocked(fp) __sgetc(fp)
#define putc_unlocked(x, fp)  __sputc(x, fp)

#define getchar_unlocked()  getc_unlocked(stdin)
#define putchar_unlocked(x) putc_unlocked(x, stdout)

#endif /* _STDIO_H_ */
