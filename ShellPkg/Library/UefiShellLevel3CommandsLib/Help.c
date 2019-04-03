/** @file
  Main file for Help shell level 3 function.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2014, ARM Limited. All rights reserved. <BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>
#include <Library/HandleParsingLib.h>

#include <Protocol/ShellDynamicCommand.h>

/**
   function to insert string items into a list in the correct alphabetical place

   the resultant list is a double NULL terminated list of NULL terminated strings.

   upon successful return the memory must be caller freed (unless passed back in
   via a loop where it will get reallocated).

   @param[in,out] DestList    double pointer to the list. may be NULL.
   @param[in,out] DestSize    pointer to the size of list. may be 0, if DestList is NULL.
   @param[in]     Item        the item to insert.

   @retval EFI_SUCCESS        the operation was successful.
**/
EFI_STATUS
LexicalInsertIntoList(
  IN OUT   CHAR16 **DestList,
  IN OUT   UINTN  *DestSize,
  IN CONST CHAR16 *Item
  )
{
  CHAR16                              *NewList;
  INTN                                LexicalMatchValue;
  CHAR16                              *LexicalSpot;
  UINTN                               SizeOfAddedNameInBytes;

  //
  // If there are none, then just return with success
  //
  if (Item == NULL || *Item == CHAR_NULL || StrLen(Item)==0) {
    return (EFI_SUCCESS);
  }

  NewList = *DestList;

  SizeOfAddedNameInBytes = StrSize(Item);
  NewList = ReallocatePool(*DestSize, (*DestSize) + SizeOfAddedNameInBytes, NewList);
  (*DestSize) = (*DestSize) + SizeOfAddedNameInBytes;

  //
  // Find the correct spot in the list
  //
  for (LexicalSpot = NewList
    ; LexicalSpot != NULL && LexicalSpot < NewList + (*DestSize)
    ; LexicalSpot += StrLen(LexicalSpot) + 1
    ) {
    //
    // Get Lexical Comparison Value between PrevCommand and Command list entry
    //
    LexicalMatchValue = gUnicodeCollation->StriColl (
                                              gUnicodeCollation,
                                              (CHAR16 *)LexicalSpot,
                                              (CHAR16 *)Item
                                              );
    //
    // The new item goes before this one.
    //
    if (LexicalMatchValue > 0 || StrLen(LexicalSpot) == 0) {
      if (StrLen(LexicalSpot) != 0) {
        //
        // Move this and all other items out of the way
        //
        CopyMem(
          LexicalSpot + (SizeOfAddedNameInBytes/sizeof(CHAR16)),
          LexicalSpot,
          (*DestSize) - SizeOfAddedNameInBytes - ((LexicalSpot - NewList) * sizeof(CHAR16))
          );
      }

      //
      // Stick this one in place
      //
      StrCpyS(LexicalSpot, SizeOfAddedNameInBytes/sizeof(CHAR16), Item);
      break;
    }
  }

  *DestList = NewList;
  return (EFI_SUCCESS);
}

/**
   function to add each command name from the linked list to the string list.

   the resultant list is a double NULL terminated list of NULL terminated strings.

   @param[in,out] DestList    double pointer to the list. may be NULL.
   @param[in,out] DestSize    pointer to the size of list. may be 0, if DestList is NULL.
   @param[in]     SourceList  the double linked list of commands.

   @retval EFI_SUCCESS        the operation was successful.
**/
EFI_STATUS
CopyListOfCommandNames(
  IN OUT   CHAR16       **DestList,
  IN OUT   UINTN        *DestSize,
  IN CONST COMMAND_LIST *SourceList
  )
{
  CONST COMMAND_LIST  *Node;

  for ( Node = (COMMAND_LIST*)GetFirstNode(&SourceList->Link)
      ; SourceList != NULL && !IsListEmpty(&SourceList->Link) && !IsNull(&SourceList->Link, &Node->Link)
      ; Node = (COMMAND_LIST*)GetNextNode(&SourceList->Link, &Node->Link)
    ) {
    LexicalInsertIntoList(DestList, DestSize, Node->CommandString);
  }
  return (EFI_SUCCESS);
}

/**
   function to add each dynamic command name to the string list.

   the resultant list is a double NULL terminated list of NULL terminated strings.

   @param[in,out] DestList    double pointer to the list. may be NULL.
   @param[in,out] DestSize    pointer to the size of list. may be 0, if DestList is NULL.

   @retval EFI_SUCCESS        the operation was successful.
   @return an error from HandleProtocol
**/
STATIC
EFI_STATUS
CopyListOfCommandNamesWithDynamic(
  IN OUT  CHAR16** DestList,
  IN OUT  UINTN    *DestSize
  )
{
  EFI_HANDLE                          *CommandHandleList;
  CONST EFI_HANDLE                    *NextCommand;
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;
  EFI_STATUS                          Status;

  CommandHandleList = GetHandleListByProtocol(&gEfiShellDynamicCommandProtocolGuid);

  //
  // If there are none, then just return with success
  //
  if (CommandHandleList == NULL) {
    return (EFI_SUCCESS);
  }

  Status = EFI_SUCCESS;

  //
  // Append those to the list.
  //
  for (NextCommand = CommandHandleList ; *NextCommand != NULL && !EFI_ERROR(Status) ; NextCommand++) {
    Status = gBS->HandleProtocol(
      *NextCommand,
      &gEfiShellDynamicCommandProtocolGuid,
      (VOID **)&DynamicCommand
      );

    if (EFI_ERROR(Status)) {
      continue;
    }

    Status = LexicalInsertIntoList(DestList, DestSize, DynamicCommand->CommandName);
  }

  SHELL_FREE_NON_NULL(CommandHandleList);
  return (Status);
}


/**
  Attempt to print help from a dynamically added command.

  @param[in]  CommandToGetHelpOn  The unicode name of the command that help is
                                  requested on.
  @param[in]  SectionToGetHelpOn  Pointer to the section specifier(s).
  @param[in]  PrintCommandText    Print the command followed by the help content
                                  or just help.

  @retval EFI_SUCCESS             The help was displayed
  @retval EFI_NOT_FOUND           The command name could not be found
  @retval EFI_DEVICE_ERROR        The help data format was incorrect.
**/
EFI_STATUS
PrintDynamicCommandHelp(
  IN CONST CHAR16  *CommandToGetHelpOn,
  IN CONST CHAR16  *SectionToGetHelpOn,
  IN BOOLEAN       PrintCommandText
 )
{
  EFI_STATUS                          Status;
  BOOLEAN                             Found;
  EFI_HANDLE                          *CommandHandleList;
  EFI_HANDLE                          *NextCommand;
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;

  Status = EFI_NOT_FOUND;
  Found = FALSE;
  CommandHandleList = NULL;

  CommandHandleList = GetHandleListByProtocol(&gEfiShellDynamicCommandProtocolGuid);

  if (CommandHandleList == NULL) {
    //
    // not found or out of resources
    //
    return Status;
  }

  for (NextCommand = CommandHandleList; *NextCommand != NULL; NextCommand++) {
    Status = gBS->HandleProtocol(
      *NextCommand,
      &gEfiShellDynamicCommandProtocolGuid,
      (VOID **)&DynamicCommand
      );

    if (EFI_ERROR(Status)) {
      continue;
    }

    //
    // Check execution break flag when printing multiple command help information.
    //
    if (ShellGetExecutionBreakFlag ()) {
      break;
    }

    if ((gUnicodeCollation->MetaiMatch (gUnicodeCollation, (CHAR16 *)DynamicCommand->CommandName, (CHAR16*)CommandToGetHelpOn)) ||
      (gEfiShellProtocol->GetAlias (CommandToGetHelpOn, NULL) != NULL && (gUnicodeCollation->MetaiMatch (gUnicodeCollation, (CHAR16 *)DynamicCommand->CommandName, (CHAR16*)(gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL)))))) {
      // Print as Shell Help if in ManPage format.
      Status = ShellPrintHelp (DynamicCommand->CommandName, SectionToGetHelpOn,
                              PrintCommandText);
      if (Status == EFI_DEVICE_ERROR) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_HELP_INV),
                        gShellLevel3HiiHandle, DynamicCommand->CommandName);
      } else if (EFI_ERROR(Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_HELP_NF),
                        gShellLevel3HiiHandle, DynamicCommand->CommandName);
      } else {
        Found = TRUE;
      }
    }
  }

  SHELL_FREE_NON_NULL(CommandHandleList);

  return (Found ? EFI_SUCCESS : Status);

}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-usage", TypeFlag},
  {L"-section", TypeMaxValue},
  {L"-verbose", TypeFlag},
  {L"-v", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'help' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunHelp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR16              *SortedCommandList;
  CONST CHAR16        *CurrentCommand;
  CHAR16              *CommandToGetHelpOn;
  CHAR16              *SectionToGetHelpOn;
  CHAR16              *HiiString;
  BOOLEAN             Found;
  BOOLEAN             PrintCommandText;
  UINTN               SortedCommandListSize;

  PrintCommandText    = TRUE;
  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  CommandToGetHelpOn  = NULL;
  SectionToGetHelpOn  = NULL;
  SortedCommandList   = NULL;
  Found               = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"help", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // Check for conflicting parameters.
    //
    if (ShellCommandLineGetFlag(Package, L"-usage")
      &&ShellCommandLineGetFlag(Package, L"-section")
      &&(ShellCommandLineGetFlag(Package, L"-verbose") || ShellCommandLineGetFlag(Package, L"-v"))
     ){
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellLevel3HiiHandle, L"help");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"help");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Get the command name we are getting help on
      //
      ASSERT(CommandToGetHelpOn == NULL);
      StrnCatGrow(&CommandToGetHelpOn, NULL, ShellCommandLineGetRawValue(Package, 1), 0);
      if (CommandToGetHelpOn == NULL && ShellCommandLineGetFlag(Package, L"-?")) {
        //
        // If we dont have a command and we got a simple -?
        // we are looking for help on help command.
        //
        StrnCatGrow(&CommandToGetHelpOn, NULL, L"help", 0);
      }

      if (CommandToGetHelpOn == NULL) {
        StrnCatGrow(&CommandToGetHelpOn, NULL, L"*", 0);
        ASSERT(SectionToGetHelpOn == NULL);
        StrnCatGrow(&SectionToGetHelpOn, NULL, L"NAME", 0);
      } else {
        PrintCommandText = FALSE;
        ASSERT(SectionToGetHelpOn == NULL);
        //
        // Get the section name for the given command name
        //
        if (ShellCommandLineGetFlag(Package, L"-section")) {
          StrnCatGrow(&SectionToGetHelpOn, NULL, ShellCommandLineGetValue(Package, L"-section"), 0);
        } else if (ShellCommandLineGetFlag(Package, L"-usage")) {
          StrnCatGrow(&SectionToGetHelpOn, NULL, L"NAME,SYNOPSIS", 0);
        } else if (ShellCommandLineGetFlag(Package, L"-verbose") || ShellCommandLineGetFlag(Package, L"-v")) {
        } else {
          //
          // The output of help <command> will display NAME, SYNOPSIS, OPTIONS, DESCRIPTION, and EXAMPLES sections.
          //
          StrnCatGrow (&SectionToGetHelpOn, NULL, L"NAME,SYNOPSIS,OPTIONS,DESCRIPTION,EXAMPLES", 0);
        }
      }

      if (gUnicodeCollation->StriColl(gUnicodeCollation, CommandToGetHelpOn, L"special") == 0) {
        //
        // we need info on the special characters
        //
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_SC_HEADER), gShellLevel3HiiHandle);
        HiiString = HiiGetString(gShellLevel3HiiHandle, STRING_TOKEN(STR_HELP_SC_DATA), NULL);
        ShellPrintEx(-1, -1, L"%s", HiiString);
        FreePool(HiiString);
        Found = TRUE;
      } else {
        SortedCommandList = NULL;
        SortedCommandListSize = 0;
        CopyListOfCommandNames(&SortedCommandList, &SortedCommandListSize, ShellCommandGetCommandList(TRUE));
        CopyListOfCommandNamesWithDynamic(&SortedCommandList, &SortedCommandListSize);

        for (CurrentCommand = SortedCommandList
          ; CurrentCommand != NULL && CurrentCommand < SortedCommandList + SortedCommandListSize/sizeof(CHAR16) && *CurrentCommand != CHAR_NULL
          ; CurrentCommand += StrLen(CurrentCommand) + 1
          ) {
          //
          // Checking execution break flag when print multiple command help information.
          //
          if (ShellGetExecutionBreakFlag ()) {
            break;
          }

          if ((gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16*)CurrentCommand, CommandToGetHelpOn)) ||
             (gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL) != NULL && (gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16*)CurrentCommand, (CHAR16*)(gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL)))))) {
            //
            // We have a command to look for help on.
            //
            Status = ShellPrintHelp(CurrentCommand, SectionToGetHelpOn, PrintCommandText);
            if (EFI_ERROR(Status)) {
              //
              // now try to match against the dynamic command list and print help
              //
              Status = PrintDynamicCommandHelp (CurrentCommand, SectionToGetHelpOn, PrintCommandText);
            }
            if (Status == EFI_DEVICE_ERROR) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_INV), gShellLevel3HiiHandle, CurrentCommand);
            } else if (EFI_ERROR(Status)) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_NF), gShellLevel3HiiHandle, CurrentCommand);
            } else {
                Found = TRUE;
            }
          }
        }

        //
        // Search the .man file for Shell applications (Shell external commands).
        //
        if (!Found) {
          Status = ShellPrintHelp(CommandToGetHelpOn, SectionToGetHelpOn, FALSE);
          if (Status == EFI_DEVICE_ERROR) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_INV), gShellLevel3HiiHandle, CommandToGetHelpOn);
          } else if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_NF), gShellLevel3HiiHandle, CommandToGetHelpOn);
          } else {
            Found = TRUE;
          }
        }
      }

      if (!Found) {
        ShellStatus = SHELL_NOT_FOUND;
      }

      //
      // free the command line package
      //
      ShellCommandLineFreeVarList (Package);
    }
  }

  if (CommandToGetHelpOn != NULL && StrCmp(CommandToGetHelpOn, L"*") == 0){
    //
    // If '*' then the command entered was 'Help' without qualifiers, This footer
    // provides additional info on help switches
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_FOOTER), gShellLevel3HiiHandle);
  }
  if (CommandToGetHelpOn != NULL) {
    FreePool(CommandToGetHelpOn);
  }
  if (SectionToGetHelpOn != NULL) {
    FreePool(SectionToGetHelpOn);
  }
  SHELL_FREE_NON_NULL(SortedCommandList);

  return (ShellStatus);
}
