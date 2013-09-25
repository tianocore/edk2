/** @file
  Defines for EFI shell environment 2 ported to EDK II build environment. (no spec)

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _SHELL_ENVIRONMENT_2_PROTOCOL_H_
#define _SHELL_ENVIRONMENT_2_PROTOCOL_H_

#define DEFAULT_INIT_ROW    1
#define DEFAULT_AUTO_LF     FALSE

/**
  This function is a prototype for a function that dumps information on a protocol
  to a given location.  The location is dependant on the implementation.  This is
  used when programatically adding shell commands.

  @param[in] Handle                 The handle the protocol is on.
  @param[in] Interface              The interface to the protocol.

**/
typedef
VOID
(EFIAPI *SHELLENV_DUMP_PROTOCOL_INFO) (
  IN EFI_HANDLE                   Handle,
  IN VOID                         *Interface
  );

/**
  This function is a prototype for each command internal to the EFI shell
  implementation.  The specific command depends on the implementation.  This is
  used when programatically adding shell commands.

  @param[in] ImageHandle        The handle to the binary shell.
  @param[in] SystemTable        The pointer to the system table.

  @retval EFI_SUCCESS           The command completed.
  @retval other                 An error occurred.  Any error is possible
                                depending on the implementation of the shell
                                command.

**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_INTERNAL_COMMAND) (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  );

/**
  This function is a prototype for one that gets a help string for a given command.
  This is used when programatically adding shell commands.  Upon successful return
  the memory allocated is up to the caller to free.

  @param[in, out] Str              Pointer to pointer to string to display for help.

  @retval EFI_SUCCESS             The help string is in the parameter Str.

**/
typedef
EFI_STATUS
(EFIAPI *SHELLCMD_GET_LINE_HELP) (
  IN OUT CHAR16                 **Str
  );

/**
Structure returned from functions that open multiple files.
**/
typedef struct {
  UINT32                    Signature;            ///< SHELL_FILE_ARG_SIGNATURE.
  LIST_ENTRY                Link;                 ///< Linked list helper.
  EFI_STATUS                Status;               ///< File's status.

  EFI_FILE_HANDLE           Parent;               ///< What is the Parent file of this file.
  UINT64                    OpenMode;             ///< How was the file opened.
  CHAR16                    *ParentName;          ///< String representation of parent.
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;    ///< DevicePath for Parent.

  CHAR16                    *FullName;            ///< Path and file name for this file.
  CHAR16                    *FileName;            ///< File name for this file.

  EFI_FILE_HANDLE           Handle;               ///< Handle to this file.
  EFI_FILE_INFO             *Info;                ///< Pointer to file info for this file.
} SHELL_FILE_ARG;

/// Signature for SHELL_FILE_ARG.
#define SHELL_FILE_ARG_SIGNATURE  SIGNATURE_32 ('g', 'r', 'a', 'f')

/**
GUID for the shell environment2 and shell environment.
**/
#define SHELL_ENVIRONMENT_PROTOCOL_GUID \
  { \
    0x47c7b221, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} \
  }

/**
GUID for the shell environment2 extension (main GUID above).
**/
#define EFI_SE_EXT_SIGNATURE_GUID \
  { \
    0xd2c18636, 0x40e5, 0x4eb5, {0xa3, 0x1b, 0x36, 0x69, 0x5f, 0xd4, 0x2c, 0x87} \
  }

#define EFI_SHELL_MAJOR_VER 0x00000001 ///< Major version of the EFI_SHELL_ENVIRONMENT2.
#define EFI_SHELL_MINOR_VER 0x00000000 ///< Minor version of the EFI_SHELL_ENVIRONMENT2.

/**
  Execute a command line.

  This function will run the CommandLine.  This includes loading any required images,
  parsing any requires scripts, and if DebugOutput is TRUE printing errors
  encountered directly to the screen.

  @param[in] ParentImageHandle  Handle of the image executing this operation.
  @param[in] CommandLine        The string command line to execute.
  @param[in] DebugOutput        TRUE indicates that errors should be printed directly.
                                FALSE supresses error messages.

  @retval EFI_SUCCESS           The command line executed and completed.
  @retval EFI_ABORTED           The operation aborted.
  @retval EFI_INVALID_PARAMETER A parameter did not have a valid value.
  @retval EFI_OUT_OF_RESOURCES  A required memory allocation failed.

@sa HandleProtocol
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_EXECUTE) (
  IN EFI_HANDLE   *ParentImageHandle,
  IN CHAR16       *CommandLine,
  IN BOOLEAN      DebugOutput
  );

/**
  This function returns a shell environment variable value.

  @param[in] Name               The pointer to the string with the shell environment
                                variable name.

  @retval NULL                  The shell environment variable's value could not be found.
  @retval !=NULL                The value of the shell environment variable Name.

**/
typedef
CHAR16 *
(EFIAPI *SHELLENV_GET_ENV) (
  IN CHAR16 *Name
  );

/**
  This function returns a shell environment map value.

  @param[in] Name               The pointer to the string with the shell environment
                                map name.

  @retval NULL                  The shell environment map's value could not be found.
  @retval !=NULL                The value of the shell environment map Name.

**/
typedef
CHAR16 *
(EFIAPI *SHELLENV_GET_MAP) (
  IN CHAR16 *Name
  );

/**
  This function will add an internal command to the shell interface.

  This will allocate all required memory, put the new command on the command
  list in the correct location.

  @param[in] Handler                The handler function to call when the command gets called.
  @param[in] Cmd                    The command name.
  @param[in] GetLineHelp            The function to call of type "get help" for this command.

  @retval EFI_SUCCESS           The command is now part of the command list.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @sa SHELLENV_INTERNAL_COMMAND
  @sa SHELLCMD_GET_LINE_HELP
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_ADD_CMD) (
  IN SHELLENV_INTERNAL_COMMAND    Handler,
  IN CHAR16                       *Cmd,
  IN SHELLCMD_GET_LINE_HELP       GetLineHelp
  );

/**
  Internal interface to add protocol handlers.

  This function is for internal shell use only.  This is how protocol handlers are added.
  This will get the current protocol info and add the new info or update existing info
  and then resave the info.

  @param[in] Protocol           The pointer to the protocol's GUID.
  @param[in] DumpToken          The function pointer to dump token function or
                                NULL.
  @param[in] DumpInfo           The function pointer to dump infomation function
                                or NULL.
  @param[in] IdString           The English name of the protocol.
**/
typedef
VOID
(EFIAPI *SHELLENV_ADD_PROT) (
  IN EFI_GUID                     *Protocol,
  IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
  IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
  IN CHAR16                       *IdString
  );

/**
  This function finds a protocol handle by a GUID.

  This function will check for already known protocols by GUID and if one is
  found it will return the name of that protocol.  If no name is found and
  GenId is TRUE it will generate ths string.

  @param[in] Protocol          The GUID of the protocol to look for.
  @param[in] GenId             Whether to generate a name string if it is not found.

  @return !NULL                The Name of the protocol.
  @retval NULL                 The Name was not found, and GenId was not TRUE.
**/
typedef
CHAR16*
(EFIAPI *SHELLENV_GET_PROT) (
  IN EFI_GUID *Protocol,
  IN BOOLEAN GenId
  );

/**
  This function returns a string array containing the current directory on
  a given device.

  If DeviceName is specified, then return the current shell directory on that
  device.  If DeviceName is NULL, then return the current directory on the
  current device.  The caller us responsible to free the returned string when
  no longer required.

  @param[in] DeviceName         The name of the device to get the current
                                directory on, or NULL for current device.

  @return String array with the current directory on the current or specified device.

**/
typedef
CHAR16*
(EFIAPI *SHELLENV_CUR_DIR) (
  IN CHAR16 *DeviceName OPTIONAL
  );

/**
  This function will open a group of files that match the Arg path, including
  support for wildcard characters ('?' and '*') in the Arg path.  If there are
  any wildcard characters in the path this function will find any and all files
  that match the wildcards.  It returns a double linked list based on the
  LIST_ENTRY linked list structure.  Use this in conjunction with the
  SHELL_FILE_ARG_SIGNATURE to get the SHELL_FILE_ARG structures that are returned.
  The memory allocated by the callee for this list is freed by making a call to
  SHELLENV_FREE_FILE_LIST.

  @param[in] Arg                 The pointer Path to files to open.
  @param[in, out] ListHead       The pointer to the allocated and initialized list head
                                 upon which to append all opened file structures.

  @retval EFI_SUCCESS           One or more files was opened and a struct of each file's
                                information was appended to ListHead.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_NOT_FOUND         No matching files could be found.
  @sa SHELLENV_FREE_FILE_LIST
**/typedef
EFI_STATUS
(EFIAPI *SHELLENV_FILE_META_ARG) (
  IN CHAR16               *Arg,
  IN OUT LIST_ENTRY       *ListHead
  );

/**
  This frees all of the nodes under the ListHead, but not ListHead itself.

  @param[in, out] ListHead       Pointer to list to free all nodes of.

  @retval EFI_SUCCESS           This function always returns EFI_SUCCESS.
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_FREE_FILE_LIST) (
  IN OUT LIST_ENTRY       *ListHead
  );

/**
  This function creates a new instance of the ShellInterface protocol for use on
  the ImageHandle.

  This function is for internal shell usage.  This will allocate and then populate
  EFI_SHELL_INTERFACE protocol.  It is the caller's responsibility to free the
  memory.

  @param[in] ImageHandle        The handle which will use the new ShellInterface
                                protocol.

  @return The newly allocated shell interface protocol.

**/
typedef
EFI_SHELL_INTERFACE*
(EFIAPI *SHELLENV_NEW_SHELL) (
  IN EFI_HANDLE ImageHandle
  );

/**
  This function determins whether a script file is currently being processed.

  A script file (.nsh file) can contain a series of commands and this is useful to
  know for some shell commands whether they are being run manually or as part of a
  script.

  @retval TRUE                  A script file is being processed.
  @retval FALSE                 A script file is not being processed.
**/
typedef
BOOLEAN
(EFIAPI *SHELLENV_BATCH_IS_ACTIVE) (
  VOID
  );

/**
  This is an internal shell function to free any and all allocated resources.
  This should be called immediately prior to closing the shell.
**/
typedef
VOID
(EFIAPI *SHELLENV_FREE_RESOURCES) (
  VOID
  );

/**
  This function enables the page break mode.

  This mode causes the output to pause after each complete screen to enable a
  user to more easily read it.  If AutoWrap is TRUE, then rows with too many
  characters will be chopped and divided into 2 rows.  If FALSE, then rows with
  too many characters may not be fully visible to the user on the screen.

  @param[in] StartRow               The row number to start this on.
  @param[in] AutoWrap               Whether to auto wrap rows that are too long.
**/
typedef
VOID
(EFIAPI *SHELLENV_ENABLE_PAGE_BREAK) (
  IN INT32      StartRow,
  IN BOOLEAN    AutoWrap
  );

/**
  This function disables the page break mode.

  Disabling this causes the output to print out exactly as coded, with no breaks
  for readability.
**/
typedef
VOID
(EFIAPI *SHELLENV_DISABLE_PAGE_BREAK) (
  VOID
  );

/**
  Get the status of the page break output mode.

  @retval FALSE                 Page break output mode is not enabled.
  @retval TRUE                  Page break output mode is enabled.
**/
typedef
BOOLEAN
(EFIAPI *SHELLENV_GET_PAGE_BREAK) (
  VOID
  );

/**
  This function sets the keys to filter for for the console in.  The valid
  values to set are:

  #define EFI_OUTPUT_SCROLL   0x00000001
  #define EFI_OUTPUT_PAUSE    0x00000002
  #define EFI_EXECUTION_BREAK 0x00000004

  @param[in] KeyFilter              The new key filter to use.
**/
typedef
VOID
(EFIAPI *SHELLENV_SET_KEY_FILTER) (
  IN UINT32      KeyFilter
  );

/**
  This function gets the keys to filter for for the console in.

  The valid values to get are:
  #define EFI_OUTPUT_SCROLL   0x00000001
  #define EFI_OUTPUT_PAUSE    0x00000002
  #define EFI_EXECUTION_BREAK 0x00000004

  @retval The current filter mask.
**/
typedef
UINT32
(EFIAPI *SHELLENV_GET_KEY_FILTER) (
  VOID
  );

/**
  This function determins if the shell application should break.

  This is used to inform a shell application that a break condition has been
  initiated.  Long loops should check this to prevent delays to the break.

  @retval TRUE                  A break has been signaled.  The application
                                should exit with EFI_ABORTED as soon as possible.
  @retval FALSE                 Continue as normal.
**/
typedef
BOOLEAN
(EFIAPI *SHELLENV_GET_EXECUTION_BREAK) (
  VOID
  );

/**
  This is an internal shell function used to increment the shell nesting level.

**/
typedef
VOID
(EFIAPI *SHELLENV_INCREMENT_SHELL_NESTING_LEVEL) (
  VOID
  );

/**
  This is an internal shell function used to decrement the shell nesting level.
**/
typedef
VOID
(EFIAPI *SHELLENV_DECREMENT_SHELL_NESTING_LEVEL) (
  VOID
  );

/**
  This function determins if the caller is running under the root shell.

  @retval TRUE                  The caller is running under the root shell.
  @retval FALSE                 The caller is not running under the root shell.

**/
typedef
BOOLEAN
(EFIAPI *SHELLENV_IS_ROOT_SHELL) (
  VOID
  );

/**
  Close the console proxy to restore the original console.

  This is an internal shell function to handle shell cascading.  It restores the
  original set of console protocols.

  @param[in] ConInHandle         The handle of ConIn.
  @param[in, out] ConIn          The pointer to the location to return the pointer to
                                 the original console input.
  @param[in] ConOutHandle        The handle of ConOut
  @param[in, out] ConOut         The pointer to the location to return the pointer to
                                 the original console output.
**/
typedef
VOID
(EFIAPI *SHELLENV_CLOSE_CONSOLE_PROXY) (
  IN     EFI_HANDLE                       ConInHandle,
  IN OUT EFI_SIMPLE_TEXT_INPUT_PROTOCOL   **ConIn,
  IN     EFI_HANDLE                       ConOutHandle,
  IN OUT EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  **ConOut
  );

//
// declarations of handle enumerator
//
/**
  For ease of use the shell maps handle #'s to short numbers.
  This is only done on request for various internal commands and the references
  are immediately freed when the internal command completes.
**/
typedef
VOID
(EFIAPI *INIT_HANDLE_ENUMERATOR) (
  VOID
  );

/**
  This is an internal shell function to enumerate the handle database.

  This function gets the next handle in the handle database.  If no handles are
  found, EFI_NOT_FOUND is returned.  If the previous Handle was the last handle,
  it is set to NULL before returning.

  This must be called after INIT_HANDLE_ENUMERATOR and before CLOSE_HANDLE_ENUMERATOR.

  @param[in, out] Handle         The pointer to pointer to Handle.  It is set
                                 on a sucessful return.

  @retval EFI_SUCCESS           The next handle in the handle database is *Handle.
  @retval EFI_NOT_FOUND         There is not another handle.
**/
typedef
EFI_STATUS
(EFIAPI *NEXT_HANDLE) (
  IN OUT EFI_HANDLE             **Handle
  );

/**
  This is an internal shell function to enumerate the handle database.

  This function skips the next SkipNum handles in the handle database.  If there
  are not enough handles left to skip that many EFI_ACCESS_DENIED is returned and
  no skip is performed.

  This must be called after INIT_HANDLE_ENUMERATOR and before CLOSE_HANDLE_ENUMERATOR.

  @param[in] SkipNum            How many handles to skip

  @retval EFI_SUCCESS           The next handle in the handle database is *Handle
  @retval EFI_ACCESS_DENIED     There are not SkipNum handles left in the database
**/
typedef
EFI_STATUS
(EFIAPI *SKIP_HANDLE) (
  IN UINTN                   SkipNum
  );

/**
  This is an internal shell function to enumerate the handle database.

  This function resets the the handle database so that NEXT_HANDLE and SKIP_HANDLE
  will start from EnumIndex on the next call.

  This must be called after INIT_HANDLE_ENUMERATOR and before CLOSE_HANDLE_ENUMERATOR.

  @param[in] EnumIndex          Where to start.

  @return The number of handles either read out or skipped before this reset.
**/
typedef
UINTN
(EFIAPI *RESET_HANDLE_ENUMERATOR) (
  IN UINTN                  EnumIndex
  );

/**
  This is an internal shell function to enumerate the handle database.

  This must be called after INIT_HANDLE_ENUMERATOR.

  This function releases all memory and resources associated with the handle database.
  After this no other handle enumerator functions except INIT_HANDLE_ENUMERATOR will
  function properly.
**/
typedef
VOID
(EFIAPI *CLOSE_HANDLE_ENUMERATOR) (
  VOID
  );

/**
  This is an internal shell function to enumerate the handle database.

  This function returns the number of handles in the handle database.

  This must be called after INIT_HANDLE_ENUMERATOR and before CLOSE_HANDLE_ENUMERATOR.

  @return The number of handles in the handle database.
**/
typedef
UINTN
(EFIAPI *GET_NUM) (
  VOID
  );

/**
Handle Enumerator structure.
**/
typedef struct {
  INIT_HANDLE_ENUMERATOR  Init;   ///< The pointer to INIT_HANDLE_ENUMERATOR function.
  NEXT_HANDLE             Next;   ///< The pointer to NEXT_HANDLE function.
  SKIP_HANDLE             Skip;   ///< The pointer to SKIP_HANDLE function.
  RESET_HANDLE_ENUMERATOR Reset;  ///< The pointer to RESET_HANDLE_ENUMERATOR function.
  CLOSE_HANDLE_ENUMERATOR Close;  ///< The pointer to CLOSE_HANDLE_ENUMERATOR function.
  GET_NUM                 GetNum; ///< The pointer to GET_NUM function.
} HANDLE_ENUMERATOR;

/**
  Signature for the PROTOCOL_INFO structure.
**/
#define PROTOCOL_INFO_SIGNATURE SIGNATURE_32 ('s', 'p', 'i', 'n')

/**
  PROTOCOL_INFO structure for protocol enumerator functions.
**/
typedef struct {
  UINTN                       Signature;   ///< PROTOCOL_INFO_SIGNATURE.
  LIST_ENTRY                  Link;        ///< Standard linked list helper member.
  //
  // The parsing info for the protocol.
  //
  EFI_GUID                    ProtocolId;  ///< The GUID for the protocol.
  CHAR16                      *IdString;   ///< The name of the protocol.
  SHELLENV_DUMP_PROTOCOL_INFO DumpToken;   ///< The pointer to DumpToken function for the protocol.
  SHELLENV_DUMP_PROTOCOL_INFO DumpInfo;    ///< The pointer to DumpInfo function for the protocol.
  //
  // Patabase info on which handles are supporting this protocol.
  //
  UINTN                       NoHandles;   ///< The number of handles producing this protocol.
  EFI_HANDLE                  *Handles;    ///< The array of handles.

} PROTOCOL_INFO;

//
// Declarations of protocol info enumerator.
//
/**
  This is an internal shell function to initialize the protocol enumerator.

  This must be called before NEXT_PROTOCOL_INFO, SKIP_PROTOCOL_INFO,
  RESET_PROTOCOL_INFO_ENUMERATOR, and CLOSE_PROTOCOL_INFO_ENUMERATOR are
  called.
**/
typedef
VOID
(EFIAPI *INIT_PROTOCOL_INFO_ENUMERATOR) (
  VOID
  );

/**
  This function is an internal shell function for enumeration of protocols.

  This function returns the next protocol on the list.  If this is called
  immediately after initialization, it will return the first protocol on the list.
  If this is called immediately after reset, it will return the first protocol again.

  This cannot be called after CLOSE_PROTOCOL_INFO_ENUMERATOR, but it must be
  called after INIT_PROTOCOL_INFO_ENUMERATOR.

  @param[in, out] ProtocolInfo   The pointer to pointer to protocol information structure.

  @retval EFI_SUCCESS           The next protocol's information was sucessfully returned.
  @retval NULL                  There are no more protocols.
**/
typedef
EFI_STATUS
(EFIAPI *NEXT_PROTOCOL_INFO) (
  IN OUT PROTOCOL_INFO            **ProtocolInfo
  );

/**
  This function is an internal shell function for enumeration of protocols.

  This cannot be called after CLOSE_PROTOCOL_INFO_ENUMERATOR, but it must be
  called after INIT_PROTOCOL_INFO_ENUMERATOR.

  This function does nothing and always returns EFI_SUCCESS.

  @retval EFI_SUCCESS           Always returned (see above).
**/
typedef
EFI_STATUS
(EFIAPI *SKIP_PROTOCOL_INFO) (
  IN UINTN                         SkipNum
  );

/**
  This function is an internal shell function for enumeration of protocols.

  This cannot be called after CLOSE_PROTOCOL_INFO_ENUMERATOR, but it must be
  called after INIT_PROTOCOL_INFO_ENUMERATOR.

  This function resets the list of protocols such that the next one in the
  list is the begining of the list.
**/
typedef
VOID
(EFIAPI *RESET_PROTOCOL_INFO_ENUMERATOR) (
  VOID
  );


/**
  This function is an internal shell function for enumeration of protocols.

  This must be called after INIT_PROTOCOL_INFO_ENUMERATOR.  After this call
  no protocol enumerator calls except INIT_PROTOCOL_INFO_ENUMERATOR may be made.

  This function frees any memory or resources associated with the protocol
  enumerator.
**/
typedef
VOID
(EFIAPI *CLOSE_PROTOCOL_INFO_ENUMERATOR) (
  VOID
  );

/**
  Protocol enumerator structure of function pointers.
**/
typedef struct {
  INIT_PROTOCOL_INFO_ENUMERATOR   Init;   ///< The pointer to INIT_PROTOCOL_INFO_ENUMERATOR function.
  NEXT_PROTOCOL_INFO              Next;   ///< The pointer to NEXT_PROTOCOL_INFO function.
  SKIP_PROTOCOL_INFO              Skip;   ///< The pointer to SKIP_PROTOCOL_INFO function.
  RESET_PROTOCOL_INFO_ENUMERATOR  Reset;  ///< The pointer to RESET_PROTOCOL_INFO_ENUMERATOR function.
  CLOSE_PROTOCOL_INFO_ENUMERATOR  Close;  ///< The pointer to CLOSE_PROTOCOL_INFO_ENUMERATOR function.
} PROTOCOL_INFO_ENUMERATOR;

/**
  This function is used to retrieve a user-friendly display name for a handle.

  If UseComponentName is TRUE then the component name protocol for this device
  or it's parent device (if required) will be used to obtain the name of the
  device.  If UseDevicePath is TRUE it will get the human readable device path
  and return that.  If both are TRUE it will try to use component name first
  and device path if that fails.

  It will use either ComponentName or ComponentName2 protocol, depending on
  what is present.

  This function will furthur verify whether the handle in question produced either
  EFI_DRIVER_CONFIGRATION_PROTOCOL or EFI_DRIVER_CONFIGURATION2_PROTOCOL and also
  whether the handle in question produced either EFI_DRIVER_DIAGNOSTICS_PROTOCOL or
  EFI_DRIVER_DIAGNOSTICS2_PROTOCOL.

  Upon successful return, the memory for *BestDeviceName is up to the caller to free.

  @param[in] DeviceHandle            The device handle whose name is desired.
  @param[in] UseComponentName        Whether to use the ComponentName protocol at all.
  @param[in] UseDevicePath           Whether to use the DevicePath protocol at all.
  @param[in] Language                The pointer to the language string to use.
  @param[in, out] BestDeviceName     The pointer to pointer to string allocated with the name.
  @param[out] ConfigurationStatus    The pointer to status for opening a Configuration protocol.
  @param[out] DiagnosticsStatus      The pointer to status for opening a Diagnostics protocol.
  @param[in] Display                 Whether to Print this out to default Print location.
  @param[in] Indent                  How many characters to indent the printing.

  @retval EFI_SUCCESS           This function always returns EFI_SUCCESS.
**/
typedef
EFI_STATUS
(EFIAPI *GET_DEVICE_NAME) (
  IN EFI_HANDLE  DeviceHandle,
  IN BOOLEAN     UseComponentName,
  IN BOOLEAN     UseDevicePath,
  IN CHAR8       *Language,
  IN OUT CHAR16  **BestDeviceName,
  OUT EFI_STATUS *ConfigurationStatus,
  OUT EFI_STATUS *DiagnosticsStatus,
  IN BOOLEAN     Display,
  IN UINTN       Indent
  );

#define EFI_SHELL_COMPATIBLE_MODE_VER L"1.1.1" ///< The string for lowest version this shell supports.
#define EFI_SHELL_ENHANCED_MODE_VER   L"1.1.2" ///< The string for highest version this shell supports.

/**
  This function gets the shell mode as stored in the shell environment
  "efishellmode".  It will not fail.

  @param[out] Mode              Returns a string representing one of the
                                2 supported modes of the shell.

  @retval EFI_SUCCESS           This function always returns success.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SHELL_MODE) (
  OUT CHAR16     **Mode
  );

/**
  Convert a file system style name to a device path.

  This function will convert a shell path name to a Device Path Protocol path.
  This function will allocate any required memory for this operation and it
  is the responsibility of the caller to free that memory when no longer required.

  If anything prevents the complete conversion free any allocated memory and
  return NULL.

  @param[in] Path               The path to convert.

  @retval !NULL                 A pointer to the callee allocated Device Path.
  @retval NULL                  The operation could not be completed.
**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *SHELLENV_NAME_TO_PATH) (
  IN CHAR16 *Path
  );

/**
  Converts a device path into a file system map name.

  If DevPath is NULL, then ASSERT.

  This function looks through the shell environment map for a map whose device
  path matches the DevPath parameter.  If one is found the Name is returned via
  Name parameter.  If sucessful the caller must free the memory allocated for
  Name.

  This function will use the internal lock to prevent changes to the map during
  the lookup operation.

  @param[in] DevPath                The device path to search for a name for.
  @param[in] ConsistMapping         What state to verify map flag VAR_ID_CONSIST.
  @param[out] Name                  On sucessful return the name of that device path.

  @retval EFI_SUCCESS           The DevPath was found and the name returned
                                in Name.
  @retval EFI_OUT_OF_RESOURCES  A required memory allocation failed.
  @retval EFI_UNSUPPORTED       The DevPath was not found in the map.
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_GET_FS_NAME) (
  IN EFI_DEVICE_PATH_PROTOCOL     * DevPath,
  IN BOOLEAN                      ConsistMapping,
  OUT CHAR16                      **Name
  );

/**
  This function will open a group of files that match the Arg path, but will not
  support the wildcard characters ('?' and '*') in the Arg path.  If there are
  any wildcard characters in the path this function will return
  EFI_INVALID_PARAMETER.  The return is a double linked list based on the
  LIST_ENTRY linked list structure.  Use this in conjunction with the
  SHELL_FILE_ARG_SIGNATURE to get the SHELL_FILE_ARG structures that are returned.
  The memory allocated by the callee for this list is freed by making a call to
  SHELLENV_FREE_FILE_LIST.

  @param[in] Arg                 The pointer to the path of the files to be opened.
  @param[in, out] ListHead       The pointer to allocated and initialized list head
                                 upon which to append all the opened file structures.

  @retval EFI_SUCCESS           One or more files was opened and a struct of each file's
                                information was appended to ListHead.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_NOT_FOUND         No matching files could be found.
  @sa SHELLENV_FREE_FILE_LIST
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_FILE_META_ARG_NO_WILDCARD) (
  IN CHAR16               *Arg,
  IN OUT LIST_ENTRY       *ListHead
  );

/**
  This function removes duplicate file listings from lists.

  This is a function for use with SHELLENV_FILE_META_ARG_NO_WILDCARD and
  SHELLENV_FILE_META_ARG.  This function will verify that there are no duplicate
  files in the list of returned files.  Any file listed twice will have one of its
  instances removed.

  @param[in] ListHead           The pointer to linked list head that was returned from
                                SHELLENV_FILE_META_ARG_NO_WILDCARD or
                                SHELLENV_FILE_META_ARG.

  @retval EFI_SUCCESS           This function always returns success.

**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_DEL_DUP_FILE) (
  IN LIST_ENTRY   * ListHead
  );

/**
  Converts a File System map name to a device path.

  If DevPath is NULL, then ASSERT().

  This function looks through the shell environment map for a map whose Name
  matches the Name parameter.  If one is found, the device path pointer is
  updated to point to that file systems device path.  The caller should not
  free the memory from that device path.

  This function will use the internal lock to prevent changes to the map during
  the lookup operation.

  @param[in] Name               The pointer to the NULL terminated UNICODE string of the
                                file system name.
  @param[out] DevPath           The pointer to pointer to DevicePath.  Only valid on
                                successful return.

  @retval EFI_SUCCESS           The conversion was successful, and the device
                                path was returned.
  @retval EFI_NOT_FOUND         The file system could not be found in the map.
**/
typedef
EFI_STATUS
(EFIAPI *SHELLENV_GET_FS_DEVICE_PATH) (
  IN CHAR16                        *Name,
  OUT EFI_DEVICE_PATH_PROTOCOL     **DevPath
  );

/// EFI_SHELL_ENVIRONMENT2 protocol structure.
typedef struct {
  SHELLENV_EXECUTE                        Execute;
  SHELLENV_GET_ENV                        GetEnv;
  SHELLENV_GET_MAP                        GetMap;
  SHELLENV_ADD_CMD                        AddCmd;
  SHELLENV_ADD_PROT                       AddProt;
  SHELLENV_GET_PROT                       GetProt;
  SHELLENV_CUR_DIR                        CurDir;
  SHELLENV_FILE_META_ARG                  FileMetaArg;
  SHELLENV_FREE_FILE_LIST                 FreeFileList;

  //
  // The following services are only used by the shell itself.
  //
  SHELLENV_NEW_SHELL                      NewShell;
  SHELLENV_BATCH_IS_ACTIVE                BatchIsActive;

  SHELLENV_FREE_RESOURCES                 FreeResources;

  //
  // GUID to differentiate ShellEnvironment2 from ShellEnvironment.
  //
  EFI_GUID                                SESGuid;
  //
  // Major Version grows if shell environment interface has been changes.
  //
  UINT32                                  MajorVersion;
  UINT32                                  MinorVersion;
  SHELLENV_ENABLE_PAGE_BREAK              EnablePageBreak;
  SHELLENV_DISABLE_PAGE_BREAK             DisablePageBreak;
  SHELLENV_GET_PAGE_BREAK                 GetPageBreak;

  SHELLENV_SET_KEY_FILTER                 SetKeyFilter;
  SHELLENV_GET_KEY_FILTER                 GetKeyFilter;

  SHELLENV_GET_EXECUTION_BREAK            GetExecutionBreak;
  SHELLENV_INCREMENT_SHELL_NESTING_LEVEL  IncrementShellNestingLevel;
  SHELLENV_DECREMENT_SHELL_NESTING_LEVEL  DecrementShellNestingLevel;
  SHELLENV_IS_ROOT_SHELL                  IsRootShell;

  SHELLENV_CLOSE_CONSOLE_PROXY            CloseConsoleProxy;
  HANDLE_ENUMERATOR                       HandleEnumerator;
  PROTOCOL_INFO_ENUMERATOR                ProtocolInfoEnumerator;
  GET_DEVICE_NAME                         GetDeviceName;
  GET_SHELL_MODE                          GetShellMode;
  SHELLENV_NAME_TO_PATH                   NameToPath;
  SHELLENV_GET_FS_NAME                    GetFsName;
  SHELLENV_FILE_META_ARG_NO_WILDCARD      FileMetaArgNoWildCard;
  SHELLENV_DEL_DUP_FILE                   DelDupFileArg;
  SHELLENV_GET_FS_DEVICE_PATH             GetFsDevicePath;
} EFI_SHELL_ENVIRONMENT2;

extern EFI_GUID gEfiShellEnvironment2Guid;
extern EFI_GUID gEfiShellEnvironment2ExtGuid;

#endif // _SHELL_ENVIRONMENT_2_PROTOCOL_H_
