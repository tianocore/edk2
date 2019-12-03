/** @file
  Main file for NULL named library for level 1 shell command functions.

  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel1CommandsLib.h"

STATIC CONST CHAR16 mFileName[] = L"ShellCommands";
EFI_HII_HANDLE gShellLevel1HiiHandle = NULL;

/**
  Return the help text filename.  Only used if no HII information found.

  @retval the filename.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameLevel1 (
  VOID
  )
{
  return (mFileName);
}

/**
  Constructor for the Shell Level 1 Commands library.

  Install the handlers for level 1 UEFI Shell 2.0 commands.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellLevel1CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // if shell level is less than 2 do nothing
  //
  if (PcdGet8(PcdShellSupportLevel) < 1) {
    return (EFI_SUCCESS);
  }

  gShellLevel1HiiHandle = HiiAddPackages (&gShellLevel1HiiGuid, gImageHandle, UefiShellLevel1CommandsLibStrings, NULL);
  if (gShellLevel1HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"stall",  ShellCommandRunStall   , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_STALL) ));
  ShellCommandRegisterCommandName(L"for",    ShellCommandRunFor     , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_FOR)   ));
  ShellCommandRegisterCommandName(L"goto",   ShellCommandRunGoto    , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_GOTO)  ));
  ShellCommandRegisterCommandName(L"if",     ShellCommandRunIf      , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_IF)    ));
  ShellCommandRegisterCommandName(L"shift",  ShellCommandRunShift   , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_SHIFT) ));
  ShellCommandRegisterCommandName(L"exit",   ShellCommandRunExit    , ShellCommandGetManFileNameLevel1, 1, L"", TRUE , gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_EXIT)  ));
  ShellCommandRegisterCommandName(L"else",   ShellCommandRunElse    , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_ELSE)  ));
  ShellCommandRegisterCommandName(L"endif",  ShellCommandRunEndIf   , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_ENDIF) ));
  ShellCommandRegisterCommandName(L"endfor", ShellCommandRunEndFor  , ShellCommandGetManFileNameLevel1, 1, L"", FALSE, gShellLevel1HiiHandle, (EFI_STRING_ID)(PcdGet8(PcdShellSupportLevel) < 3 ? 0 : STRING_TOKEN(STR_GET_HELP_ENDFOR)));

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellLevel1CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellLevel1HiiHandle != NULL) {
    HiiRemovePackages(gShellLevel1HiiHandle);
  }
  return (EFI_SUCCESS);
}

/**
  Test a node to see if meets the criterion.

  It functions so that count starts at 1 and it increases or decreases when it
  hits the specified tags.  when it hits zero the location has been found.

  DecrementerTag and IncrementerTag are used to get around for/endfor and
  similar paired types where the entire middle should be ignored.

  If label is used it will be used instead of the count.

  @param[in] Function          The function to use to enumerate through the
                               list.  Normally GetNextNode or GetPreviousNode.
  @param[in] DecrementerTag    The tag to decrement the count at.
  @param[in] IncrementerTag    The tag to increment the count at.
  @param[in] Label             A label to look for.
  @param[in, out] ScriptFile   The pointer to the current script file structure.
  @param[in] MovePast          TRUE makes function return 1 past the found
                               location.
  @param[in] FindOnly          TRUE to not change the ScriptFile.
  @param[in] CommandNode       The pointer to the Node to test.
  @param[in, out] TargetCount  The pointer to the current count.
**/
BOOLEAN
TestNodeForMove (
  IN CONST LIST_MANIP_FUNC      Function,
  IN CONST CHAR16               *DecrementerTag,
  IN CONST CHAR16               *IncrementerTag,
  IN CONST CHAR16               *Label OPTIONAL,
  IN OUT SCRIPT_FILE            *ScriptFile,
  IN CONST BOOLEAN              MovePast,
  IN CONST BOOLEAN              FindOnly,
  IN CONST SCRIPT_COMMAND_LIST  *CommandNode,
  IN OUT UINTN                  *TargetCount
  )
{
  BOOLEAN             Found;
  CHAR16              *CommandName;
  CHAR16              *CommandNameWalker;
  CHAR16              *TempLocation;

  Found = FALSE;

  //
  // get just the first part of the command line...
  //
  CommandName   = NULL;
  CommandName   = StrnCatGrow(&CommandName, NULL, CommandNode->Cl, 0);
  if (CommandName == NULL) {
    return (FALSE);
  }

  CommandNameWalker = CommandName;

  //
  // Skip leading spaces and tabs.
  //
  while ((CommandNameWalker[0] == L' ') || (CommandNameWalker[0] == L'\t')) {
    CommandNameWalker++;
  }
  TempLocation  = StrStr(CommandNameWalker, L" ");

  if (TempLocation != NULL) {
    *TempLocation = CHAR_NULL;
  }

  //
  // did we find a nested item ?
  //
  if (gUnicodeCollation->StriColl(
      gUnicodeCollation,
      (CHAR16*)CommandNameWalker,
      (CHAR16*)IncrementerTag) == 0) {
    (*TargetCount)++;
  } else if (gUnicodeCollation->StriColl(
      gUnicodeCollation,
      (CHAR16*)CommandNameWalker,
      (CHAR16*)DecrementerTag) == 0) {
    if (*TargetCount > 0) {
      (*TargetCount)--;
    }
  }

  //
  // did we find the matching one...
  //
  if (Label == NULL) {
    if (*TargetCount == 0) {
      Found = TRUE;
      if (!FindOnly) {
        if (MovePast) {
          ScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)(*Function)(&ScriptFile->CommandList, &CommandNode->Link);
        } else {
          ScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)CommandNode;
        }
      }
    }
  } else {
    if (gUnicodeCollation->StriColl(
      gUnicodeCollation,
      (CHAR16*)CommandNameWalker,
      (CHAR16*)Label) == 0
      && (*TargetCount) == 0) {
      Found = TRUE;
      if (!FindOnly) {
        //
        // we found the target label without loops
        //
        if (MovePast) {
          ScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)(*Function)(&ScriptFile->CommandList, &CommandNode->Link);
        } else {
          ScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)CommandNode;
        }
      }
    }
  }

  //
  // Free the memory for this loop...
  //
  FreePool(CommandName);
  return (Found);
}

/**
  Move the script pointer from 1 tag (line) to another.

  It functions so that count starts at 1 and it increases or decreases when it
  hits the specified tags.  when it hits zero the location has been found.

  DecrementerTag and IncrementerTag are used to get around for/endfor and
  similar paired types where the entire middle should be ignored.

  If label is used it will be used instead of the count.

  @param[in] Function          The function to use to enumerate through the
                               list.  Normally GetNextNode or GetPreviousNode.
  @param[in] DecrementerTag    The tag to decrement the count at.
  @param[in] IncrementerTag    The tag to increment the count at.
  @param[in] Label             A label to look for.
  @param[in, out] ScriptFile   The pointer to the current script file structure.
  @param[in] MovePast          TRUE makes function return 1 past the found
                               location.
  @param[in] FindOnly          TRUE to not change the ScriptFile.
  @param[in] WrapAroundScript  TRUE to wrap end-to-begining or vise versa in
                               searching.
**/
BOOLEAN
MoveToTag (
  IN CONST LIST_MANIP_FUNC      Function,
  IN CONST CHAR16               *DecrementerTag,
  IN CONST CHAR16               *IncrementerTag,
  IN CONST CHAR16               *Label OPTIONAL,
  IN OUT SCRIPT_FILE            *ScriptFile,
  IN CONST BOOLEAN              MovePast,
  IN CONST BOOLEAN              FindOnly,
  IN CONST BOOLEAN              WrapAroundScript
  )
{
  SCRIPT_COMMAND_LIST *CommandNode;
  BOOLEAN             Found;
  UINTN               TargetCount;

  if (Label == NULL) {
    TargetCount       = 1;
  } else {
    TargetCount       = 0;
  }

  if (ScriptFile == NULL) {
    return FALSE;
  }

  for (CommandNode = (SCRIPT_COMMAND_LIST *)(*Function)(&ScriptFile->CommandList, &ScriptFile->CurrentCommand->Link), Found = FALSE
    ;  !IsNull(&ScriptFile->CommandList, &CommandNode->Link)&& !Found
    ;  CommandNode = (SCRIPT_COMMAND_LIST *)(*Function)(&ScriptFile->CommandList, &CommandNode->Link)
   ){
    Found = TestNodeForMove(
      Function,
      DecrementerTag,
      IncrementerTag,
      Label,
      ScriptFile,
      MovePast,
      FindOnly,
      CommandNode,
      &TargetCount);
  }

  if (WrapAroundScript && !Found) {
    for (CommandNode = (SCRIPT_COMMAND_LIST *)GetFirstNode(&ScriptFile->CommandList), Found = FALSE
      ;  CommandNode != ScriptFile->CurrentCommand && !Found
      ;  CommandNode = (SCRIPT_COMMAND_LIST *)(*Function)(&ScriptFile->CommandList, &CommandNode->Link)
     ){
      Found = TestNodeForMove(
        Function,
        DecrementerTag,
        IncrementerTag,
        Label,
        ScriptFile,
        MovePast,
        FindOnly,
        CommandNode,
        &TargetCount);
    }
  }
  return (Found);
}

