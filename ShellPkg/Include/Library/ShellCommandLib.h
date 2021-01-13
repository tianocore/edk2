/** @file
  Provides interface to shell internal functions for shell commands.

  This library is for use ONLY by shell commands linked into the shell application.
  This library will not function if it is used for UEFI Shell 2.0 Applications.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_COMMAND_LIB_
#define _SHELL_COMMAND_LIB_

#include <Uefi.h>

#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/UefiBootServicesTableLib.h>

//
// The extern global protocol poionters.
//
extern        EFI_UNICODE_COLLATION_PROTOCOL    *gUnicodeCollation;
extern        CONST CHAR16*                     SupportLevel[];

//
// The map list objects.
//
typedef struct {
  LIST_ENTRY                    Link;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  CHAR16                        *MapName;
  CHAR16                        *CurrentDirectoryPath;
  UINT64                         Flags;
} SHELL_MAP_LIST;
/// List of Mappings - DeviceName and Drive Letter(ism).
extern        SHELL_MAP_LIST                      gShellMapList;
/// Pointer to node of current directory in the mMapList.
extern        SHELL_MAP_LIST                      *gShellCurMapping;

/**
  Returns the help MAN fileName for a given shell command.

  @retval !NULL   The unicode string of the MAN filename.
  @retval NULL    An error ocurred.

**/
typedef
CONST CHAR16 *
(EFIAPI *SHELL_GET_MAN_FILENAME)(
  VOID
  );

/**
  Runs a shell command on a given command line.

  The specific operation of a given shell command is specified in the UEFI Shell
  Specification 2.0, or in the source of the given command.

  Upon completion of the command run the shell protocol and environment variables
  may have been updated due to the operation.

  @param[in] ImageHandle              The ImageHandle to the app, or NULL if
                                      the command built into shell.
  @param[in] SystemTable              The pointer to the system table.

  @retval  RETURN_SUCCESS             The shell command was sucessful.
  @retval  RETURN_UNSUPPORTED         The command is not supported.
**/
typedef
SHELL_STATUS
(EFIAPI *SHELL_RUN_COMMAND)(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Registers the handlers of type SHELL_RUN_COMMAND and
  SHELL_GET_MAN_FILENAME for each shell command.

  If the ShellSupportLevel is greater than the value of
  PcdShellSupportLevel, then return RETURN_UNSUPPORTED.

  Registers the the handlers specified by GetHelpInfoHandler and CommandHandler
  with the command specified by CommandString. If the command named by
  CommandString has already been registered, then return
  RETURN_ALREADY_STARTED.

  If there are not enough resources available to register the handlers, then
  RETURN_OUT_OF_RESOURCES is returned.

  If CommandString is NULL, then ASSERT().
  If GetHelpInfoHandler is NULL, then ASSERT().
  If CommandHandler is NULL, then ASSERT().
  If ProfileName is NULL, then ASSERT().

  @param[in]  CommandString         The pointer to the command name.  This is the
                                    name to look for on the command line in
                                    the shell.
  @param[in]  CommandHandler        The pointer to a function that runs the
                                    specified command.
  @param[in]  GetManFileName        The pointer to a function that provides man
                                    filename.
  @param[in]  ShellMinSupportLevel  The minimum Shell Support Level which has this
                                    function.
  @param[in]  ProfileName           The profile name to require for support of this
                                    function.
  @param[in]  CanAffectLE           Indicates whether this command's return value
                                    can change the LASTERROR environment variable.
  @param[in]  HiiHandle             The handle of this command's HII entry.
  @param[in]  ManFormatHelp         The HII locator for the help text.

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                    register the shell command.
  @retval RETURN_UNSUPPORTED        The ShellMinSupportLevel was higher than the
                                    currently allowed support level.
  @retval RETURN_ALREADY_STARTED    The CommandString represents a command that
                                    is already registered.  Only one handler set for
                                    a given command is allowed.
  @sa SHELL_GET_MAN_FILENAME
  @sa SHELL_RUN_COMMAND
**/
RETURN_STATUS
EFIAPI
ShellCommandRegisterCommandName (
  IN CONST  CHAR16                      *CommandString,
  IN        SHELL_RUN_COMMAND           CommandHandler,
  IN        SHELL_GET_MAN_FILENAME      GetManFileName,
  IN        UINT32                      ShellMinSupportLevel,
  IN CONST  CHAR16                      *ProfileName,
  IN CONST  BOOLEAN                     CanAffectLE,
  IN CONST  EFI_HII_HANDLE              HiiHandle,
  IN CONST  EFI_STRING_ID               ManFormatHelp
  );

/**
  Checks if a command string has been registered for CommandString, and if so, it runs
  the previously registered handler for that command with the command line.

  If CommandString is NULL, then ASSERT().

  If Sections is specified, then each section name listed will be compared in a case sensitive
  manner to the section names described in Appendix B UEFI Shell 2.0 Specification. If the section exists,
  it is appended to the returned help text. If the section does not exist, no
  information is returned. If Sections is NULL, then all help text information
  available is returned.

  @param[in]   CommandString         The pointer to the command name.  This is the name
                                     found on the command line in the shell.
  @param[in, out] RetVal             The pointer to the return value from the command handler.

  @param[in, out]  CanAffectLE       Indicates whether this command's return value
                                     needs to be placed into LASTERROR environment variable.

  @retval RETURN_SUCCESS            The handler was run.
  @retval RETURN_NOT_FOUND          The CommandString did not match a registered
                                    command name.
  @sa SHELL_RUN_COMMAND
**/
RETURN_STATUS
EFIAPI
ShellCommandRunCommandHandler (
  IN CONST CHAR16               *CommandString,
  IN OUT SHELL_STATUS           *RetVal,
  IN OUT BOOLEAN                *CanAffectLE OPTIONAL
  );

/**
  Checks if a command string has been registered for CommandString, and if so, it
  returns the MAN filename specified for that command.

  If CommandString is NULL, then ASSERT().

  @param[in]  CommandString         The pointer to the command name.  This is the name
                                    found on the command line in the shell.

  @retval NULL                      The CommandString was not a registered command.
  @retval other                     The name of the MAN file.
  @sa SHELL_GET_MAN_FILENAME
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameHandler (
  IN CONST CHAR16               *CommandString
  );


typedef struct {
  LIST_ENTRY  Link;
  CHAR16      *CommandString;
} COMMAND_LIST;

/**
  Get the list of all available shell internal commands.  This is a linked list,
  via the LIST_ENTRY structure.  Enumerate through it using the BaseLib linked
  list functions.  Do not modify the values.

  @param[in] Sort       TRUE to alphabetically sort the values first.  FALSE otherwise.

  @return A linked list of all available shell commands.
**/
CONST COMMAND_LIST*
EFIAPI
ShellCommandGetCommandList (
  IN CONST BOOLEAN Sort
  );

typedef struct {
  LIST_ENTRY  Link;
  CHAR16      *CommandString;
  CHAR16      *Alias;
} ALIAS_LIST;

/**
  Registers aliases to be set as part of the initialization of the shell application.

  If Command is NULL, then ASSERT().
  If Alias is NULL, then ASSERT().

  @param[in]  Command               The pointer to the Command.
  @param[in]  Alias                 The pointer to Alias.

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                    register the shell command.
**/
RETURN_STATUS
EFIAPI
ShellCommandRegisterAlias (
  IN CONST CHAR16                       *Command,
  IN CONST CHAR16                       *Alias
  );

/**
  Get the list of all shell alias commands.  This is a linked list,
  via LIST_ENTRY structure.  Enumerate through it using the BaseLib linked
  list functions.  Do not modify the values.

  @return A linked list of all requested shell aliases.
**/
CONST ALIAS_LIST*
EFIAPI
ShellCommandGetInitAliasList (
  VOID
  );

/**
  Determine if a given alias is on the list of built in aliases.

  @param[in] Alias              The alias to test for.

  @retval TRUE                  The alias is a built in alias.
  @retval FALSE                 The alias is not a built in alias.
**/
BOOLEAN
EFIAPI
ShellCommandIsOnAliasList (
  IN CONST CHAR16 *Alias
  );

/**
  Checks if a command is already on the list.

  @param[in] CommandString        The command string to check for on the list.

  @retval TRUE  CommandString represents a registered command.
  @retval FALSE CommandString does not represent a registered command.
**/
BOOLEAN
EFIAPI
ShellCommandIsCommandOnList (
  IN CONST  CHAR16                      *CommandString
  );

/**
  Get the help text for a command.

  @param[in] CommandString        The command name.

  @retval NULL  No help text was found.
  @return       The string of the help text.  The caller required to free.
**/
CHAR16*
EFIAPI
ShellCommandGetCommandHelp (
  IN CONST  CHAR16                      *CommandString
  );

/**
  Function to make sure that the above pointers are valid.
**/
EFI_STATUS
EFIAPI
CommandInit (
  VOID
  );

/**
  Function to determine current state of ECHO.  Echo determines if lines from scripts
  and ECHO commands are enabled.

  @retval TRUE    Echo is currently enabled.
  @retval FALSE   Echo is currently disabled.
**/
BOOLEAN
EFIAPI
ShellCommandGetEchoState (
  VOID
  );

/**
  Function to set current state of ECHO.  Echo determines if lines from scripts
  and ECHO commands are enabled.

  @param[in] State    TRUE to enable Echo, FALSE otherwise.
**/
VOID
EFIAPI
ShellCommandSetEchoState (
  IN BOOLEAN State
  );



/**
  Indicate that the current shell or script should exit.

  @param[in] ScriptOnly   TRUE if exiting a script; FALSE otherwise.
  @param[in] ErrorCode    The 64 bit error code to return.
**/
VOID
EFIAPI
ShellCommandRegisterExit (
  IN BOOLEAN      ScriptOnly,
  IN CONST UINT64 ErrorCode
  );

/**
  Retrieve the Exit code.

  @return the value passed into RegisterExit.
**/
UINT64
EFIAPI
ShellCommandGetExitCode (
  VOID
  );

/**
  Retrieve the Exit indicator.

  @retval TRUE      Exit was indicated.
  @retval FALSE     Exit was not indicated.
**/
BOOLEAN
EFIAPI
ShellCommandGetExit (
  VOID
  );

/**
  Retrieve the Exit script indicator.

  If ShellCommandGetExit returns FALSE, then the return from this is undefined.

  @retval TRUE      ScriptOnly was indicated.
  @retval FALSE     ScriptOnly was not indicated.
**/
BOOLEAN
EFIAPI
ShellCommandGetScriptExit (
  VOID
  );

typedef struct {
  LIST_ENTRY      Link;     ///< List enumerator items.
  UINTN           Line;     ///< What line of the script file this was on.
  CHAR16          *Cl;      ///< The original command line.
  VOID            *Data;    ///< The data structure format dependant upon Command. (not always used)
  BOOLEAN         Reset;    ///< Reset the command (it must be treated like a initial run (but it may have data already))
} SCRIPT_COMMAND_LIST;

typedef struct {
  CHAR16              *ScriptName;        ///< The filename of this script.
  CHAR16              **Argv;             ///< The parmameters to the script file.
  UINTN               Argc;               ///< The count of parameters.
  LIST_ENTRY          CommandList;        ///< The script converted to a list of commands (SCRIPT_COMMAND_LIST objects).
  SCRIPT_COMMAND_LIST *CurrentCommand;    ///< The command currently being operated.  If !=NULL must be a member of CommandList.
  LIST_ENTRY          SubstList;          ///< A list of current script loop alias' (ALIAS_LIST objects) (Used for the for %-based replacement).
} SCRIPT_FILE;

/**
  Function to return a pointer to the currently running script file object.

  @retval NULL        A script file is not currently running.
  @return             A pointer to the current script file object.
**/
SCRIPT_FILE*
EFIAPI
ShellCommandGetCurrentScriptFile (
  VOID
  );

/**
  Function to set a new script as the currently running one.

  This function will correctly stack and unstack nested scripts.

  @param[in] Script   The pointer to new script information structure.  If NULL,
                      it removes and de-allocates the topmost Script structure.

  @return             A pointer to the current running script file after this
                      change.  It is NULL if removing the final script.
**/
SCRIPT_FILE*
EFIAPI
ShellCommandSetNewScript (
  IN SCRIPT_FILE *Script OPTIONAL
  );

/**
  Function to cleanup all memory from a SCRIPT_FILE structure.

  @param[in] Script     The pointer to the structure to cleanup.
**/
VOID
EFIAPI
DeleteScriptFileStruct (
  IN SCRIPT_FILE *Script
  );

/**
  Function to get the current Profile string.

  This is used to retrieve what profiles were installed.

  @retval NULL  There are no installed profiles.
  @return       A semicolon-delimited list of profiles.
**/
CONST CHAR16 *
EFIAPI
ShellCommandGetProfileList (
  VOID
  );

typedef enum {
  MappingTypeFileSystem,
  MappingTypeBlockIo,
  MappingTypeMax
} SHELL_MAPPING_TYPE;

/**
  Function to generate the next default mapping name.

  If the return value is not NULL then it must be callee freed.

  @param Type                   What kind of mapping name to make.

  @retval NULL                  a memory allocation failed.
  @return a new map name string
**/
CHAR16*
EFIAPI
ShellCommandCreateNewMappingName(
  IN CONST SHELL_MAPPING_TYPE Type
  );

/**
  Function to initialize the table for creating consistent map names.

  @param[out] Table             The pointer to pointer to pointer to DevicePathProtocol object.

  @retval EFI_SUCCESS           The table was created successfully.
**/
EFI_STATUS
EFIAPI
ShellCommandConsistMappingInitialize (
  EFI_DEVICE_PATH_PROTOCOL           ***Table
  );

/**
  Function to uninitialize the table for creating consistent map names.

  The parameter must have been received from ShellCommandConsistMappingInitialize.

  @param[out] Table             The pointer to pointer to DevicePathProtocol object.

  @retval EFI_SUCCESS           The table was deleted successfully.
**/
EFI_STATUS
EFIAPI
ShellCommandConsistMappingUnInitialize (
  EFI_DEVICE_PATH_PROTOCOL      **Table
  );

/**
  Create a consistent mapped name for the device specified by DevicePath
  based on the Table.

  This must be called after ShellCommandConsistMappingInitialize() and
  before ShellCommandConsistMappingUnInitialize() is called.

  @param[in] DevicePath   The pointer to the dev path for the device.
  @param[in] Table        The Table of mapping information.

  @retval NULL            A consistent mapped name could not be created.
  @return                 A pointer to a string allocated from pool with the device name.
**/
CHAR16*
EFIAPI
ShellCommandConsistMappingGenMappingName (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL      **Table
  );

/**
  Function to search the list of mappings for the first matching node on the
  list based on the MapKey.

  @param[in] MapKey             The pointer to the string key to search for in the map.

  @return the node on the list.
**/
SHELL_MAP_LIST*
EFIAPI
ShellCommandFindMapItem (
  IN CONST CHAR16               *MapKey
  );

/**
  Function to add a map node to the list of map items and update the "path" environment variable (optionally).

  If Path is TRUE (during initialization only), the path environment variable will also be updated to include
  default paths on the new map name...

  Path should be FALSE when this function is called from the protocol SetMap function.

  @param[in] Name               The human readable mapped name.
  @param[in] DevicePath         The Device Path for this map.
  @param[in] Flags              The Flags attribute for this map item.
  @param[in] Path               TRUE to update path, FALSE to skip this step (should only be TRUE during initialization).

  @retval EFI_SUCCESS           The addition was sucessful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
EFIAPI
ShellCommandAddMapItemAndUpdatePath(
  IN CONST CHAR16                   *Name,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST UINT64                   Flags,
  IN CONST BOOLEAN                  Path
  );

/**
  Creates the default map names for each device path in the system with
  a protocol depending on the Type.

  Also sets up the default path environment variable if Type is FileSystem.

  @retval EFI_SUCCESS           All map names were created sucessfully.
  @retval EFI_NOT_FOUND         No protocols were found in the system.
  @return                       Error returned from gBS->LocateHandle().

  @sa LocateHandle
**/
EFI_STATUS
EFIAPI
ShellCommandCreateInitialMappingsAndPaths(
  VOID
  );

/**
  Add mappings for any devices without one.  Do not change any existing maps.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellCommandUpdateMapping (
  VOID
  );

/**
  Converts a SHELL_FILE_HANDLE to an EFI_FILE_PROTOCOL*.

  @param[in] Handle     The SHELL_FILE_HANDLE to convert.

  @return a EFI_FILE_PROTOCOL* representing the same file.
**/
EFI_FILE_PROTOCOL*
EFIAPI
ConvertShellHandleToEfiFileProtocol(
  IN CONST SHELL_FILE_HANDLE Handle
  );

/**
  Remove a SHELL_FILE_HANDLE frmo the list of SHELL_FILE_HANDLES.

  @param[in] Handle     The SHELL_FILE_HANDLE to remove.

  @retval TRUE          The item was removed.
  @retval FALSE         The item was not found.
**/
BOOLEAN
EFIAPI
ShellFileHandleRemove(
  IN CONST SHELL_FILE_HANDLE Handle
  );

/**
  Converts a EFI_FILE_PROTOCOL* to an SHELL_FILE_HANDLE.

  @param[in] Handle     The pointer to EFI_FILE_PROTOCOL to convert.
  @param[in] Path       The path to the file for verification.

  @return a SHELL_FILE_HANDLE representing the same file.
**/
SHELL_FILE_HANDLE
EFIAPI
ConvertEfiFileProtocolToShellHandle(
  IN CONST EFI_FILE_PROTOCOL *Handle,
  IN CONST CHAR16            *Path
  );

/**
  Find the path that was logged with the specified SHELL_FILE_HANDLE.

  @param[in] Handle     The SHELL_FILE_HANDLE to query on.

  @return A pointer to the path for the file.
**/
CONST CHAR16*
EFIAPI
ShellFileHandleGetPath(
  IN CONST SHELL_FILE_HANDLE Handle
  );


/**
  Function to determine if a SHELL_FILE_HANDLE is at the end of the file.

  This will NOT work on directories.

  If Handle is NULL, then ASSERT.

  @param[in] Handle     the file handle

  @retval TRUE          the position is at the end of the file
  @retval FALSE         the position is not at the end of the file
**/
BOOLEAN
EFIAPI
ShellFileHandleEof(
  IN SHELL_FILE_HANDLE Handle
  );

typedef struct {
  LIST_ENTRY    Link;
  void          *Buffer;
} BUFFER_LIST;

/**
  Frees any BUFFER_LIST defined type.

  @param[in] List   The pointer to the list head.
**/
VOID
EFIAPI
FreeBufferList (
  IN BUFFER_LIST *List
  );

/**
  Function printing hex output to the console.

  @param[in] Indent       Number of spaces to indent.
  @param[in] Offset       Offset to start with.
  @param[in] DataSize     Length of data.
  @param[in] UserData     Pointer to some data.
**/
VOID
EFIAPI
DumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  );

/**
  Dump HEX data into buffer.

  @param[in] Buffer     HEX data to be dumped in Buffer.
  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
CHAR16*
EFIAPI
CatSDumpHex (
  IN CHAR16  *Buffer,
  IN UINTN   Indent,
  IN UINTN   Offset,
  IN UINTN   DataSize,
  IN VOID    *UserData
  );

//
// Determines the ordering operation for ShellSortFileList().
//
typedef enum {
  //
  // Sort the EFI_SHELL_FILE_INFO objects by the FileName field, in increasing
  // order, using gUnicodeCollation->StriColl().
  //
  ShellSortFileListByFileName,
  //
  // Sort the EFI_SHELL_FILE_INFO objects by the FullName field, in increasing
  // order, using gUnicodeCollation->StriColl().
  //
  ShellSortFileListByFullName,
  ShellSortFileListMax
} SHELL_SORT_FILE_LIST;

/**
  Sort an EFI_SHELL_FILE_INFO list, optionally moving duplicates to a separate
  list.

  @param[in,out] FileList  The list of EFI_SHELL_FILE_INFO objects to sort.

                           If FileList is NULL on input, then FileList is
                           considered an empty, hence already sorted, list.

                           Otherwise, if (*FileList) is NULL on input, then
                           EFI_INVALID_PARAMETER is returned.

                           Otherwise, the caller is responsible for having
                           initialized (*FileList)->Link with
                           InitializeListHead(). No other fields in the
                           (**FileList) head element are accessed by this
                           function.

                           On output, (*FileList) is sorted according to Order.
                           If Duplicates is NULL on input, then duplicate
                           elements are preserved, sorted stably, on
                           (*FileList). If Duplicates is not NULL on input,
                           then duplicates are moved (stably sorted) to the
                           new, dynamically allocated (*Duplicates) list.

  @param[out] Duplicates   If Duplicates is NULL on input, (*FileList) will be
                           a monotonically ordered list on output, with
                           duplicates stably sorted.

                           If Duplicates is not NULL on input, (*FileList) will
                           be a strictly monotonically oredered list on output,
                           with duplicates separated (stably sorted) to
                           (*Duplicates). All fields except Link will be
                           zero-initialized in the (**Duplicates) head element.
                           If no duplicates exist, then (*Duplicates) is set to
                           NULL on output.

  @param[in] Order         Determines the comparison operation between
                           EFI_SHELL_FILE_INFO objects.

  @retval EFI_INVALID_PARAMETER  (UINTN)Order is greater than or equal to
                                 (UINTN)ShellSortFileListMax. Neither the
                                 (*FileList) nor the (*Duplicates) list has
                                 been modified.

  @retval EFI_INVALID_PARAMETER  (*FileList) was NULL on input. Neither the
                                 (*FileList) nor the (*Duplicates) list has
                                 been modified.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed. Neither the
                                 (*FileList) nor the (*Duplicates) list has
                                 been modified.

  @retval EFI_SUCCESS            Sorting successful, including the case when
                                 FileList is NULL on input.
**/
EFI_STATUS
EFIAPI
ShellSortFileList (
  IN OUT EFI_SHELL_FILE_INFO  **FileList,
     OUT EFI_SHELL_FILE_INFO  **Duplicates OPTIONAL,
  IN     SHELL_SORT_FILE_LIST Order
  );
#endif //_SHELL_COMMAND_LIB_
