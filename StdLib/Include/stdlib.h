/** @file
  The header <stdlib.h> declares five types and several functions of general
  utility, and defines several macros.

  The files stddef.h and stdlib.h are "catch all" headers for definitions and declarations
  that don't fit well in the other headers.  There are two separate header files because
  the contents of <stddef.h> are valid in both freestanding and hosted environment, while the
  header <stdlib.h> contains elements that are only valid in a hosted environment.

  The following macros are defined in this file:<BR>
  @verbatim
    EXIT_FAILURE    An expression indicating application failure, used as an argument to exit().
    EXIT_SUCCESS    An expression indicating application success, used as an argument to exit().
    RAND_MAX        The maximum value returned by the rand function.
    MB_CUR_MAX      Maximum number of bytes in a multibyte character for the current locale.
    ATEXIT_MAX      Maximum number of routines that may be registered by the atexit function.
  @endverbatim

  The following types are defined in this file:<BR>
  @verbatim
    size_t      Unsigned integer type of the result of the sizeof operator.
    wchar_t     The type of a wide character.
    div_t       Type of the value returned by the div function.
    ldiv_t      Type of the value returned by the ldiv function.
    lldiv_t     Type of the value returned by the lldiv function.
  @endverbatim

  The following functions are declared in this file:<BR>
  @verbatim
    ################  Communication with the environment
    void        abort   (void) __noreturn;
    int         atexit  (void (*)(void));
    void        exit    (int status) __noreturn;
    void        _Exit   (int status) __noreturn;
    char       *getenv  (const char *name);
    int         setenv  (register const char * name,
                         register const char * value, int rewrite);
    int         system  (const char *string);

    ################  Integer arithmetic functions
    int         abs     (int j);
    long        labs    (long j);
    long long   llabs   (long long j);
    div_t       div     (int numer, int denom);
    ldiv_t      ldiv    (long numer, long denom);
    lldiv_t     lldiv   (long long numer, long long denom);

    ################  Pseudo-random sequence generation functions
    int         rand    (void);
    void        srand   (unsigned seed);

    ################  Memory management functions
    void       *calloc  (size_t Num, size_t Size);
    void        free    (void *);
    void       *malloc  (size_t);
    void       *realloc (void *Ptr, size_t NewSize);

    ################  Searching and Sorting utilities
    void       *bsearch (const void *key,  const void *base0,
                         size_t nmemb,     size_t size,
                         int (*compar)(const void *, const void *));
    void        qsort   (void *base, size_t nmemb, size_t size,
                         int (*compar)(const void *, const void *));

    ################  Multibyte/wide character conversion functions
    int         mblen   (const char *, size_t);
    int         mbtowc  (wchar_t * __restrict, const char * __restrict, size_t);
    int         wctomb  (char *, wchar_t);

    ################  Multibyte/wide string conversion functions
    size_t      mbstowcs  (wchar_t * __restrict dest,
                           const char * __restrict src, size_t limit);
    size_t      wcstombs  (char * __restrict dest,
                           const wchar_t * __restrict src, size_t limit);

    ################  Miscelaneous functions for *nix compatibility
    char       *realpath    (char *file_name, char *resolved_name);
    const char *getprogname (void);
    void        setprogname (const char *progname);

    ############  Integer Numeric conversion functions
    int                   atoi      (const char *nptr);
    long                  atol      (const char *nptr);
    long long             atoll     (const char *nptr);
    long                  strtol    (const char * __restrict nptr,
                                     char ** __restrict endptr, int base);
    unsigned long         strtoul   (const char * __restrict nptr,
                                     char ** __restrict endptr, int base);
    long long             strtoll   (const char * __restrict nptr,
                                     char ** __restrict endptr, int base);
    unsigned long long    strtoull  (const char * __restrict nptr,
                                     char ** __restrict endptr, int base);

    #########  Floating-point Numeric conversion functions
    double                atof      (const char *);
    double                strtod    (const char * __restrict nptr,
                                     char ** __restrict endptr);
    float                 strtof    (const char * __restrict nptr,
                                     char ** __restrict endptr);
    long double           strtold   (const char * __restrict nptr,
                                     char ** __restrict endptr);
  @endverbatim

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _STDLIB_H
#define _STDLIB_H
#include  <sys/EfiCdefs.h>

#ifdef _EFI_SIZE_T_
  /** Unsigned integer type of the result of the sizeof operator. **/
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    /** Type of a wide (Unicode) character. **/
    typedef _EFI_WCHAR_T wchar_t;
    #undef  _EFI_WCHAR_T
    #undef _BSD_WCHAR_T_
  #endif
#endif

/// A structure type that is the type of the value returned by the div function.
typedef struct {
  int quot;   /**< quotient */
  int rem;    /**< remainder */
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

/** @{
    Expand to integer constant expressions that can be used as the argument to
    the exit function to return unsuccessful or successful termination status,
    respectively, to the host environment.
**/
#define EXIT_FAILURE  1
#define EXIT_SUCCESS  0
/*@}*/

/** Expands to an integer constant expression that is the maximum value
    returned by the rand function.
**/
#define RAND_MAX  0x7fffffff

/** Expands to a positive integer expression with type size_t that is the
    maximum number of bytes in a multibyte character for the extended character
    set specified by the current locale (category LC_CTYPE), which is never
    greater than MB_LEN_MAX.

    Since UEFI only supports the Unicode Base Multilingual Plane (BMP),
    correctly formed characters will only produce 1, 2, or 3-byte UTF-8 characters.
**/
#define MB_CUR_MAX  3

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
void    abort(void) __noreturn;

/** The atexit function registers the function pointed to by func, to be
    called without arguments at normal program termination.

    The implementation supports the registration of up to 32 functions.

    @param[in]  Handler   Pointer to the function to register as one of the
                          routines to call at application exit time.

    @return   The atexit function returns zero if the registration succeeds,
              nonzero if it fails.
**/
int     atexit(void (*Handler)(void));

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

    Finally, control is returned to the host environment.

    @param[in]  status    A value to be returned when the application exits.

    @return   If the value of status is zero, or EXIT_SUCCESS, status is
              returned unchanged. If the value of status is EXIT_FAILURE,
              RETURN_ABORTED is returned.  Otherwise, status is returned unchanged.
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

    @param[in]  status    A value to be returned when the application exits.

    @return   If the value of status is zero, or EXIT_SUCCESS, status is
              returned unchanged. If the value of status is EXIT_FAILURE,
              RETURN_ABORTED is returned.  Otherwise, status is returned unchanged.
**/
void    _Exit(int status) __noreturn;

/** The getenv function searches an environment list, provided by the host
    environment, for a string that matches the string pointed to by name.  The
    set of environment names and the method for altering the environment list
    are determined by the underlying UEFI Shell implementation.

    @param[in]  name    Pointer to a string naming the environment variable to retrieve.

    @return   The getenv function returns a pointer to a string associated with
              the matched list member.  The string pointed to shall not be
              modified by the program, but may be overwritten by a subsequent
              call to the getenv function.  If the specified name cannot be
              found, a null pointer is returned.
**/
char   *getenv(const char *name);

/** Add or update a variable in the environment list.

    @param[in]  name     Address of a zero terminated name string.
    @param[in]  value    Address of a zero terminated value string.
    @param[in]  rewrite  TRUE allows overwriting existing values.

    @retval  0  Returns 0 upon success.
    @retval -1  Returns -1 upon failure, sets errno with more information.
**/
int
setenv (
  register const char * name,
  register const char * value,
  int rewrite
  );

/** If string is a null pointer, the system function determines whether the
    host environment has a command processor. If string is not a null pointer,
    the system function passes the string pointed to by string to that command
    processor to be executed in a manner which the implementation shall
    document; this might then cause the program calling system to behave in a
    non-conforming manner or to terminate.

    @param[in]  string    Pointer to the command string to be executed.

    @return   If the argument is a null pointer, the system function returns
              nonzero only if a command processor is available. If the argument
              is not a null pointer, and the system function does return, it
              returns an implementation-defined value.
**/
int     system(const char *string);


/* ################  Integer arithmetic functions  ######################## */

/** Computes the absolute value of an integer j.

    @param[in]  j   The value to find the absolute value of.

    @return   The absolute value of j.
**/
int     abs(int j);

/** Computes the absolute value of a long integer j.

    @param[in]  j   The value to find the absolute value of.

    @return   The absolute value of j.
**/
long    labs(long j);

/** Computes the absolute value of a long long integer j.

    @param[in]  j   The value to find the absolute value of.

    @return   The absolute value of j.
**/
long long
        llabs(long long j);

/** Computes numer / denom and numer % denom in a single operation.

    @param[in]  numer   The numerator for the division.
    @param[in]  denom   The denominator for the division.

    @return   Returns a structure of type div_t, comprising both the
              quotient and the remainder.
**/
div_t   div(int numer, int denom);

/** Computes numer / denom and numer % denom in a single operation.

    @param[in]  numer   The numerator for the division.
    @param[in]  denom   The denominator for the division.

    @return   Returns a structure of type ldiv_t, comprising both the
              quotient and the remainder.
**/
ldiv_t  ldiv(long numer, long denom);

/** Computes numer / denom and numer % denom in a single operation.

    @param[in]  numer   The numerator for the division.
    @param[in]  denom   The denominator for the division.

    @return   Returns a structure of type lldiv_t, comprising both the
              quotient and the remainder.
**/
lldiv_t lldiv(long long numer, long long denom);

/* ############  Integer Numeric conversion functions  #################### */

/** The atoi function converts the initial portion of the string pointed to by
    nptr to int representation.  Except for the behavior on error, it is
    equivalent to:
      - atoi: (int)strtol(nptr, (char **)NULL, 10)

    @param[in]  nptr  Pointer to the string to be converted.

    @return   The atoi function returns the converted value.
**/
int     atoi(const char *nptr);

/** The atol function converts the initial portion of the string pointed to by
    nptr to long int representation.  Except for the behavior on error, it is
    equivalent to:
      - atol: strtol(nptr, (char **)NULL, 10)

    @param[in]  nptr  Pointer to the string to be converted.

    @return   The atol function returns the converted value.
**/
long    atol(const char *nptr);

/** The atoll function converts the initial portion of the string pointed to by
    nptr to long long int representation.  Except for the behavior on error, it
    is equivalent to:
      - atoll: strtoll(nptr, (char **)NULL, 10)

    @param[in]  nptr  Pointer to the string to be converted.

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
    that of an integer constant, optionally preceded
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

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.
    @param[in]    base    The base, 0 to 36, of the number represented by the input string.

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

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.
    @param[in]    base    The base, 0 to 36, of the number represented by the input string.

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

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.
    @param[in]    base    The base, 0 to 36, of the number represented by the input string.

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

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.
    @param[in]    base    The base, 0 to 36, of the number represented by the input string.

    @return   The strtoull function returns the converted value, if any. If no
              conversion could be performed, zero is returned. If the correct
              value is outside the range of representable values, ULLONG_MAX is
              returned and the value of the macro ERANGE is stored in errno.
**/
unsigned long long
        strtoull(const char * __restrict nptr, char ** __restrict endptr, int base);

/* #########  Floating-point Numeric conversion functions  ################ */

/** Convert the initial part of a string to double representation.

    @param[in]  nptr  Pointer to the string to be converted.

    @return   The floating-point value representing the string nptr.
**/
double  atof(const char *nptr);

/** @{
    The strtod, strtof, and strtold functions convert the initial portion of
    the string pointed to by nptr to double, float, and long double
    representation, respectively. First, they decompose the input string into
    three parts: an initial, possibly empty, sequence of white-space characters
    (as specified by the isspace function), a subject sequence resembling a
    floating-point constant or representing an infinity or NaN; and a final
    string of one or more unrecognized characters, including the terminating
    null character of the input string. Then, they attempt to convert the
    subject sequence to a floating-point number, and return the result.
*/

/** Convert a string to a double and point to the character after the last converted.

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.

    @return   A floating-point value representing the string nptr.
              A pointer to the final string is stored in the object pointed to
              by endptr, provided that endptr is not a null pointer.
              If the subject sequence is empty or does not have the expected
              form, no conversion is performed; the value of nptr is stored in
              the object pointed to by endptr, provided that endptr is not a null pointer.
**/
double  strtod(const char * __restrict nptr, char ** __restrict endptr);

/** Convert a string to a float and point to the character after the last converted.

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.

    @return   A floating-point value representing the string nptr.
              A pointer to the final string is stored in the object pointed to
              by endptr, provided that endptr is not a null pointer.
              If the subject sequence is empty or does not have the expected
              form, no conversion is performed; the value of nptr is stored in
              the object pointed to by endptr, provided that endptr is not a null pointer.
**/
float   strtof(const char * __restrict nptr, char ** __restrict endptr);

/** Convert a string to a long double and point to the character after the last converted.

    @param[in]    nptr    Pointer to the string to be converted.
    @param[out]   endptr  If not NULL, points to an object to receive a pointer to the final string.

    @return   A floating-point value representing the string nptr.
              A pointer to the final string is stored in the object pointed to
              by endptr, provided that endptr is not a null pointer.
              If the subject sequence is empty or does not have the expected
              form, no conversion is performed; the value of nptr is stored in
              the object pointed to by endptr, provided that endptr is not a null pointer.
**/
long double
        strtold(const char * __restrict nptr, char ** __restrict endptr);
/*@}*/

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

    @param[in]  seed    The value used to "seed" the random number generator with.
**/
void    srand(unsigned seed);

/* ################  Memory management functions  ######################### */

/** The calloc function allocates space for an array of Num objects, each of
    whose size is Size.  The space is initialized to all bits zero.

    @param[in]  Num   The number of objects to allocate space for.
    @param[in]  Size  The size, in bytes, of each object.

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
void    free(void *Ptr);

/** The malloc function allocates space for an object whose size is specified
    by size and whose value is indeterminate.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    @param  Size    Size, in bytes, of the region to allocate.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.<BR>
              If NULL is returned, errno may contain:
              - EINVAL: Requested Size is zero.
              - ENOMEM: Memory could not be allocated.
**/
void   *malloc(size_t Size);

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

/** The bsearch function searches an array of Nmemb objects, the initial
    element of which is pointed to by Base, for an element that matches the
    object pointed to by Key. The size of each element of the array is
    specified by Size.

    The comparison function pointed to by Compar is called with two arguments
    that point to the Key object and to an array element, in that order. The
    function returns an integer less than, equal to, or greater than zero if
    the Key object is considered, respectively, to be less than, to match, or
    to be greater than the array element. The array consists of: all the
    elements that compare less than, all the elements that compare equal to,
    and all the elements that compare greater than the key object,
    in that order.

    @param[in]  Key     Pointer to the object to search for.
    @param[in]  Base    Pointer to the first element of an array to search.
    @param[in]  Nmemb   Number of objects in the search array.
    @param[in]  Size    The size of each object in the search array.
    @param[in]  Compar  Pointer to the function used to compare two objects.

    @return   The bsearch function returns a pointer to a matching element of the
              array, or a null pointer if no match is found. If two elements
              compare as equal, which element is matched is unspecified.
**/
void   *bsearch(  const void *Key,  const void *Base,
                  size_t Nmemb,     size_t Size,
                  int (*Compar)(const void *, const void *)
        );

/** The qsort function sorts an array of Nmemb objects, the initial element of
    which is pointed to by Base.  The size of each object is specified by Size.

    The contents of the array are sorted into ascending order according to a
    comparison function pointed to by Compar, which is called with two
    arguments that point to the objects being compared. The function shall
    return an integer less than, equal to, or greater than zero if the first
    argument is considered to be respectively less than, equal to, or greater
    than the second.

    If two elements compare as equal, their order in the resulting sorted array
    is unspecified.

    @param[in,out]  Base    Pointer to the first element of an array to sort.
    @param[in]      Nmemb   Number of objects in the array.
    @param[in]      Size    The size of each object in the array.
    @param[in]      Compar  Pointer to the function used to compare two objects.
**/
void    qsort( void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *));

/* ################  Multibyte/wide character conversion functions  ####### */

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
**/
int     mblen(const char *S, size_t N);

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
**/
int     mbtowc(wchar_t * __restrict Pwc, const char * __restrict S, size_t N);

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
**/
int     wctomb(char *S, wchar_t WC);

/* ################  Multibyte/wide string conversion functions  ########## */

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
**/
size_t  mbstowcs(wchar_t * __restrict Dest, const char * __restrict Src, size_t Limit);

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
    @param[in]    Limit   Maximum number of bytes to be written to Dest.

    @return   If a wide character is encountered that does not correspond to a
              valid multibyte character, the wcstombs function returns
              (size_t)(-1). Otherwise, the wcstombs function returns the number
              of bytes modified, not including a terminating null character,
              if any.
**/
size_t  wcstombs(char * __restrict Dest, const wchar_t * __restrict Src, size_t Limit);

/* ##############  Miscelaneous functions for *nix compatibility  ########## */

/** The realpath() function shall derive, from the pathname pointed to by
    file_name, an absolute pathname that names the same file, whose resolution
    does not involve '.', '..', or symbolic links. The generated pathname shall
    be stored as a null-terminated string, up to a maximum of {PATH_MAX} bytes,
    in the buffer pointed to by resolved_name.

    If resolved_name is a null pointer, the behavior of realpath() is
    implementation-defined.

    @param[in]      file_name         The filename to convert.
    @param[in,out]  resolved_name     The resultant name.

    @retval NULL                    An error occured.
    @retval resolved_name.
**/
char * realpath(char *file_name, char *resolved_name);

/** The getprogname() function returns the name of the program.  If the name
    has not been set yet, it will return NULL.

  @return   The getprogname function returns NULL if the program's name has not
            been set, otherwise it returns the name of the program.
**/
const char * getprogname(void);

/** The setprogname() function sets the name of the program.

  @param[in]  progname    The name of the program.  This memory must be retained
                          by the caller until no calls to "getprogname" will be
                          called.
**/
void setprogname(const char *progname);

/* ###############  Functions specific to this implementation  ############# */

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
OneWcToMcLen(const wchar_t InCh);

/*  Determine the number of bytes needed to represent a Wide character string
    as a MBCS string of given maximum length.  Will optionally return the number
    of wide characters that would be consumed.

    @param[in]    Src       Pointer to a wide character string.
    @param[in]    Limit     Maximum number of bytes the converted string may occupy.
    @param[out]   NumChar   Pointer to where to store the number of wide characters, or NULL.

    @return     The number of bytes required to convert Src to MBCS,
                not including the terminating NUL.  If NumChar is not NULL, the number
                of characters represented by the return value will be written to
                where it points.
**/
size_t
EFIAPI
EstimateWtoM(const wchar_t * Src, size_t Limit, size_t *NumChar);

/** Determine the number of characters in a MBCS string.

    @param[in]    Src     The string to examine

    @return   The number of characters represented by the MBCS string.
**/
size_t
EFIAPI
CountMbcsChars(const char *Src);

__END_DECLS

#endif  /* _STDLIB_H */
