/** @file
  Macros, types, and functions for performing I/O.

  The following functions are declared in this file:<BR>
@verbatim
    ################### Operations on files.   ####
    int       remove          (const char *FileName);
    int       rename          (const char *, const char *);
    FILE     *tmpfile         (void);
    char     *tmpnam          (char *);

    ################### File access functions.   ####
    int       fclose          (FILE *);
    int       fflush          (FILE *);
    FILE     *fopen           (const char * __restrict ,
                               const char * __restrict);
    FILE     *freopen         (const char * __restrict,
                               const char * __restrict, FILE * __restrict);
    void      setbuf          (FILE * __restrict, char * __restrict);
    int       setvbuf         (FILE * __restrict, char * __restrict,
                               int, size_t);

    ################### Formatted Input/Output Functions.  ####
    int       fprintf         (FILE * __restrict stream,
                               const char * __restrict format, ...);
    int       fscanf          (FILE * __restrict, const char * __restrict, ...);
    int       printf          (const char * __restrict, ...);
    int       scanf           (const char * __restrict, ...);
    int       sprintf         (char * __restrict, const char * __restrict, ...);
    int       sscanf          (const char * __restrict,
                               const char * __restrict, ...);
    int       vfprintf        (FILE * __restrict,
                               const char * __restrict, va_list);
    int       vprintf         (const char * __restrict, va_list);
    int       vsprintf        (char * __restrict,
                               const char * __restrict, va_list);

    ################### Character Input/Output Functions. ####
    int       fgetc           (FILE *);
    char     *fgets           (char * __restrict, int, FILE * __restrict);
    int       fputc           (int, FILE *);
    int       fputs           (const char * __restrict, FILE * __restrict);
    int       getc            (FILE *);
    int       getchar         (void);
    char     *gets            (char *);
    int       putc            (int, FILE *);
    int       putchar         (int);
    int       puts            (const char *);
    int       ungetc          (int, FILE *);

    ################### Direct Input/Output Functions. ####
    size_t    fread           (void * __restrict, size_t, size_t,
                               FILE * __restrict);
    size_t    fwrite          (const void * __restrict, size_t, size_t,
                               FILE * __restrict);

    ################### File Positioning Functions.  ####
    int       fgetpos         (FILE * __restrict, fpos_t * __restrict);
    int       fseek           (FILE *, long, int);
    int       fsetpos         (FILE *, const fpos_t *);
    long      ftell           (FILE *);
    void      rewind          (FILE *);

    ################### Error-handling Functions.  ####
    void      clearerr        (FILE *);
    int       feof            (FILE *);
    int       ferror          (FILE *);
    void      perror          (const char *);

    ################### Functions NOT specified by C95  ####

    FILE     *fdopen          (int, const char *);
    void      flockfile       (FILE *);
    int       ftrylockfile    (FILE *);
    void      funlockfile     (FILE *);
    int       getc_unlocked   (FILE *);
    int       getchar_unlocked(void);
    int       putc_unlocked   (int, FILE *);
    int       putchar_unlocked(int);
    int       pclose          (FILE *);
    FILE     *popen           (const char *, const char *);
    int       snprintf        (char * __restrict, size_t,
                               const char * __restrict, ...);
    int       vsnprintf       (char * __restrict, size_t,
                               const char * __restrict, va_list);
    char     *mkdtemp         (char *);
    int       mkstemp         (char *);
    char     *mktemp          (char *);
    char     *tempnam         (const char *, const char *);
    int       fseeko          (FILE *, off_t, int);
    char     *fgetln          (FILE * __restrict, size_t * __restrict);
    char     *fparseln        (FILE *, size_t *, size_t *, const char[3], int);
    int       fpurge          (FILE *);
    void      setbuffer       (FILE *, char *, int);
    int       setlinebuf      (FILE *);
    int       vasprintf       (char ** __restrict, const char * __restrict,
                               va_list);
    int       vscanf          (const char * __restrict, va_list);
    int       vsscanf         (const char * __restrict,
                             const char * __restrict, va_list);
@endverbatim

  @note   To fit things in six character monocase externals, the stdio
          code uses the prefix `__s' for stdio objects, typically followed
          by a three-character attempt at a mnemonic.


  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
    NetBSD: stdio.h,v 1.66.2.3 2007/08/24 20:07:38 liamjfoy Exp
 */
#ifndef _STDIO_H_
#define _STDIO_H_

#include  <stdarg.h>
#include  <limits.h>
#include  <sys/ansi.h>
#include  <machine/ansi.h>

#ifdef _EFI_SIZE_T_
  /** size_t is the unsigned integer type of the result of the sizeof operator. **/
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

/** @{
    An object type capable of holding all information necessary to specify any
    position within a file.

    Each wide-oriented stream has an associated mbstate_t object that stores the
    current parse state of the stream.  A successful call to fgetpos stores a
    representation of the value of this mbstate_t object as part of the value
    of the fpos_t object.  A later successful call to fsetpos using the same
    stored fpos_t value restores the value of the associated mbstate_t object
    as well as the position within the controlled stream.

    This is fairly grotesque, but pure ANSI code must not inspect the
    innards of an fpos_t anyway.  The library internally uses off_t,
    which we assume is exactly as big as eight chars.
**/
#if (!defined(_ANSI_SOURCE) && !defined(__STRICT_ANSI__)) || defined(_LIBC)
typedef __off_t fpos_t;
#else
typedef struct __sfpos {
  __off_t _pos;
} fpos_t;
#endif
/*@}*/

/* stdio buffers */
struct __sbuf {
  unsigned char *_base;
  int _size;
};

/** Structure which holds all the information needed to control a stream or file.
 *
 * The following always hold:<BR>
 *
 *  - if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *    - _lbfsize is -_bf._size, else _lbfsize is 0
 *  - if _flags&__SRD, _w is 0
 *  - if _flags&__SWR, _r is 0
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
 */
typedef struct __sFILE {
  unsigned char  *_p;         /**< current position in (some) buffer */
  int             _r;         /**< read space left for getc() */
  int             _w;         /**< write space left for putc() */
  unsigned short  _flags;     /**< flags, below; this FILE is free if 0 */
  short           _file;      /**< fileno, if Unix descriptor, else -1 */
  struct  __sbuf  _bf;        /**< the buffer (at least 1 byte, if !NULL) */
  int             _lbfsize;   /**< 0 or -_bf._size, for inline putc */

  /* operations */
  void           *_cookie;    /**< cookie passed to io functions */
  int           (*_close)(void *);
  int           (*_read) (void *, char *, int);
  fpos_t        (*_seek) (void *, fpos_t, int);
  int           (*_write)(void *, const char *, int);

  /** file extension */
  struct  __sbuf  _ext;

  /** @{
      Separate buffer for long sequences of ungetc().
  **/
  unsigned char  *_up;        /**< saved _p when _p is doing ungetc data */
  int             _ur;        /**< saved _r when _r is counting ungetc data */
  /*@}*/

  /* tricks to meet minimum requirements even when malloc() fails */
  unsigned char   _ubuf[3 * MB_LEN_MAX];   /**< guarantee an ungetc() buffer */
  unsigned char   _nbuf[1 * MB_LEN_MAX];   /**< guarantee a getc() buffer */

  /** separate buffer for fgetln() when line crosses buffer boundary */
  struct  __sbuf  _lb;        /* buffer for fgetln() */

  /* Unix stdio files get aligned to block boundaries on fseek() */
  int             _blksize;   /**< stat.st_blksize (may be != _bf._size) */
  fpos_t          _offset;    /**< current lseek offset */
} FILE;

__BEGIN_DECLS
extern FILE   __sF[];
__END_DECLS

#define __SLBF  0x0001    /**< line buffered */
#define __SNBF  0x0002    /**< unbuffered */
#define __SRD   0x0004    /**< OK to read */
#define __SWR   0x0008    /**< OK to write */
  /* RD and WR are never simultaneously asserted */
#define __SRW   0x0010    /**< open for reading & writing */
#define __SEOF  0x0020    /**< found EOF */
#define __SERR  0x0040    /**< found error */
#define __SMBF  0x0080    /**< _buf is from malloc */
#define __SAPP  0x0100    /**< fdopen()ed in append mode */
#define __SSTR  0x0200    /**< this is an sprintf/snprintf string */
#define __SOPT  0x0400    /**< do fseek() optimization */
#define __SNPT  0x0800    /**< do not do fseek() optimization */
#define __SOFF  0x1000    /**< set iff _offset is in fact correct */
#define __SMOD  0x2000    /**< true => fgetln modified _p text */
#define __SALC  0x4000    /**< allocate string space dynamically */

/*  The following three definitions are for ANSI C, which took them
    from System V, which brilliantly took internal interface macros and
    made them official arguments to setvbuf(), without renaming them.
    Hence, these ugly _IOxxx names are *supposed* to appear in user code.

    Although numbered as their counterparts above, the implementation
    does not rely on this.
 */
#define _IOFBF  0   /**< setvbuf should set fully buffered */
#define _IOLBF  1   /**< setvbuf should set line buffered */
#define _IONBF  2   /**< setvbuf should set unbuffered */

#define BUFSIZ  1024    /**< size of buffer used by setbuf */
#define EOF     (-1)    /**< A constant integer expression indicating end-of-file. */

/** FOPEN_MAX is a minimum maximum, and is the number of streams that
    stdio can provide without attempting to allocate further resources
    (which could fail).  Do not use this for anything.
 */
#define FOPEN_MAX     OPEN_MAX    /* must be <= OPEN_MAX <sys/syslimits.h> */

/** Size needed for an array of char large enough to hold the longest file name string. */
#define FILENAME_MAX  PATH_MAX    /* must be <= PATH_MAX <sys/syslimits.h> */

/** Size needed for an array of char large enough to hold the file name string
    generated by the tmpname() function.
**/
#define L_tmpnam      PATH_MAX    /* must be == PATH_MAX */

#ifndef TMP_MAX
#define TMP_MAX     308915776     /**< The maximum number of unique file names
                                       that can be generated by tmpnam(). **/
#endif

/* Always ensure that these are consistent with <fcntl.h>! */
#ifndef SEEK_SET
#define SEEK_SET  0 /**< set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR  1 /**< set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END  2 /**< set file offset to EOF plus offset */
#endif

#define stdin   (&__sF[0])    /**< FILE reference for the STanDard INput stream. */
#define stdout  (&__sF[1])    /**< FILE reference for the STanDard OUTput stream. */
#define stderr  (&__sF[2])    /**< FILE reference for the STanDard ERRor stream. */

__BEGIN_DECLS
/* Functions defined in C95 standard. ###################################### */

/* ################ Operations on files.   */

/** Remove (delete) a file.

    @param[in]    FileName    The path to the file to be removed.

    @retval   Zero      The operation succeeded.
    @retval   Non-zero  The operation failed.
**/
int       remove  (const char *FileName);

/** Rename the file named OldName to NewName.

    @param[in]  OldName   The name of the existing file to be renamed.
    @param[in]  NewName   The new name of the file.

    @retval   Zero      The operation succeeded.
    @retval   Non-zero  The operation failed.  OldName still exists and has been unmodified.
                        If OldName does not exist, or a file named NewName already exists,
                        rename() will fail are return a non-zero value.
**/
int       rename  (const char *OldName, const char *NewName);

/** Create a guaranteed unique temporary file.
    A binary file is created in the _PATH_TMP directory that is guaranteed to
    have a unique name.  The file will be open for update with mode "wb+" and
    its FILE pointer returned upon successfull completion.  When the file is
    closed, or when the creating program terminates, the file will be removed.

    @retval   NULL      The temporary file could not be created.
    @retval   non-NULL  The returned value is a pointer to the FILE object
                        associated with the newly created and open temporary file.
**/
FILE     *tmpfile (void);

/** Generate a string that is a valid file name, in the _PATH_TMP directory, that
    is not the same as the name of an existing file.  The function can potentially
    generate up to TMP_MAX different strings.

    @param[out]   Buffer    A pointer to an array of at least L_tmpnam char elements.
                            or NULL.  If non-NULL, the tmpnam function writes its
                            result into that array and returns the argument
                            as its value.

    @return       If no suitable string can be generated a NULL pointer is returned.
                  Otherwise, if Buffer is NULL, the result is produced in an internal
                  static object and a pointer to that object is returned.  If Buffer
                  is non-null, the results are written into the array pointed to by
                  Buffer and Buffer is returned.
**/
char     *tmpnam  (char *Buffer);

/* ################ File access functions.   */

/** Close the open stream, specified by fp, and de-associate it from any file or device.

    @param[in]    fp    Pointer to a stream object, of type FILE, associated with a
                        file or device.

    @retval   Zero      The stream was successfully closed.
    @retval   Non-zero  There was an error closing the stream.
**/
int       fclose  (FILE *fp);

/** Empties any buffers associated with the stream specified by fp.

    @param[in]    fp    Pointer to a stream object, of type FILE, associated with a
                        file or device.

    @retval   Zero      The stream's buffers were successfully emptied.
    @retval   EOF       There was an error writing to the stream.
**/
int       fflush  (FILE *fp);

/** Associates a file, named by Path, with a stream and prepares it for subsequent
    operations.

    The parameter Mode points to a string specifying behavior characteristics for
    the opened file.  The recognized Mode strings are:
      - r     Open text file for reading.
      - w     Truncate file to zero length or create text file for writing.
      - a     Open or create a text file for writing at end-of-file (append).
      - rb    Open binary file for reading.
      - wb    Truncate file to zero length or create binary file for writing.
      - ab    Open or create a binary file for writing at end-of-file (append).
      - r+    Open text file for update (reading and writing).
      - w+    Truncate file to zero length or create text file for update.
      - a+    Open or create a text file for update, writing at end-of-file.
      - r+b or rb+  Open binary file for update (reading and writing).
      - w+b or wb+  Truncate file to zero length or create binary file for update.
      - a+b or ab+  Open or create a binary file for update, writing at end-of-file.

      Opening a file with read mode fails if the file does not exist.

      Opening a file with append mode causes all writes to the file to be forced to
      the current end-of-file, regardless of any intervening calls to fseek.

    @param[in]    Path    The path or name of the file or device to open.
    @param[in]    Mode    The mode in which the file is to be opened.

    @return     A pointer to a FILE object associated with the opened file is returned
                if the file was opened successfully.  Otherwise, NULL is returned.
**/
FILE     *fopen   (const char * __restrict Path, const char * __restrict Mode);

/** Closes the file associated with Ofp then opens the file specified by Path and associates it with
    stream Ofp.

    Any errors that occur when closing Ofp are ignored.  The file specified by Path is opened with mode Mode
    and associated with stream Ofp instead of producing a new stream object.

    If Path is NULL, the mode of the file associated with Ofp is changed to Mode.

    @param[in]    Path    The path or name of the file or device to open.
    @param[in]    Mode    The mode in which the file is to be opened.
    @param[in]    Ofp     Pointer to the FILE object to be closed and associated with the new file.

    @return       If Path was not able to be opened, or the mode changed, NULL is returned;
                  otherwise Ofp is returned.
**/
FILE     *freopen (const char * __restrict Path, const char * __restrict Mode, FILE * __restrict Ofp);

/** Establishes Fully Buffered or Non-buffered mode for a stream, fp, using Buff as the buffer.

    The file associated with fp must have been successfully opened with no operations, other than
    possibly an unsuccessful call to setvbuf, performed prior to the call to setbuf.

    If Buff is non-NULL, the stream associated with fp is set to Fully Buffered mode using the
    array pointed to by Buff as the buffer.  The buffer is assumed to be BUFSIZ char long.
    This is equivalent to calling setvbuf(fp, Buff, _IOFBF, BUFSIZ);

    If Buff is NULL, stream fp is set to Non-buffered mode.
    This is equivalent to calling setvbuf(fp, NULL, _IONBF, 0);

    @param[in]  fp      Pointer to the FILE object which will have its buffer set.
    @param[in]  Buff    The buffer to use for fp, or NULL.
**/
void      setbuf  (FILE * __restrict fp, char * __restrict Buff);

/** Establishes a buffering mode and buffer for use by operations performed on the file associated with fp.

    The file associated with fp must have been successfully opened with no operations, other than
    possibly an unsuccessful call to setvbuf, performed prior to the call to setbuf.

    Parameter BufMode determines how stream fp will be buffered:
      - _IOFBF causes I/O to be fully buffered.
      - _IOLBF causes I/O to be line buffered.
      - _IONBF causes I/O to be unbuffered.

    If Buff is not NULL, it points to an array to be used as an I/O buffer for stream fp.  The
    buffer is set to BufSize char in length.  Otherwise, an array of BufSize char is allocated
    by the setvbuf function if BufMode is not _IONBF.

    It is an error for BufSize to be zero unless BufMode is _IONBF, in which case BufSize is ignored.

    @param[in]  fp        Pointer to the FILE object which will have its buffer set.
    @param[in]  Buff      The buffer to use for fp, or NULL.
    @param[in]  BufMode   The buffering mode to use.
    @param[in]  BufSize   The size of the buffer to use, specified in char.

    @retval   Zero      The buffer and mode were established successfully.
    @retval   Non-zero  The request can not be honored, or an invalid value for BufMode was given.
**/
int       setvbuf (FILE * __restrict fp, char * __restrict Buff, int BufMode, size_t BufSize);

/* ################ Formatted Input/Output Functions.  */

/** The fprintf function writes output to the stream pointed to by stream,
    under control of the string pointed to by format that specifies how
    subsequent arguments are converted for output. If there are insufficient
    arguments for the format, the behavior is indeterminate. If the format is
    exhausted while arguments remain, the excess arguments are evaluated
    (as always) but are otherwise ignored. The fprintf function returns when
    the end of the format string is encountered.

    The format is interpreted as a multibyte character sequence, beginning and ending
    in its initial shift state. The format is composed of zero or more directives:
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
        zero. If a precision appears with any other conversion specifier, it
        is ignored.
      - An optional length modifier that specifies the size of the argument.
      - A conversion specifier character that specifies the type of conversion
        to be applied.

    As noted above, a field width, or precision, or both, may be indicated by
    an asterisk. In this case, an int argument supplies the field width or
    precision. The arguments specifying field width, or precision, or both, shall
    appear (in that order) before the argument (if any) to be converted. A negative
    field width argument is taken as a - flag followed by a positive field width.
    A negative precision argument is interpreted as if the precision were omitted.

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
      #     The result is converted to an "alternative form".
              - For o conversion, it increases the precision, if and only if necessary,
                to force the first digit of the result to be a zero (if the value
                and precision are both 0, a single 0 is printed).
              - For x (or X) conversion, a nonzero result has 0x (or 0X) prefixed to it.
              - For e, E, f, F, g, and G conversions, the result of converting a
                floating-point number always contains a decimal-point character,
                even if no digits follow it. (Normally, a decimal-point character
                appears in the result of these conversions only if a digit follows
                it.)
              - For g and G conversions, trailing zeros are not removed from
                the result. For other conversions, it is ignored.
      0     For d, i, o, u, x, X, e, E, f, F, g, and G conversions, leading
            zeros (following any indication of sign or base) are used to pad to
            the field width rather than performing space padding, except when
            converting an infinity or NaN. If the 0 and - flags both appear,
            the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a
            precision is specified, the 0 flag is ignored.

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
    specified above, it is ignored.

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
                A double argument representing an infinity is converted in
            the style [-]inf. A double argument representing a NaN is
            converted in the style [-]nan. The F conversion specifier produces INF,
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
            pointer is converted to a sequence of printing characters.
      n         The argument shall be a pointer to signed integer into which is
            written the number of characters written to the output stream so
            far by this call to fprintf. No argument is converted, but one is
            consumed. If the conversion specification includes any flags, a
            field width, or a precision, they will be ignored.
      %         A % character is written. No argument is converted. The
            complete conversion specification shall be %%.

    In no case does a nonexistent or small field width cause truncation of a
    field; if the result of a conversion is wider than the field width, the
    field is expanded to contain the conversion result.

    @param[in]  stream    An open File specifier to which the output is sent.
    @param[in]  format    A multi-byte character sequence containing characters
                          to be copied unchanged, and conversion specifiers
                          which convert their associated arguments.
    @param      ...       Variable number of parameters as required by format.

    @return     The fprintf function returns the number of characters
                transmitted, or a negative value if an output or encoding
                error occurred.
**/
int       fprintf (FILE * __restrict stream, const char * __restrict format, ...);

/** Reads characters from stream, under control of format, storing the converted values
    in variables pointed to by the variable-length parameter list.

    The format is interpreted as a multibyte character sequence, beginning and ending
    in its initial shift state. The format is composed of zero or more directives:
    one or more white-space characters, an ordinary multibyte character
    (neither % nor a white-space character), or a conversion specification.

    Each conversion specification is introduced by the character %. After
    the %, the following appear in sequence:
      - An optional assignment-suppressing character, *.
      - An optional decimal integer, greater than zero, that specifies the
        maximum field width (in characters).
      - An optional length modifier that specifies the size of the receiving object.
      - A conversion specifier character that specifies the type of conversion
        to be applied.

    The fscanf function executes each directive of the format in turn. If a directive fails, as
    detailed below, the function returns. Failures are described as input failures (due to the
    occurrence of an encoding error or the unavailability of input characters), or matching
    failures (due to inappropriate input).

    A directive composed of white-space character(s) is executed by reading input up to the
    first non-white-space character (which remains unread), or until no more characters can
    be read.

    A directive that is an ordinary multibyte character is executed by reading the next
    characters of the stream. If any of those characters differ from the ones composing the
    directive, the directive fails and the differing and subsequent characters remain unread.
    Similarly, if end-of-file, an encoding error, or a read error prevents a character from being
    read, the directive fails.

    The length modifiers and their meanings are:
      - hh            Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to signed
                      char or unsigned char.
      - h             Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to short
                      int or unsigned short int.
      - l (ell)       Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to
                      long int or unsigned long int; that a following a, A, e,
                      E, f, F, g, or G conversion specifier applies to an
                      argument with type pointer to double; or that a following
                      c, s, or [ conversion specifier applies to an argument
                      with type pointer to wchar_t.
      - ll (ell-ell)  Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to
                      long long int or unsigned long long int.
      - j             Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to
                      intmax_t or uintmax_t.
      - z             Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to
                      size_t or the corresponding signed integer type.
      - t             Specifies that a following d, i, o, u, x, X, or n conversion
                      specifier applies to an argument with type pointer to
                      ptrdiff_t or the corresponding unsigned integer type.
      - L             Specifies that a following e, E, f, F, g, or G
                      conversion specifier applies to an argument with type
                      pointer to long double.

    If a length modifier appears with any conversion specifier other than as specified above,
    it will be ignored.

    The conversion specifiers and their meanings are:
      - d       Matches an optionally signed decimal integer, whose format is
                the same as expected for the subject sequence of the strtol
                function with the value 10 for the base argument. The
                corresponding argument shall be a pointer to signed integer.
      - i       Matches an optionally signed integer, whose format is the same
                as expected for the subject sequence of the strtol function
                with the value 0 for the base argument. The corresponding
                argument shall be a pointer to signed integer.
      - o       Matches an optionally signed octal integer, whose format is the
                same as expected for the subject sequence of the strtoul
                function with the value 8 for the base argument. The
                corresponding argument shall be a pointer to unsigned integer.
      - u       Matches an optionally signed decimal integer, whose format is
                the same as expected for the subject sequence of the strtoul
                function with the value 10 for the base argument. The
                corresponding argument shall be a pointer to unsigned integer.
      - x       Matches an optionally signed hexadecimal integer, whose format
                is the same as expected for the subject sequence of the strtoul
                function with the value 16 for the base argument. The
                corresponding argument shall be a pointer to unsigned integer.
      - e,f,g   Matches an optionally signed floating-point number, infinity,
                or NaN, whose format is the same as expected for the subject
                sequence of the strtod function. The corresponding argument
                shall be a pointer to floating.
      - c       Matches a sequence of characters of exactly the number
                specified by the field width (1 if no field width is present
                in the directive).  If no l length modifier is present, the
                corresponding argument shall be a pointer to the initial
                element of a character array large enough to accept the
                sequence. No null character is added.<BR><BR>
                If an l length modifier is present, the input shall be a
                sequence of multibyte characters that begins in the initial
                shift state. Each multibyte character in the sequence is
                converted to a wide character as if by a call to the mbrtowc
                function, with the conversion state described by an mbstate_t
                object initialized to zero before the first multibyte character
                is converted. The corresponding argument shall be a pointer to
                the initial element of an array of wchar_t large enough to
                accept the resulting sequence of wide characters.  No null wide
                character is added.
      - s       Matches a sequence of non-white-space characters.
                If no l length modifier is present, the corresponding argument
                shall be a pointer to the initial element of a character array
                large enough to accept the sequence and a terminating null
                character, which will be added automatically.  If an l length
                modifier is present, the input shall be a sequence of multibyte
                characters that begins in the initial shift state. Each
                multibyte character is converted to a wide character as if by a
                call to the mbrtowc function, with the conversion state
                described by an mbstate_t object initialized to zero before the
                first multibyte character is converted. The corresponding
                argument shall be a pointer to the initial element of an array
                of wchar_t large enough to accept the sequence and the
                terminating null wide character, which will be added automatically.
      - [       Matches a nonempty sequence of characters from a set of
                expected characters (the scanset).<BR><BR>
                If no l length modifier is present, the corresponding argument
                shall be a pointer to the initial element of a character array
                large enough to accept the sequence and a terminating null
                character, which will be added automatically.  If an l length
                modifier is present, the input shall be a sequence of multibyte
                characters that begins in the initial shift state. Each
                multibyte character is converted to a wide character as if by a
                call to the mbrtowc function, with the conversion state
                described by an mbstate_t object initialized to zero before the
                first multibyte character is converted. The corresponding
                argument shall be a pointer to the initial element of an array
                of wchar_t large enough to accept the sequence and the
                terminating null wide character, which will be added
                automatically.<BR><BR>
                The conversion specifier includes all subsequent characters in
                the format string, up to and including the matching right
                bracket (]). The characters between the brackets (the scanlist)
                compose the scanset, unless the character after the left
                bracket is a circumflex (^), in which case the scanset contains
                all characters that do not appear in the scanlist between the
                circumflex and the right bracket. If the conversion specifier
                begins with [] or [^], the right bracket character is in the
                scanlist and the next following right bracket character is the
                matching right bracket that ends the specification; otherwise
                the first following right bracket character is the one that
                ends the specification. If a - character is in the scanlist and
                is not the first, nor the second where the first character is
                a ^, nor the last character, it will be treated as a regular character.
      - p       Matches a set of sequences, which are the same as the set of
                sequences that are produced by the %p conversion of the fprintf
                function. The corresponding argument must be a pointer to a
                pointer to void. The input item is converted to a pointer value.
                If the input item is a value converted earlier during the same
                program execution, the pointer that results will compare equal
                to that value; otherwise the behavior of the %p conversion is
                indeterminate.
      - n       No input is consumed. The corresponding argument shall be a
                pointer to signed integer into which is to be written the
                number of characters read from the input stream so far by this
                call to the fscanf function. Execution of a %n directive does
                not increment the assignment count returned at the completion
                of execution of the fscanf function. No argument is converted,
                but one is consumed. If the conversion specification includes
                an assignment suppressing character the conversion specification
                is ignored.  If the conversion specification contains a
                field width, the field width will be ignored.
      - %       Matches a single % character; no conversion or assignment occurs.

    @param[in]  stream    An open File specifier from which the input is read.
    @param[in]  format    A multi-byte character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param      ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return     The fscanf function returns EOF if an input failure occurs before
                any conversion.  Otherwise the number of input items assigned
                is returned; which can be fewer than provided for, or even zero
                in the event of an early matching failure.
**/
int       fscanf  (FILE * __restrict stream, const char * __restrict format, ...);

/** Formatted print to stdout.

    The printf function is equivalent to fprintf with stdout used as the output stream.

    @param[in]  format    A multi-byte character sequence containing characters
                          to be copied unchanged, and conversion specifiers
                          which convert their associated arguments.  Copied and
                          converted characters are sent to the output stream.
    @param      ...       Variable number of parameters as required by format.

    @return     The printf function returns the number of characters
                transmitted, or a negative value if an output or encoding
                error occurred.
**/
int       printf  (const char * __restrict format, ...);

/** Formatted input from stdin.

    The scanf function is equivalent to fscanf with stdin used as the input stream.

    @param[in]  format    A multi-byte character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param[out] ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return     The scanf function returns EOF if an input failure occurs before
                any conversion.  Otherwise the number of input items assigned
                is returned; which can be fewer than provided for, or even zero
                in the event of an early matching failure.
**/
int       scanf   (const char * __restrict format, ...);

/** Formatted output to a buffer.

    The sprintf function is equivalent to fprintf, except that the output is
    written into array Buff instead of to a stream.  A null character is written
    at the end of the characters written; it is not counted as part of the
    returned value.

    @param[out]   Buff      A pointer to the array to receive the formatted output.
    @param[in]    Format    A multi-byte character sequence containing characters
                            to be copied unchanged, and conversion specifiers
                            which convert their associated arguments.  Copied and
                            converted characters are written to the array pointed
                            to by Buff.
    @param        ...       Variable number of parameters as required by format.

    @return   The sprintf function returns the number of characters written in
              the array, not counting the terminating null character, or a
              negative value if an encoding error occurred.
**/
int       sprintf (char * __restrict Buff, const char * __restrict Format, ...);

/** Formatted input from a string.

    The sscanf function is equivalent to fscanf, except that input is obtained
    from a string rather than from a stream.  Reaching the end of the string
    is equivalent to encountering end-of-file for the fscanf function.

    @param[in]  Buff      Pointer to the string from which to obtain input.
    @param[in]  Format    A multi-byte character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param[out] ...       Variable number of parameters, as required by format,
                          specifying the objects to receive the converted input.

    @return     The scanf function returns EOF if an input failure occurs before
                any conversion.  Otherwise the number of input items assigned
                is returned; which can be fewer than provided for, or even zero
                in the event of an early matching failure.
**/
int       sscanf  (const char * __restrict Buff, const char * __restrict Format, ...);

/** Print formatted values from an argument list.

    The vfprintf function is equivalent to fprintf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start macro.
    The vfprintf function does not invoke the va_end macro.

    @param[in]  Stream    The output stream to receive the formatted output.
    @param[in]  Format    A multi-byte character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param[in]  Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vfprintf function returns the number of characters transmitted,
              or a negative value if an output or encoding error occurred.
**/
int       vfprintf(FILE * __restrict Stream, const char * __restrict Format, va_list Args);

/** Formatted print, to stdout, from an argument list.

    The vprintf function is equivalent to printf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start
    macro (and possibly subsequent va_arg calls). The vprintf function does
    not invoke the va_end macro.

    @param[in]  Format    A multi-byte character sequence containing characters
                          to be matched against, and conversion specifiers
                          which convert their associated arguments.  Converted
                          items are stored according to their associated arguments.
    @param[in]  Args      A list of arguments, initialized by the va_start macro
                          and accessed using the va_arg macro, used to satisfy
                          the directives in the Format string.

    @return   The vprintf function returns the number of characters transmitted,
              or a negative value if an output or encoding error occurred.
**/
int       vprintf (const char * __restrict Format, va_list Args);

/** Formatted print, to a buffer, from an argument list.

    The vsprintf function is equivalent to sprintf, with the variable argument
    list replaced by Args, which must have been initialized by the va_start
    macro. The vsprintf function does not invoke the va_end macro.

    @param[out]   Buff      A pointer to the array to receive the formatted output.
    @param[in]    Format    A multi-byte character sequence containing characters
                            to be copied unchanged, and conversion specifiers
                            which convert their associated arguments.  Copied and
                            converted characters are written to the array pointed
                            to by Buff.
    @param[in]    Args      A list of arguments, initialized by the va_start macro
                            and accessed using the va_arg macro, used to satisfy
                            the directives in the Format string.

    @return   The vsprintf function returns the number of characters written in
              the array, not counting the terminating null character, or a
              negative value if an encoding error occurred.
**/
int       vsprintf(char * __restrict Buff, const char * __restrict Format, va_list Args);

/* ################ Character Input/Output Functions. */

/** Get a character from an input Stream.

    If the end-of-file indicator for the input stream pointed to by Stream is
    not set, and a next character is present, the fgetc function obtains that
    character as an unsigned char converted to an int and advances the
    associated file position indicator for the stream.

    @param[in]  Stream    An input stream from which to obtain a character.

    @return   If the end-of-file indicator for the stream is set, or if the
              stream is at end-of-file, the end-of-file indicator for the
              stream is set and the fgetc function returns EOF.  Otherwise,
              the fgetc function returns the next character from the input
              stream pointed to by Stream.  If a read error occurs, the
              error indicator for the stream is set and the fgetc function
              returns EOF.
**/
int       fgetc   (FILE *Stream);

/** Read a string from an input stream into a buffer.

    The fgets function reads at most one less than the number of characters
    specified by Limit from the stream pointed to by Stream into the array
    pointed to by Buff.  No additional characters are read after a
    new-line character (which is retained) or after end-of-file.  A null
    character is written immediately after the last character read into the array.

    @param[out] Buff      A pointer to the array to receive the input string.
    @param[in]  Limit     The maximum number of characters to put into Buff,
                          including the terminating null character.
    @param[in]  Stream    An input stream from which to obtain a character.

    @return   The fgets function returns Buff if successful. If end-of-file is
              encountered and no characters have been read into the array, the
              contents of the array remain unchanged and a null pointer is
              returned. If a read error occurs during the operation, the array
              contents are indeterminate and a null pointer is returned.
**/
char     *fgets   (char * __restrict Buff, int Limit, FILE * __restrict Stream);

/** Write a character to an output stream.

    The fputc function writes the character specified by C (converted to an
    unsigned char) to the output stream pointed to by Stream, at the position
    indicated by the associated file position indicator for the stream
    (if defined), and advances the indicator appropriately. If the file cannot
    support positioning requests, or if the stream was opened with append mode,
    the character is appended to the output stream.

    @param[in]  C       The character to be written to Stream.
    @param[in]  Stream  The output stream that C is to be written to.

    @return   The fputc function returns the character written. If a write
              error occurs, the error indicator for the stream is set and
              fputc returns EOF.
**/
int       fputc   (int C, FILE *Stream);

/** Write a string to an output stream.

    The fputs function writes String to the stream pointed to by Stream.  The
    terminating null character is not written.

    @param[in]  String  The character string to be written to Stream.
    @param[in]  Stream  The output stream that String is to be written to.

    @return   The fputs function returns EOF if a write error occurs; otherwise
              it returns a non-negative value.
**/
int       fputs   (const char * __restrict String, FILE * __restrict Stream);

/** Get a character from an input stream.

    The getc function is equivalent to fgetc, except that if it is implemented
    as a macro, it may evaluate stream more than once, so the argument should
    never be an expression with side effects.

    @param[in]  Stream    An input stream from which to obtain a character.

    @return   If the end-of-file indicator for the stream is set, or if the
              stream is at end-of-file, the end-of-file indicator for the
              stream is set and getc returns EOF.  Otherwise, getc returns
              the next character from the input stream pointed to by Stream.
              If a read error occurs, the error indicator for the stream is set
              and getc returns EOF.
**/
int       getc    (FILE *);

/** Get a character from stdin.

    The getchar function is equivalent to getc with the argument stdin.

    @return   If the end-of-file indicator for stdin is set, or if stdin
              is at end-of-file, the end-of-file indicator is set and getchar
              returns EOF.  Otherwise, getchar returns the next character from
              stdin.  If a read error occurs, the error indicator for stdin is
              set and getchar returns EOF.
**/
int       getchar (void);

/** Read a string from stdin into a buffer.

    The gets function reads characters from the input stream pointed to by
    stdin, into the array pointed to by Buff, until end-of-file is encountered
    or a new-line character is read.  Any new-line character is discarded, and
    a null character is written immediately after the last character read into
    the array.

    @param[out] Buff      A pointer to the array to receive the input string.

    @return   The gets function returns Buff if successful.  If end-of-file is
              encountered and no characters have been read into the array, the
              contents of the array remain unchanged and a null pointer is
              returned. If a read error occurs during the operation, the array
              contents are indeterminate and a null pointer is returned.
**/
char     *gets    (char *Buff);

/** Write a character to an output stream.

    The putc function is equivalent to fputc, except that if it is implemented
    as a macro, it may evaluate Stream more than once, so that argument should
    never be an expression with side effects.

    @param[in]  C       The character to be written to Stream.
    @param[in]  Stream  The output stream that C is to be written to.

    @return   The putc function returns the character written. If a write
              error occurs, the error indicator for the stream is set and
              putc returns EOF.
**/
int       putc    (int C, FILE *Stream);

/** Write a character to stdout.

    The putchar function is equivalent to putc with stdout as the Stream argument.

    @param[in]    C   The character to be written to stdout.

    @return   The putchar function returns the character written. If a write
              error occurs, the error indicator for stdout is set and putchar
              returns EOF.
**/
int       putchar (int C);

/** Write String to stdout.

    The puts function writes the string pointed to by String to the stream
    pointed to by stdout, and appends a new-line character to the output. The
    terminating null character is not written.

    @param[in]  String    A pointer to the character string to write to stdout.

    @return   The puts function returns EOF if a write error occurs; otherwise
              it returns a non-negative value.
**/
int       puts    (const char *String);

/** Return a character to the input Stream as if it had not been read.

    The ungetc function pushes the character specified by C (converted to an
    unsigned char) back onto the input stream pointed to by Stream. Pushed-back
    characters will be returned by subsequent reads on that stream in the
    reverse order of their being pushed.  A successful intervening call
    (with the stream pointed to by Stream) to a file positioning function
    (fseek, fsetpos, or rewind) discards any pushed-back characters for the
    stream. The external storage corresponding to the stream is unchanged.

    One character of pushback is guaranteed. If the ungetc function is called
    too many times on the same stream without an intervening read or file
    positioning operation on that stream, the operation will fail.

    If the value of C equals that of the macro EOF, the operation fails and the
    input stream is unchanged.

    A successful call to the ungetc function clears the end-of-file indicator
    for the stream.  The value of the file position indicator for the stream
    after reading or discarding all pushed-back characters is the same as it
    was before the characters were pushed back.  For a binary stream, its
    file position indicator is decremented by each successful call to the
    ungetc function; if its value was zero before a call, it will remain zero
    after the call.

    @param[in]  C       The character to push back onto the Stream.
    @param[in]  Stream  The output stream that C is to be pushed back onto.

    @return   The ungetc function returns the character pushed back,
              or EOF if the operation fails.
**/
int       ungetc  (int C, FILE *Stream);

/* ################ Direct Input/Output Functions. */

/** Read Num elements of size Size from a Stream into a Buffer.

    The fread function reads, into the array pointed to by Buffer, up to Num
    elements, whose size is specified by Size, from the stream pointed to by
    Stream.  For each object, Size calls are made to the fgetc function and the
    results stored, in the order read, in an array of unsigned char exactly
    overlaying the Buffer object.  The file position indicator for the stream
    (if defined) is advanced by the number of characters successfully read. If
    an error occurs, the resulting value of the file position indicator for the
    stream is indeterminate.

    @param[out]   Buffer    Pointer to an object to receive the read data.
    @param[in]    Size      Size of each element to be read.
    @param[in]    Num       Number of elements to read.
    @param[in]    Stream    Input stream to read the data from.

    @return   The fread function returns the number of elements successfully
              read, which may be less than Num if a read error or end-of-file
              is encountered.  If Size or Num is zero, fread returns zero and
              the contents of the array and the state of the stream remain
              unchanged.
**/
size_t    fread   (void   * __restrict  Buffer,
                   size_t               Size,
                   size_t               Num,
                   FILE   * __restrict  Stream
                  );

/** Write Num elements of size Size from Buffer to Stream.

    The fwrite function writes, from the array pointed to by Buffer, up to Num
    elements whose size is specified by Size, to the stream pointed to by
    Stream.  For each object, Size calls are made to the fputc function, taking
    the values (in order) from an array of unsigned char exactly overlaying the
    Buffer object.  The file position indicator for the stream (if defined) is
    advanced by the number of characters successfully written.  If an error
    occurs, the resulting value of the file position indicator for the stream is
    indeterminate.

    @param[out]   Buffer    Pointer to an object containing the data to be written.
    @param[in]    Size      Size of each element to be written.
    @param[in]    Num       Number of elements to write.
    @param[in]    Stream    Output stream to write the data to.

    @return   The fwrite function returns the number of elements successfully
              written, which will be less than Num only if a write error is
              encountered.  If Size or Num is zero, fwrite returns zero and
              the state of the stream remains unchanged.
**/
size_t    fwrite  (const void   * __restrict  Buffer,
                   size_t                     Size,
                   size_t                     Num,
                   FILE         * __restrict  Stream
                  );

/* ################ File Positioning Functions.  */

/** Get a stream's position and parse state.

    The fgetpos function stores the current values of the parse state (if any)
    and file position indicator for the stream pointed to by Stream in the
    object pointed to by Pos.  The values stored contain unspecified
    information usable by the fsetpos function for repositioning the stream
    to its position at the time of the call to the fgetpos function.

    @param[in]    Stream    Stream to get current position of.
    @param[out]   Pos       Object to receive the stream's state and position information.

    @return   If successful, the fgetpos function returns zero; if either
              parameter is NULL, the fgetpos function returns nonzero and
              stores EINVAL in errno.
**/
int       fgetpos (FILE * __restrict Stream, fpos_t * __restrict Pos);

/** Set the file position for a stream.

    The fseek function sets the file position indicator for the stream pointed
    to by Stream.  If a read or write error occurs, the error indicator for the
    stream is set and fseek fails.

    For a binary stream, the new position, measured in characters from the
    beginning of the file, is obtained by adding Offset to the position
    specified by Whence. The specified position is the beginning of the file if
    Whence is SEEK_SET, the current value of the file position indicator if
    SEEK_CUR, or end-of-file if SEEK_END.

    For a text stream, Offset must either be zero or a value returned by an
    earlier successful call to the ftell function, on a stream associated with
    the same file, and Whence must be SEEK_SET.

    After determining the new position, a successful call to the fseek function
    undoes any effects of the ungetc function on the stream, clears the
    end-of-file indicator for the stream, and then establishes the new position.
    After a successful fseek call, the next operation on an update stream may
    be either input or output.

    @param[in]  Stream  The I/O stream to set the position of.
    @param[in]  Offset  The position, interpreted depending upon the value of
                        Whence, that the stream is to be positioned to.
    @param[in]  Whence  A value indicating how Offset is to be interpreted:
                          - SEEK_SET indicates Offset is an absolute position.
                          - SEEK_END indicates Offset is relative to the end of the file.
                          - SEEK_CUR indicates Offset is relative to the current position.

@return   The fseek function returns nonzero only for a request that cannot be satisfied.
**/
int       fseek   (FILE *Stream, long Offset, int Whence);

/** Set a stream's position and parse state.

    The fsetpos function sets the mbstate_t object (if any) and file position
    indicator for the stream pointed to by Stream according to the value of the
    object pointed to by Pos, which is a value that was obtained from an
    earlier successful call to the fgetpos function on a stream associated with
    the same file. If a read or write error occurs, the error indicator for the
    stream is set and fsetpos fails.

    A successful call to the fsetpos function undoes any effects of the ungetc
    function on the stream, clears the end-of-file indicator for the stream,
    and then establishes the new parse state and position. After a successful
    fsetpos call, the next operation on an update stream may be either input or output.

    @param[in]    Stream    Stream to set current position of.
    @param[in]    Pos       Object containing the state and position information.

    @return   If successful, the fsetpos function returns zero; on failure, the
              fsetpos function returns nonzero and stores EINVAL, or ESPIPE,
              in errno; depending upon whether the error was because of an invalid
              parameter, or because Stream is not seekable.
**/
int       fsetpos (FILE *Stream, const fpos_t *Pos);

/** Get Stream's current position.

    The ftell function obtains the current value of the file position indicator
    for the stream pointed to by Stream. For a binary stream, the value is the
    number of characters from the beginning of the file. For a text stream, its
    file position indicator contains unspecified information, usable by the
    fseek function for returning the file position indicator for the stream to
    its position at the time of the ftell call; the difference between two such
    return values is not necessarily a meaningful measure of the number of
    characters written or read.

    @param[in]  Stream    Pointer to the FILE object to get the current position of.

    @return   If successful, the ftell function returns the current value of
              the file position indicator for the stream.  On failure, the
              ftell function returns -1L and stores ESPIPE in errno indicating
              that the stream is not seekable.
**/
long      ftell   (FILE *Stream);

/** Restore a Stream's file position to the beginning of the file.

    The rewind function sets the file position indicator for the stream pointed
    to by Stream to the beginning of the file and clears the stream's error indicator.

    @param[in]  Stream    Pointer to the stream to be positioned to its beginning.
**/
void      rewind  (FILE *Stream);

/* ################ Error-handling Functions.  */

/** Clear a Stream's error and end-of-file indicators.

    @param[in]  Stream    Pointer to the stream to be cleared of errors.
**/
void      clearerr(FILE *Stream);

/** Test the end-of-file indicator for Stream.

    @param[in]  Stream    Pointer to the FILE object to be tested for EOF.

    @return   The feof function returns non-zero if, and only if, the end-of-file
              indicator is set for Stream.
**/
int       feof    (FILE *Stream);

/** Test the error indicator for Stream.

    @param[in]  Stream    Pointer to the stream to be tested for error.

    @return   The ferror function returns non-zero if, and only if, the error
              indicator is set for Stream.
**/
int       ferror  (FILE *Stream);

/** Print an error message to stderr based upon the value of errno and String.

    The perror function maps the error number in the integer expression errno
    to an error message.  It writes a sequence of characters to the standard
    error stream thus: first (if String is not a null pointer and the character
    pointed to by String is not the null character), the string pointed to by
    String followed by a colon (:) and a space; then an appropriate error
    message string followed by a new-line character. The contents of the error
    message strings are the same as those returned by the strerror function
    with argument errno.

    @param[in]  String    A text string to prefix the output error message with.

    @sa strerror in <string.h>
**/
void      perror  (const char *String);

__END_DECLS

/*
 * IEEE Std 1003.1-90
 */
__BEGIN_DECLS
FILE  *fdopen(int, const char *);
__END_DECLS

/*
 * IEEE Std 1003.1c-95, also adopted by X/Open CAE Spec Issue 5 Version 2
 */
__BEGIN_DECLS
void    flockfile       (FILE *);
int     ftrylockfile    (FILE *);
void    funlockfile     (FILE *);
int     getc_unlocked   (FILE *);
int     getchar_unlocked(void);
int     putc_unlocked   (int, FILE *);
int     putchar_unlocked(int);
__END_DECLS

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
  //int   getw(FILE *);
  //int   putw(int, FILE *);
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
int     fseeko(FILE *, off_t, int);
off_t   ftello(FILE *);
__END_DECLS

/*
 * Routines that are purely local.
 */
#define FPARSELN_UNESCESC   0x01
#define FPARSELN_UNESCCONT  0x02
#define FPARSELN_UNESCCOMM  0x04
#define FPARSELN_UNESCREST  0x08
#define FPARSELN_UNESCALL   0x0f

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
  //int     vfscanf(FILE * __restrict, const char * __restrict,
  //      va_list)
  //      __attribute__((__format__(__scanf__, 2, 0)));
  int     vsscanf(const char * __restrict, const char * __restrict,
        va_list)
        __attribute__((__format__(__scanf__, 2, 0)));
  //const char *fmtcheck(const char *, const char *)
  //      __attribute__((__format_arg__(2)));
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
