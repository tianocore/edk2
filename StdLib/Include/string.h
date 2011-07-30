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
  arguments on such a call shall still have valid values.

  For all functions declared in this header, each character shall be
  interpreted as if it had the type unsigned char (and therefore every possible
  object representation is valid and has a different value).

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

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

/** The memcpy function copies n characters from the object pointed to by s2
    into the object pointed to by s1. If copying takes place between objects
    that overlap, the behavior is undefined.

    @return   The memcpy function returns the value of s1.
**/
void     *memcpy(void * __restrict s1, const void * __restrict s2, size_t n);

/** The memmove function copies n characters from the object pointed to by s2
    into the object pointed to by s1. Copying takes place as if the n
    characters from the object pointed to by s2 are first copied into a
    temporary array of n characters that does not overlap the objects pointed
    to by s1 and s2, and then the n characters from the temporary array are
    copied into the object pointed to by s1.

    @return   The memmove function returns the value of s1.
**/
void     *memmove(void *s1, const void *s2, size_t n);

/** The strcpy function copies the string pointed to by s2 (including the
    terminating null character) into the array pointed to by s1. If copying
    takes place between objects that overlap, the behavior is undefined.

    @return   The strcpy function returns the value of s1.
**/
char     *strcpy(char * __restrict s1, const char * __restrict s2);

/** The strncpy function copies not more than n characters (characters that
    follow a null character are not copied) from the array pointed to by s2 to
    the array pointed to by s1. If copying takes place between objects that
    overlap, the behavior is undefined.

    If the array pointed to by s2 is a string that is shorter than n
    characters, null characters are appended to the copy in the array pointed
    to by s1, until n characters in all have been written.

    @return   The strncpy function returns the value of s1.
**/
char     *strncpy(char * __restrict s1, const char * __restrict s2, size_t n);

/** The strncpyX function copies not more than n-1 characters (characters that
    follow a null character are not copied) from the array pointed to by s2 to
    the array pointed to by s1. Array s1 is guaranteed to be NULL terminated.
    If copying takes place between objects that overlap,
    the behavior is undefined.

    strncpyX exists because normal strncpy does not indicate if the copy was
    terminated because of exhausting the buffer or reaching the end of s2.

    @return   The strncpyX function returns 0 if the copy operation was
              terminated because it reached the end of s1.  Otherwise,
              a non-zero value is returned indicating how many characters
              remain in s1.
**/
int       strncpyX(char * __restrict s1, const char * __restrict s2, size_t n);

/* ################   Concatenation Functions   ########################### */

/** The strcat function appends a copy of the string pointed to by s2
    (including the terminating null character) to the end of the string pointed
    to by s1. The initial character of s2 overwrites the null character at the
    end of s1. If copying takes place between objects that overlap, the
    behavior is undefined.

    @return   The strcat function returns the value of s1.
**/
char     *strcat(char * __restrict s1, const char * __restrict s2);

/** The strncat function appends not more than n characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by s2 to the end of the string pointed to by s1. The initial character of
    s2 overwrites the null character at the end of s1. A terminating null
    character is always appended to the result. If copying takes place
    between objects that overlap, the behavior is undefined.

    @return   The strncat function returns the value of s1.
**/
char     *strncat(char * __restrict s1, const char * __restrict s2, size_t n);

/** The strncatX function appends not more than n characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by s2 to the end of the string pointed to by s1. The initial character of
    s2 overwrites the null character at the end of s1. The result is always
    terminated with a null character. If copying takes place between objects
    that overlap, the behavior is undefined.

    strncatX exists because normal strncat does not indicate if the operation
    was terminated because of exhausting n or reaching the end of s2.

    @return   The strncatX function returns 0 if the operation was terminated
              because it reached the end of s1.  Otherwise, a non-zero value is
              returned indicating how many characters remain in s1.
**/
int       strncatX(char * __restrict s1, const char * __restrict s2, size_t n);

/* ################   Comparison Functions   ############################## */

/** The memcmp function compares the first n characters of the object pointed
    to by s1 to the first n characters of the object pointed to by s2.

    @return   The memcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the object pointed to by s1 is
              greater than, equal to, or less than the object pointed to by s2.
**/
int       memcmp(const void *s1, const void *s2, size_t n);

/** The strcmp function compares the string pointed to by s1 to the string
    pointed to by s2.

    @return   The strcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the string pointed to by s1 is
              greater than, equal to, or less than the string pointed to by s2.
**/
int       strcmp(const char *s1, const char *s2);

/** The strcoll function compares the string pointed to by s1 to the string
    pointed to by s2, both interpreted as appropriate to the LC_COLLATE
    category of the current locale.

    @return   The strcoll function returns an integer greater than, equal to,
              or less than zero, accordingly as the string pointed to by s1 is
              greater than, equal to, or less than the string pointed to by s2
              when both are interpreted as appropriate to the current locale.
**/
int       strcoll(const char *s1, const char *s2);

/** The strncmp function compares not more than n characters (characters that
    follow a null character are not compared) from the array pointed to by s1
    to the array pointed to by s2.

    @return   The strncmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the possibly null-terminated
              array pointed to by s1 is greater than, equal to, or less than
              the possibly null-terminated array pointed to by s2.
**/
int       strncmp(const char *s1, const char *s2, size_t n);

/** The strxfrm function transforms the string pointed to by s2 and places the
    resulting string into the array pointed to by s1. The transformation is
    such that if the strcmp function is applied to two transformed strings, it
    returns a value greater than, equal to, or less than zero, corresponding to
    the result of the strcoll function applied to the same two original
    strings. No more than n characters are placed into the resulting array
    pointed to by s1, including the terminating null character. If n is zero,
    s1 is permitted to be a null pointer. If copying takes place between
    objects that overlap, the behavior is undefined.

    @return   The strxfrm function returns the length of the transformed string
              (not including the terminating null character). If the value
              returned is n or more, the contents of the array pointed to by s1
              are indeterminate.
**/
size_t    strxfrm(char * __restrict s1, const char * __restrict s2, size_t n);

/* ################   Search Functions   ################################## */

/** The memchr function locates the first occurrence of c (converted to an
    unsigned char) in the initial n characters (each interpreted as
    unsigned char) of the object pointed to by s.

    @return   The memchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the object.
**/
void     *memchr(const void *s, int c, size_t n);

/** The strchr function locates the first occurrence of c (converted to a char)
    in the string pointed to by s. The terminating null character is considered
    to be part of the string.

    @return   The strchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the string.
**/
char     *strchr(const char *s, int c);

/** The strcspn function computes the length of the maximum initial segment of
    the string pointed to by s1 which consists entirely of characters NOT from
    the string pointed to by s2.

    @return   The strcspn function returns the length of the segment.
**/
size_t    strcspn(const char *s1, const char *s2);

/** The strpbrk function locates the first occurrence in the string pointed to
    by s1 of any character from the string pointed to by s2.

    @return   The strpbrk function returns a pointer to the character, or a
              null pointer if no character from s2 occurs in s1.
**/
char     *strpbrk(const char *s1, const char *s2);

/** The strrchr function locates the last occurrence of c (converted to a char)
    in the string pointed to by s. The terminating null character is considered
    to be part of the string.

    @return   The strrchr function returns a pointer to the character, or a
              null pointer if c does not occur in the string.
**/
char     *strrchr(const char *s, int c);

/** The strspn function computes the length of the maximum initial segment of
    the string pointed to by s1 which consists entirely of characters from the
    string pointed to by s2.

    @return   The strspn function returns the length of the segment.
**/
size_t    strspn(const char *s1 , const char *s2);

/** The strstr function locates the first occurrence in the string pointed to
    by s1 of the sequence of characters (excluding the terminating null
    character) in the string pointed to by s2.

    @return   The strstr function returns a pointer to the located string, or a
              null pointer if the string is not found. If s2 points to a string
              with zero length, the function returns s1.
**/
char     *strstr(const char *s1 , const char *s2);

/** A sequence of calls to the strtok function breaks the string pointed to by
    s1 into a sequence of tokens, each of which is delimited by a character
    from the string pointed to by s2. The first call in the sequence has a
    non-null first argument; subsequent calls in the sequence have a null first
    argument. The separator string pointed to by s2 may be different from call
    to call.

    The first call in the sequence searches the string pointed to by s1 for the
    first character that is not contained in the current separator string
    pointed to by s2. If no such character is found, then there are no tokens
    in the string pointed to by s1 and the strtok function returns a null
    pointer. If such a character is found, it is the start of the first token.

    The strtok function then searches from there for a character that is
    contained in the current separator string. If no such character is found,
    the current token extends to the end of the string pointed to by s1, and
    subsequent searches for a token will return a null pointer. If such a
    character is found, it is overwritten by a null character, which terminates
    the current token. The strtok function saves a pointer to the following
    character, from which the next search for a token will start.

    Each subsequent call, with a null pointer as the value of the first
    argument, starts searching from the saved pointer and behaves as
    described above.

    @return   The strtok function returns a pointer to the first character of a
              token, or a null pointer if there is no token.
**/
char     *strtok(char * __restrict s1, const char * __restrict s2);

/* ################   Miscellaneous Functions   ########################### */

/** The memset function copies the value of c (converted to an unsigned char)
    into each of the first n characters of the object pointed to by s.

    @return   The memset function returns the value of s.
**/
void     *memset(void *s, int c, size_t n);

/** The strerror function maps the number in errnum to a message string.
    Typically, the values for errnum come from errno, but strerror shall map
    any value of type int to a message.

    The implementation shall behave as if no library function calls the
    strerror function.

    @return   The strerror function returns a pointer to the string, the
              contents of which are locale specific.  The array pointed to
              shall not be modified by the program, but may be overwritten by
              a subsequent call to the strerror function.
**/
char     *strerror(int num);

/** The strlen function computes the length of the string pointed to by s.

    @return   The strlen function returns the number of characters that
              precede the terminating null character.
**/
size_t    strlen(const char *);


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
