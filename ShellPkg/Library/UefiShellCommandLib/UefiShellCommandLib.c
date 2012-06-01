/** @file
  Provides interface to shell internal functions for shell commands.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellCommandLib.h"

/// The tag for use in identifying UNICODE files.
/// If the file is UNICODE, the first 16 bits of the file will equal this value.
enum {
  gUnicodeFileTag = 0xFEFF
};

// STATIC local variables
STATIC SHELL_COMMAND_INTERNAL_LIST_ENTRY  mCommandList;
STATIC SCRIPT_FILE_LIST                   mScriptList;
STATIC ALIAS_LIST                         mAliasList;
STATIC BOOLEAN                            mEchoState;
STATIC BOOLEAN                            mExitRequested;
STATIC UINT64                             mExitCode;
STATIC BOOLEAN                            mExitScript;
STATIC CHAR16                             *mProfileList;
STATIC UINTN                              mProfileListSize;
STATIC UINTN                              mFsMaxCount = 0;
STATIC UINTN                              mBlkMaxCount = 0;
STATIC BUFFER_LIST                        mFileHandleList;

// global variables required by library class.
EFI_UNICODE_COLLATION_PROTOCOL    *gUnicodeCollation            = NULL;
EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *gDevPathToText               = NULL;
SHELL_MAP_LIST                    gShellMapList;
SHELL_MAP_LIST                    *gShellCurDir                 = NULL;

CONST CHAR16* SupportLevel[] = {
  L"Minimal",
  L"Scripting",
  L"Basic",
  L"Interactive"
};

/**
  Function to make sure that the global protocol pointers are valid.
  must be called after constructor before accessing the pointers.
**/
EFI_STATUS
EFIAPI
CommandInit(
  VOID
  )
{
  EFI_STATUS Status;
  if (gUnicodeCollation == NULL) {
    Status = gBS->LocateProtocol(&gEfiUnicodeCollation2ProtocolGuid, NULL, (VOID**)&gUnicodeCollation);
    if (EFI_ERROR(Status)) {
      return (EFI_DEVICE_ERROR);
    }
  }
  if (gDevPathToText == NULL) {
    Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID**)&gDevPathToText);
    if (EFI_ERROR(Status)) {
      return (EFI_DEVICE_ERROR);
    }
  }
  return (EFI_SUCCESS);
}

/**
  Constructor for the Shell Command library.

  Initialize the library and determine if the underlying is a UEFI Shell 2.0 or an EFI shell.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS   the initialization was complete sucessfully
**/
RETURN_STATUS
EFIAPI
ShellCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  InitializeListHead(&gShellMapList.Link);
  InitializeListHead(&mCommandList.Link);
  InitializeListHead(&mAliasList.Link);
  InitializeListHead(&mScriptList.Link);
  InitializeListHead(&mFileHandleList.Link);
  mEchoState = TRUE;

  mExitRequested    = FALSE;
  mExitScript       = FALSE;
  mProfileListSize  = 0;
  mProfileList      = NULL;

  if (gUnicodeCollation == NULL) {
    Status = gBS->LocateProtocol(&gEfiUnicodeCollation2ProtocolGuid, NULL, (VOID**)&gUnicodeCollation);
    if (EFI_ERROR(Status)) {
      return (EFI_DEVICE_ERROR);
    }
  }

  return (RETURN_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval RETURN_SUCCESS this function always returns success
**/
RETURN_STATUS
EFIAPI
ShellCommandLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;
  COMMAND_LIST                      *Node2;
  SCRIPT_FILE_LIST                  *Node3;
  SHELL_MAP_LIST                    *MapNode;
  //
  // enumerate throught the list and free all the memory
  //
  while (!IsListEmpty (&mCommandList.Link)) {
    Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode(&mCommandList.Link);
    RemoveEntryList(&Node->Link);
    SHELL_FREE_NON_NULL(Node->CommandString);
    FreePool(Node);
    DEBUG_CODE(Node = NULL;);
  }

  //
  // enumerate through the init command list and free all memory
  //
  while (!IsListEmpty (&mAliasList.Link)) {
    Node2 = (COMMAND_LIST *)GetFirstNode(&mAliasList.Link);
    RemoveEntryList(&Node2->Link);
    SHELL_FREE_NON_NULL(Node2->CommandString);
    FreePool(Node2);
    DEBUG_CODE(Node2 = NULL;);
  }

  //
  // enumerate throught the list and free all the memory
  //
  while (!IsListEmpty (&mScriptList.Link)) {
    Node3 = (SCRIPT_FILE_LIST *)GetFirstNode(&mScriptList.Link);
    RemoveEntryList(&Node3->Link);
    DeleteScriptFileStruct(Node3->Data);
    FreePool(Node3);
  }

  //
  // enumerate throught the mappings list and free all the memory
  //
  if (!IsListEmpty(&gShellMapList.Link)) {
    for (MapNode = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
         ; !IsListEmpty (&gShellMapList.Link)
         ; MapNode = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
        ){
      ASSERT(MapNode != NULL);
      RemoveEntryList(&MapNode->Link);
      SHELL_FREE_NON_NULL(MapNode->DevicePath);
      SHELL_FREE_NON_NULL(MapNode->MapName);
      SHELL_FREE_NON_NULL(MapNode->CurrentDirectoryPath);
      FreePool(MapNode);
    }
  }
  if (!IsListEmpty(&mFileHandleList.Link)){
    FreeBufferList(&mFileHandleList);
  }

  if (mProfileList != NULL) {
    FreePool(mProfileList);
  }

  gUnicodeCollation            = NULL;
  gDevPathToText               = NULL;
  gShellCurDir                 = NULL;

  return (RETURN_SUCCESS);
}

/**
  Checks if a command is already on the list.

  @param[in] CommandString        The command string to check for on the list.
**/
BOOLEAN
EFIAPI
ShellCommandIsCommandOnList (
  IN CONST  CHAR16                      *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;

  //
  // assert for NULL parameter
  //
  ASSERT(CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode(&mCommandList.Link)
      ; !IsNull(&mCommandList.Link, &Node->Link)
      ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode(&mCommandList.Link, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)CommandString,
          Node->CommandString) == 0
       ){
      return (TRUE);
    }
  }
  return (FALSE);
}

/**
  Get the help text for a command.

  @param[in] CommandString        The command name.

  @retval NULL  No help text was found.
  @return       String of help text. Caller reuiqred to free.
**/
CHAR16*
EFIAPI
ShellCommandGetCommandHelp (
  IN CONST  CHAR16                      *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;

  //
  // assert for NULL parameter
  //
  ASSERT(CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode(&mCommandList.Link)
      ; !IsNull(&mCommandList.Link, &Node->Link)
      ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode(&mCommandList.Link, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)CommandString,
          Node->CommandString) == 0
       ){
      return (HiiGetString(Node->HiiHandle, Node->ManFormatHelp, NULL));
    }
  }
  return (NULL);
}

/**
  Registers handlers of type SHELL_RUN_COMMAND and
  SHELL_GET_MAN_FILENAME for each shell command.

  If the ShellSupportLevel is greater than the value of the
  PcdShellSupportLevel then return RETURN_UNSUPPORTED.

  Registers the handlers specified by GetHelpInfoHandler and CommandHandler
  with the command specified by CommandString. If the command named by
  CommandString has already been registered, then return
  RETURN_ALREADY_STARTED.

  If there are not enough resources available to register the handlers then
  RETURN_OUT_OF_RESOURCES is returned.

  If CommandString is NULL, then ASSERT().
  If GetHelpInfoHandler is NULL, then ASSERT().
  If CommandHandler is NULL, then ASSERT().
  If ProfileName is NULL, then ASSERT().

  @param[in]  CommandString         Pointer to the command name.  This is the
                                    name to look for on the command line in
                                    the shell.
  @param[in]  CommandHandler        Pointer to a function that runs the
                                    specified command.
  @param[in]  GetManFileName        Pointer to a function that provides man
                                    filename.
  @param[in]  ShellMinSupportLevel  minimum Shell Support Level which has this
                                    function.
  @param[in]  ProfileName           profile name to require for support of this
                                    function.
  @param[in]  CanAffectLE           indicates whether this command's return value
                                    can change the LASTERROR environment variable.
  @param[in]  HiiHandle             Handle of this command's HII entry.
  @param[in]  ManFormatHelp         HII locator for the help text.

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                    register the shell command.
  @retval RETURN_UNSUPPORTED        the ShellMinSupportLevel was higher than the
                                    currently allowed support level.
  @retval RETURN_ALREADY_STARTED    The CommandString represents a command that
                                    is already registered.  Only 1 handler set for
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
  IN CONST  EFI_HANDLE                  HiiHandle,
  IN CONST  EFI_STRING_ID               ManFormatHelp
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;

  //
  // ASSERTs for NULL parameters
  //
  ASSERT(CommandString  != NULL);
  ASSERT(GetManFileName != NULL);
  ASSERT(CommandHandler != NULL);
  ASSERT(ProfileName    != NULL);

  //
  // check for shell support level
  //
  if (PcdGet8(PcdShellSupportLevel) < ShellMinSupportLevel) {
    return (RETURN_UNSUPPORTED);
  }

  //
  // check for already on the list
  //
  if (ShellCommandIsCommandOnList(CommandString)) {
    return (RETURN_ALREADY_STARTED);
  }

  //
  // allocate memory for new struct
  //
  Node = AllocateZeroPool(sizeof(SHELL_COMMAND_INTERNAL_LIST_ENTRY));
  ASSERT(Node != NULL);
  Node->CommandString = AllocateZeroPool(StrSize(CommandString));
  ASSERT(Node->CommandString != NULL);

  //
  // populate the new struct
  //
  StrCpy(Node->CommandString, CommandString);

  Node->GetManFileName  = GetManFileName;
  Node->CommandHandler  = CommandHandler;
  Node->LastError       = CanAffectLE;
  Node->HiiHandle       = HiiHandle;
  Node->ManFormatHelp   = ManFormatHelp;

  if ( StrLen(ProfileName)>0
    && ((mProfileList != NULL
    && StrStr(mProfileList, ProfileName) == NULL) || mProfileList == NULL)
   ){
    ASSERT((mProfileList == NULL && mProfileListSize == 0) || (mProfileList != NULL));
    if (mProfileList == NULL) {
      //
      // If this is the first make a leading ';'
      //
      StrnCatGrow(&mProfileList, &mProfileListSize, L";", 0);
    }
    StrnCatGrow(&mProfileList, &mProfileListSize, ProfileName, 0);
    StrnCatGrow(&mProfileList, &mProfileListSize, L";", 0);
  }

  //
  // add the new struct to the list
  //
  InsertTailList (&mCommandList.Link, &Node->Link);

  return (RETURN_SUCCESS);
}

/**
  Function to get the current Profile string.

  @retval NULL  There are no installed profiles.
  @return       A semi-colon delimited list of profiles.
**/
CONST CHAR16 *
EFIAPI
ShellCommandGetProfileList (
  VOID
  )
{
  return (mProfileList);
}

/**
  Checks if a command string has been registered for CommandString and if so it runs
  the previously registered handler for that command with the command line.

  If CommandString is NULL, then ASSERT().

  If Sections is specified, then each section name listed will be compared in a casesensitive
  manner, to the section names described in Appendix B UEFI Shell 2.0 spec. If the section exists,
  it will be appended to the returned help text. If the section does not exist, no
  information will be returned. If Sections is NULL, then all help text information
  available will be returned.

  @param[in]  CommandString          Pointer to the command name.  This is the name
                                     found on the command line in the shell.
  @param[in, out] RetVal             Pointer to the return vaule from the command handler.

  @param[in, out]  CanAffectLE       indicates whether this command's return value
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
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;

  //
  // assert for NULL parameters
  //
  ASSERT(CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode(&mCommandList.Link)
      ; !IsNull(&mCommandList.Link, &Node->Link)
      ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode(&mCommandList.Link, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)CommandString,
          Node->CommandString) == 0
       ){
      if (CanAffectLE != NULL) {
        *CanAffectLE = Node->LastError;
      }
      if (RetVal != NULL) {
        *RetVal = Node->CommandHandler(NULL, gST);
      } else {
        Node->CommandHandler(NULL, gST);
      }
      return (RETURN_SUCCESS);
    }
  }
  return (RETURN_NOT_FOUND);
}

/**
  Checks if a command string has been registered for CommandString and if so it
  returns the MAN filename specified for that command.

  If CommandString is NULL, then ASSERT().

  @param[in]  CommandString         Pointer to the command name.  This is the name
                                    found on the command line in the shell.\

  @retval NULL                      the commandString was not a registered command.
  @return other                     the name of the MAN file.
  @sa SHELL_GET_MAN_FILENAME
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameHandler (
  IN CONST CHAR16               *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY *Node;

  //
  // assert for NULL parameters
  //
  ASSERT(CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode(&mCommandList.Link)
      ; !IsNull(&mCommandList.Link, &Node->Link)
      ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode(&mCommandList.Link, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)CommandString,
          Node->CommandString) == 0
       ){
      return (Node->GetManFileName());
    }
  }
  return (NULL);
}

/**
  Get the list of all available shell internal commands.  This is a linked list
  (via LIST_ENTRY structure).  enumerate through it using the BaseLib linked
  list functions.  do not modify the values.

  @param[in] Sort       TRUE to alphabetically sort the values first.  FALSE otherwise.

  @return a Linked list of all available shell commands.
**/
CONST COMMAND_LIST*
EFIAPI
ShellCommandGetCommandList (
  IN CONST BOOLEAN Sort
  )
{
//  if (!Sort) {
//    return ((COMMAND_LIST*)(&mCommandList));
//  }
  return ((COMMAND_LIST*)(&mCommandList));
}

/**
  Registers aliases to be set as part of the initialization of the shell application.

  If Command is NULL, then ASSERT().
  If Alias is NULL, then ASSERT().

  @param[in]  Command               Pointer to the Command
  @param[in]  Alias                 Pointer to Alias

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                    register the shell command.
**/
RETURN_STATUS
EFIAPI
ShellCommandRegisterAlias (
  IN CONST CHAR16                       *Command,
  IN CONST CHAR16                       *Alias
  )
{
  ALIAS_LIST *Node;

  //
  // Asserts for NULL
  //
  ASSERT(Command != NULL);
  ASSERT(Alias   != NULL);

  //
  // allocate memory for new struct
  //
  Node = AllocateZeroPool(sizeof(ALIAS_LIST));
  ASSERT(Node != NULL);
  Node->CommandString = AllocateZeroPool(StrSize(Command));
  Node->Alias = AllocateZeroPool(StrSize(Alias));
  ASSERT(Node->CommandString != NULL);
  ASSERT(Node->Alias != NULL);

  //
  // populate the new struct
  //
  StrCpy(Node->CommandString, Command);
  StrCpy(Node->Alias        , Alias );

  //
  // add the new struct to the list
  //
  InsertTailList (&mAliasList.Link, &Node->Link);

  return (RETURN_SUCCESS);
}

/**
  Get the list of all shell alias commands.  This is a linked list
  (via LIST_ENTRY structure).  enumerate through it using the BaseLib linked
  list functions.  do not modify the values.

  @return a Linked list of all requested shell alias'.
**/
CONST ALIAS_LIST*
EFIAPI
ShellCommandGetInitAliasList (
  VOID
  )
{
    return (&mAliasList);
}

/**
  Determine if a given alias is on the list of built in alias'.

  @param[in] Alias              The alias to test for

  @retval TRUE                  The alias is a built in alias
  @retval FALSE                 The alias is not a built in alias
**/
BOOLEAN
EFIAPI
ShellCommandIsOnAliasList(
  IN CONST CHAR16 *Alias
  )
{
  ALIAS_LIST *Node;

  //
  // assert for NULL parameter
  //
  ASSERT(Alias != NULL);

  //
  // check for the Alias
  //
  for ( Node = (ALIAS_LIST *)GetFirstNode(&mAliasList.Link)
      ; !IsNull(&mAliasList.Link, &Node->Link)
      ; Node = (ALIAS_LIST *)GetNextNode(&mAliasList.Link, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    ASSERT(Node->Alias != NULL);
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)Alias,
          Node->CommandString) == 0
       ){
      return (TRUE);
    }
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          (CHAR16*)Alias,
          Node->Alias) == 0
       ){
      return (TRUE);
    }
  }
  return (FALSE);
}

/**
  Function to determine current state of ECHO.  Echo determins if lines from scripts
  and ECHO commands are enabled.

  @retval TRUE    Echo is currently enabled
  @retval FALSE   Echo is currently disabled
**/
BOOLEAN
EFIAPI
ShellCommandGetEchoState(
  VOID
  )
{
  return (mEchoState);
}

/**
  Function to set current state of ECHO.  Echo determins if lines from scripts
  and ECHO commands are enabled.

  If State is TRUE, Echo will be enabled.
  If State is FALSE, Echo will be disabled.

  @param[in] State      How to set echo.
**/
VOID
EFIAPI
ShellCommandSetEchoState(
  IN BOOLEAN State
  )
{
  mEchoState = State;
}

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
  )
{
  mExitRequested = (BOOLEAN)(!mExitRequested);
  if (mExitRequested) {
    mExitScript    = ScriptOnly;
  } else {
    mExitScript    = FALSE;
  }
  mExitCode = ErrorCode;
}

/**
  Retrieve the Exit indicator.

  @retval TRUE      Exit was indicated.
  @retval FALSE     Exis was not indicated.
**/
BOOLEAN
EFIAPI
ShellCommandGetExit (
  VOID
  )
{
  return (mExitRequested);
}

/**
  Retrieve the Exit code.

  If ShellCommandGetExit returns FALSE than the return from this is undefined.

  @return the value passed into RegisterExit.
**/
UINT64
EFIAPI
ShellCommandGetExitCode (
  VOID
  )
{
  return (mExitCode);
}
/**
  Retrieve the Exit script indicator.

  If ShellCommandGetExit returns FALSE than the return from this is undefined.

  @retval TRUE      ScriptOnly was indicated.
  @retval FALSE     ScriptOnly was not indicated.
**/
BOOLEAN
EFIAPI
ShellCommandGetScriptExit (
  VOID
  )
{
  return (mExitScript);
}

/**
  Function to cleanup all memory from a SCRIPT_FILE structure.

  @param[in] Script     The pointer to the structure to cleanup.
**/
VOID
EFIAPI
DeleteScriptFileStruct (
  IN SCRIPT_FILE *Script
  )
{
  UINT8       LoopVar;

  if (Script == NULL) {
    return;
  }

  for (LoopVar = 0 ; LoopVar < Script->Argc ; LoopVar++) {
    SHELL_FREE_NON_NULL(Script->Argv[LoopVar]);
  }
  if (Script->Argv != NULL) {
    SHELL_FREE_NON_NULL(Script->Argv);
  }
  Script->CurrentCommand = NULL;
  while (!IsListEmpty (&Script->CommandList)) {
    Script->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetFirstNode(&Script->CommandList);
    if (Script->CurrentCommand != NULL) {
      RemoveEntryList(&Script->CurrentCommand->Link);
      if (Script->CurrentCommand->Cl != NULL) {
        SHELL_FREE_NON_NULL(Script->CurrentCommand->Cl);
      }
      if (Script->CurrentCommand->Data != NULL) {
        SHELL_FREE_NON_NULL(Script->CurrentCommand->Data);
      }
      SHELL_FREE_NON_NULL(Script->CurrentCommand);
    }
  }
  SHELL_FREE_NON_NULL(Script->ScriptName);
  SHELL_FREE_NON_NULL(Script);
}

/**
  Function to return a pointer to the currently running script file object.

  @retval NULL        A script file is not currently running.
  @return             A pointer to the current script file object.
**/
SCRIPT_FILE*
EFIAPI
ShellCommandGetCurrentScriptFile (
  VOID
  )
{
  SCRIPT_FILE_LIST *List;
  if (IsListEmpty (&mScriptList.Link)) {
    return (NULL);
  }
  List = ((SCRIPT_FILE_LIST*)GetFirstNode(&mScriptList.Link));
  return (List->Data);
}

/**
  Function to set a new script as the currently running one.

  This function will correctly stack and unstack nested scripts.

  @param[in] Script   Pointer to new script information structure.  if NULL
                      will remove and de-allocate the top-most Script structure.

  @return             A pointer to the current running script file after this
                      change.  NULL if removing the final script.
**/
SCRIPT_FILE*
EFIAPI
ShellCommandSetNewScript (
  IN SCRIPT_FILE *Script OPTIONAL
  )
{
  SCRIPT_FILE_LIST *Node;
  if (Script == NULL) {
    if (IsListEmpty (&mScriptList.Link)) {
      return (NULL);
    }
    Node = (SCRIPT_FILE_LIST *)GetFirstNode(&mScriptList.Link);
    RemoveEntryList(&Node->Link);
    DeleteScriptFileStruct(Node->Data);
    FreePool(Node);
  } else {
    Node = AllocateZeroPool(sizeof(SCRIPT_FILE_LIST));
    if (Node == NULL) {
      return (NULL);
    }
    Node->Data = Script;
    InsertHeadList(&mScriptList.Link, &Node->Link);
  }
  return (ShellCommandGetCurrentScriptFile());
}

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
  )
{
  CHAR16  *String;
  ASSERT(Type < MappingTypeMax);

  String = NULL;

  String = AllocateZeroPool(PcdGet8(PcdShellMapNameLength) * sizeof(String[0]));
  UnicodeSPrint(
    String,
    PcdGet8(PcdShellMapNameLength) * sizeof(String[0]),
    Type == MappingTypeFileSystem?L"FS%d:":L"BLK%d:",
    Type == MappingTypeFileSystem?mFsMaxCount++:mBlkMaxCount++);

  return (String);
}

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
  )
{
  EFI_STATUS      Status;
  SHELL_MAP_LIST  *MapListNode;
  CONST CHAR16    *OriginalPath;
  CHAR16          *NewPath;
  UINTN           NewPathSize;

  NewPathSize = 0;
  NewPath = NULL;
  OriginalPath = NULL;
  Status = EFI_SUCCESS;

  MapListNode = AllocateZeroPool(sizeof(SHELL_MAP_LIST));
  if (MapListNode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    MapListNode->Flags = Flags;
    MapListNode->MapName = AllocateZeroPool(StrSize(Name));
    MapListNode->DevicePath = DuplicateDevicePath(DevicePath);
    if ((MapListNode->MapName == NULL) || (MapListNode->DevicePath == NULL)){
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      StrCpy(MapListNode->MapName, Name);
      InsertTailList(&gShellMapList.Link, &MapListNode->Link);
    }
  }
  if (EFI_ERROR(Status)) {
    if (MapListNode != NULL) {
      if (MapListNode->DevicePath != NULL) {
        FreePool(MapListNode->DevicePath);
      }
      if (MapListNode->MapName != NULL) {
        FreePool(MapListNode->MapName);
      }
      FreePool(MapListNode);
    }
  } else if (Path) {
    //
    // Since there was no error and Path was TRUE
    // Now add the correct path for that mapping
    //
    OriginalPath = gEfiShellProtocol->GetEnv(L"path");
    ASSERT((NewPath == NULL && NewPathSize == 0) || (NewPath != NULL));
    if (OriginalPath != NULL) {
      StrnCatGrow(&NewPath, &NewPathSize, OriginalPath, 0);
    } else {
      StrnCatGrow(&NewPath, &NewPathSize, L".\\", 0);
    }
    StrnCatGrow(&NewPath, &NewPathSize, L";", 0);
    StrnCatGrow(&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow(&NewPath, &NewPathSize, L"\\efi\\tools\\;", 0);
    StrnCatGrow(&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow(&NewPath, &NewPathSize, L"\\efi\\boot\\;", 0);
    StrnCatGrow(&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow(&NewPath, &NewPathSize, L"\\", 0);

    Status = gEfiShellProtocol->SetEnv(L"path", NewPath, TRUE);
    ASSERT_EFI_ERROR(Status);
    FreePool(NewPath);
  }
  return (Status);
}

/**
  Creates the default map names for each device path in the system with
  a protocol depending on the Type.

  Creates the consistent map names for each device path in the system with
  a protocol depending on the Type.

  Note: This will reset all mappings in the system("map -r").

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
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *HandleList;
  UINTN                     Count;
  EFI_DEVICE_PATH_PROTOCOL  **DevicePathList;
  CHAR16                    *NewDefaultName;
  CHAR16                    *NewConsistName;
  EFI_DEVICE_PATH_PROTOCOL  **ConsistMappingTable;
  SHELL_MAP_LIST            *MapListNode;

  HandleList  = NULL;

  //
  // Reset the static members back to zero
  //
  mFsMaxCount = 0;
  mBlkMaxCount = 0;

  gEfiShellProtocol->SetEnv(L"path", L"", TRUE);

  //
  // First empty out the existing list.
  //
  if (!IsListEmpty(&gShellMapList.Link)) {
    for ( MapListNode = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
        ; !IsListEmpty(&gShellMapList.Link)
        ; MapListNode = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
       ){
          RemoveEntryList(&MapListNode->Link);
          FreePool(MapListNode);
    } // for loop
  }

  //
  // Find each handle with Simple File System
  //
  HandleList = GetHandleListByProtocol(&gEfiSimpleFileSystemProtocolGuid);
  if (HandleList != NULL) {
    //
    // Do a count of the handles
    //
    for (Count = 0 ; HandleList[Count] != NULL ; Count++);

    //
    // Get all Device Paths
    //
    DevicePathList = AllocateZeroPool(sizeof(EFI_DEVICE_PATH_PROTOCOL*) * Count);
    ASSERT(DevicePathList != NULL);

    for (Count = 0 ; HandleList[Count] != NULL ; Count++) {
      DevicePathList[Count] = DevicePathFromHandle(HandleList[Count]);
    }

    //
    // Sort all DevicePaths
    //
    PerformQuickSort(DevicePathList, Count, sizeof(EFI_DEVICE_PATH_PROTOCOL*), DevicePathCompare);

    ShellCommandConsistMappingInitialize(&ConsistMappingTable);
    //
    // Assign new Mappings to all...
    //
    for (Count = 0 ; HandleList[Count] != NULL ; Count++) {
      //
      // Get default name first
      //
      NewDefaultName = ShellCommandCreateNewMappingName(MappingTypeFileSystem);
      ASSERT(NewDefaultName != NULL);
      Status = ShellCommandAddMapItemAndUpdatePath(NewDefaultName, DevicePathList[Count], 0, TRUE);
      ASSERT_EFI_ERROR(Status);
      FreePool(NewDefaultName);

      //
      // Now do consistent name
      //
      NewConsistName = ShellCommandConsistMappingGenMappingName(DevicePathList[Count], ConsistMappingTable);
      if (NewConsistName != NULL) {
        Status = ShellCommandAddMapItemAndUpdatePath(NewConsistName, DevicePathList[Count], 0, FALSE);
        ASSERT_EFI_ERROR(Status);
        FreePool(NewConsistName);
      }
    }

    ShellCommandConsistMappingUnInitialize(ConsistMappingTable);

    SHELL_FREE_NON_NULL(HandleList);
    SHELL_FREE_NON_NULL(DevicePathList);

    HandleList = NULL;
  } else {
    Count = (UINTN)-1;
  }

  //
  // Find each handle with Block Io
  //
  HandleList = GetHandleListByProtocol(&gEfiBlockIoProtocolGuid);
  if (HandleList != NULL) {
    for (Count = 0 ; HandleList[Count] != NULL ; Count++);

    //
    // Get all Device Paths
    //
    DevicePathList = AllocateZeroPool(sizeof(EFI_DEVICE_PATH_PROTOCOL*) * Count);
    ASSERT(DevicePathList != NULL);

    for (Count = 0 ; HandleList[Count] != NULL ; Count++) {
      DevicePathList[Count] = DevicePathFromHandle(HandleList[Count]);
    }

    //
    // Sort all DevicePaths
    //
    PerformQuickSort(DevicePathList, Count, sizeof(EFI_DEVICE_PATH_PROTOCOL*), DevicePathCompare);

    //
    // Assign new Mappings to all...
    //
    for (Count = 0 ; HandleList[Count] != NULL ; Count++) {
      //
      // Get default name first
      //
      NewDefaultName = ShellCommandCreateNewMappingName(MappingTypeBlockIo);
      ASSERT(NewDefaultName != NULL);
      Status = ShellCommandAddMapItemAndUpdatePath(NewDefaultName, DevicePathList[Count], 0, FALSE);
      ASSERT_EFI_ERROR(Status);
      FreePool(NewDefaultName);
    }

    SHELL_FREE_NON_NULL(HandleList);
    SHELL_FREE_NON_NULL(DevicePathList);
  } else if (Count == (UINTN)-1) {
    return (EFI_NOT_FOUND);
  }

  return (EFI_SUCCESS);
}

/**
  Converts a SHELL_FILE_HANDLE to an EFI_FILE_PROTOCOL*.

  @param[in] Handle     The SHELL_FILE_HANDLE to convert.

  @return a EFI_FILE_PROTOCOL* representing the same file.
**/
EFI_FILE_PROTOCOL*
EFIAPI
ConvertShellHandleToEfiFileProtocol(
  IN CONST SHELL_FILE_HANDLE Handle
  )
{
  return ((EFI_FILE_PROTOCOL*)(Handle));
}

/**
  Converts a EFI_FILE_PROTOCOL* to an SHELL_FILE_HANDLE.

  @param[in] Handle     The pointer to EFI_FILE_PROTOCOL to convert.
  @param[in] Path       The path to the file for verification.

  @return               A SHELL_FILE_HANDLE representing the same file.
  @retval NULL          There was not enough memory.
**/
SHELL_FILE_HANDLE
EFIAPI
ConvertEfiFileProtocolToShellHandle(
  IN CONST EFI_FILE_PROTOCOL *Handle,
  IN CONST CHAR16            *Path
  )
{
  SHELL_COMMAND_FILE_HANDLE *Buffer;
  BUFFER_LIST               *NewNode;

  if (Path != NULL) {
    Buffer              = AllocateZeroPool(sizeof(SHELL_COMMAND_FILE_HANDLE));
    if (Buffer == NULL) {
      return (NULL);
    }
    NewNode             = AllocateZeroPool(sizeof(BUFFER_LIST));
    if (NewNode == NULL) {
      return (NULL);
    }
    Buffer->FileHandle  = (EFI_FILE_PROTOCOL*)Handle;
    Buffer->Path        = StrnCatGrow(&Buffer->Path, NULL, Path, 0);
    if (Buffer->Path == NULL) {
      return (NULL);
    }
    NewNode->Buffer     = Buffer;

    InsertHeadList(&mFileHandleList.Link, &NewNode->Link);
  }
  return ((SHELL_FILE_HANDLE)(Handle));
}

/**
  Find the path that was logged with the specified SHELL_FILE_HANDLE.

  @param[in] Handle     The SHELL_FILE_HANDLE to query on.

  @return A pointer to the path for the file.
**/
CONST CHAR16*
EFIAPI
ShellFileHandleGetPath(
  IN CONST SHELL_FILE_HANDLE Handle
  )
{
  BUFFER_LIST               *Node;

  for (Node = (BUFFER_LIST*)GetFirstNode(&mFileHandleList.Link)
    ;  !IsNull(&mFileHandleList.Link, &Node->Link)
    ;  Node = (BUFFER_LIST*)GetNextNode(&mFileHandleList.Link, &Node->Link)
   ){
    if ((Node->Buffer) && (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->FileHandle == Handle)){
      return (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->Path);
    }
  }
  return (NULL);
}

/**
  Remove a SHELL_FILE_HANDLE from the list of SHELL_FILE_HANDLES.

  @param[in] Handle     The SHELL_FILE_HANDLE to remove.

  @retval TRUE          The item was removed.
  @retval FALSE         The item was not found.
**/
BOOLEAN
EFIAPI
ShellFileHandleRemove(
  IN CONST SHELL_FILE_HANDLE Handle
  )
{
  BUFFER_LIST               *Node;

  for (Node = (BUFFER_LIST*)GetFirstNode(&mFileHandleList.Link)
    ;  !IsNull(&mFileHandleList.Link, &Node->Link)
    ;  Node = (BUFFER_LIST*)GetNextNode(&mFileHandleList.Link, &Node->Link)
   ){
    if ((Node->Buffer) && (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->FileHandle == Handle)){
      RemoveEntryList(&Node->Link);
      SHELL_FREE_NON_NULL(((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->Path);
      SHELL_FREE_NON_NULL(Node->Buffer);
      SHELL_FREE_NON_NULL(Node);
      return (TRUE);
    }
  }
  return (FALSE);
}

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
  )
{
  EFI_FILE_INFO *Info;
  UINT64        Pos;
  BOOLEAN       RetVal;

  //
  // ASSERT if Handle is NULL
  //
  ASSERT(Handle != NULL);

  gEfiShellProtocol->GetFilePosition(Handle, &Pos);
  Info = gEfiShellProtocol->GetFileInfo (Handle);
  ASSERT(Info != NULL);
  gEfiShellProtocol->SetFilePosition(Handle, Pos);

  if (Info == NULL) {
    return (FALSE);
  }

  if (Pos == Info->FileSize) {
    RetVal = TRUE;
  } else {
    RetVal = FALSE;
  }

  FreePool (Info);

  return (RetVal);
}

/**
  Frees any BUFFER_LIST defined type.

  @param[in] List     The BUFFER_LIST object to free.
**/
VOID
EFIAPI
FreeBufferList (
  IN BUFFER_LIST *List
  )
{
  BUFFER_LIST               *BufferListEntry;

  if (List == NULL){
    return;
  }
  //
  // enumerate through the buffer list and free all memory
  //
  for ( BufferListEntry = ( BUFFER_LIST *)GetFirstNode(&List->Link)
      ; !IsListEmpty (&List->Link)
      ; BufferListEntry = (BUFFER_LIST *)GetFirstNode(&List->Link)
     ){
    RemoveEntryList(&BufferListEntry->Link);
    ASSERT(BufferListEntry->Buffer != NULL);
    if (BufferListEntry->Buffer != NULL) {
      FreePool(BufferListEntry->Buffer);
    }
    FreePool(BufferListEntry);
  }
}

