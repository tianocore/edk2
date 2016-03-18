/** @file
  Device Abstraction Utility Routines

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Depends upon: <kfile.h>
**/
#ifndef __DEV_UTILITY_H__
#define __DEV_UTILITY_H__

#define CON_COOKIE    0x62416F49    ///< 'IoAb'

typedef enum {
  PathAbsolute = 0,
  PathRelative,
  PathMapping,
  PathError
} PATH_CLASS;

typedef struct _Device_Node {
  LIST_ENTRY        DevList;        ///< List of registered device abstractions
  const CHAR16     *DevName;
  const GUID       *DevProto;
  void             *InstanceList;   ///< Array of instances for this device
  FO_OPEN           OpenFunc;
  UINT32            InstanceSize;   ///< Size of each instance in the InstanceList
  UINT32            NumInstances;   ///< Number of instances in InstanceList
  UINT32            OpModes;        ///< Supported modes of operation
} DeviceNode;

__BEGIN_DECLS

extern LIST_ENTRY       daDeviceList;     ///< List of registered devices.
extern DeviceNode      *daDefaultDevice;  ///< Device to use if nothing else found
extern DeviceNode      *daRootDevice;     ///< Device containing the root file system
extern DeviceNode      *daCurrentDevice;  ///< Device currently being accessed

/** Add a new device to the device list.

    @param  DevName       Name of the device to add.
    @param  DevProto      Pointer to the GUID identifying the protocol associated with this device.
                          If DevProto is NULL, startup code will not try to find instances
                          of this device.
    @param  OpenFunc      Pointer to the device's Open function.
    @param  InstanceList  Optional pointer to the device's initialized instance list.
                          If InstanceList is NULL, the application startup code will
                          scan for instances of the protocol identified by DevProto and
                          populate the InstanceList in the order those protocols are found.
    @param  NumInstance   Number of instances in InstanceList.
    @param  Modes         Bit-mapped flags indicating operations (R, W, RW, ...) permitted to this device.

**/
DeviceNode * EFIAPI __DevRegister( const CHAR16 *DevName, GUID *DevProto, FO_OPEN OpenFunc,
                                   void *InstanceList, int NumInstance, UINT32 InstanceSize, UINT32 Modes);

/** Find a DeviceNode matching DevName or DevProto, or both.

    If DevName is NULL, then the device name is not used in the search.
    If DevProto is NULL, then the protocol GUID is not used in the search.
    If both are NULL, then INVALID_PARAMETER is returned.
    If both DevName and DevProto are specified, then both must match.
    If both are specified but only one matches, then DEVICE_ERROR is returned.

    @param  DevName     Name of the Device Abstraction to find.
    @param  DevProto    GUID of the Protocol associated with the Device Abstraction.
    @param  Node        Pointer to the pointer to receive the found Device Node's address.

    @retval RETURN_SUCCESS              The desired Device Node was found.
    @retval RETURN_INVALID_PARAMETER    Both DevName and DevProto are NULL or Node is NULL.
    @retval RETURN_DEVICE_ERROR         One, but not both, of DevNode and DevProto matched.
**/
EFI_STATUS EFIAPI __DevSearch( CHAR16 *DevName, GUID *DevProto, DeviceNode **Node);

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

    @retval PathAbsolute  Path is an absolute path. NewPath points to the first '\'.
    @retval PathRelative  Path is a relative path. NewPath = Path.
    @retval PathMapping   Path is a mapping path.  NewPath points to the ':'.
    @retval PathError     Path is NULL.
**/
PATH_CLASS EFIAPI ClassifyPath(IN wchar_t *Path, OUT wchar_t **NewPath, int * const Length);

/*  Normalize a narrow-character path and produce a wide-character path
    that has forward slashes replaced with backslashes.
    Backslashes are directory separators in UEFI File Paths.

    It is the caller's responsibility to eventually free() the returned buffer.

    @param[in]    path    A pointer to the narrow-character path to be normalized.

    @return     A pointer to a buffer containing the normalized, wide-character, path.
*/
wchar_t *
NormalizePath( const char *path);

/** Process a MBCS path returning the final absolute path and the target device.

    @param path
    @param FullPath
    @param DevNode

    @retval RETURN_SUCCESS
    @retval RETURN_INVALID_PARAMETER
    @retval RETURN_NOT_FOUND
**/
RETURN_STATUS
EFIAPI
ParsePath( const char *path, wchar_t **FullPath, DeviceNode **DevNode, int *Which, wchar_t **MapPath);

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
PathInstance( const wchar_t *Path, int length);

/** Transform a relative path into an absolute path.

    If Path is NULL, return NULL.
    Otherwise, pre-pend the CWD to Path then process the resulting path to:
      - Replace "/./" with "/"
      - Replace "/<dirname>/../" with "/"
      - Do not allow one to back up past the root, "/"

    Also sets the Current Working Device to the Root Device.

    Path must be a previously allocated buffer.  PathAdjust will
    allocate a new buffer to hold the results of the transformation
    then free Path.  A pointer to the newly allocated buffer is returned.

    @param[in]  Path    A pointer to the path to be transformed.  This buffer
                        will always be freed.

    @return   A pointer to a buffer containing the transformed path.
**/
wchar_t *
EFIAPI
PathAdjust(wchar_t *Path);

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
PathAlias(wchar_t *Path, DeviceNode **Node);

/**
  Parses a normalized wide character path and returns a pointer to the entry
  following the last \.  If a \ is not found in the path the return value will
  be the same as the input value.  All error conditions return NULL.

  The behavior when passing in a path that has not been normalized is undefined.

  @param  Path - A pointer to a wide character string containing a path to a
                 directory or a file.

  @return Pointer to the file name or terminal directory.  NULL if an error is
          detected.
**/
wchar_t *
EFIAPI
GetFileNameFromPath(
  const wchar_t   *Path
  );

__END_DECLS

#endif  /* __DEV_UTILITY_H__ */
