/** @file
  C Run-Time Libraries (CRT) Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>

int errno = 0;

FILE  *stderr = NULL;
FILE  *stdin  = NULL;
FILE  *stdout = NULL;

typedef
INTN
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
  VOID        *Pivot;
  UINTN       LoopCount;
  UINTN       NextSwapLocation;

  ASSERT(BufferToSort    != NULL);
  ASSERT(CompareFunction != NULL);
  ASSERT(Buffer          != NULL);

  if (Count < 2 || ElementSize  < 1) {
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
  for (LoopCount = 0; LoopCount < Count - 1;  LoopCount++)
  {
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
  // Swap pivot to it's final position (NextSwapLocaiton)
  //
  CopyMem (Buffer, Pivot, ElementSize);
  CopyMem (Pivot, (UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), ElementSize);
  CopyMem ((UINT8 *)BufferToSort + (NextSwapLocation * ElementSize), Buffer, ElementSize);

  //
  // Now recurse on 2 paritial lists.  Neither of these will have the 'pivot' element.
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

//---------------------------------------------------------
// Standard C Run-time Library Interface Wrapper
//---------------------------------------------------------

//
// -- String Manipulation Routines --
//

/* Scan a string for the last occurrence of a character */
char *strrchr (const char *str, int c)
{
  char * save;

  for (save = NULL; ; ++str) {
    if (*str == c) {
      save = (char *)str;
    }
    if (*str == 0) {
      return (save);
    }
  }
}

/* Read formatted data from a string */
int sscanf (const char *buffer, const char *format, ...)
{
  //
  // Null sscanf() function implementation to satisfy the linker, since
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

//
// -- Character Classification Routines --
//

/* Determines if a particular character is a decimal-digit character */
int isdigit (int c)
{
  //
  // <digit> ::= [0-9]
  //
  return (('0' <= (c)) && ((c) <= '9'));
}

/* Determine if an integer represents character that is a hex digit */
int isxdigit (int c)
{
  //
  // <hexdigit> ::= [0-9] | [a-f] | [A-F]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (('a' <= (c)) && ((c) <= 'f')) ||
          (('A' <= (c)) && ((c) <= 'F')));
}

/* Determines if a particular character represents a space character */
int isspace (int c)
{
  //
  // <space> ::= [ ]
  //
  return ((c) == ' ');
}

/* Determine if a particular character is an alphanumeric character */
int isalnum (int c)
{
  //
  // <alnum> ::= [0-9] | [a-z] | [A-Z]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (('a' <= (c)) && ((c) <= 'z')) ||
          (('A' <= (c)) && ((c) <= 'Z')));
}

/* Determines if a particular character is in upper case */
int isupper (int c)
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
long strtol (const char *nptr, char **endptr, int base)
{
  //
  // Null strtol() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Convert strings to an unsigned long-integer value */
unsigned long strtoul (const char *nptr, char **endptr, int base)
{
  //
  // Null strtoul() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Convert character to lowercase */
int tolower (int c)
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
void qsort (void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
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
char *getenv (const char *varname)
{
  //
  // Null getenv() function implementation to satisfy the linker, since there is
  // no direct functionality logic dependency in present UEFI cases.
  //
  return NULL;
}

//
// -- Stream I/O Routines --
//

/* Write formatted output using a pointer to a list of arguments */
int vfprintf (FILE *stream, const char *format, VA_LIST arg)
{
  return 0;
}

/* Write data to a stream */
size_t fwrite (const void *buffer, size_t size, size_t count, FILE *stream)
{
  return 0;
}

//
//  -- Dummy OpenSSL Support Routines --
//

int BIO_printf (void *bio, const char *format, ...)
{
  return 0;
}

int BIO_snprintf(char *buf, size_t n, const char *format, ...)
{
  return 0;
}

void *UI_OpenSSL(void)
{
  return NULL;
}

int X509_load_cert_file (VOID *ctx, const char *file, int type)
{
  return 0;
}

int X509_load_crl_file (VOID *ctx, const char *file, int type)
{
  return 0;
}

int chmod (const char *c, mode_t m)
{
  return -1;
}

int close (int f)
{
  return -1;
}

void closelog (void)
{

}

#ifdef __GNUC__

typedef
VOID
(EFIAPI *NoReturnFuncPtr)(
  VOID
  ) __attribute__((__noreturn__));


STATIC
VOID
EFIAPI
NopFunction (
  VOID
  )
{
}


void exit (int e)
{
  NoReturnFuncPtr NoReturnFunc;

  NoReturnFunc = (NoReturnFuncPtr) NopFunction;

  NoReturnFunc ();
}

#else

void exit (int e)
{
}

#endif

int fclose (FILE *f)
{
  return 0;
}

FILE *fopen (const char *c, const char *m)
{
  return NULL;
}

size_t fread (void *b, size_t c, size_t i, FILE *f)
{
  return 0;
}

int fprintf (FILE *f, const char *s, ...)
{
  return 0;
}

uid_t getuid (void)
{
  return 0;
}

uid_t geteuid (void)
{
  return 0;
}

gid_t getgid (void)
{
  return 0;
}

gid_t getegid (void)
{
  return 0;
}

off_t lseek (int a, off_t o, int d)
{
  return 0;
}

void openlog (const char *c, int a, int b)
{

}

ssize_t read (int f, void *b, size_t c)
{
  return 0;
}

int stat (const char *c, struct stat *s)
{
  return -1;
}

int strcasecmp (const char *c, const char *s)
{
  return 0;
}

int strncasecmp (const char *c, const char *s, size_t l)
{
  return 0;
}

void syslog (int a, const char *c, ...)
{

}

ssize_t write (int f, const void *b, size_t l)
{
  return 0;
}
