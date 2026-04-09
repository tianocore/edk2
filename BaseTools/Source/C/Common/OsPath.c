/** @file
Functions useful to operate file directories by parsing file path.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CommonLib.h"
#include "OsPath.h"

//
// Functions implementations
//

#if 0
  //
  // BUGBUG: Not fully implemented yet.
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

{
  CHAR8 *Return;
  CHAR8 *Pos;
  CHAR8 Char;
  UINTN Length;
  INTN  Offset;

  Length = strlen (FilePath);

  if (Length == 0) {
    return NULL;
  }

  //
  // Check for the root directory case
  //
  if (
    (Length == 3 && isalpha (FilePath[0]) && (strcmp(FilePath + 1, ":\\") == 0)) ||
    (strcmp(FilePath, "/") == 0)
    ) {
    return NULL;
  }

  //
  // If the path ends with a path separator, then just append ".."
  //
  Char = FilePath[Length - 1];
  if (Char == '/' || Char == '\\') {
    return OsPathJoin (FilePath, "..");
  }

  //
  //
  //
  for (Offset = Length; Offset > 0; Offset--) {
    if ((Return[Offset] == '/') || (Return[Offset] == '\\')) {
      Return[Offset] = '\0';
      return Return;
    }
  }
}
#endif


#if 0
  //
  // BUGBUG: Not fully implemented yet.
  //

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
{
  CHAR8   *Pos;
  INTN    Offset;
  BOOLEAN TryAgain;
  UINTN   Length;
  UINTN   Remaining;
  UINTN   SubLength;

  do {
    TryAgain = FALSE;
    Length = strlen (Path);

    for (Offset = 0; Offset < Length; Offset++) {
      Remaining = Length - Offset;

      //
      // Collapse '//' -> '/'
      //
      if (
          (Remaining >= 2) &&
          ((Offset > 0) || (Path[0] != '\\')) &&
          IsDirSep (Path[Offset]) && IsDirSep (Path[Offset + 1])
         ) {
        memmove (&Path[Offset], &Path[Offset + 1], Remaining);
        TryAgain = TRUE;
        break;
      }

      //
      // Collapse '/./' -> '/'
      //
      if ((Remaining >= 3) && IsDirSep (Path[Offset]) &&
           (Path[Offset + 1] == '.') && IsDirSep (Path[Offset + 2])
         ) {
        memmove (&Path[Offset], &Path[Offset + 1], Remaining);
        TryAgain = TRUE;
        break;
      }

      //
      // Collapse 'a/../b' -> 'b'
      //
      // BUGBUG: Not implemented yet

    }

  } while (TryAgain);

  Return = CloneString (FilePath);
  if (Return == NULL) {
    return NULL;
  }

  Length = strlen (Return);

  //
  // Check for the root directory case
  //
  if (
    (Length == 3 && isalpha (Return[0]) && (strcmp(Return + 1, ":\\") == 0)) ||
    (strcmp(Return, "/") == 0)
    ) {
    free (Return);
    return NULL;
  }

  //
  //
  //
  for (Offset = Length; Offset > 0; Offset--) {
    if ((Return[Offset] == '/') || (Return[Offset] == '\\')) {
      Return[Offset] = '\0';
      return Return;
    }
  }
}
#endif

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
{
  CHAR8 *Result;
  INTN    Offset;

  Result = (CHAR8 *) malloc (strlen (OldPath) + strlen (Peer) + 1);
  if (Result == NULL) {
    return NULL;
  }

  strcpy (Result, OldPath);

  //
  // Search for the last '/' or '\' in the string.  If found, replace
  // everything following it with Peer
  //
  for (Offset = strlen (Result); Offset >= 0; Offset--) {
    if ((Result[Offset] == '/') || (Result[Offset] == '\\')) {
      Result[Offset + 1] = '\0';
      strcat (Result, Peer);
      return Result;
    }
  }

  //
  // Neither a '/' nor a '\' was found.  Therefore, we simply return Peer.
  //
  strcpy (Result, Peer);
  return Result;
}

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
{
  FILE    *InputFile;
  InputFile = fopen (LongFilePath (InputFileName), "rb");
  if (InputFile == NULL) {
    return FALSE;
  } else {
    fclose (InputFile);
    return TRUE;
  }
}



