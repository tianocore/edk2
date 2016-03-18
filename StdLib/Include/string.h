/** @file
    The header <string.h> declares one type and several functions, and defines
    one macro useful for manipulating arrays of character type and other objects
    treated as arrays of character type.  Various methods are used for
    determining the lengths of the arrays, but in all cases a char * or void *
    argument points to the initial (lowest addressed) character of the array. If
    an array is accessed beyond the end of an object, the behavior is undefined.

    Where an argument declared as size_t n specifies the length of the array for
    a function, n can have the value zero on a call to that function. Unless
    explicitly stated otherwise in the description of those functions, pointer
    arguments on such a call must still have valid values.

    For all functions declared in this header, each character shall be
    interpreted as if it had the type unsigned char (and therefore every possible
    object representation is valid and has a different value).

    The following macros are defined in this file:<BR>
    @verbatim
      NULL
      bcopy(a,b,c)    ( memcpy((void *)b, (const void *)a, (size_t)c))
      bcmp(a,b,c)     ( memcmp((void *)a, (void *)b, (size_t)c))
    @endverbatim

    The following types are defined in this file:<BR>
    @verbatim
      size_t      Unsigned integer type of the result of the sizeof operator.
    @endverbatim

    The following functions are declared in this file:<BR>
    @verbatim
      ################ Copying Functions
      void     *memcpy      (void * __restrict s1, const void * __restrict s2, size_t n);
      void     *memmove     (void *s1, const void *s2, size_t n);
      char     *strcpy      (char * __restrict s1, const char * __restrict s2);
      char     *strncpy     (char * __restrict s1, const char * __restrict s2, size_t n);
      int       strncpyX    (char * __restrict s1, const char * __restrict s2, size_t n);

      ################ Concatenation Functions
      char     *strcat      (char * __restrict s1, const char * __restrict s2);
      char     *strncat     (char * __restrict s1, const char * __restrict s2, size_t n);
      int       strncatX    (char * __restrict s1, const char * __restrict s2, size_t n);

      ################ Comparison Functions
      int       memcmp      (const void *s1, const void *s2, size_t n);
      int       strcmp      (const char *s1, const char *s2);
      int       strcoll     (const char *s1, const char *s2);
      int       strncmp     (const char *s1, const char *s2, size_t n);
      size_t    strxfrm     (char * __restrict s1, const char * __restrict s2, size_t n);

      ################ Search Functions
      void     *memchr      (const void *s, int c, size_t n);
      char     *strchr      (const char *s, int c);
      size_t    strcspn     (const char *s1, const char *s2);
      char     *strpbrk     (const char *s1, const char *s2);
      char     *strrchr     (const char *s, int c);
      size_t    strspn      (const char *s1 , const char *s2);
      char     *strstr      (const char *s1 , const char *s2);
      char     *strtok      (char * __restrict s1, const char * __restrict s2);

      ################ Miscellaneous Functions
      void     *memset      (void *s, int c, size_t n);
      char     *strerror    (int num);
      size_t    strlen      (const char *);

      ################ BSD Compatibility Functions
      char     *strdup      (const char *);
      int       strerror_r  (int, char *, size_t);
      int       strcasecmp  (const char *s1, const char *s2);
      void     *memccpy     (void *, const void *, int, size_t);
      int       strncasecmp (const char *s1, const char *s2, size_t n);
      size_t    strlcpy     (char *destination, const char *source, size_t size);
      size_t    strlcat     (char *destination, const char *source, size_t size);
      char     *strsep      (register char **stringp, register const char *delim);
    @endverbatim

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _STRING_H
#define _STRING_H
#include  <sys/EfiCdefs.h>

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

__BEGIN_DECLS

/* ################   Copying Functions   ################################# */

/** The memcpy function copies N characters from the object pointed to by Src
    into the object pointed to by Dest. If copying takes place between objects
    that overlap, the behavior is undefined.

    @param[out]   Dest  Pointer to the destination of the copy operation.
    @param[in]    Src   Pointer to the Source data to be copied.
    @param[in]    N     Number of characters (bytes) to be copied.

    @return   The memcpy function returns the value of Dest.
**/
void     *memcpy(void * __restrict Dest, const void * __restrict Src, size_t N);

/** The memmove function copies N characters from the object pointed to by Src
    into the object pointed to by Dest. Copying takes place as if the N
    characters from the object pointed to by Src are first copied into a
    temporary array of N characters that does not overlap the objects pointed
    to by Dest and Src, and then the N characters from the temporary array are
    copied into the object pointed to by Dest.

    @param[out]   Dest  Pointer to the destination of the copy operation.
    @param[in]    Src   Pointer to the Source data to be copied.
    @param[in]    N     Number of characters (bytes) to be copied.

    @return   The memmove function returns the value of Dest.
**/
void     *memmove(void *Dest, const void *Src, size_t N);

/** The strcpy function copies the string pointed to by Src (including the
    terminating null character) into the array pointed to by Dest. If copying
    takes place between objects that overlap, the behavior is undefined.

    @param[out]   Dest  Pointer to the destination of the copy operation.
    @param[in]    Src   Pointer to the Source data to be copied.

    @return   The strcpy function returns the value of Dest.
**/
char     *strcpy(char * __restrict Dest, const char * __restrict Src);

/** The strncpy function copies not more than N characters (characters that
    follow a null character are not copied) from the array pointed to by Src to
    the array pointed to by Dest. If copying takes place between objects that
    overlap, the behavior is undefined.

    If the array pointed to by Src is a string that is shorter than N
    characters, null characters are appended to the copy in the array pointed
    to by Dest, until N characters in all have been written.

    @param[out]   Dest  Pointer to the destination of the copy operation.
    @param[in]    Src   Pointer to the Source data to be copied.
    @param[in]    N     Number of characters (bytes) to be copied.

    @return   The strncpy function returns the value of Dest.
**/
char     *strncpy(char * __restrict Dest, const char * __restrict Src, size_t N);

/** The strncpyX function copies not more than N-1 characters (characters that
    follow a null character are not copied) from the array pointed to by Src to
    the array pointed to by Dest. Array Dest is guaranteed to be NULL terminated.
    If copying takes place between objects that overlap,
    the behavior is undefined.

    strncpyX exists because normal strncpy does not indicate if the copy was
    terminated because of exhausting the buffer or reaching the end of Src.

    @param[out]   Dest  Pointer to the destination of the copy operation.
    @param[in]    Src   Pointer to the Source data to be copied.
    @param[in]    N     Number of characters (bytes) to be copied.

    @return   The strncpyX function returns 0 if the copy operation was
              terminated because it reached the end of Dest.  Otherwise,
              a non-zero value is returned indicating how many characters
              remain in Dest.
**/
int       strncpyX(char * __restrict Dest, const char * __restrict Src, size_t N);

/* ################   Concatenation Functions   ########################### */

/** The strcat function appends a copy of the string pointed to by Src
    (including the terminating null character) to the end of the string pointed
    to by Dest. The initial character of Src overwrites the null character at the
    end of Dest. If copying takes place between objects that overlap, the
    behavior is undefined.

    @param[out]   Dest  Pointer to the destination of the concatenation operation.
    @param[in]    Src   Pointer to the Source data to be concatenated.

    @return   The strcat function returns the value of Dest.
**/
char     *strcat(char * __restrict Dest, const char * __restrict Src);

/** The strncat function appends not more than N characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by Src to the end of the string pointed to by Dest. The initial character of
    Src overwrites the null character at the end of Dest. A terminating null
    character is always appended to the result. If copying takes place
    between objects that overlap, the behavior is undefined.

    @param[out]   Dest  Pointer to the destination of the concatenation operation.
    @param[in]    Src   Pointer to the Source data to be concatenated.
    @param[in]    N     Max Number of characters (bytes) to be concatenated.

    @return   The strncat function returns the value of Dest.
**/
char     *strncat(char * __restrict Dest, const char * __restrict Src, size_t N);

/** The strncatX function appends not more than N characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by Src to the end of the string pointed to by Dest. The initial character of
    Src overwrites the null character at the end of Dest. The result is always
    terminated with a null character. If copying takes place between objects
    that overlap, the behavior is undefined.

    strncatX exists because normal strncat does not indicate if the operation
    was terminated because of exhausting N or reaching the end of Src.

    @param[out]   Dest  Pointer to the destination of the concatenation operation.
    @param[in]    Src   Pointer to the Source data to be concatenated.
    @param[in]    N     Max Number of characters (bytes) to be concatenated.

    @return   The strncatX function returns 0 if the operation was terminated
              because it reached the end of Dest.  Otherwise, a non-zero value is
              returned indicating how many characters remain in Dest.
**/
int       strncatX(char * __restrict s1, const char * __restrict s2, size_t n);

/* ################   Comparison Functions   ############################## */

/** The memcmp function compares the first N characters of the object pointed
    to by S1 to the first N characters of the object pointed to by S2.

    @param[out]   S1  Pointer to the first object to be compared.
    @param[in]    S2  Pointer to the object to be compared to S1.
    @param[in]    N   Max Number of characters (bytes) to be compared.

    @return   The memcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the object pointed to by S1 is
              greater than, equal to, or less than the object pointed to by S2.
**/
int       memcmp(const void *S1, const void *S2, size_t N);

/** The strcmp function compares the string pointed to by S1 to the string
    pointed to by S2.

    @param[out]   S1  Pointer to the first string to be compared.
    @param[in]    S2  Pointer to the string to be compared to S1.

    @return   The strcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the string pointed to by S1 is
              greater than, equal to, or less than the string pointed to by S2.
**/
int       strcmp(const char *S1, const char *S2);

/** The strcoll function compares the string pointed to by S1 to the string
    pointed to by S2, both interpreted as appropriate to the LC_COLLATE
    category of the current locale.

    @param[out]   S1  Pointer to the first string to be compared.
    @param[in]    S2  Pointer to the string to be compared to S1.

    @return   The strcoll function returns an integer greater than, equal to,
              or less than zero, accordingly as the string pointed to by S1 is
              greater than, equal to, or less than the string pointed to by S2
              when both are interpreted as appropriate to the current locale.
**/
int       strcoll(const char *S1, const char *S2);

/** The strncmp function compares not more than N characters (characters that
    follow a null character are not compared) from the array pointed to by S1
    to the array pointed to by S2.

    @param[out]   S1  Pointer to the first object to be compared.
    @param[in]    S2  Pointer to the object to be compared to S1.
    @param[in]    N   Max Number of characters (bytes) to be compared.

    @return   The strncmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the possibly null-terminated
              array pointed to by S1 is greater than, equal to, or less than
              the possibly null-terminated array pointed to by S2.
**/
int       strncmp(const char *S1, const char *S2, size_t N);

/** The strxfrm function transforms the string pointed to by Src and places the
    resulting string into the array pointed to by Dest. The transformation is
    such that if the strcmp function is applied to two transformed strings, it
    returns a value greater than, equal to, or less than zero, corresponding to
    the result of the strcoll function applied to the same two original
    strings. No more than N characters are placed into the resulting array
    pointed to by Dest, including the terminating null character. If N is zero,
    Dest is permitted to be a null pointer. If copying takes place between
    objects that overlap, the behavior is undefined.

    @param[out]   Dest  Pointer to the object to receive the transformed string.
    @param[in]    Src   Pointer to the string to be transformed.
    @param[in]    N     Max Number of characters (bytes) to be transformed.

    @return   The strxfrm function returns the length of the transformed string
              (not including the terminating null character). If the value
              returned is N or more, the contents of the array pointed to by Dest
              are indeterminate.
**/
size_t    strxfrm(char * __restrict Dest, const char * __restrict Src, size_t N);

/* ################   Search Functions   ################################## */

/** The memchr function locates the first occurrence of C (converted to an
    unsigned char) in the initial N characters (each interpreted as
    unsigned char) of the object pointed to by S.

    @param[in]    S   Pointer to the object to be searched.
    @param[in]    C   The character value to search for.
    @param[in]    N   Max Number of characters (bytes) to be searched.

    @return   The memchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the object.
**/
void     *memchr(const void *S, int C, size_t N);

/** The strchr function locates the first occurrence of C (converted to a char)
    in the string pointed to by S. The terminating null character is considered
    to be part of the string.

    @param[in]    S   Pointer to the object to be searched.
    @param[in]    C   The character value to search for.

    @return   The strchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the string.
**/
char     *strchr(const char *S, int C);

/** The strcspn function computes the length of the maximum initial segment of
    the string pointed to by S1 which consists entirely of characters NOT from
    the string pointed to by S2.

    @param[in]    S1  Pointer to the object to be searched.
    @param[in]    S2  Pointer to the list of characters to search for.

    @return   The strcspn function returns the length of the segment.
**/
size_t    strcspn(const char *S1, const char *S2);

/** The strpbrk function locates the first occurrence in the string pointed to
    by S1 of any character from the string pointed to by S2.

    @param[in]    S1  Pointer to the object to be searched.
    @param[in]    S2  Pointer to the list of characters to search for.

    @return   The strpbrk function returns a pointer to the character, or a
              null pointer if no character from S2 occurs in S1.
**/
char     *strpbrk(const char *S1, const char *S2);

/** The strrchr function locates the last occurrence of C (converted to a char)
    in the string pointed to by S. The terminating null character is considered
    to be part of the string.

    @param[in]    S   Pointer to the object to be searched.
    @param[in]    C   The character value to search for.

    @return   The strrchr function returns a pointer to the character, or a
              null pointer if C does not occur in the string.
**/
char     *strrchr(const char *S, int C);

/** The strspn function computes the length of the maximum initial segment of
    the string pointed to by S1 which consists entirely of characters from the
    string pointed to by S2.

    @param[in]    S1  Pointer to the object to be searched.
    @param[in]    S2  Pointer to the list of characters to search for.

    @return   The strspn function returns the length of the segment.
**/
size_t    strspn(const char *S1 , const char *S2);

/** The strstr function locates the first occurrence in the string pointed to
    by S1 of the sequence of characters (excluding the terminating null
    character) in the string pointed to by S2.

    @param[in]    S1  Pointer to the object to be searched.
    @param[in]    S2  Pointer to the sequence of characters to search for.

    @return   The strstr function returns a pointer to the located string, or a
              null pointer if the string is not found. If S2 points to a string
              with zero length, the function returns S1.
**/
char     *strstr(const char *S1 , const char *S2);

/** Break a string into a sequence of tokens.

    A sequence of calls to the strtok function breaks the string pointed to by
    S1 into a sequence of tokens, each of which is delimited by a character
    from the string pointed to by S2. The first call in the sequence has a
    non-null first argument; subsequent calls in the sequence have a null first
    argument. The separator string pointed to by S2 may be different from call
    to call.

    The first call in the sequence searches the string pointed to by S1 for the
    first character that is not contained in the current separator string
    pointed to by S2. If no such character is found, then there are no tokens
    in the string pointed to by S1 and the strtok function returns a null
    pointer. If such a character is found, it is the start of the first token.

    The strtok function then searches from there for a character that is
    contained in the current separator string. If no such character is found,
    the current token extends to the end of the string pointed to by S1, and
    subsequent searches for a token will return a null pointer. If such a
    character is found, it is overwritten by a null character, which terminates
    the current token. The strtok function saves a pointer to the following
    character, from which the next search for a token will start.

    Each subsequent call, with a null pointer as the value of the first
    argument, starts searching from the saved pointer and behaves as
    described above.

    @param[in]    S1  Pointer to the string to be tokenized.
    @param[in]    S2  Pointer to a list of separator characters.

    @return   The strtok function returns a pointer to the first character of a
              token, or a null pointer if there is no token.
**/
char     *strtok(char * __restrict S1, const char * __restrict S2);

/* ################   Miscellaneous Functions   ########################### */

/** The memset function copies the value of C (converted to an unsigned char)
    into each of the first N characters of the object pointed to by S.

    @param[out]   S   Pointer to the first element of the object to be set.
    @param[in]    C   Value to store in each element of S.
    @param[in]    N   Number of elements in S to be set.

    @return   The memset function returns the value of S.
**/
void     *memset(void *S, int C, size_t N);

/** The strerror function maps the number in Num to a message string.
    Typically, the values for Num come from errno, but strerror shall map
    any value of type int to a message.

    @param[in]  Num   A value to be converted to a message.

    @return   The strerror function returns a pointer to the string, the
              contents of which are locale specific.  The array pointed to
              must not be modified by the program, but may be overwritten by
              a subsequent call to the strerror function.
**/
char     *strerror(int Num);

/** The strlen function computes the length of the string pointed to by S.

    @param[in]  S   Pointer to the string to determine the length of.

    @return   The strlen function returns the number of characters that
              precede the terminating null character.
**/
size_t    strlen(const char *S);


/* ################   BSD Compatibility Functions   ####################### */

char   *strdup    (const char *);
int     strerror_r(int, char *, size_t);
int     strcasecmp(const char *s1, const char *s2);
void   *memccpy   (void *, const void *, int, size_t);
int     strncasecmp(const char *s1, const char *s2, size_t n);
size_t  strlcpy(char *destination, const char *source, size_t size);
size_t  strlcat(char *destination, const char *source, size_t size);

// bcopy is is a void function with the src/dest arguments reversed, being used in socket lib
#define bcopy(a,b,c) ( memcpy((void *)b, (const void *)a, (size_t)c))

// bcmp is same as memcmp, returns 0 for successful compare, non-zero otherwise
#define bcmp(a,b,c) ( memcmp((void *)a, (void *)b, (size_t)c))

/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char *
strsep(
  register char **stringp,
  register const char *delim
  );

__END_DECLS

#endif  /* _STRING_H */
