/** @file
  The header <stdlib.h> declares five types and several functions of general
  utility, and defines several macros.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _STDLIB_H
#define _STDLIB_H
#include  <sys/EfiCdefs.h>

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    typedef _EFI_WCHAR_T wchar_t;
    #undef  _EFI_WCHAR_T
    #undef _BSD_WCHAR_T_
  #endif
#endif

/// A structure type that is the type of the value returned by the div function.
typedef struct {
  int quot;   /* quotient */
  int rem;    /* remainder */
} div_t;

/// A structure type that is the type of the value returned by the ldiv function.
typedef struct {
  long  quot;
  long  rem;
} ldiv_t;

/// A structure type that is the type of the value returned by the lldiv function.
typedef struct {
  long long quot;
  long long rem;
} lldiv_t;

/** Expand to integer constant expressions that can be used as the argument to
    the exit function to return unsuccessful or successful termination status,
    respectively, to the host environment.
**/
#define EXIT_FAILURE  1
#define EXIT_SUCCESS  0

/** Expands to an integer constant expression that is the maximum value
    returned by the rand function.

    The value of the RAND_MAX macro shall be at least 32767.
**/
#define RAND_MAX  0x7fffffff

/** Expands to a positive integer expression with type size_t that is the
    maximum number of bytes in a multibyte character for the extended character
    set specified by the current locale (category LC_CTYPE), which is never
    greater than MB_LEN_MAX.
**/
#define MB_CUR_MAX  2

/** Maximum number of functions that can be registered by atexit.

    The C standard states that the implementation shall support the
    registration of at least 32 functions.
**/
#define ATEXIT_MAX  32

__BEGIN_DECLS

/* ################  Communication with the environment  ################## */

/** The abort function causes abnormal program termination to occur, unless
    the signal SIGABRT is being caught and the signal handler does not return.

    Open streams with unwritten buffered data are not flushed, open
    streams are not closed, and temporary files are not removed by abort.

    Unsuccessful termination is returned to the host environment by means of
    the function call, raise(SIGABRT).

    @sa signal.h
**/
void    abort(void);

/** The atexit function registers the function pointed to by func, to be
    called without arguments at normal program termination.

    The implementation supports the registration of up to 32 functions.

    @return   The atexit function returns zero if the registration succeeds,
              nonzero if it fails.
**/
int     atexit(void (*)(void));

/** The exit function causes normal program termination to occur. If more than
    one call to the exit function is executed by a program,
    the behavior is undefined.

    First, all functions registered by the atexit function are called, in the
    reverse order of their registration, except that a function is called
    after any previously registered functions that had already been called at
    the time it was registered. If, during the call to any such function, a
    call to the longjmp function is made that would terminate the call to the
    registered function, the behavior is undefined.

    Next, all open streams with unwritten buffered data are flushed, all open
    streams are closed, and all files created by the tmpfile function
    are removed.

    Finally, control is returned to the host environment. If the value of
    status is zero, or EXIT_SUCCESS, status is returned unchanged. If the value
    of status is EXIT_FAILURE, EAPPLICATION is returned.
    Otherwise, status is returned unchanged.
**/
void    exit(int status) __noreturn;

/** The _Exit function causes normal program termination to occur and control
    to be returned to the host environment.

    No functions registered by the atexit function or signal handlers
    registered by the signal function are called.  Open streams with unwritten
    buffered data are not flushed, open streams are not closed, and temporary
    files are not removed by abort.

    The status returned to the host environment is determined in the same way
    as for the exit function.
**/
void    _Exit(int status) __noreturn;

/** The getenv function searches an environment list, provided by the host
    environment, for a string that matches the string pointed to by name.  The
    set of environment names and the method for altering the environment list
    are determined by the underlying UEFI Shell implementation.

    @return   The getenv function returns a pointer to a string associated with
              the matched list member.  The string pointed to shall not be
              modified by the program, but may be overwritten by a subsequent
              call to the getenv function.  If the specified name cannot be
              found, a null pointer is returned.
**/
char   *getenv(const char *name);

/** If string is a null pointer, the system function determines whether the
    host environment has a command processor. If string is not a null pointer,
    the system function passes the string pointed to by string to that command
    processor to be executed in a manner which the implementation shall
    document; this might then cause the program calling system to behave in a
    non-conforming manner or to terminate.

    @return   If the argument is a null pointer, the system function returns
              nonzero only if a command processor is available. If the argument
              is not a null pointer, and the system function does return, it
              returns an implementation-defined value.
**/
int     system(const char *string);


/* ################  Integer arithmetic functions  ######################## */

/** Computes the absolute value of an integer j.

    @return   The absolute value of j.
**/
int     abs(int j);

/** Computes the absolute value of an integer j.

    @return   The absolute value of j.
**/
long    labs(long j);

/** Computes the absolute value of an integer j.

    @return   The absolute value of j.
**/
long long
        llabs(long long j);

/** Computes numer / denom and numer % denom in a single operation.

    @return   Returns a structure of type div_t, comprising both the
              quotient and the remainder.
**/
div_t   div(int numer, int denom);

/** Computes numer / denom and numer % denom in a single operation.

    @return   Returns a structure of type ldiv_t, comprising both the
              quotient and the remainder.
**/
ldiv_t  ldiv(long numer, long denom);

/** Computes numer / denom and numer % denom in a single operation.

    @return   Returns a structure of type lldiv_t, comprising both the
              quotient and the remainder.
**/
lldiv_t lldiv(long long numer, long long denom);

/* ############  Integer Numeric conversion functions  #################### */

/** The atoi function converts the initial portion of the string pointed to by
    nptr to int representation.  Except for the behavior on error, it is
    equivalent to:
      - atoi: (int)strtol(nptr, (char **)NULL, 10)

  @return   The atoi function returns the converted value.
**/
int     atoi(const char *nptr);

/** The atol function converts the initial portion of the string pointed to by
    nptr to long int representation.  Except for the behavior on error, it is
    equivalent to:
      - atol: strtol(nptr, (char **)NULL, 10)

  @return   The atol function returns the converted value.
**/
long    atol(const char *nptr);

/** The atoll function converts the initial portion of the string pointed to by
    nptr to long long int representation.  Except for the behavior on error, it
    is equivalent to:
      - atoll: strtoll(nptr, (char **)NULL, 10)

  @return   The atoll function returns the converted value.
**/
long long
        atoll(const char *nptr);

/** The strtol, strtoll, strtoul, and strtoull functions convert the initial
    portion of the string pointed to by nptr to long int, long long int,
    unsigned long int, and unsigned long long int representation, respectively.
    First, they decompose the input string into three parts: an initial,
    possibly empty, sequence of white-space characters (as specified by the
    isspace function), a subject sequence resembling an integer represented in
    some radix determined by the value of base, and a final string of one or
    more unrecognized characters, including the terminating null character of
    the input string. Then, they attempt to convert the subject sequence to an
    integer, and return the result.

    If the value of base is zero, the expected form of the subject sequence is
    that of an integer constant as described in 6.4.4.1, optionally preceded
    by a plus or minus sign, but not including an integer suffix. If the value
    of base is between 2 and 36 (inclusive), the expected form of the subject
    sequence is a sequence of letters and digits representing an integer with
    the radix specified by base, optionally preceded by a plus or minus sign,
    but not including an integer suffix. The letters from a (or A) through z
    (or Z) are ascribed the values 10 through 35; only letters and digits whose
    ascribed values are less than that of base are permitted. If the value of
    base is 16, the characters 0x or 0X may optionally precede the sequence of
    letters and digits, following the sign if present.

    The subject sequence is defined as the longest initial subsequence of the
    input string, starting with the first non-white-space character, that is of
    the expected form. The subject sequence contains no characters if the input
    string is empty or consists entirely of white space, or if the first
    non-white-space character is other than a sign or a permissible letter or digit.

    If the subject sequence has the expected form and the value of base is
    zero, the sequence of characters starting with the first digit is
    interpreted as an integer constant. If the subject sequence has the
    expected form and the value of base is between 2 and 36, it is used as the
    base for conversion, ascribing to each letter its value as given above. If
    the subject sequence begins with a minus sign, the value resulting from the
    conversion is negated (in the return type). A pointer to the final string
    is stored in the object pointed to by endptr, provided that endptr is
    not a null pointer.

    In other than the "C" locale, additional locale-specific subject sequence
    forms may be accepted.

    If the subject sequence is empty or does not have the expected form, no
    conversion is performed; the value of nptr is stored in the object pointed
    to by endptr, provided that endptr is not a null pointer.

  @return   The strtol, strtoll, strtoul, and strtoull functions return the
            converted value, if any. If no conversion could be performed, zero
            is returned. If the correct value is outside the range of
            representable values, LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX,
            ULONG_MAX, or ULLONG_MAX is returned (according to the return type
            and sign of the value, if any), and the value of the macro ERANGE
            is stored in errno.
**/
long    strtol(const char * __restrict nptr, char ** __restrict endptr, int base);

/** The strtoul function converts the initial portion of the string pointed to
    by nptr to unsigned long int representation.

    See the description for strtol for more information.

  @return   The strtoul function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, ULONG_MAX is
            returned and the value of the macro ERANGE is stored in errno.
**/
unsigned long
        strtoul(const char * __restrict nptr, char ** __restrict endptr, int base);

/** The strtoll function converts the initial portion of the string pointed to
    by nptr to long long int representation.

    See the description for strtol for more information.

  @return   The strtoll function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, LLONG_MIN or
            LLONG_MAX is returned (according to the sign of the value, if any),
            and the value of the macro ERANGE is stored in errno.
**/
long long
        strtoll(const char * __restrict nptr, char ** __restrict endptr, int base);

/** The strtoull function converts the initial portion of the string pointed to
    by nptr to unsigned long long int representation.

    See the description for strtol for more information.

  @return   The strtoull function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, ULLONG_MAX is
            returned and the value of the macro ERANGE is stored in errno.
**/
unsigned long long
        strtoull(const char * __restrict nptr, char ** __restrict endptr, int base);

/* #########  Floating-point Numeric conversion functions  ################ */

/**

  @return
**/
double  atof(const char *);

/**

  @return
**/
double  strtod(const char * __restrict nptr, char ** __restrict endptr);

/**

  @return
**/
float   strtof(const char * __restrict nptr, char ** __restrict endptr);

/**

  @return
**/
long double
        strtold(const char * __restrict nptr, char ** __restrict endptr);

/* ################  Pseudo-random sequence generation functions  ######### */

/** The rand function computes a sequence of pseudo-random integers in the
    range 0 to RAND_MAX.

    @return   The rand function returns a pseudo-random integer.
**/
int     rand(void);

/** The srand function uses the argument as a seed for a new sequence of
    pseudo-random numbers to be returned by subsequent calls to rand.

    If srand is then called with the same seed value, the sequence of
    pseudo-random numbers shall be repeated. If rand is called before any calls
    to srand have been made, the same sequence shall be generated as when srand
    is first called with a seed value of 1.
**/
void    srand(unsigned seed);

/* ################  Memory management functions  ######################### */

/** The calloc function allocates space for an array of Num objects, each of
    whose size is Size.  The space is initialized to all bits zero.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.
**/
void   *calloc(size_t Num, size_t Size);

/** The free function causes the space pointed to by Ptr to be deallocated,
    that is, made available for further allocation.

    If Ptr is a null pointer, no action occurs.  Otherwise, if the argument
    does not match a pointer earlier returned by the calloc, malloc, or realloc
    function, or if the space has been deallocated by a call to free or
    realloc, the behavior is undefined.

    @param  Ptr     Pointer to a previously allocated region of memory to be freed.

**/
void    free(void *);

/** The malloc function allocates space for an object whose size is specified
    by size and whose value is indeterminate.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    @param  size    Size, in bytes, of the region to allocate.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.<BR>
              If NULL is returned, errno may contain:
              - EINVAL: Requested Size is zero.
              - ENOMEM: Memory could not be allocated.
**/
void   *malloc(size_t);

/** The realloc function changes the size of the object pointed to by Ptr to
    the size specified by NewSize.

    The contents of the object are unchanged up to the lesser of the new and
    old sizes.  If the new size is larger, the value of the newly allocated
    portion of the object is indeterminate.

    If Ptr is a null pointer, the realloc function behaves like the malloc
    function for the specified size.

    If Ptr does not match a pointer earlier returned by the calloc, malloc, or
    realloc function, or if the space has been deallocated by a call to the free
    or realloc function, the behavior is undefined.

    If the space cannot be allocated, the object pointed to by Ptr is unchanged.

    If NewSize is zero and Ptr is not a null pointer, the object it points to
    is freed.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    @param  Ptr     Pointer to a previously allocated region of memory to be resized.
    @param  NewSize Size, in bytes, of the new object to allocate space for.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.  If NewSize is zero,
              NULL is returned and errno will be unchanged.
**/
void   *realloc(void *Ptr, size_t NewSize);

/* ################  Searching and Sorting utilities  ##################### */

/** The bsearch function searches an array of nmemb objects, the initial
    element of which is pointed to by base, for an element that matches the
    object pointed to by key. The size of each element of the array is
    specified by size.

    The comparison function pointed to by compar is called with two arguments
    that point to the key object and to an array element, in that order. The
    function returns an integer less than, equal to, or greater than zero if
    the key object is considered, respectively, to be less than, to match, or
    to be greater than the array element. The array consists of: all the
    elements that compare less than, all the elements that compare equal to,
    and all the elements that compare greater than the key object,
    in that order.

  @return   The bsearch function returns a pointer to a matching element of the
            array, or a null pointer if no match is found. If two elements
            compare as equal, which element is matched is unspecified.
**/
void *
bsearch(  const void *key,  const void *base0,
          size_t nmemb,     size_t size,
          int (*compar)(const void *, const void *)
);

/** The qsort function sorts an array of nmemb objects, the initial element of
    which is pointed to by base.  The size of each object is specified by size.

    The contents of the array are sorted into ascending order according to a
    comparison function pointed to by compar, which is called with two
    arguments that point to the objects being compared. The function shall
    return an integer less than, equal to, or greater than zero if the first
    argument is considered to be respectively less than, equal to, or greater
    than the second.

    If two elements compare as equal, their order in the resulting sorted array
    is unspecified.
**/
void qsort( void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));

/* ################  Multibyte/wide character conversion functions  ####### */

/**

  @return
**/
int     mblen(const char *, size_t);

/**

  @return
**/
int     mbtowc(wchar_t * __restrict, const char * __restrict, size_t);

/**

  @return
**/
int     wctomb(char *, wchar_t);

/* ################  Multibyte/wide string conversion functions  ########## */

/**

  @return
**/
size_t  mbstowcs(wchar_t * __restrict , const char * __restrict, size_t);

/**

  @return
**/
size_t  wcstombs(char * __restrict, const wchar_t * __restrict, size_t);

__END_DECLS

#endif  /* _STDLIB_H */
