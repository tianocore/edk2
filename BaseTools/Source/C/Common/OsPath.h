/** @file
Header file for helper functions useful to operate file directories by parsing 
file path.

Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
  Peer        The new path name to concatinate to become the peer path

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
