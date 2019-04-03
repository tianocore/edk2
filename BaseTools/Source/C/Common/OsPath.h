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

CHAR8*
OsPathDirName (
  IN CHAR8    *FilePath
  )
;
/**

Routine Description:

  This function returns the directory path which contains the particular path.
  Some examples:
    "a/b/c"  -> "a/b"
    "a/b/c/" -> "a/b"
    "a"      -> "."
    "."      -> ".."
    "/"      -> NULL

  This function does not check for the existence of the file.

  The caller must free the string returned.

Arguments:

  FilePath     Path name of file to get the parent directory for.

Returns:

  NULL if error

**/


VOID
OsPathNormPathInPlace (
  IN CHAR8    *Path
  )
;
/**

Routine Description:

  This function returns the directory path which contains the particular path.
  Some examples:
    "a/b/../c" -> "a/c"
    "a/b//c"   -> "a/b/c"
    "a/./b"    -> "a/b"

  This function does not check for the existence of the file.

Arguments:

  Path     Path name of file to normalize

Returns:

  The string is altered in place.

**/


CHAR8*
OsPathPeerFilePath (
  IN CHAR8    *OldPath,
  IN CHAR8    *Peer
  )
;
/**

Routine Description:

  This function replaces the final portion of a path with an alternative
  'peer' filename.  For example:
    "a/b/../c", "peer" -> "a/b/../peer"
    "a/b/", "peer"     -> "a/b/peer"
    "/a", "peer"       -> "/peer"
    "a", "peer"        -> "peer"

  This function does not check for the existence of the file.

Arguments:

  OldPath     Path name of replace the final segment
  Peer        The new path name to concatenate to become the peer path

Returns:

  A CHAR8* string, which must be freed by the caller

**/


BOOLEAN
OsPathExists (
  IN CHAR8    *InputFileName
  )
;
/**

Routine Description:

  Checks if a file exists

Arguments:

  InputFileName     The name of the file to check for existence

Returns:

  TRUE              The file exists
  FALSE             The file does not exist

**/


#endif
