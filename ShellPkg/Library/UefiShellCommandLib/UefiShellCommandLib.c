/** @file
  Provides interface to shell internal functions for shell commands.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellCommandLib.h"

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
STATIC UINTN                              mFsMaxCount  = 0;
STATIC UINTN                              mBlkMaxCount = 0;
STATIC BUFFER_LIST                        mFileHandleList;

STATIC CONST CHAR8  Hex[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  'A',
  'B',
  'C',
  'D',
  'E',
  'F'
};

// global variables required by library class.
EFI_UNICODE_COLLATION_PROTOCOL  *gUnicodeCollation = NULL;
SHELL_MAP_LIST                  gShellMapList;
SHELL_MAP_LIST                  *gShellCurMapping = NULL;

CONST CHAR16  *SupportLevel[] = {
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
CommandInit (
  VOID
  )
{
  UINTN                           NumHandles;
  EFI_HANDLE                      *Handles;
  EFI_UNICODE_COLLATION_PROTOCOL  *Uc;
  CHAR8                           *BestLanguage;
  UINTN                           Index;
  EFI_STATUS                      Status;
  CHAR8                           *PlatformLang;

  if (gUnicodeCollation == NULL) {
    GetEfiGlobalVariable2 (EFI_PLATFORM_LANG_VARIABLE_NAME, (VOID **)&PlatformLang, NULL);

    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiUnicodeCollation2ProtocolGuid,
                    NULL,
                    &NumHandles,
                    &Handles
                    );
    if (EFI_ERROR (Status)) {
      NumHandles = 0;
      Handles    = NULL;
    }

    for (Index = 0; Index < NumHandles; Index++) {
      //
      // Open Unicode Collation Protocol
      //
      Status = gBS->OpenProtocol (
                      Handles[Index],
                      &gEfiUnicodeCollation2ProtocolGuid,
                      (VOID **)&Uc,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Without clue provided use the first Unicode Collation2 protocol.
      // This may happen when PlatformLang is NULL or when no installed Unicode
      // Collation2 protocol instance supports PlatformLang.
      //
      if (gUnicodeCollation == NULL) {
        gUnicodeCollation = Uc;
      }

      if (PlatformLang == NULL) {
        break;
      }

      //
      // Find the best matching matching language from the supported languages
      // of Unicode Collation2 protocol.
      //
      BestLanguage = GetBestLanguage (
                       Uc->SupportedLanguages,
                       FALSE,
                       PlatformLang,
                       NULL
                       );
      if (BestLanguage != NULL) {
        FreePool (BestLanguage);
        gUnicodeCollation = Uc;
        break;
      }
    }

    if (Handles != NULL) {
      FreePool (Handles);
    }

    if (PlatformLang != NULL) {
      FreePool (PlatformLang);
    }
  }

  return (gUnicodeCollation == NULL) ? EFI_UNSUPPORTED : EFI_SUCCESS;
}

/**
  Constructor for the Shell Command library.

  Initialize the library and determine if the underlying is a UEFI Shell 2.0 or an EFI shell.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS   the initialization was complete successfully
**/
RETURN_STATUS
EFIAPI
ShellCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  InitializeListHead (&gShellMapList.Link);
  InitializeListHead (&mCommandList.Link);
  InitializeListHead (&mAliasList.Link);
  InitializeListHead (&mScriptList.Link);
  InitializeListHead (&mFileHandleList.Link);
  mEchoState = TRUE;

  mExitRequested   = FALSE;
  mExitScript      = FALSE;
  mProfileListSize = 0;
  mProfileList     = NULL;

  Status = CommandInit ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return (RETURN_SUCCESS);
}

/**
  Frees list of file handles.

  @param[in] List     The list to free.
**/
VOID
FreeFileHandleList (
  IN BUFFER_LIST  *List
  )
{
  BUFFER_LIST  *BufferListEntry;

  if (List == NULL) {
    return;
  }

  //
  // enumerate through the buffer list and free all memory
  //
  for ( BufferListEntry = (BUFFER_LIST *)GetFirstNode (&List->Link)
        ; !IsListEmpty (&List->Link)
        ; BufferListEntry = (BUFFER_LIST *)GetFirstNode (&List->Link)
        )
  {
    RemoveEntryList (&BufferListEntry->Link);
    ASSERT (BufferListEntry->Buffer != NULL);
    SHELL_FREE_NON_NULL (((SHELL_COMMAND_FILE_HANDLE *)(BufferListEntry->Buffer))->Path);
    SHELL_FREE_NON_NULL (BufferListEntry->Buffer);
    SHELL_FREE_NON_NULL (BufferListEntry);
  }
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
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Node;
  ALIAS_LIST                         *Node2;
  SCRIPT_FILE_LIST                   *Node3;
  SHELL_MAP_LIST                     *MapNode;

  //
  // enumerate throught the list and free all the memory
  //
  while (!IsListEmpty (&mCommandList.Link)) {
    Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link);
    RemoveEntryList (&Node->Link);
    SHELL_FREE_NON_NULL (Node->CommandString);
    FreePool (Node);
    DEBUG_CODE (
      Node = NULL;
      );
  }

  //
  // enumerate through the alias list and free all memory
  //
  while (!IsListEmpty (&mAliasList.Link)) {
    Node2 = (ALIAS_LIST *)GetFirstNode (&mAliasList.Link);
    RemoveEntryList (&Node2->Link);
    SHELL_FREE_NON_NULL (Node2->CommandString);
    SHELL_FREE_NON_NULL (Node2->Alias);
    SHELL_FREE_NON_NULL (Node2);
    DEBUG_CODE (
      Node2 = NULL;
      );
  }

  //
  // enumerate throught the list and free all the memory
  //
  while (!IsListEmpty (&mScriptList.Link)) {
    Node3 = (SCRIPT_FILE_LIST *)GetFirstNode (&mScriptList.Link);
    RemoveEntryList (&Node3->Link);
    DeleteScriptFileStruct (Node3->Data);
    FreePool (Node3);
  }

  //
  // enumerate throught the mappings list and free all the memory
  //
  if (!IsListEmpty (&gShellMapList.Link)) {
    for (MapNode = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
         ; !IsListEmpty (&gShellMapList.Link)
         ; MapNode = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
         )
    {
      ASSERT (MapNode != NULL);
      RemoveEntryList (&MapNode->Link);
      SHELL_FREE_NON_NULL (MapNode->DevicePath);
      SHELL_FREE_NON_NULL (MapNode->MapName);
      SHELL_FREE_NON_NULL (MapNode->CurrentDirectoryPath);
      FreePool (MapNode);
    }
  }

  if (!IsListEmpty (&mFileHandleList.Link)) {
    FreeFileHandleList (&mFileHandleList);
  }

  if (mProfileList != NULL) {
    FreePool (mProfileList);
  }

  gUnicodeCollation = NULL;
  gShellCurMapping  = NULL;

  return (RETURN_SUCCESS);
}

/**
  Find a dynamic command protocol instance given a command name string.

  @param CommandString  the command name string

  @return instance      the command protocol instance, if dynamic command instance found
  @retval NULL          no dynamic command protocol instance found for name
**/
CONST EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL *
ShellCommandFindDynamicCommand (
  IN CONST CHAR16  *CommandString
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *CommandHandleList;
  EFI_HANDLE                          *NextCommand;
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;

  CommandHandleList = GetHandleListByProtocol (&gEfiShellDynamicCommandProtocolGuid);
  if (CommandHandleList == NULL) {
    //
    // not found or out of resources
    //
    return NULL;
  }

  for (NextCommand = CommandHandleList; *NextCommand != NULL; NextCommand++) {
    Status = gBS->HandleProtocol (
                    *NextCommand,
                    &gEfiShellDynamicCommandProtocolGuid,
                    (VOID **)&DynamicCommand
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)CommandString,
                             (CHAR16 *)DynamicCommand->CommandName
                             ) == 0
        )
    {
      FreePool (CommandHandleList);
      return (DynamicCommand);
    }
  }

  FreePool (CommandHandleList);
  return (NULL);
}

/**
  Checks if a command exists as a dynamic command protocol instance

  @param[in] CommandString        The command string to check for on the list.
**/
BOOLEAN
ShellCommandDynamicCommandExists (
  IN CONST CHAR16  *CommandString
  )
{
  return (BOOLEAN)((ShellCommandFindDynamicCommand (CommandString) != NULL));
}

/**
  Checks if a command is already on the internal command list.

  @param[in] CommandString        The command string to check for on the list.
**/
BOOLEAN
ShellCommandIsCommandOnInternalList (
  IN CONST  CHAR16  *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Node;

  //
  // assert for NULL parameter
  //
  ASSERT (CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link)
        ; !IsNull (&mCommandList.Link, &Node->Link)
        ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode (&mCommandList.Link, &Node->Link)
        )
  {
    if (Node->CommandString == NULL) {
      ASSERT (FALSE);
      continue;
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)CommandString,
                             Node->CommandString
                             ) == 0
        )
    {
      return (TRUE);
    }
  }

  return (FALSE);
}

/**
  Checks if a command exists, either internally or through the dynamic command protocol.

  @param[in] CommandString        The command string to check for on the list.
**/
BOOLEAN
EFIAPI
ShellCommandIsCommandOnList (
  IN CONST  CHAR16  *CommandString
  )
{
  if (ShellCommandIsCommandOnInternalList (CommandString)) {
    return TRUE;
  }

  return ShellCommandDynamicCommandExists (CommandString);
}

/**
 Get the help text for a dynamic command.

  @param[in] CommandString        The command name.

  @retval NULL  No help text was found.
  @return       String of help text. Caller required to free.
**/
CHAR16 *
ShellCommandGetDynamicCommandHelp (
  IN CONST  CHAR16  *CommandString
  )
{
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;

  DynamicCommand = (EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *)ShellCommandFindDynamicCommand (CommandString);
  if (DynamicCommand == NULL) {
    return (NULL);
  }

  //
  // TODO: how to get proper language?
  //
  return DynamicCommand->GetHelp (DynamicCommand, "en");
}

/**
  Get the help text for an internal command.

  @param[in] CommandString        The command name.

  @retval NULL  No help text was found.
  @return       String of help text. Caller reuiqred to free.
**/
CHAR16 *
ShellCommandGetInternalCommandHelp (
  IN CONST  CHAR16  *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Node;

  //
  // assert for NULL parameter
  //
  ASSERT (CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link)
        ; !IsNull (&mCommandList.Link, &Node->Link)
        ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode (&mCommandList.Link, &Node->Link)
        )
  {
    if (Node->CommandString == NULL) {
      ASSERT (FALSE);
      continue;
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)CommandString,
                             Node->CommandString
                             ) == 0
        )
    {
      return (HiiGetString (Node->HiiHandle, Node->ManFormatHelp, NULL));
    }
  }

  return (NULL);
}

/**
  Get the help text for a command.

  @param[in] CommandString        The command name.

  @retval NULL  No help text was found.
  @return       String of help text.Caller reuiqred to free.
**/
CHAR16 *
EFIAPI
ShellCommandGetCommandHelp (
  IN CONST  CHAR16  *CommandString
  )
{
  CHAR16  *HelpStr;

  HelpStr = ShellCommandGetInternalCommandHelp (CommandString);

  if (HelpStr == NULL) {
    HelpStr = ShellCommandGetDynamicCommandHelp (CommandString);
  }

  return HelpStr;
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
  IN CONST  CHAR16                  *CommandString,
  IN        SHELL_RUN_COMMAND       CommandHandler,
  IN        SHELL_GET_MAN_FILENAME  GetManFileName,
  IN        UINT32                  ShellMinSupportLevel,
  IN CONST  CHAR16                  *ProfileName,
  IN CONST  BOOLEAN                 CanAffectLE,
  IN CONST  EFI_HII_HANDLE          HiiHandle,
  IN CONST  EFI_STRING_ID           ManFormatHelp
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Node;
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Command;
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *PrevCommand;
  INTN                               LexicalMatchValue;

  //
  // Initialize local variables.
  //
  Command           = NULL;
  PrevCommand       = NULL;
  LexicalMatchValue = 0;

  //
  // ASSERTs for NULL parameters
  //
  ASSERT (CommandString  != NULL);
  ASSERT (GetManFileName != NULL);
  ASSERT (CommandHandler != NULL);
  ASSERT (ProfileName    != NULL);

  //
  // check for shell support level
  //
  if (PcdGet8 (PcdShellSupportLevel) < ShellMinSupportLevel) {
    return (RETURN_UNSUPPORTED);
  }

  //
  // check for already on the list
  //
  if (ShellCommandIsCommandOnList (CommandString)) {
    return (RETURN_ALREADY_STARTED);
  }

  //
  // allocate memory for new struct
  //
  Node = AllocateZeroPool (sizeof (SHELL_COMMAND_INTERNAL_LIST_ENTRY));
  if (Node == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Node->CommandString = AllocateCopyPool (StrSize (CommandString), CommandString);
  if (Node->CommandString == NULL) {
    FreePool (Node);
    return RETURN_OUT_OF_RESOURCES;
  }

  Node->GetManFileName = GetManFileName;
  Node->CommandHandler = CommandHandler;
  Node->LastError      = CanAffectLE;
  Node->HiiHandle      = HiiHandle;
  Node->ManFormatHelp  = ManFormatHelp;

  if (  (StrLen (ProfileName) > 0)
     && ((  (mProfileList != NULL)
         && (StrStr (mProfileList, ProfileName) == NULL)) || (mProfileList == NULL))
        )
  {
    ASSERT ((mProfileList == NULL && mProfileListSize == 0) || (mProfileList != NULL));
    if (mProfileList == NULL) {
      //
      // If this is the first make a leading ';'
      //
      StrnCatGrow (&mProfileList, &mProfileListSize, L";", 0);
    }

    StrnCatGrow (&mProfileList, &mProfileListSize, ProfileName, 0);
    StrnCatGrow (&mProfileList, &mProfileListSize, L";", 0);
  }

  //
  // Insert a new entry on top of the list
  //
  InsertHeadList (&mCommandList.Link, &Node->Link);

  //
  // Move a new registered command to its sorted ordered location in the list
  //
  for (Command = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link),
       PrevCommand = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link)
       ; !IsNull (&mCommandList.Link, &Command->Link)
       ; Command = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode (&mCommandList.Link, &Command->Link))
  {
    //
    // Get Lexical Comparison Value between PrevCommand and Command list entry
    //
    if ((PrevCommand->CommandString != NULL) && (Command->CommandString != NULL)) {
      LexicalMatchValue = gUnicodeCollation->StriColl (
                                               gUnicodeCollation,
                                               PrevCommand->CommandString,
                                               Command->CommandString
                                               );

      //
      // Swap PrevCommand and Command list entry if PrevCommand list entry
      // is alphabetically greater than Command list entry
      //
      if (LexicalMatchValue > 0) {
        Command = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)SwapListEntries (&PrevCommand->Link, &Command->Link);
      } else if (LexicalMatchValue < 0) {
        //
        // PrevCommand entry is lexically lower than Command entry
        //
        break;
      }
    }
  }

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
  IN CONST CHAR16      *CommandString,
  IN OUT SHELL_STATUS  *RetVal,
  IN OUT BOOLEAN       *CanAffectLE OPTIONAL
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY   *Node;
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;

  //
  // assert for NULL parameters
  //
  ASSERT (CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link)
        ; !IsNull (&mCommandList.Link, &Node->Link)
        ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode (&mCommandList.Link, &Node->Link)
        )
  {
    if (Node->CommandString == NULL) {
      ASSERT (FALSE);
      continue;
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)CommandString,
                             Node->CommandString
                             ) == 0
        )
    {
      if (CanAffectLE != NULL) {
        *CanAffectLE = Node->LastError;
      }

      if (RetVal != NULL) {
        *RetVal = Node->CommandHandler (NULL, gST);
      } else {
        Node->CommandHandler (NULL, gST);
      }

      return (RETURN_SUCCESS);
    }
  }

  //
  // An internal command was not found, try to find a dynamic command
  //
  DynamicCommand = (EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *)ShellCommandFindDynamicCommand (CommandString);
  if (DynamicCommand != NULL) {
    if (RetVal != NULL) {
      *RetVal = DynamicCommand->Handler (DynamicCommand, gST, gEfiShellParametersProtocol, gEfiShellProtocol);
    } else {
      DynamicCommand->Handler (DynamicCommand, gST, gEfiShellParametersProtocol, gEfiShellProtocol);
    }

    return (RETURN_SUCCESS);
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
CONST CHAR16 *
EFIAPI
ShellCommandGetManFileNameHandler (
  IN CONST CHAR16  *CommandString
  )
{
  SHELL_COMMAND_INTERNAL_LIST_ENTRY  *Node;

  //
  // assert for NULL parameters
  //
  ASSERT (CommandString != NULL);

  //
  // check for the command
  //
  for ( Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetFirstNode (&mCommandList.Link)
        ; !IsNull (&mCommandList.Link, &Node->Link)
        ; Node = (SHELL_COMMAND_INTERNAL_LIST_ENTRY *)GetNextNode (&mCommandList.Link, &Node->Link)
        )
  {
    if (Node->CommandString == NULL) {
      ASSERT (FALSE);
      continue;
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)CommandString,
                             Node->CommandString
                             ) == 0
        )
    {
      return (Node->GetManFileName ());
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
CONST COMMAND_LIST *
EFIAPI
ShellCommandGetCommandList (
  IN CONST BOOLEAN  Sort
  )
{
  //  if (!Sort) {
  //    return ((COMMAND_LIST*)(&mCommandList));
  //  }
  return ((COMMAND_LIST *)(&mCommandList));
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
  IN CONST CHAR16  *Command,
  IN CONST CHAR16  *Alias
  )
{
  ALIAS_LIST  *Node;
  ALIAS_LIST  *CommandAlias;
  ALIAS_LIST  *PrevCommandAlias;
  INTN        LexicalMatchValue;

  //
  // Asserts for NULL
  //
  ASSERT (Command != NULL);
  ASSERT (Alias   != NULL);

  //
  // allocate memory for new struct
  //
  Node = AllocateZeroPool (sizeof (ALIAS_LIST));
  if (Node == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Node->CommandString = AllocateCopyPool (StrSize (Command), Command);
  if (Node->CommandString == NULL) {
    FreePool (Node);
    return RETURN_OUT_OF_RESOURCES;
  }

  Node->Alias = AllocateCopyPool (StrSize (Alias), Alias);
  if (Node->Alias == NULL) {
    FreePool (Node->CommandString);
    FreePool (Node);
    return RETURN_OUT_OF_RESOURCES;
  }

  InsertHeadList (&mAliasList.Link, &Node->Link);

  //
  // Move a new pre-defined registered alias to its sorted ordered location in the list
  //
  for ( CommandAlias = (ALIAS_LIST *)GetFirstNode (&mAliasList.Link),
        PrevCommandAlias = (ALIAS_LIST *)GetFirstNode (&mAliasList.Link)
        ; !IsNull (&mAliasList.Link, &CommandAlias->Link)
        ; CommandAlias = (ALIAS_LIST *)GetNextNode (&mAliasList.Link, &CommandAlias->Link))
  {
    //
    // Get Lexical comparison value between PrevCommandAlias and CommandAlias List Entry
    //
    LexicalMatchValue = gUnicodeCollation->StriColl (
                                             gUnicodeCollation,
                                             PrevCommandAlias->Alias,
                                             CommandAlias->Alias
                                             );

    //
    // Swap PrevCommandAlias and CommandAlias list entry if PrevCommandAlias list entry
    // is alphabetically greater than CommandAlias list entry
    //
    if (LexicalMatchValue > 0) {
      CommandAlias = (ALIAS_LIST *)SwapListEntries (&PrevCommandAlias->Link, &CommandAlias->Link);
    } else if (LexicalMatchValue < 0) {
      //
      // PrevCommandAlias entry is lexically lower than CommandAlias entry
      //
      break;
    }
  }

  return (RETURN_SUCCESS);
}

/**
  Get the list of all shell alias commands.  This is a linked list
  (via LIST_ENTRY structure).  enumerate through it using the BaseLib linked
  list functions.  do not modify the values.

  @return a Linked list of all requested shell alias'.
**/
CONST ALIAS_LIST *
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
ShellCommandIsOnAliasList (
  IN CONST CHAR16  *Alias
  )
{
  ALIAS_LIST  *Node;

  //
  // assert for NULL parameter
  //
  ASSERT (Alias != NULL);

  //
  // check for the Alias
  //
  for ( Node = (ALIAS_LIST *)GetFirstNode (&mAliasList.Link)
        ; !IsNull (&mAliasList.Link, &Node->Link)
        ; Node = (ALIAS_LIST *)GetNextNode (&mAliasList.Link, &Node->Link)
        )
  {
    ASSERT (Node->CommandString != NULL);
    ASSERT (Node->Alias != NULL);
    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)Alias,
                             Node->CommandString
                             ) == 0
        )
    {
      return (TRUE);
    }

    if (gUnicodeCollation->StriColl (
                             gUnicodeCollation,
                             (CHAR16 *)Alias,
                             Node->Alias
                             ) == 0
        )
    {
      return (TRUE);
    }
  }

  return (FALSE);
}

/**
  Function to determine current state of ECHO.  Echo determines if lines from scripts
  and ECHO commands are enabled.

  @retval TRUE    Echo is currently enabled
  @retval FALSE   Echo is currently disabled
**/
BOOLEAN
EFIAPI
ShellCommandGetEchoState (
  VOID
  )
{
  return (mEchoState);
}

/**
  Function to set current state of ECHO.  Echo determines if lines from scripts
  and ECHO commands are enabled.

  If State is TRUE, Echo will be enabled.
  If State is FALSE, Echo will be disabled.

  @param[in] State      How to set echo.
**/
VOID
EFIAPI
ShellCommandSetEchoState (
  IN BOOLEAN  State
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
  IN BOOLEAN       ScriptOnly,
  IN CONST UINT64  ErrorCode
  )
{
  mExitRequested = (BOOLEAN)(!mExitRequested);
  if (mExitRequested) {
    mExitScript = ScriptOnly;
  } else {
    mExitScript = FALSE;
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
  IN SCRIPT_FILE  *Script
  )
{
  UINTN  LoopVar;

  if (Script == NULL) {
    return;
  }

  for (LoopVar = 0; LoopVar < Script->Argc; LoopVar++) {
    SHELL_FREE_NON_NULL (Script->Argv[LoopVar]);
  }

  if (Script->Argv != NULL) {
    SHELL_FREE_NON_NULL (Script->Argv);
  }

  Script->CurrentCommand = NULL;
  while (!IsListEmpty (&Script->CommandList)) {
    Script->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetFirstNode (&Script->CommandList);
    if (Script->CurrentCommand != NULL) {
      RemoveEntryList (&Script->CurrentCommand->Link);
      if (Script->CurrentCommand->Cl != NULL) {
        SHELL_FREE_NON_NULL (Script->CurrentCommand->Cl);
      }

      if (Script->CurrentCommand->Data != NULL) {
        SHELL_FREE_NON_NULL (Script->CurrentCommand->Data);
      }

      SHELL_FREE_NON_NULL (Script->CurrentCommand);
    }
  }

  SHELL_FREE_NON_NULL (Script->ScriptName);
  SHELL_FREE_NON_NULL (Script);
}

/**
  Function to return a pointer to the currently running script file object.

  @retval NULL        A script file is not currently running.
  @return             A pointer to the current script file object.
**/
SCRIPT_FILE *
EFIAPI
ShellCommandGetCurrentScriptFile (
  VOID
  )
{
  SCRIPT_FILE_LIST  *List;

  if (IsListEmpty (&mScriptList.Link)) {
    return (NULL);
  }

  List = ((SCRIPT_FILE_LIST *)GetFirstNode (&mScriptList.Link));
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
SCRIPT_FILE *
EFIAPI
ShellCommandSetNewScript (
  IN SCRIPT_FILE  *Script OPTIONAL
  )
{
  SCRIPT_FILE_LIST  *Node;

  if (Script == NULL) {
    if (IsListEmpty (&mScriptList.Link)) {
      return (NULL);
    }

    Node = (SCRIPT_FILE_LIST *)GetFirstNode (&mScriptList.Link);
    RemoveEntryList (&Node->Link);
    DeleteScriptFileStruct (Node->Data);
    FreePool (Node);
  } else {
    Node = AllocateZeroPool (sizeof (SCRIPT_FILE_LIST));
    if (Node == NULL) {
      return (NULL);
    }

    Node->Data = Script;
    InsertHeadList (&mScriptList.Link, &Node->Link);
  }

  return (ShellCommandGetCurrentScriptFile ());
}

/**
  Function to generate the next default mapping name.

  If the return value is not NULL then it must be callee freed.

  @param Type                   What kind of mapping name to make.

  @retval NULL                  a memory allocation failed.
  @return a new map name string
**/
CHAR16 *
EFIAPI
ShellCommandCreateNewMappingName (
  IN CONST SHELL_MAPPING_TYPE  Type
  )
{
  CHAR16  *String;

  ASSERT (Type < MappingTypeMax);

  String = NULL;

  String = AllocateZeroPool (PcdGet8 (PcdShellMapNameLength) * sizeof (String[0]));
  if (String == NULL) {
    return (NULL);
  }

  UnicodeSPrint (
    String,
    PcdGet8 (PcdShellMapNameLength) * sizeof (String[0]),
    Type == MappingTypeFileSystem ? L"FS%d:" : L"BLK%d:",
    Type == MappingTypeFileSystem ? mFsMaxCount++ : mBlkMaxCount++
    );

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

  @retval EFI_SUCCESS           The addition was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
EFIAPI
ShellCommandAddMapItemAndUpdatePath (
  IN CONST CHAR16                    *Name,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CONST UINT64                    Flags,
  IN CONST BOOLEAN                   Path
  )
{
  EFI_STATUS      Status;
  SHELL_MAP_LIST  *MapListNode;
  CONST CHAR16    *OriginalPath;
  CHAR16          *NewPath;
  UINTN           NewPathSize;

  NewPathSize  = 0;
  NewPath      = NULL;
  OriginalPath = NULL;
  Status       = EFI_SUCCESS;

  MapListNode = AllocateZeroPool (sizeof (SHELL_MAP_LIST));
  if (MapListNode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    MapListNode->Flags      = Flags;
    MapListNode->MapName    = AllocateCopyPool (StrSize (Name), Name);
    MapListNode->DevicePath = DuplicateDevicePath (DevicePath);
    if ((MapListNode->MapName == NULL) || (MapListNode->DevicePath == NULL)) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      InsertTailList (&gShellMapList.Link, &MapListNode->Link);
    }
  }

  if (EFI_ERROR (Status)) {
    if (MapListNode != NULL) {
      if (MapListNode->DevicePath != NULL) {
        FreePool (MapListNode->DevicePath);
      }

      if (MapListNode->MapName != NULL) {
        FreePool (MapListNode->MapName);
      }

      FreePool (MapListNode);
    }
  } else if (Path) {
    //
    // Since there was no error and Path was TRUE
    // Now add the correct path for that mapping
    //
    OriginalPath = gEfiShellProtocol->GetEnv (L"path");
    ASSERT ((NewPath == NULL && NewPathSize == 0) || (NewPath != NULL));
    if (OriginalPath != NULL) {
      StrnCatGrow (&NewPath, &NewPathSize, OriginalPath, 0);
      StrnCatGrow (&NewPath, &NewPathSize, L";", 0);
    }

    StrnCatGrow (&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow (&NewPath, &NewPathSize, L"\\efi\\tools\\;", 0);
    StrnCatGrow (&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow (&NewPath, &NewPathSize, L"\\efi\\boot\\;", 0);
    StrnCatGrow (&NewPath, &NewPathSize, Name, 0);
    StrnCatGrow (&NewPath, &NewPathSize, L"\\", 0);

    Status = gEfiShellProtocol->SetEnv (L"path", NewPath, TRUE);
    ASSERT_EFI_ERROR (Status);
    FreePool (NewPath);
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

  @retval EFI_SUCCESS           All map names were created successfully.
  @retval EFI_NOT_FOUND         No protocols were found in the system.
  @return                       Error returned from gBS->LocateHandle().

  @sa LocateHandle
**/
EFI_STATUS
EFIAPI
ShellCommandCreateInitialMappingsAndPaths (
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
  CONST CHAR16              *CurDir;
  CHAR16                    *SplitCurDir;
  CHAR16                    *MapName;
  SHELL_MAP_LIST            *MapListItem;

  ConsistMappingTable = NULL;
  SplitCurDir         = NULL;
  MapName             = NULL;
  MapListItem         = NULL;
  HandleList          = NULL;

  //
  // Reset the static members back to zero
  //
  mFsMaxCount  = 0;
  mBlkMaxCount = 0;

  gEfiShellProtocol->SetEnv (L"path", L"", TRUE);

  //
  // First empty out the existing list.
  //
  if (!IsListEmpty (&gShellMapList.Link)) {
    for ( MapListNode = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
          ; !IsListEmpty (&gShellMapList.Link)
          ; MapListNode = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
          )
    {
      RemoveEntryList (&MapListNode->Link);
      SHELL_FREE_NON_NULL (MapListNode->DevicePath);
      SHELL_FREE_NON_NULL (MapListNode->MapName);
      SHELL_FREE_NON_NULL (MapListNode->CurrentDirectoryPath);
      FreePool (MapListNode);
    } // for loop
  }

  //
  // Find each handle with Simple File System
  //
  HandleList = GetHandleListByProtocol (&gEfiSimpleFileSystemProtocolGuid);
  if (HandleList != NULL) {
    //
    // Do a count of the handles
    //
    for (Count = 0; HandleList[Count] != NULL; Count++) {
    }

    //
    // Get all Device Paths
    //
    DevicePathList = AllocateZeroPool (sizeof (EFI_DEVICE_PATH_PROTOCOL *) * Count);
    if (DevicePathList == NULL) {
      SHELL_FREE_NON_NULL (HandleList);
      return EFI_OUT_OF_RESOURCES;
    }

    for (Count = 0; HandleList[Count] != NULL; Count++) {
      DevicePathList[Count] = DevicePathFromHandle (HandleList[Count]);
    }

    //
    // Sort all DevicePaths
    //
    PerformQuickSort (DevicePathList, Count, sizeof (EFI_DEVICE_PATH_PROTOCOL *), DevicePathCompare);

    Status = ShellCommandConsistMappingInitialize (&ConsistMappingTable);
    if (EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (HandleList);
      SHELL_FREE_NON_NULL (DevicePathList);
      return Status;
    }

    //
    // Assign new Mappings to all...
    //
    for (Count = 0; HandleList[Count] != NULL; Count++) {
      //
      // Get default name first
      //
      NewDefaultName = ShellCommandCreateNewMappingName (MappingTypeFileSystem);
      if (NewDefaultName == NULL) {
        ASSERT (NewDefaultName != NULL);
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      Status = ShellCommandAddMapItemAndUpdatePath (NewDefaultName, DevicePathList[Count], 0, TRUE);
      ASSERT_EFI_ERROR (Status);
      FreePool (NewDefaultName);

      //
      // Now do consistent name
      //
      NewConsistName = ShellCommandConsistMappingGenMappingName (DevicePathList[Count], ConsistMappingTable);
      if (NewConsistName != NULL) {
        Status = ShellCommandAddMapItemAndUpdatePath (NewConsistName, DevicePathList[Count], 0, FALSE);
        ASSERT_EFI_ERROR (Status);
        FreePool (NewConsistName);
      }
    }

    if (ConsistMappingTable != NULL) {
      ShellCommandConsistMappingUnInitialize (ConsistMappingTable);
    }

    SHELL_FREE_NON_NULL (HandleList);
    SHELL_FREE_NON_NULL (DevicePathList);

    HandleList = NULL;

    //
    // gShellCurMapping point to node of current file system in the gShellMapList. When reset all mappings,
    // all nodes in the gShellMapList will be free. Then gShellCurMapping will be a dangling pointer, So,
    // after created new mappings, we should reset the gShellCurMapping pointer back to node of current file system.
    //
    if (gShellCurMapping != NULL) {
      gShellCurMapping = NULL;
      CurDir           = gEfiShellProtocol->GetEnv (L"cwd");
      if (CurDir != NULL) {
        MapName = AllocateCopyPool (StrSize (CurDir), CurDir);
        if (MapName == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        SplitCurDir = StrStr (MapName, L":");
        if (SplitCurDir == NULL) {
          SHELL_FREE_NON_NULL (MapName);
          return EFI_UNSUPPORTED;
        }

        *(SplitCurDir + 1) = CHAR_NULL;
        MapListItem        = ShellCommandFindMapItem (MapName);
        if (MapListItem != NULL) {
          gShellCurMapping = MapListItem;
        }

        SHELL_FREE_NON_NULL (MapName);
      }
    }
  } else {
    Count = (UINTN)-1;
  }

  //
  // Find each handle with Block Io
  //
  HandleList = GetHandleListByProtocol (&gEfiBlockIoProtocolGuid);
  if (HandleList != NULL) {
    for (Count = 0; HandleList[Count] != NULL; Count++) {
    }

    //
    // Get all Device Paths
    //
    DevicePathList = AllocateZeroPool (sizeof (EFI_DEVICE_PATH_PROTOCOL *) * Count);
    if (DevicePathList == NULL) {
      SHELL_FREE_NON_NULL (HandleList);
      return EFI_OUT_OF_RESOURCES;
    }

    for (Count = 0; HandleList[Count] != NULL; Count++) {
      DevicePathList[Count] = DevicePathFromHandle (HandleList[Count]);
    }

    //
    // Sort all DevicePaths
    //
    PerformQuickSort (DevicePathList, Count, sizeof (EFI_DEVICE_PATH_PROTOCOL *), DevicePathCompare);

    //
    // Assign new Mappings to all...
    //
    for (Count = 0; HandleList[Count] != NULL; Count++) {
      //
      // Get default name first
      //
      NewDefaultName = ShellCommandCreateNewMappingName (MappingTypeBlockIo);
      if (NewDefaultName == NULL) {
        ASSERT (NewDefaultName != NULL);
        SHELL_FREE_NON_NULL (HandleList);
        SHELL_FREE_NON_NULL (DevicePathList);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = ShellCommandAddMapItemAndUpdatePath (NewDefaultName, DevicePathList[Count], 0, FALSE);
      ASSERT_EFI_ERROR (Status);
      FreePool (NewDefaultName);
    }

    SHELL_FREE_NON_NULL (HandleList);
    SHELL_FREE_NON_NULL (DevicePathList);
  } else if (Count == (UINTN)-1) {
    return (EFI_NOT_FOUND);
  }

  return (EFI_SUCCESS);
}

/**
  Add mappings for any devices without one.  Do not change any existing maps.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellCommandUpdateMapping (
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

  HandleList = NULL;
  Status     = EFI_SUCCESS;

  //
  // remove mappings that represent removed devices.
  //

  //
  // Find each handle with Simple File System
  //
  HandleList = GetHandleListByProtocol (&gEfiSimpleFileSystemProtocolGuid);
  if (HandleList != NULL) {
    //
    // Do a count of the handles
    //
    for (Count = 0; HandleList[Count] != NULL; Count++) {
    }

    //
    // Get all Device Paths
    //
    DevicePathList = AllocateZeroPool (sizeof (EFI_DEVICE_PATH_PROTOCOL *) * Count);
    if (DevicePathList == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    for (Count = 0; HandleList[Count] != NULL; Count++) {
      DevicePathList[Count] = DevicePathFromHandle (HandleList[Count]);
    }

    //
    // Sort all DevicePaths
    //
    PerformQuickSort (DevicePathList, Count, sizeof (EFI_DEVICE_PATH_PROTOCOL *), DevicePathCompare);

    Status = ShellCommandConsistMappingInitialize (&ConsistMappingTable);
    if (EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (HandleList);
      SHELL_FREE_NON_NULL (DevicePathList);
      return Status;
    }

    //
    // Assign new Mappings to remainders
    //
    for (Count = 0; !EFI_ERROR (Status) && HandleList[Count] != NULL; Count++) {
      //
      // Skip ones that already have
      //
      if (gEfiShellProtocol->GetMapFromDevicePath (&DevicePathList[Count]) != NULL) {
        continue;
      }

      //
      // Get default name
      //
      NewDefaultName = ShellCommandCreateNewMappingName (MappingTypeFileSystem);
      if (NewDefaultName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      //
      // Call shell protocol SetMap function now...
      //
      Status = gEfiShellProtocol->SetMap (DevicePathList[Count], NewDefaultName);

      if (!EFI_ERROR (Status)) {
        //
        // Now do consistent name
        //
        NewConsistName = ShellCommandConsistMappingGenMappingName (DevicePathList[Count], ConsistMappingTable);
        if (NewConsistName != NULL) {
          Status = gEfiShellProtocol->SetMap (DevicePathList[Count], NewConsistName);
          FreePool (NewConsistName);
        }
      }

      FreePool (NewDefaultName);
    }

    ShellCommandConsistMappingUnInitialize (ConsistMappingTable);
    SHELL_FREE_NON_NULL (HandleList);
    SHELL_FREE_NON_NULL (DevicePathList);

    HandleList = NULL;
  } else {
    Count = (UINTN)-1;
  }

  //
  // Do it all over again for gEfiBlockIoProtocolGuid
  //

  return (Status);
}

/**
  Converts a SHELL_FILE_HANDLE to an EFI_FILE_PROTOCOL*.

  @param[in] Handle     The SHELL_FILE_HANDLE to convert.

  @return a EFI_FILE_PROTOCOL* representing the same file.
**/
EFI_FILE_PROTOCOL *
EFIAPI
ConvertShellHandleToEfiFileProtocol (
  IN CONST SHELL_FILE_HANDLE  Handle
  )
{
  return ((EFI_FILE_PROTOCOL *)(Handle));
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
ConvertEfiFileProtocolToShellHandle (
  IN CONST EFI_FILE_PROTOCOL  *Handle,
  IN CONST CHAR16             *Path
  )
{
  SHELL_COMMAND_FILE_HANDLE  *Buffer;
  BUFFER_LIST                *NewNode;

  if (Path != NULL) {
    Buffer = AllocateZeroPool (sizeof (SHELL_COMMAND_FILE_HANDLE));
    if (Buffer == NULL) {
      return (NULL);
    }

    NewNode = AllocateZeroPool (sizeof (BUFFER_LIST));
    if (NewNode == NULL) {
      SHELL_FREE_NON_NULL (Buffer);
      return (NULL);
    }

    Buffer->FileHandle = (EFI_FILE_PROTOCOL *)Handle;
    Buffer->Path       = StrnCatGrow (&Buffer->Path, NULL, Path, 0);
    if (Buffer->Path == NULL) {
      SHELL_FREE_NON_NULL (NewNode);
      SHELL_FREE_NON_NULL (Buffer);
      return (NULL);
    }

    NewNode->Buffer = Buffer;

    InsertHeadList (&mFileHandleList.Link, &NewNode->Link);
  }

  return ((SHELL_FILE_HANDLE)(Handle));
}

/**
  Find the path that was logged with the specified SHELL_FILE_HANDLE.

  @param[in] Handle     The SHELL_FILE_HANDLE to query on.

  @return A pointer to the path for the file.
**/
CONST CHAR16 *
EFIAPI
ShellFileHandleGetPath (
  IN CONST SHELL_FILE_HANDLE  Handle
  )
{
  BUFFER_LIST  *Node;

  for (Node = (BUFFER_LIST *)GetFirstNode (&mFileHandleList.Link)
       ; !IsNull (&mFileHandleList.Link, &Node->Link)
       ; Node = (BUFFER_LIST *)GetNextNode (&mFileHandleList.Link, &Node->Link)
       )
  {
    if ((Node->Buffer) && (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->FileHandle == Handle)) {
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
ShellFileHandleRemove (
  IN CONST SHELL_FILE_HANDLE  Handle
  )
{
  BUFFER_LIST  *Node;

  for (Node = (BUFFER_LIST *)GetFirstNode (&mFileHandleList.Link)
       ; !IsNull (&mFileHandleList.Link, &Node->Link)
       ; Node = (BUFFER_LIST *)GetNextNode (&mFileHandleList.Link, &Node->Link)
       )
  {
    if ((Node->Buffer) && (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->FileHandle == Handle)) {
      RemoveEntryList (&Node->Link);
      SHELL_FREE_NON_NULL (((SHELL_COMMAND_FILE_HANDLE *)Node->Buffer)->Path);
      SHELL_FREE_NON_NULL (Node->Buffer);
      SHELL_FREE_NON_NULL (Node);
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
ShellFileHandleEof (
  IN SHELL_FILE_HANDLE  Handle
  )
{
  EFI_FILE_INFO  *Info;
  UINT64         Pos;
  BOOLEAN        RetVal;

  //
  // ASSERT if Handle is NULL
  //
  ASSERT (Handle != NULL);

  gEfiShellProtocol->GetFilePosition (Handle, &Pos);
  Info = gEfiShellProtocol->GetFileInfo (Handle);
  gEfiShellProtocol->SetFilePosition (Handle, Pos);

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
  IN BUFFER_LIST  *List
  )
{
  BUFFER_LIST  *BufferListEntry;

  if (List == NULL) {
    return;
  }

  //
  // enumerate through the buffer list and free all memory
  //
  for ( BufferListEntry = (BUFFER_LIST *)GetFirstNode (&List->Link)
        ; !IsListEmpty (&List->Link)
        ; BufferListEntry = (BUFFER_LIST *)GetFirstNode (&List->Link)
        )
  {
    RemoveEntryList (&BufferListEntry->Link);
    if (BufferListEntry->Buffer != NULL) {
      FreePool (BufferListEntry->Buffer);
    }

    FreePool (BufferListEntry);
  }
}

/**
  Dump some hexadecimal data to the screen.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
VOID
EFIAPI
DumpHex (
  IN UINTN  Indent,
  IN UINTN  Offset,
  IN UINTN  DataSize,
  IN VOID   *UserData
  )
{
  UINT8  *Data;

  CHAR8  Val[50];

  CHAR8  Str[20];

  UINT8  TempByte;
  UINTN  Size;
  UINTN  Index;

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte           = Data[Index];
      Val[Index * 3 + 0] = Hex[TempByte >> 4];
      Val[Index * 3 + 1] = Hex[TempByte & 0xF];
      Val[Index * 3 + 2] = (CHAR8)((Index == 7) ? '-' : ' ');
      Str[Index]         = (CHAR8)((TempByte < ' ' || TempByte > '~') ? '.' : TempByte);
    }

    Val[Index * 3] = 0;
    Str[Index]     = 0;
    ShellPrintEx (-1, -1, L"%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str);

    Data     += Size;
    Offset   += Size;
    DataSize -= Size;
  }
}

/**
  Dump HEX data into buffer.

  @param[in] Buffer     HEX data to be dumped in Buffer.
  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
CHAR16 *
EFIAPI
CatSDumpHex (
  IN CHAR16  *Buffer,
  IN UINTN   Indent,
  IN UINTN   Offset,
  IN UINTN   DataSize,
  IN VOID    *UserData
  )
{
  UINT8   *Data;
  UINT8   TempByte;
  UINTN   Size;
  UINTN   Index;
  CHAR8   Val[50];
  CHAR8   Str[20];
  CHAR16  *RetVal;
  CHAR16  *TempRetVal;

  Data   = UserData;
  RetVal = Buffer;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte           = Data[Index];
      Val[Index * 3 + 0] = Hex[TempByte >> 4];
      Val[Index * 3 + 1] = Hex[TempByte & 0xF];
      Val[Index * 3 + 2] = (CHAR8)((Index == 7) ? '-' : ' ');
      Str[Index]         = (CHAR8)((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3] = 0;
    Str[Index]     = 0;
    TempRetVal     = CatSPrint (RetVal, L"%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;

    Data     += Size;
    Offset   += Size;
    DataSize -= Size;
  }

  return RetVal;
}

/**
  ORDERED_COLLECTION_USER_COMPARE function for SHELL_SORT_UNIQUE_NAME objects.

  @param[in] Unique1AsVoid  The first SHELL_SORT_UNIQUE_NAME object (Unique1),
                            passed in as a pointer-to-VOID.

  @param[in] Unique2AsVoid  The second SHELL_SORT_UNIQUE_NAME object (Unique2),
                            passed in as a pointer-to-VOID.

  @retval <0  If Unique1 compares less than Unique2.

  @retval  0  If Unique1 compares equal to Unique2.

  @retval >0  If Unique1 compares greater than Unique2.
**/
STATIC
INTN
EFIAPI
UniqueNameCompare (
  IN CONST VOID  *Unique1AsVoid,
  IN CONST VOID  *Unique2AsVoid
  )
{
  CONST SHELL_SORT_UNIQUE_NAME  *Unique1;
  CONST SHELL_SORT_UNIQUE_NAME  *Unique2;

  Unique1 = Unique1AsVoid;
  Unique2 = Unique2AsVoid;

  //
  // We need to cast away CONST for EFI_UNICODE_COLLATION_STRICOLL.
  //
  return gUnicodeCollation->StriColl (
                              gUnicodeCollation,
                              (CHAR16 *)Unique1->Alias,
                              (CHAR16 *)Unique2->Alias
                              );
}

/**
  ORDERED_COLLECTION_KEY_COMPARE function for SHELL_SORT_UNIQUE_NAME objects.

  @param[in] UniqueAliasAsVoid  The CHAR16 string UniqueAlias, passed in as a
                                pointer-to-VOID.

  @param[in] UniqueAsVoid       The SHELL_SORT_UNIQUE_NAME object (Unique),
                                passed in as a pointer-to-VOID.

  @retval <0  If UniqueAlias compares less than Unique->Alias.

  @retval  0  If UniqueAlias compares equal to Unique->Alias.

  @retval >0  If UniqueAlias compares greater than Unique->Alias.
**/
STATIC
INTN
EFIAPI
UniqueNameAliasCompare (
  IN CONST VOID  *UniqueAliasAsVoid,
  IN CONST VOID  *UniqueAsVoid
  )
{
  CONST CHAR16                  *UniqueAlias;
  CONST SHELL_SORT_UNIQUE_NAME  *Unique;

  UniqueAlias = UniqueAliasAsVoid;
  Unique      = UniqueAsVoid;

  //
  // We need to cast away CONST for EFI_UNICODE_COLLATION_STRICOLL.
  //
  return gUnicodeCollation->StriColl (
                              gUnicodeCollation,
                              (CHAR16 *)UniqueAlias,
                              (CHAR16 *)Unique->Alias
                              );
}

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
  IN OUT EFI_SHELL_FILE_INFO   **FileList,
  OUT EFI_SHELL_FILE_INFO      **Duplicates OPTIONAL,
  IN     SHELL_SORT_FILE_LIST  Order
  )
{
  LIST_ENTRY                *FilesHead;
  ORDERED_COLLECTION        *Sort;
  LIST_ENTRY                *FileEntry;
  EFI_SHELL_FILE_INFO       *FileInfo;
  SHELL_SORT_UNIQUE_NAME    *Unique;
  EFI_STATUS                Status;
  EFI_SHELL_FILE_INFO       *Dupes;
  LIST_ENTRY                *NextFileEntry;
  CONST CHAR16              *Alias;
  ORDERED_COLLECTION_ENTRY  *SortEntry;
  LIST_ENTRY                *TargetFileList;
  ORDERED_COLLECTION_ENTRY  *NextSortEntry;
  VOID                      *UniqueAsVoid;

  if ((UINTN)Order >= (UINTN)ShellSortFileListMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (FileList == NULL) {
    //
    // FileList is considered empty, hence already sorted, with no duplicates.
    //
    if (Duplicates != NULL) {
      *Duplicates = NULL;
    }

    return EFI_SUCCESS;
  }

  if (*FileList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FilesHead = &(*FileList)->Link;

  //
  // Collect all the unique names.
  //
  Sort = OrderedCollectionInit (UniqueNameCompare, UniqueNameAliasCompare);
  if (Sort == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BASE_LIST_FOR_EACH (FileEntry, FilesHead) {
    FileInfo = (EFI_SHELL_FILE_INFO *)FileEntry;

    //
    // Try to record the name of this file as a unique name.
    //
    Unique = AllocatePool (sizeof (*Unique));
    if (Unique == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto UninitSort;
    }

    Unique->Alias = ((Order == ShellSortFileListByFileName) ?
                     FileInfo->FileName :
                     FileInfo->FullName);
    InitializeListHead (&Unique->SameNameList);

    Status = OrderedCollectionInsert (Sort, NULL, Unique);
    if (EFI_ERROR (Status)) {
      //
      // Only two errors are possible: memory allocation failed, or this name
      // has been encountered before. In either case, the
      // SHELL_SORT_UNIQUE_NAME object being constructed has to be released.
      //
      FreePool (Unique);
      //
      // Memory allocation failure is fatal, while having seen the same name
      // before is normal.
      //
      if (Status == EFI_OUT_OF_RESOURCES) {
        goto UninitSort;
      }

      ASSERT (Status == EFI_ALREADY_STARTED);
    }
  }

  //
  // Set Dupes to suppress incorrect compiler/analyzer warnings.
  //
  Dupes = NULL;

  //
  // If separation of duplicates has been requested, allocate the list for
  // them.
  //
  if (Duplicates != NULL) {
    Dupes = AllocateZeroPool (sizeof (*Dupes));
    if (Dupes == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto UninitSort;
    }

    InitializeListHead (&Dupes->Link);
  }

  //
  // No memory allocation beyond this point; thus, no chance to fail. We can
  // now migrate the EFI_SHELL_FILE_INFO objects from (*FileList) to Sort.
  //
  BASE_LIST_FOR_EACH_SAFE (FileEntry, NextFileEntry, FilesHead) {
    FileInfo = (EFI_SHELL_FILE_INFO *)FileEntry;
    //
    // Look up the SHELL_SORT_UNIQUE_NAME that matches FileInfo's name.
    //
    Alias = ((Order == ShellSortFileListByFileName) ?
             FileInfo->FileName :
             FileInfo->FullName);
    SortEntry = OrderedCollectionFind (Sort, Alias);
    ASSERT (SortEntry != NULL);
    Unique = OrderedCollectionUserStruct (SortEntry);
    //
    // Move FileInfo from (*FileList) to the end of the list of files whose
    // names all compare identical to FileInfo's name.
    //
    RemoveEntryList (&FileInfo->Link);
    InsertTailList (&Unique->SameNameList, &FileInfo->Link);
  }

  //
  // All EFI_SHELL_FILE_INFO objects originally in (*FileList) have been
  // distributed to Sort. Now migrate them back to (*FileList), advancing in
  // unique name order.
  //
  for (SortEntry = OrderedCollectionMin (Sort);
       SortEntry != NULL;
       SortEntry = OrderedCollectionNext (SortEntry))
  {
    Unique = OrderedCollectionUserStruct (SortEntry);
    //
    // The first FileInfo encountered for each unique name goes back on
    // (*FileList) unconditionally. Further FileInfo instances for the same
    // unique name -- that is, duplicates -- are either returned to (*FileList)
    // or separated, dependent on the caller's request.
    //
    TargetFileList = FilesHead;
    BASE_LIST_FOR_EACH_SAFE (FileEntry, NextFileEntry, &Unique->SameNameList) {
      RemoveEntryList (FileEntry);
      InsertTailList (TargetFileList, FileEntry);
      if (Duplicates != NULL) {
        TargetFileList = &Dupes->Link;
      }
    }
  }

  //
  // We're done. If separation of duplicates has been requested, output the
  // list of duplicates -- and free that list at once, if it's empty (i.e., if
  // no duplicates have been found).
  //
  if (Duplicates != NULL) {
    if (IsListEmpty (&Dupes->Link)) {
      FreePool (Dupes);
      *Duplicates = NULL;
    } else {
      *Duplicates = Dupes;
    }
  }

  Status = EFI_SUCCESS;

  //
  // Fall through.
  //
UninitSort:
  for (SortEntry = OrderedCollectionMin (Sort);
       SortEntry != NULL;
       SortEntry = NextSortEntry)
  {
    NextSortEntry = OrderedCollectionNext (SortEntry);
    OrderedCollectionDelete (Sort, SortEntry, &UniqueAsVoid);
    Unique = UniqueAsVoid;
    ASSERT (IsListEmpty (&Unique->SameNameList));
    FreePool (Unique);
  }

  OrderedCollectionUninit (Sort);

  return Status;
}
