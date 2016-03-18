/** @file
  Implementation of the Posix access() function.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <ctype.h>
#include  <errno.h>
#include  <sys/stat.h>
#include  <string.h>
#include  <unistd.h>
#include  <sys/syslimits.h>

/** Save some typing later on. */
#define GOOD_MODE   (F_OK | X_OK | W_OK | R_OK)

/** Determine accessibility of a file.
    The access() function checks the file, named by the pathname pointed to by
    the Path argument, for accessibility according to the bit pattern contained
    in Mode.

    The value of Mode is either the bitwise-inclusive OR of the access
    permissions to be checked (R_OK, W_OK, X_OK) or the existence test (F_OK).

    If Path ends in '/' or '\\', the target must be a directory, otherwise it doesn't matter.
    A file is executable if it is NOT a directory and it ends in ".efi".

    @param[in]    Path    Path or name of the file to be checked.
    @param[in]    Mode    Access permissions to check for.

    @retval   0   Successful completion.
    @retval  -1   File is not accessible with the given Mode.  The error condition
                  is indicated by errno.  Values of errno specific to the access
                  function include: EACCES, ENOENT, ENOTDIR, ENAMETOOLONG
**/
int
access(
  const char *Path,
  int         Mode
  )
{
  struct stat     FileStat;
  int             retval      = -1;
  size_t          PLength;
  uint32_t        WantDir;
  uint32_t        DirMatch;

  if((Path == NULL) || ((Mode & ~GOOD_MODE) != 0)) {
    errno = EINVAL;
  }
  else {
    PLength = strlen(Path);
    if(PLength > PATH_MAX) {
      errno = ENAMETOOLONG;
    }
    else {
      retval = stat(Path, &FileStat);
      if(retval == 0) {
        /* Path exists.  FileStat now holds valid information. */
        WantDir   = isDirSep(Path[PLength - 1]);   // Does Path end in '/' or '\\' ?
        DirMatch = (! WantDir && (! S_ISDIR(FileStat.st_mode)));

        /* Test each permission individually.  */
        do {
          if(Mode == F_OK) {  /* Existence test. */
            if(DirMatch) {    /* This is a directory or file as desired. */
              retval = 0;
            }
            else {
              /* Indicate why we failed the test. */
              errno = (WantDir) ? ENOTDIR : EISDIR;
            }
            break;  /* F_OK does not combine with any other tests. */
          }
          if(Mode & R_OK) {
            if((FileStat.st_mode & READ_PERMS) == 0) {
              /* No read permissions.
                 For UEFI, everything should have READ permissions.
              */
              errno = EDOOFUS;  /* Programming Error. */
              break;
            }
          }
          if(Mode & W_OK) {
            if((FileStat.st_mode & WRITE_PERMS) == 0) {
              /* No write permissions. */
              errno = EACCES;  /* Writing is not OK. */
              break;
            }
          }
          if(Mode & X_OK) {
            /* In EDK II, executable files end in ".efi" */
            if(strcmp(&Path[PLength-4], ".efi") != 0) {
              /* File is not an executable. */
              errno = EACCES;
              break;
            }
          }
          retval = 0;
        } while(FALSE);
      }
      else {
        /* File or path does not exist. */
        errno = ENOENT;
      }
    }
  }
  return retval;
}
