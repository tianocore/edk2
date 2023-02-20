/** @file
Header file for helper functions useful to operate file directories by parsing
file path.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_OS_PATH_H
#define _EFI_OS_PATH_H

#include <Common/UefiBaseTypes.h>

//
// Functions declarations
//

/**
  This function returns the directory path which contains the particular path.
  Some examples:
    "a/b/c"  -> "a/b"
    "a/b/c/" -> "a/b"
    "a"      -> "."
    "."      -> ".."
    "/"      -> NULL

  This function does not check for the existence of the file.

  The caller must free the string returned.

  @param FilePath     Path name of file to get the parent directory for.

  @return NULL if error
**/
CHAR8*
OsPathDirName (
  IN CHAR8    *FilePath
  )
;

/**
  This function returns the directory path which contains the particular path.
  Some examples:
    "a/b/../c" -> "a/c"
    "a/b//c"   -> "a/b/c"
    "a/./b"    -> "a/b"

  This function does not check for the existence of the file.

  @param Path     Path name of file to normalize

  @return The string is altered in place.
**/
VOID
OsPathNormPathInPlace (
  IN CHAR8    *Path
  )
;

/**
  This function replaces the final portion of a path with an alternative
  'peer' filename.  For example:
    "a/b/../c", "peer" -> "a/b/../peer"
    "a/b/", "peer"     -> "a/b/peer"
    "/a", "peer"       -> "/peer"
    "a", "peer"        -> "peer"

  This function does not check for the existence of the file.

  @param OldPath     Path name of replace the final segment
  @param Peer        The new path name to concatenate to become the peer path

  @return A CHAR8* string, which must be freed by the caller
**/
CHAR8*
OsPathPeerFilePath (
  IN CHAR8    *OldPath,
  IN CHAR8    *Peer
  )
;

/**
  Checks if a file exists

  @param InputFileName     The name of the file to check for existence

  @retval TRUE              The file exists
  @retval FALSE             The file does not exist
**/
BOOLEAN
OsPathExists (
  IN CHAR8    *InputFileName
  )
;

#endif
