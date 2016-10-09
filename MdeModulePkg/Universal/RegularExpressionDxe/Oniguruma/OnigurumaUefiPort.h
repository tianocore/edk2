/** @file
  
  Module to rewrite stdlib references within Oniguruma

  (C) Copyright 2014-2015 Hewlett Packard Enterprise Development LP<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef ONIGURUMA_UEFI_PORT_H
#define ONIGURUMA_UEFI_PORT_H

#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#undef _WIN32
#define P_(args) args

#define SIZEOF_LONG sizeof(long)
#define SIZEOF_INT  sizeof(int)
typedef UINTN size_t;

#define malloc(n) AllocatePool(n)
#define calloc(n,s) AllocateZeroPool((n)*(s))

#define free(p)             \
  do {                      \
    VOID *EvalOnce;         \
                            \
    EvalOnce = (p);         \
    if (EvalOnce != NULL) { \
      FreePool (EvalOnce);  \
    }                       \
  } while (FALSE)

#define realloc(OldPtr,NewSize,OldSize) ReallocatePool(OldSize,NewSize,OldPtr)
#define xmemmove(Dest,Src,Length) CopyMem(Dest,Src,Length)
#define xmemcpy(Dest,Src,Length) CopyMem(Dest,Src,Length)
#define xmemset(Buffer,Value,Length) SetMem(Buffer,Length,Value)

#define va_init_list(a,b) VA_START(a,b)
#define va_list VA_LIST
#define va_arg(a,b) VA_ARG(a,b)
#define va_end(a) VA_END(a)

#define FILE VOID
#define stdout NULL
#define fprintf(...)
#define fputs(a,b)
#define vsnprintf (int)AsciiVSPrint
#define _vsnprintf vsnprintf

#define setlocale(a,b)
#define LC_ALL 0

#define MAX_STRING_SIZE 0x1000
#define strlen_s(String,MaxSize)            AsciiStrnLenS (String, MaxSize)
#define strcat_s(Dest,MaxSize,Src)          AsciiStrCatS (Dest, MaxSize, Src)
#define strncpy_s(Dest,MaxSize,Src,Length)  AsciiStrnCpyS (Dest, MaxSize, Src, Length)
#define strcmp                              OnigStrCmp

int OnigStrCmp (char* Str1, char* Str2);

int EFIAPI sprintf_s (char *str, size_t sizeOfBuffer, char const *fmt, ...);

#define exit(n) ASSERT(FALSE);

#endif // !ONIGURUMA_UEFI_PORT_H
