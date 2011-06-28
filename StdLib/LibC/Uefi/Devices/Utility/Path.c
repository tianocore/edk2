/** @file
    Device Abstraction: Path manipulation utilities.

    Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Library/BaseLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <stdlib.h>
#include  <wchar.h>
#include  <wctype.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <MainData.h>

/** Identify the type of path pointed to by Path.

    Paths are classified based upon their initial character sequences.
      ^\\       Absolute Path
      ^\.       Relative Path
      ^[^:\\]:  Mapping Path
      .*        Relative Path

    Mapping paths are broken into two parts at the ':'.  The part to the left of the ':'
    is the Map Name, pointed to by Path, and the part to the right of the ':' is pointed
    to by NewPath.

    If Path was not a Mapping Path, then NewPath is set to Path.

    @param[in]    Path      Pointer to the path to be classified.
    @param[out]   NewPath   Pointer to the path portion of a mapping path.
    @param[out]   Length    Length of the Map Name portion of the path.

    @retval PathAbsolute  Path is an absolute path. NewPath points to the first '\'.
    @retval PathRelative  Path is a relative path. NewPath = Path.
    @retval PathMapping   Path is a mapping path.  NewPath points to the character following ':'.
    @retval PathError     Path is NULL.
**/
PATH_CLASS
EFIAPI
ClassifyPath(
  IN  wchar_t    *        Path,
  OUT wchar_t   **        NewPath,
  OUT int        * const  Length
  )
{
  size_t    MapLen;

  if(Path == NULL) {
    return PathError;   // Bad parameter
  }
  if(NewPath != NULL) {
    *NewPath = Path;    // Setup default condition
  }
  if((*Path == L'\\') || (*Path == L'\0')) {
    return PathAbsolute;
  }
  if(*Path == L'.') {
    return PathRelative;
  }
  /* The easy stuff has been done, now see if this is a mapping path.
      See if there is a ':' in Path that isn't the first character and is before
      any '\\' characters.
  */
  MapLen = wcscspn(Path, L"\\:");
  if(Length != NULL) {
    *Length = (int)MapLen;
  }
  /*  MapLen == 0       means that the first character is a ':'
             == PathLen means that there are no '\\' or ':'
      Otherwise, Path[MapLen] == ':'  for a mapping path
                              or '\\' for a relative path.
  */
  if(MapLen == 0) {
    return PathError;
  }
  if(Path[MapLen] == L':') {
    if(NewPath != NULL) {
      *NewPath = &Path[MapLen + 1];   // Point to character after then ':'.  Might be '\0'.
    }
    return PathMapping;
  }
  return PathRelative;
}

/*  Normalize a narrow-character path and produce a wide-character path
    that has forward slashes replaced with backslashes.
    Backslashes are directory separators in UEFI File Paths.

    It is the caller's responsibility to eventually free() the returned buffer.

    @param[in]    path    A pointer to the narrow-character path to be normalized.

    @return     A pointer to a buffer containing the normalized, wide-character, path.
*/
wchar_t *
NormalizePath( const char *path)
{
  wchar_t  *temp;
  wchar_t  *OldPath;
  wchar_t  *NewPath;
  size_t    Length;

  OldPath = AsciiStrToUnicodeStr(path, gMD->UString);
  Length  = wcslen(OldPath) + 1;

  NewPath = calloc(Length, sizeof(wchar_t));
  if(NewPath != NULL) {
    temp = NewPath;
    for( ; *OldPath; ++OldPath) {
      if(*OldPath == L'/') {
        *temp = L'\\';
      }
      else {
        *temp = *OldPath;
      }
      ++temp;
    }
  }
  else {
    errno     = ENOMEM;
    EFIerrno  = RETURN_OUT_OF_RESOURCES;
  }
  return NewPath;
}

/** Process a wide character string representing a Mapping Path and extract the instance number.

    The instance number is the sequence of decimal digits immediately to the left
    of the ":" in the Map Name portion of a Mapping Path.

    This function is called with a pointer to beginning of the Map Name.
    Thus Path[Length] must be a ':' and Path[Length - 1] must be a decimal digit.
    If either of these are not true, an instance value of 0 is returned.

    If Path is NULL, an instance value of 0 is returned.

    @param[in]  Path    Points to the beginning of a Mapping Path
    @param[in]  Length  Number of valid characters to the left of the ':'

    @return   Returns either 0 or the value of the contiguous digits to the left of the ':'.
**/
int
EFIAPI
PathInstance(
  const wchar_t  *Path,
        int       Length
  )
{
  wchar_t    *temp;
  int         instance    = 0;

  if((Path != NULL) && (Path[Length] == L':') && (Length > 0)) {
    for(temp = __UNCONST(&Path[Length - 1]); Length > 0; --Length) {
      if(!iswdigit(*temp)) {
        break;
      }
      --temp;
    }
    instance = (int)wcstol(temp+1, NULL, 10);
  }
  return instance;
}

/** Transform a relative path into an absolute path.

    If Path is NULL, return NULL.
    Otherwise, pre-pend the CWD to Path then process the resulting path to:
      - Replace "/./" with "/"
      - Replace "/<dirname>/../" with "/"
      - Do not allow one to back up past the root, "/"

    Also sets the Current Working Device to the Root Device.

    Path must be a previously allocated buffer.  PathAdjust will
    allocate a new buffer to hold the results of the transformation
    and free Path.  A pointer to the newly allocated buffer is returned.

    @param[in]  Path    A pointer to the path to be transformed.  This buffer
                        will always be freed.

    @return   A pointer to a buffer containing the transformed path.
**/
wchar_t *
EFIAPI
PathAdjust(
  wchar_t *Path
  )
{
  wchar_t    *NewPath;

  NewPath = calloc(PATH_MAX, sizeof(wchar_t));
  if(NewPath != NULL) {
    wmemcpy(NewPath, Path, PATH_MAX);
  }
  else {
    errno = ENOMEM;
  }
  free(Path);
  return NewPath;
}

/** Replace the leading portion of Path with any aliases.

    Aliases are read from /etc/fstab.  If there is an associated device, the
    Current Working Device is set to that device.

    Path must be a previously allocated buffer.  PathAlias will
    allocate a new buffer to hold the results of the transformation
    then free Path.  A pointer to the newly allocated buffer is returned.

    @param[in]    Path    A pointer to the original, unaliased, path.  This
                          buffer is always freed.
    @param[out]   Node    Filled in with a pointer to the Device Node describing
                          the device abstraction associated with this path.

    @return     A pointer to a buffer containing the aliased path.
**/
wchar_t *
EFIAPI
PathAlias(
  wchar_t      *Path,
  DeviceNode  **Node
  )
{
  wchar_t    *NewPath;

  NewPath = calloc(PATH_MAX, sizeof(wchar_t));
  if(NewPath != NULL) {
    wmemcpy(NewPath, Path, PATH_MAX);
  }
  else {
    errno = ENOMEM;
  }
  free(Path);
  *Node = NULL;
  return NewPath;
}

/** Parse a path producing the target device, device instance, and file path.

    @param[in]    path
    @param[out]   FullPath
    @param[out]   DevNode
    @param[out]   Which

    @retval   RETURN_SUCCESS              The path was parsed successfully.
    @retval   RETURN_NOT_FOUND            The path does not map to a valid device.
    @retval   RETURN_OUT_OF_RESOURCES     Insufficient memory to calloc a MapName buffer.
                                          The errno variable is set to ENOMEM.
    @retval   RETURN_INVALID_PARAMETER    The path parameter is not valid.
                                          The errno variable is set to EINVAL.
**/
RETURN_STATUS
EFIAPI
ParsePath(
  IN    const char   *path,
  OUT   wchar_t     **FullPath,
  OUT   DeviceNode  **DevNode,
  OUT   int          *Which
  )
{
  int                 MapLen;
  PATH_CLASS          PathClass;
  wchar_t            *NewPath;
  wchar_t            *WPath     = NULL;
  wchar_t            *MPath     = NULL;
  DeviceNode         *Node      = NULL;
  RETURN_STATUS       Status    = RETURN_NOT_FOUND;
  int                 Instance  = 0;
  BOOLEAN             ReMapped;

  ReMapped  = FALSE;

  // Convert name from MBCS to WCS and change '/' to '\\'
  WPath = NormalizePath( path);
  PathClass = ClassifyPath(WPath, &NewPath, &MapLen);

reclassify:
  switch(PathClass) {
    case PathMapping:
      if(!ReMapped) {
        if((NewPath == NULL) || (*NewPath == L'\0')) { /* Nothing after the ':' */
          PathClass = PathAbsolute;
        }
        else {
          Instance = PathInstance(WPath, MapLen);
          PathClass = ClassifyPath(NewPath, NULL, NULL);
        }
        ReMapped = TRUE;
        if(WPath[MapLen] == L':') {
          // Get the Map Name, including the trailing ':'. */
          MPath = calloc(MapLen+2, sizeof(wchar_t));
          if(MPath != NULL) {
            wmemcpy(MPath, WPath, MapLen+2);
          }
          else {
            errno = ENOMEM;
            Status = RETURN_OUT_OF_RESOURCES;
            break;    // Exit the switch(PathClass) statement.
          }
        }
        if(WPath != NewPath) {
          /* Shift the RHS of the path down to the start of the buffer. */
          wmemmove(WPath, NewPath, wcslen(NewPath)+1);
          NewPath = WPath;
        }
        goto reclassify;
      }
      /*  Fall through to PathError if Remapped.
          This means that the path looked like "foo:bar:something".
      */

    case PathError:
      errno = EINVAL;
      Status = RETURN_INVALID_PARAMETER;
      break;

    case PathRelative:
      /*  Transform a relative path into an Absolute path.
          Prepends CWD and handles ./ and ../ entries.
          It is the caller's responsibility to free the space
          allocated to WPath.
      */
      WPath = PathAdjust(NewPath);    // WPath was malloc()ed by PathAdjust

    case PathAbsolute:
      /*  Perform any path aliasing.  For example: /dev/foo -> { node.foo, "" }
          The current volume and directory are updated in the path as needed.
          It is the caller's responsibility to free the space
          allocated to WPath.
      */
    Status = RETURN_SUCCESS;
      WPath = PathAlias(WPath, &Node);       // PathAlias frees its argument and malloc()s a new one.
      break;
  }
  if(!RETURN_ERROR(Status)) {
    *FullPath = WPath;
    *Which    = Instance;

    /*  At this point, WPath is an absolute path,
        MPath is either NULL or points to the Map Name,
        and Instance is the instance number.
    */
    if(MPath == NULL) {
      /* This is NOT a mapped path. */
      if(Node == NULL) {
        Node = daDefaultDevice;
      }
      if(Node != NULL) {
        Status = RETURN_SUCCESS;
      }
    }
    else {
      /* This is a mapped path. */
      Status = __DevSearch( MPath, NULL, &Node);
      if(Status == RETURN_NOT_FOUND) {
        Node = daDefaultDevice;

        if(Node != NULL) {
          Status = RETURN_SUCCESS;
        }
      }
    }
    if(DevNode != NULL) {
      *DevNode = Node;
    }
  }
  if(MPath != NULL) {
    free(MPath);    // We don't need this any more.
  }
  return Status;
}
