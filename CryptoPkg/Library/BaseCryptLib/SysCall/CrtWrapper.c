/** @file
  C Run-Time Libraries (CRT) Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>

int  errno = 0;

FILE  *stderr = NULL;
FILE  *stdin  = NULL;
FILE  *stdout = NULL;

typedef
int
(*SORT_COMPARE)(
  IN  VOID  *Buffer1,
  IN  VOID  *Buffer2
  );

//
// Duplicated from EDKII BaseSortLib for qsort() wrapper
//
STATIC
VOID
QuickSortWorker (
  IN OUT    VOID          *BufferToSort,
  IN CONST  UINTN         Count,
  IN CONST  UINTN         ElementSize,
  IN        SORT_COMPARE  CompareFunction,
  IN        VOID          *Buffer
  )
{
  VOID   *Pivot;
  UINTN  LoopCount;
  UINTN  NextSwapLocation;

  ASSERT (BufferToSort    != NULL);
  ASSERT (CompareFunction != NULL);
  ASSERT (Buffer          != NULL);

  if ((Count < 2) || (ElementSize  < 1)) {
    return;
  }

  NextSwapLocation = 0;

  //
  // Pick a pivot (we choose last element)
  //
  Pivot = ((UINT8 *)BufferToSort + ((Count - 1) * ElementSize));

  //
  // Now get the pivot such that all on "left" are below it
  // and everything "right" are above it
  //
  for (LoopCount = 0; LoopCount < Count - 1; LoopCount++) {
    //
    // If the element is less than the pivot
    //
    if (CompareFunction ((VOID *)((UINT8 *)BufferToSort + ((LoopCount) * ElementSize)), Pivot) <= 0) {
      //
      // Swap
      //
      CopyMem (Buffer, (UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), ElementSize);
      CopyMem ((UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), (UINT8 *)BufferToSort + ((LoopCount) * ElementSize), ElementSize);
      CopyMem ((UINT8 *)BufferToSort + ((LoopCount) * ElementSize), Buffer, ElementSize);

      //
      // Increment NextSwapLocation
      //
      NextSwapLocation++;
    }
  }

  //
  // Swap pivot to its final position (NextSwapLocation)
  //
  CopyMem (Buffer, Pivot, ElementSize);
  CopyMem (Pivot, (UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), ElementSize);
  CopyMem ((UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), Buffer, ElementSize);

  //
  // Now recurse on 2 partial lists.  Neither of these will have the 'pivot' element.
  // IE list is sorted left half, pivot element, sorted right half...
  //
  QuickSortWorker (
    BufferToSort,
    NextSwapLocation,
    ElementSize,
    CompareFunction,
    Buffer
    );

  QuickSortWorker (
    (UINT8 *)BufferToSort + (NextSwapLocation + 1) * ElementSize,
    Count - NextSwapLocation - 1,
    ElementSize,
    CompareFunction,
    Buffer
    );

  return;
}

// ---------------------------------------------------------
// Standard C Run-time Library Interface Wrapper
// ---------------------------------------------------------

//
// -- String Manipulation Routines --
//

char *
strchr (
  const char  *str,
  int         ch
  )
{
  return ScanMem8 (str, AsciiStrSize (str), (UINT8)ch);
}

/* Scan a string for the last occurrence of a character */
char *
strrchr (
  const char  *str,
  int         c
  )
{
  char  *save;

  for (save = NULL; ; ++str) {
    if (*str == c) {
      save = (char *)str;
    }

    if (*str == 0) {
      return (save);
    }
  }
}

/* Compare first n bytes of string s1 with string s2, ignoring case */
int
strncasecmp (
  const char  *s1,
  const char  *s2,
  size_t      n
  )
{
  int  Val;

  ASSERT (s1 != NULL);
  ASSERT (s2 != NULL);

  if (n != 0) {
    do {
      Val = tolower (*s1) - tolower (*s2);
      if (Val != 0) {
        return Val;
      }

      ++s1;
      ++s2;
      if (*s1 == '\0') {
        break;
      }
    } while (--n != 0);
  }

  return 0;
}

/* Read formatted data from a string */
int
sscanf (
  const char  *buffer,
  const char  *format,
  ...
  )
{
  //
  // Null sscanf() function implementation to satisfy the linker, since
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Maps errnum to an error-message string */
char *
strerror (
  int  errnum
  )
{
  return NULL;
}

/* Computes the length of the maximum initial segment of the string pointed to by s1
   which consists entirely of characters from the string pointed to by s2. */
size_t
strspn (
  const char  *s1,
  const char  *s2
  )
{
  UINT8   Map[32];
  UINT32  Index;
  size_t  Count;

  for (Index = 0; Index < 32; Index++) {
    Map[Index] = 0;
  }

  while (*s2) {
    Map[*s2 >> 3] |= (1 << (*s2 & 7));
    s2++;
  }

  if (*s1) {
    Count = 0;
    while (Map[*s1 >> 3] & (1 << (*s1 & 7))) {
      Count++;
      s1++;
    }

    return Count;
  }

  return 0;
}

/* Computes the length of the maximum initial segment of the string pointed to by s1
   which consists entirely of characters not from the string pointed to by s2. */
size_t
strcspn (
  const char  *s1,
  const char  *s2
  )
{
  UINT8   Map[32];
  UINT32  Index;
  size_t  Count;

  for (Index = 0; Index < 32; Index++) {
    Map[Index] = 0;
  }

  while (*s2) {
    Map[*s2 >> 3] |= (1 << (*s2 & 7));
    s2++;
  }

  Map[0] |= 1;

  Count = 0;
  while (!(Map[*s1 >> 3] & (1 << (*s1 & 7)))) {
    Count++;
    s1++;
  }

  return Count;
}

char *
strcpy (
  char        *strDest,
  const char  *strSource
  )
{
  AsciiStrCpyS (strDest, AsciiStrnSizeS (strSource, MAX_STRING_SIZE - 1), strSource);
  return strDest;
}

char *
strncpy (
  char        *strDest,
  const char  *strSource,
  size_t      count
  )
{
  UINTN  DestMax = MAX_STRING_SIZE;

  if (count < MAX_STRING_SIZE) {
    DestMax = count + 1;
  } else {
    count = MAX_STRING_SIZE-1;
  }

  AsciiStrnCpyS (strDest, DestMax, strSource, (UINTN)count);

  return strDest;
}

char *
strcat (
  char        *strDest,
  const char  *strSource
  )
{
  UINTN  DestMax;

  DestMax = AsciiStrnLenS (strDest, MAX_STRING_SIZE) + AsciiStrnSizeS (strSource, MAX_STRING_SIZE);

  if (DestMax > MAX_STRING_SIZE) {
    DestMax = MAX_STRING_SIZE;
  }

  AsciiStrCatS (strDest, DestMax, strSource);

  return strDest;
}

int
strcmp (
  const char  *s1,
  const char  *s2
  )
{
  return (int)AsciiStrCmp (s1, s2);
}

//
// -- Character Classification Routines --
//

/* Determines if a particular character is a decimal-digit character */
int
isdigit (
  int  c
  )
{
  //
  // <digit> ::= [0-9]
  //
  return (('0' <= (c)) && ((c) <= '9'));
}

/* Determine if an integer represents character that is a hex digit */
int
isxdigit (
  int  c
  )
{
  //
  // <hexdigit> ::= [0-9] | [a-f] | [A-F]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (('a' <= (c)) && ((c) <= 'f')) ||
          (('A' <= (c)) && ((c) <= 'F')));
}

/* Determines if a particular character represents a space character */
int
isspace (
  int  c
  )
{
  //
  // <space> ::= [ ]
  //
  return ((c) == ' ');
}

/* Determine if a particular character is an alphanumeric character */
int
isalnum (
  int  c
  )
{
  //
  // <alnum> ::= [0-9] | [a-z] | [A-Z]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (('a' <= (c)) && ((c) <= 'z')) ||
          (('A' <= (c)) && ((c) <= 'Z')));
}

/* Determines if a particular character is in upper case */
int
isupper (
  int  c
  )
{
  //
  // <uppercase letter> := [A-Z]
  //
  return (('A' <= (c)) && ((c) <= 'Z'));
}

//
// -- Data Conversion Routines --
//

/* Convert strings to a long-integer value */
long
strtol (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  //
  // Null strtol() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Convert strings to an unsigned long-integer value */
unsigned long
strtoul (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  //
  // Null strtoul() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Convert character to lowercase */
int
tolower (
  int  c
  )
{
  if (('A' <= (c)) && ((c) <= 'Z')) {
    return (c - ('A' - 'a'));
  }

  return (c);
}

//
// -- Searching and Sorting Routines --
//

/* Performs a quick sort */
void
qsort (
  void *base,
  size_t num,
  size_t width,
  int ( *compare )(const void *, const void *)
  )
{
  VOID  *Buffer;

  ASSERT (base    != NULL);
  ASSERT (compare != NULL);

  //
  // Use CRT-style malloc to cover BS and RT memory allocation.
  //
  Buffer = malloc (width);
  ASSERT (Buffer != NULL);

  //
  // Re-use PerformQuickSort() function Implementation in EDKII BaseSortLib.
  //
  QuickSortWorker (base, (UINTN)num, (UINTN)width, (SORT_COMPARE)compare, Buffer);

  free (Buffer);
  return;
}

//
// -- Process and Environment Control Routines --
//

/* Get a value from the current environment */
char *
getenv (
  const char  *varname
  )
{
  //
  // Null getenv() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return NULL;
}

/* Get a value from the current environment */
char *
secure_getenv (
  const char  *varname
  )
{
  //
  // Null secure_getenv() function implementation to satisfy the linker, since
  // there is no direct functionality logic dependency in present UEFI cases.
  //
  // From the secure_getenv() manual: 'just like getenv() except that it
  // returns NULL in cases where "secure execution" is required'.
  //
  return NULL;
}

//
// -- Stream I/O Routines --
//

/* Write data to a stream */
size_t
fwrite (
  const void  *buffer,
  size_t      size,
  size_t      count,
  FILE        *stream
  )
{
  return 0;
}

#ifdef __GNUC__

typedef
VOID
(EFIAPI *NoReturnFuncPtr)(
  VOID
  ) __attribute__ ((__noreturn__));

STATIC
VOID
EFIAPI
NopFunction (
  VOID
  )
{
}

void
abort (
  void
  )
{
  NoReturnFuncPtr  NoReturnFunc;

  NoReturnFunc = (NoReturnFuncPtr)NopFunction;

  NoReturnFunc ();
}

#else

void
abort (
  void
  )
{
  // Do nothing
}

#endif

int
fclose (
  FILE  *f
  )
{
  return 0;
}

FILE *
fopen (
  const char  *c,
  const char  *m
  )
{
  return NULL;
}

size_t
fread (
  void    *b,
  size_t  c,
  size_t  i,
  FILE    *f
  )
{
  return 0;
}

uid_t
getuid (
  void
  )
{
  return 0;
}

uid_t
geteuid (
  void
  )
{
  return 0;
}

gid_t
getgid (
  void
  )
{
  return 0;
}

gid_t
getegid (
  void
  )
{
  return 0;
}

int
printf (
  char const  *fmt,
  ...
  )
{
  return 0;
}
