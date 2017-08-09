//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Unix implementation of RcSim Library.

  Copyright (c) 2016 - 2017, Intel Corporation.  All rights reserved.
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.
**/



#include "UnixSimLib.h"

//
// Typedefs for the functions
//

typedef int (Sim_fopen_s) (FILE** pFile, const char *filename, const char *mode);
typedef int (Sim_freopen_s) (FILE** pFile, const char *path, const char *mode, FILE *stream);
typedef int (Sim_fflush) (FILE *stream);
typedef void (Sim_perror) (const char * _ErrMsg);
typedef int (Sim_printf)(const char *format, ... );
typedef char* (Sim_fgets)( char *str, int n, FILE *stream);
typedef char* (Sim_strtok_s) (char *strToken, const char *strDelimit, char **context);
typedef int (Sim_fseek) (FILE *stream, long offset, int origin);
typedef int (Sim_ferror) (FILE *stream);
typedef char* (Sim_strchr) (const char *str, int c);
typedef int (Sim_vprintf) (const char *format, va_list argptr);
typedef clock_t (Sim_clock) (void);
typedef int (Sim_fclose) (FILE *stream);
typedef void (Sim_exit) (int status);
typedef int (Sim_fprintf) (FILE *stream, const char *format, ...);
typedef size_t (Sim_fread) (void *buffer, size_t size, size_t count, FILE *stream);
typedef unsigned long (Sim_strtoul) (const char *nptr, char **endptr, int base);
typedef void* (Sim_malloc) (size_t size);
typedef int (Sim_vfprintf) (FILE *stream, const char *format, va_list argptr);
typedef FILE* (Sim_freopen) (const char *path, const char *mode, FILE *stream);
typedef char* (Sim_getenv) (const char *varname);
typedef size_t (Sim_fwrite) (const void *buffer, size_t size, size_t count, FILE *stream);
typedef void (Sim_free) (void *memblock);
typedef int (Sim_putchar) (int c);
typedef int (Sim_isspace) (int c);
typedef char *(Sim_strtok) (char *strToken, const char *strDelimit);
typedef long (Sim_ftell) (FILE *stream );
typedef size_t (Sim_fread_s) (void *buffer, size_t bufferSize, size_t elementSize, size_t count, FILE *stream);
typedef int (Sim_puts) (const char * str);
typedef unsigned int (Sim_sleep) (unsigned int seconds);

//
// Global function pointers
//

Sim_fopen_s *g_fopen_s_func = NULL;
Sim_freopen_s *g_freopen_s_func = NULL;
Sim_fflush *g_fflush_func = NULL;
Sim_perror *g_perror_func = NULL;
Sim_printf *g_printf_func = NULL;
Sim_fgets *g_fgets_func = NULL;
Sim_strtok_s *g_strtok_s_func = NULL;
Sim_fseek *g_fseek_func = NULL;
Sim_ferror *g_ferror_func = NULL;
Sim_strchr *g_strchr_func = NULL;
Sim_vprintf *g_vprintf_func = NULL;
Sim_clock *g_clock_func = NULL;
Sim_fclose *g_fclose_func = NULL;
Sim_exit *g_exit_func = NULL;
Sim_fprintf *g_fprintf_func = NULL;
Sim_fread *g_fread_func = NULL;
Sim_strtoul *g_strtoul_func = NULL;
Sim_malloc *g_malloc_func = NULL;
Sim_vfprintf *g_vfprintf_func = NULL;
Sim_freopen *g_freopen_func = NULL;
Sim_getenv *g_getenv_func = NULL;
Sim_fwrite *g_fwrite_func = NULL;
Sim_free *g_free_func = NULL;
Sim_putchar *g_putchar_func = NULL;
Sim_isspace *g_isspace_func = NULL;
Sim_strtok *g_strtok_func = NULL;
Sim_ftell *g_ftell_func = NULL;
Sim_fread_s *g_fread_s_func = NULL;
Sim_puts *g_puts_func = NULL;
Sim_sleep *g_sleep_func = NULL;

//
// Implementation of fopen_s
//

int
fopen_s (
  FILE** pFile,
  const char *filename,
  const char *mode
  )
{
  int result;

  result = (*g_fopen_s_func)(pFile, filename, mode);

  return result;

} // fopen_s

int
freopen_s (
  FILE** pFile,
  const char *path,
  const char *mode,
  FILE *stream
  )
{
  int result;

  result = (*g_freopen_s_func)(pFile, path, mode, stream);

  return result;

} // freopen_s

int
fflush (
  FILE *stream
  )
{

  // Temporarily turned off

  return 0;

} // fflush

void
perror (
  const char * _ErrMsg
  )
{

  (*g_perror_func)(_ErrMsg);

  return;

} // perror

int
printf (
  const char *format, ...
  )
{
  int result;
  va_list List;

  va_start (List, format);

  //
  // We call the runtime's vprintf function
  // instead of printf to properly handle
  // variable numbers of arguments.
  //

  result = (*g_vprintf_func) (format, List);

  va_end(List);

  return result;

} // printf

char*
fgets (
  char *str,
  int n,
  FILE *stream
  )
{
  char *result;

  result = (*g_fgets_func) (str, n, stream);

  return result;

} // fgets

char*
strtok_s (
  char *strToken,
  const char *strDelimit,
  char **context
  )
{
  char *result;

  result = (*g_strtok_s_func) (strToken, strDelimit, context);

  return result;

} // strtok_s

int
fseek (
  FILE *stream,
  long offset,
  int origin
  )
{
  int result;

  result = (*g_fseek_func) (stream, offset, origin);

  return result;

} // fseek

int
ferror (
  FILE *stream
  )
{
  int result;

  result = (*g_ferror_func) (stream);

  return result;

} // ferror

char*
strchr (
  const char *str,
  int c
  )
{
  char *result;

  result = (*g_strchr_func) (str, c);

  return result;

} // strchr

int
vprintf (
  const char *format,
  va_list argptr
  )
{
  int result;

  result = (*g_vprintf_func) (format, argptr);

  return result;

} // vprintf

clock_t
clock (
  void
  )
{
  clock_t result;

  result = (*g_clock_func) ();

  return result;

} // clock

int
fclose (
  FILE *stream
  )
{
  int result;

  result = (*g_fclose_func) (stream);

  return result;

} // fclose

void
exit (
  int status
  )
{

  (*g_exit_func) (status);

} // fclose

int
fprintf (
  FILE *stream,
  const char *format,
  ...
  )
{
  int result;
  va_list List;

  va_start (List, format);

  //
  // use vprintf to handle variable args
  //

  result = vfprintf (stream, format, List);

  va_end (List);

  return result;

} // fprintf

size_t
fread (
  void *buffer,
  size_t size,
  size_t count,
  FILE *stream
  )
{
  size_t result;

  result = (*g_fread_func) (buffer, size, count, stream);

  return result;

} // fread

unsigned long
strtoul (
  const char *nptr,
  char **endptr,
  int base
  )
{
  unsigned long result;

  result = (*g_strtoul_func) (nptr, endptr, base);

  return result;

} // strtoul

void*
malloc (
  size_t size
  )
{
  void *result;

  result = (*g_malloc_func) (size);

  return result;

} // malloc

int
vfprintf (
  FILE *stream,
  const char *format,
  va_list argptr
  )
{
  int result;

  result = (*g_vfprintf_func) (stream, format, argptr);

  return result;

} // vfprintf

FILE*
freopen (
  const char *path,
  const char *mode,
  FILE *stream
  )
{
  FILE *result;

  result = (*g_freopen_func) (path, mode, stream);

  return result;

} // freopen

char*
getenv (
  const char *varname
  )
{
  char *result;

  result = (*g_getenv_func)(varname);

  return result;

} // getenv


size_t
fwrite (
  const void *buffer,
  size_t size,
  size_t count,
  FILE *stream
  )
{
  size_t result;

  result = (*g_fwrite_func)(buffer, size, count, stream);

  return result;

} // fwrite

void
free (
  void *memblock
  )
{

  //!!!! temp removed not working
  //(*g_free_func)(memblock);

  return;

} // free

int
putchar (
  int c
  )
{
  int result;

  result = (*g_putchar_func)(c);

  return result;

} // putchar

int
isspace (
  int c
  )
{
  int result;

  result = (*g_isspace_func)(c);

  return result;

} // isspace

char *
strtok (
  char *strToken,
  const char *strDelimit
  )
{
  char *result;

  result = (*g_strtok_func)(strToken, strDelimit);

  return result;

} // strtok

long
ftell (
  FILE *stream
  )
{
  long result;

  result = (*g_ftell_func)(stream);

  return result;

} // ftell

size_t
fread_s (
  void *buffer,
  size_t bufferSize,
  size_t elementSize,
  size_t count,
  FILE *stream
  )
{
  size_t result;

  result = (*g_fread_s_func)(buffer, bufferSize, elementSize, count, stream);

  return result;

} // fread_s

int
puts (
  const char * str
  )
{
  int result;

  result = (*g_puts_func)(str);

  return result;

} // puts

int
sleep (
  unsigned int seconds
  )
{
  int result;

  result = (*g_sleep_func)(seconds);

  return result;

} // sleep

/**
  Initialize the library

  @param    None


  @retval  EFI_SUCCESS    Library initialized successfully.
  @retval  !EFI_SUCCESS   Library failed to initialize successfully.

**/

EFI_STATUS
EFIAPI
InitSimLib (
  VOID
  )
{
  EFI_STATUS                Status;
  EMU_DYNAMIC_LOAD_PROTOCOL *DynamicLoadPpi;
  VOID                      *LibraryHandle = NULL;

  TRACE_ENTER ();

  //
  // Get the dynamic load ppi
  //

  Status = PeiServicesLocatePpi (&gEmuDynamicLoadProtocolGuid,
                                  0,
                                  NULL,
                                  (VOID **)&DynamicLoadPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  LibraryHandle = DynamicLoadPpi->Dlopen ("libc.so.6", 1);
  CHECK_NULL_RETURN_OR_ASSERT (LibraryHandle, EFI_INVALID_PARAMETER, "LibraryHandle is null\n");

  //
  // Set our function pointers
  //

  g_fflush_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fflush");
  CHECK_NULL_RETURN_OR_ASSERT (g_fflush_func, EFI_NOT_FOUND, "fflush == NULL\n");

  g_perror_func = DynamicLoadPpi->Dlsym (LibraryHandle, "perror");
  CHECK_NULL_RETURN_OR_ASSERT (g_perror_func, EFI_NOT_FOUND, "perror == NULL\n");

  g_printf_func = DynamicLoadPpi->Dlsym (LibraryHandle, "printf");
  CHECK_NULL_RETURN_OR_ASSERT (g_printf_func, EFI_NOT_FOUND, "printf == NULL\n");

  g_fgets_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fgets");
  CHECK_NULL_RETURN_OR_ASSERT (g_fgets_func, EFI_NOT_FOUND, "fgets == NULL\n");

  g_fseek_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fseek");
  CHECK_NULL_RETURN_OR_ASSERT (g_fseek_func, EFI_NOT_FOUND, "fseek == NULL\n");

  g_ferror_func = DynamicLoadPpi->Dlsym (LibraryHandle, "ferror");
  CHECK_NULL_RETURN_OR_ASSERT (g_ferror_func, EFI_NOT_FOUND, "ferror == NULL\n");

  g_strchr_func = DynamicLoadPpi->Dlsym (LibraryHandle, "strchr");
  CHECK_NULL_RETURN_OR_ASSERT (g_strchr_func, EFI_NOT_FOUND, "strchr == NULL\n");

  g_vprintf_func = DynamicLoadPpi->Dlsym (LibraryHandle, "vprintf");
  CHECK_NULL_RETURN_OR_ASSERT (g_vprintf_func, EFI_NOT_FOUND, "vprintf == NULL\n");

  g_clock_func = DynamicLoadPpi->Dlsym (LibraryHandle, "clock");
  CHECK_NULL_RETURN_OR_ASSERT (g_clock_func, EFI_NOT_FOUND, "clock == NULL\n");

  g_fclose_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fclose");
  CHECK_NULL_RETURN_OR_ASSERT (g_fclose_func, EFI_NOT_FOUND, "fclose == NULL\n");

  g_exit_func = DynamicLoadPpi->Dlsym (LibraryHandle, "exit");
  CHECK_NULL_RETURN_OR_ASSERT (g_exit_func, EFI_NOT_FOUND, "exit == NULL\n");

  g_fprintf_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fprintf");
  CHECK_NULL_RETURN_OR_ASSERT (g_fprintf_func, EFI_NOT_FOUND, "fprintf == NULL\n");

  g_fread_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fread");
  CHECK_NULL_RETURN_OR_ASSERT (g_fread_func, EFI_NOT_FOUND, "fread == NULL\n");

  g_strtoul_func = DynamicLoadPpi->Dlsym (LibraryHandle, "strtoul");
  CHECK_NULL_RETURN_OR_ASSERT (g_strtoul_func, EFI_NOT_FOUND, "strtoul == NULL\n");

  g_malloc_func = DynamicLoadPpi->Dlsym (LibraryHandle, "malloc");
  CHECK_NULL_RETURN_OR_ASSERT (g_malloc_func, EFI_NOT_FOUND, "malloc == NULL\n");

  g_vfprintf_func = DynamicLoadPpi->Dlsym (LibraryHandle, "vfprintf");
  CHECK_NULL_RETURN_OR_ASSERT (g_vfprintf_func, EFI_NOT_FOUND, "vfprintf == NULL\n");

  g_freopen_func = DynamicLoadPpi->Dlsym (LibraryHandle, "freopen");
  CHECK_NULL_RETURN_OR_ASSERT (g_freopen_func, EFI_NOT_FOUND, "freopen == NULL\n");

  g_getenv_func = DynamicLoadPpi->Dlsym (LibraryHandle, "getenv");
  CHECK_NULL_RETURN_OR_ASSERT (g_getenv_func, EFI_NOT_FOUND, "getenv == NULL\n");

  g_fwrite_func = DynamicLoadPpi->Dlsym (LibraryHandle, "fwrite");
  CHECK_NULL_RETURN_OR_ASSERT (g_fwrite_func, EFI_NOT_FOUND, "fwrite == NULL\n");

  g_putchar_func = DynamicLoadPpi->Dlsym (LibraryHandle, "putchar");
  CHECK_NULL_RETURN_OR_ASSERT (g_putchar_func, EFI_NOT_FOUND, "putchar == NULL\n");

  g_isspace_func = DynamicLoadPpi->Dlsym (LibraryHandle, "isspace");
  CHECK_NULL_RETURN_OR_ASSERT (g_isspace_func, EFI_NOT_FOUND, "isspace == NULL\n");

  g_strtok_func = DynamicLoadPpi->Dlsym (LibraryHandle, "strtok");
  CHECK_NULL_RETURN_OR_ASSERT (g_strtok_func, EFI_NOT_FOUND, "strtok == NULL\n");

  g_ftell_func = DynamicLoadPpi->Dlsym (LibraryHandle, "ftell");
  CHECK_NULL_RETURN_OR_ASSERT (g_ftell_func, EFI_NOT_FOUND, "ftell == NULL\n");

  g_puts_func = DynamicLoadPpi->Dlsym (LibraryHandle, "puts");
  CHECK_NULL_RETURN_OR_ASSERT (g_puts_func, EFI_NOT_FOUND, "puts == NULL\n");

  g_sleep_func = DynamicLoadPpi->Dlsym (LibraryHandle, "sleep");
  CHECK_NULL_RETURN_OR_ASSERT (g_sleep_func, EFI_NOT_FOUND, "sleep == NULL\n");

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // InitSimLib


