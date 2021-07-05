/** @file

  Module to rewrite stdlib references within Oniguruma

  (C) Copyright 2014-2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef ONIGURUMA_UEFI_PORT_H
#define ONIGURUMA_UEFI_PORT_H

#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>


#define ONIG_NO_STANDARD_C_HEADERS
#define ONIG_NO_PRINT
#define P_(args) args

#define INT_MAX   0x7FFFFFFF
#define LONG_MAX  0x7FFFFFFF
#define UINT_MAX  0xFFFFFFFF
#define ULONG_MAX 0xFFFFFFFF


#define SIZEOF_LONG      4
#define SIZEOF_LONG_LONG 8
typedef UINTN       size_t;
typedef UINT32      uint32_t;
typedef INTN        intptr_t;

#ifndef offsetof
#define offsetof OFFSET_OF
#endif

#ifdef MDE_CPU_IA32
#define SIZEOF_VOIDP 4
#endif

#ifdef MDE_CPU_X64
#define SIZEOF_VOIDP 8
#endif

#define calloc(n,s) AllocateZeroPool((n)*(s))
#define xmemmove(Dest,Src,Length) CopyMem(Dest,Src,Length)
#define xmemcpy(Dest,Src,Length) CopyMem(Dest,Src,Length)
#define xmemset(Buffer,Value,Length) SetMem(Buffer,Length,Value)

#define va_init_list(a,b) VA_START(a,b)
#define va_list VA_LIST
#define va_arg(a,b) VA_ARG(a,b)
#define va_end(a) VA_END(a)
#define va_start VA_START

#define FILE VOID
#define stdout NULL
#define fprintf(...)
#define fputs(a,b)
#define vsnprintf (int)AsciiVSPrint
#define _vsnprintf vsnprintf
#define xsnprintf sprintf_s
#define xvsnprintf  vsnprintf
#define alloca malloc

#define setlocale(a,b)
#define LC_ALL 0

#define UCHAR_MAX 255
#define MAX_STRING_SIZE 0x1000
#define strlen_s(String,MaxSize)            AsciiStrnLenS (String, MaxSize)
#define xstrncpy(Dest, Src, MaxSize)        strcat_s(Dest,MaxSize,Src)
#define xstrcat(Dest,Src,MaxSize)           strcat(Dest,Src,MaxSize)
#define strcat(Dest,Src,MaxSize)            strcat_s(Dest,MaxSize,Src)
#define strcat_s(Dest,MaxSize,Src)          AsciiStrCatS (Dest, MaxSize, Src)
#define strncpy_s(Dest,MaxSize,Src,Length)  AsciiStrnCpyS (Dest, MaxSize, Src, Length)
#define strcmp                              OnigStrCmp

int OnigStrCmp (const char* Str1, const char* Str2);

int EFIAPI sprintf_s (char *str, size_t sizeOfBuffer, char const *fmt, ...);
int strlen(const char* str);
void* malloc(size_t size);
void* realloc(void *ptr, size_t size);
void* memcpy (void *dest, const void *src, unsigned int count);
void* memset (void *dest, char ch, unsigned int count);
void free(void *ptr);

#define exit(n) ASSERT(FALSE);

#endif // !ONIGURUMA_UEFI_PORT_H
