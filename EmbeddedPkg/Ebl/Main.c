/** @file
  Basic command line parser for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Ebl.h"

// Globals for command history processing
INTN mCmdHistoryEnd     = -1;
INTN mCmdHistoryStart   = -1;
INTN mCmdHistoryCurrent = -1;
CHAR8 mCmdHistory[MAX_CMD_HISTORY][MAX_CMD_LINE];
CHAR8 *mCmdBlank = "";

// Globals to remember current screen geometry
UINTN gScreenColumns;
UINTN gScreenRows;

// Global to turn on/off breaking commands with prompts before they scroll the screen
BOOLEAN gPageBreak = TRUE;

VOID
RingBufferIncrement (
  IN  INTN  *Value
  )
{
  *Value = *Value + 1;

  if (*Value >= MAX_CMD_HISTORY) {
    *Value = 0;
  }
}

VOID
RingBufferDecrement (
  IN  INTN  *Value
  )
{
  *Value = *Value - 1;

  if (*Value < 0) {
    *Value = MAX_CMD_HISTORY - 1;
  }
}

/**
  Save this command in the circular history buffer. Older commands are
  overwritten with newer commands.

  @param  Cmd   Command line to archive the history of.

  @return None

**/
VOID
SetCmdHistory (
  IN  CHAR8 *Cmd
  )
{
  // Don't bother adding empty commands to the list
  if (AsciiStrLen(Cmd) != 0) {

    // First entry
    if (mCmdHistoryStart == -1) {
      mCmdHistoryStart   = 0;
      mCmdHistoryEnd     = 0;
    } else {
      // Record the new command at the next index
      RingBufferIncrement(&mCmdHistoryStart);

      // If the next index runs into the end index, shuffle end back by one
      if (mCmdHistoryStart == mCmdHistoryEnd) {
        RingBufferIncrement(&mCmdHistoryEnd);
      }
    }

    // Copy the new command line into the ring buffer
    AsciiStrnCpy(&mCmdHistory[mCmdHistoryStart][0], Cmd, MAX_CMD_LINE);
  }

  // Reset the command history for the next up arrow press
  mCmdHistoryCurrent = mCmdHistoryStart;
}


/**
  Retreave data from the Command History buffer. Direction maps into up arrow
  an down arrow on the command line

  @param  Direction  Command forward or back

  @return The Command history based on the Direction

**/
CHAR8 *
GetCmdHistory (
  IN UINT16   Direction
  )
{
  CHAR8 *HistoricalCommand = NULL;

  // No history yet?
  if (mCmdHistoryCurrent == -1) {
    HistoricalCommand = mCmdBlank;
    goto Exit;
  }

  if (Direction == SCAN_UP) {
    HistoricalCommand = &mCmdHistory[mCmdHistoryCurrent][0];

    // if we just echoed the last command, hang out there, don't wrap around
    if (mCmdHistoryCurrent == mCmdHistoryEnd) {
      goto Exit;
    }

    // otherwise, back up by one
    RingBufferDecrement(&mCmdHistoryCurrent);

  } else if (Direction == SCAN_DOWN) {

    // if we last echoed the start command, put a blank prompt out
    if (mCmdHistoryCurrent == mCmdHistoryStart) {
      HistoricalCommand = mCmdBlank;
      goto Exit;
    }

    // otherwise increment the current pointer and return that command
    RingBufferIncrement(&mCmdHistoryCurrent);
    RingBufferIncrement(&mCmdHistoryCurrent);

    HistoricalCommand = &mCmdHistory[mCmdHistoryCurrent][0];
    RingBufferDecrement(&mCmdHistoryCurrent);
  }

Exit:
  return HistoricalCommand;
}


/**
  Parse the CmdLine and break it up into Argc (arg count) and Argv (array of
  pointers to each argument). The Cmd buffer is altered and separators are
  converted to string terminators. This allows Argv to point into CmdLine.
  A CmdLine can support multiple commands. The next command in the command line
  is returned if it exists.

  @param  CmdLine String to parse for a set of commands
  @param  Argc    Returns the number of arguments in the CmdLine current command
  @param  Argv    Argc pointers to each string in CmdLine

  @return Next Command in the command line or NULL if non exists
**/
CHAR8 *
ParseArguments (
  IN  CHAR8  *CmdLine,
  OUT UINTN  *Argc,
  OUT CHAR8  **Argv
  )
{
  UINTN   Arg;
  CHAR8   *Char;
  BOOLEAN LookingForArg;
  BOOLEAN InQuote;

  *Argc = 0;
  if (AsciiStrLen (CmdLine) == 0) {
    return NULL;
  }

  // Walk a single command line. A CMD_SEPARATOR allows multiple commands on a single line
  InQuote       = FALSE;
  LookingForArg = TRUE;
  for (Char = CmdLine, Arg = 0; *Char != '\0'; Char++) {
    if (!InQuote && *Char == CMD_SEPARATOR) {
      break;
    }

    // Perform any text conversion here
    if (*Char == '\t') {
      // TAB to space
      *Char = ' ';
    }

    if (LookingForArg) {
      // Look for the beginning of an Argv[] entry
      if (*Char == '"') {
        Argv[Arg++] = ++Char;
        LookingForArg = FALSE;
        InQuote = TRUE;
      } else if (*Char != ' ') {
        Argv[Arg++] = Char;
        LookingForArg = FALSE;
      }
    } else {
      // Looking for the terminator of an Argv[] entry
      if (!InQuote && (*Char == ' ')) {
        *Char = '\0';
        LookingForArg = TRUE;
      } else if (!InQuote && (*Char == '"') && (*(Char-1) != '\\')) {
        InQuote = TRUE;
      } else if (InQuote && (*Char == '"') && (*(Char-1) != '\\')) {
        *Char = '\0';
        InQuote = FALSE;
      }
    }
  }

  *Argc = Arg;

  if (*Char == CMD_SEPARATOR) {
    // Replace the command delimiter with null and return pointer to next command line
    *Char = '\0';
    return ++Char;
  }

  return NULL;
}


/**
  Return a keypress or optionally timeout if a timeout value was passed in.
  An optional callback function is called every second when waiting for a
  timeout.

  @param  Key           EFI Key information returned
  @param  TimeoutInSec  Number of seconds to wait to timeout
  @param  CallBack      Callback called every second during the timeout wait

  @return EFI_SUCCESS  Key was returned
  @return EFI_TIMEOUT  If the TimoutInSec expired

**/
EFI_STATUS
EblGetCharKey (
  IN OUT EFI_INPUT_KEY            *Key,
  IN     UINTN                    TimeoutInSec,
  IN     EBL_GET_CHAR_CALL_BACK   CallBack   OPTIONAL
  )
{
  EFI_STATUS    Status;
  UINTN         WaitCount;
  UINTN         WaitIndex;
  EFI_EVENT     WaitList[2];

  WaitCount   = 1;
  WaitList[0] = gST->ConIn->WaitForKey;
  if (TimeoutInSec != 0) {
    // Create a time event for 1 sec duration if we have a timeout
    gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &WaitList[1]);
    gBS->SetTimer (WaitList[1], TimerPeriodic, EFI_SET_TIMER_TO_SECOND);
    WaitCount++;
  }

  for (;;) {
    Status = gBS->WaitForEvent (WaitCount, WaitList, &WaitIndex);
    ASSERT_EFI_ERROR (Status);

    switch (WaitIndex) {
    case 0:
      // Key event signaled
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
      if (!EFI_ERROR (Status)) {
        if (WaitCount == 2) {
          gBS->CloseEvent (WaitList[1]);
        }
        return EFI_SUCCESS;
      }
      break;

    case 1:
      // Periodic 1 sec timer signaled
      TimeoutInSec--;
      if (CallBack != NULL) {
        // Call the users callback function if registered
        CallBack (TimeoutInSec);
      }
      if (TimeoutInSec == 0) {
        gBS->CloseEvent (WaitList[1]);
        return EFI_TIMEOUT;
      }
      break;
    default:
      ASSERT (FALSE);
    }
  }
}


/**
  This routine is used prevent command output data from scrolling off the end
  of the screen. The global gPageBreak is used to turn on or off this feature.
  If the CurrentRow is near the end of the screen pause and print out a prompt
  If the use hits Q to quit return TRUE else for any other key return FALSE.
  PrefixNewline is used to figure out if a newline is needed before the prompt
  string. This depends on the last print done before calling this function.
  CurrentRow is updated by one on a call or set back to zero if a prompt is
  needed.

  @param  CurrentRow  Used to figure out if its the end of the page and updated
  @param  PrefixNewline  Did previous print issue a newline

  @return TRUE if Q was hit to quit, FALSE in all other cases.

**/
BOOLEAN
EblAnyKeyToContinueQtoQuit (
  IN  UINTN   *CurrentRow,
  IN  BOOLEAN PrefixNewline
  )
{
  EFI_INPUT_KEY     InputKey;

  if (!gPageBreak) {
    // global disable for this feature
    return FALSE;
  }

  if (*CurrentRow >= (gScreenRows - 2)) {
    if (PrefixNewline) {
      AsciiPrint ("\n");
    }
    AsciiPrint ("Any key to continue (Q to quit): ");
    EblGetCharKey (&InputKey, 0, NULL);
    AsciiPrint ("\n");

    // Time to promt to stop the screen. We have to leave space for the prompt string
    *CurrentRow = 0;
    if (InputKey.UnicodeChar == 'Q' || InputKey.UnicodeChar == 'q') {
      return TRUE;
    }
  } else {
    *CurrentRow += 1;
  }

  return FALSE;
}


/**
  Set the text color of the EFI Console. If a zero is passed in reset to
  default text/background color.

  @param  Attribute   For text and background color

**/
VOID
EblSetTextColor (
  UINTN   Attribute
  )
{
  if (Attribute == 0) {
    // Set the text color back to default
    Attribute = (UINTN)PcdGet32 (PcdEmbeddedDefaultTextColor);
  }

  gST->ConOut->SetAttribute (gST->ConOut, Attribute);
}


/**
  Collect the keyboard input for a cmd line. Carriage Return, New Line, or ESC
  terminates the command line. You can edit the command line via left arrow,
  delete and backspace and they all back up and erase the command line.
  No edit of command line is possible without deletion at this time!
  The up arrow and down arrow fill Cmd with information from the history
  buffer.

  @param  Cmd         Command line to return
  @param  CmdMaxSize  Maximum size of Cmd

  @return The Status of EblGetCharKey()

**/
EFI_STATUS
GetCmd (
  IN OUT  CHAR8   *Cmd,
  IN      UINTN   CmdMaxSize
  )
{
  EFI_STATUS    Status;
  UINTN         Index;
  UINTN         Index2;
  CHAR8         Char;
  CHAR8         *History;
  EFI_INPUT_KEY Key;

  for (Index = 0; Index < CmdMaxSize - 1;) {
    Status = EblGetCharKey (&Key, 0, NULL);
    if (EFI_ERROR (Status)) {
      Cmd[Index] = '\0';
      AsciiPrint ("\n");
      return Status;
    }

    Char = (CHAR8)Key.UnicodeChar;
    if ((Char == '\n') || (Char == '\r') || (Char == 0x7f)) {
      Cmd[Index] = '\0';
      if (FixedPcdGetBool(PcdEmbeddedShellCharacterEcho) == TRUE) {
        AsciiPrint ("\n\r");
      }
      return EFI_SUCCESS;
    } else if ((Char == '\b') || (Key.ScanCode == SCAN_LEFT) || (Key.ScanCode == SCAN_DELETE)){
      if (Index != 0) {
        Index--;
        //
        // Update the display
        //
        AsciiPrint ("\b \b");
      }
    } else if ((Key.ScanCode == SCAN_UP) || Key.ScanCode == SCAN_DOWN) {
      History = GetCmdHistory (Key.ScanCode);
      //
      // Clear display line
      //
      for (Index2 = 0; Index2 < Index; Index2++) {
        AsciiPrint ("\b \b");
      }
      AsciiPrint (History);
      Index = AsciiStrLen (History);
      AsciiStrnCpy (Cmd, History, CmdMaxSize);
    } else {
      Cmd[Index++] = Char;
      if (FixedPcdGetBool(PcdEmbeddedShellCharacterEcho) == TRUE) {
        AsciiPrint ("%c", Char);
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Print the boot up banner for the EBL.
**/
VOID
EblPrintStartupBanner (
  VOID
  )
{
  AsciiPrint ("Embedded Boot Loader (");
  EblSetTextColor (EFI_YELLOW);
  AsciiPrint ("EBL");
  EblSetTextColor (0);
  AsciiPrint (") prototype. Built at %a on %a\n",__TIME__, __DATE__);
  AsciiPrint ("THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN 'AS IS' BASIS,\nWITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.\n");
  AsciiPrint ("Please send feedback to edk2-devel@lists.sourceforge.net\n");
}


/**
  Send null requests to all removable media block IO devices so the a media add/remove/change
  can be detected in real before we execute a command.

  This is mainly due to the fact that the FAT driver does not do this today so you can get stale
  dir commands after an SD Card has been removed.
**/
VOID
EblProbeRemovableMedia (
  VOID
  )
{
  UINTN         Index;
  UINTN         Max;
  EFI_OPEN_FILE *File;

  //
  // Probe for media insertion/removal in removable media devices
  //
  Max = EfiGetDeviceCounts (EfiOpenBlockIo);
  if (Max != 0) {
    for (Index = 0; Index < Max; Index++) {
      File = EfiDeviceOpenByType (EfiOpenBlockIo, Index);
      if (File != NULL) {
        if (File->FsBlockIoMedia->RemovableMedia) {
          // Probe to see if media is present (or not) or media changed
          //  this causes the ReinstallProtocolInterface() to fire in the
          //  block io driver to update the system about media change events
          File->FsBlockIo->ReadBlocks (File->FsBlockIo, File->FsBlockIo->Media->MediaId, (EFI_LBA)0, 0, NULL);
        }
        EfiClose (File);
      }
    }
  }
}




/**
  Print the prompt for the EBL.
**/
VOID
EblPrompt (
  VOID
  )
{
  EblSetTextColor (EFI_YELLOW);
  AsciiPrint ("%a %a",(CHAR8 *)PcdGetPtr (PcdEmbeddedPrompt), EfiGetCwd ());
  EblSetTextColor (0);
  AsciiPrint ("%a", ">");
}



/**
  Parse a command line and execute the commands. The ; separator allows
  multiple commands for each command line. Stop processing if one of the
  commands returns an error.

  @param  CmdLine          Command Line to process.
  @param  MaxCmdLineSize   MaxSize of the Command line

  @return EFI status of the Command

**/
EFI_STATUS
ProcessCmdLine (
  IN CHAR8      *CmdLine,
  IN UINTN      MaxCmdLineSize
  )
{
  EFI_STATUS          Status;
  EBL_COMMAND_TABLE   *Cmd;
  CHAR8               *Ptr;
  UINTN               Argc;
  CHAR8               *Argv[MAX_ARGS];

  // Parse the command line. The loop processes commands separated by ;
  for (Ptr = CmdLine, Status = EFI_SUCCESS; Ptr != NULL;) {
    Ptr = ParseArguments (Ptr, &Argc, Argv);
    if (Argc != 0) {
      Cmd = EblGetCommand (Argv[0]);
      if (Cmd != NULL) {
        // Execute the Command!
        Status = Cmd->Command (Argc, Argv);
        if (Status == EFI_ABORTED) {
          // exit command so lets exit
          break;
        } else if (Status == EFI_TIMEOUT) {
          // pause command got input so don't process any more cmd on this cmd line
          break;
        } else if (EFI_ERROR (Status)) {
          AsciiPrint ("%a returned %r error\n", Cmd->Name, Status);
          // if any command fails stop processing CmdLine
          break;
        }
      } else {
        AsciiPrint ("The command '%a' is not supported.\n", Argv[0]);
      }
    }
  }

  return Status;
}



/**
  Embedded Boot Loader (EBL) - A simple EFI command line application for embedded
  devices. PcdEmbeddedAutomaticBootCommand is a complied in command line that
  gets executed automatically. The ; separator allows multiple commands
  for each command line.

  @param  ImageHandle   EFI ImageHandle for this application.
  @param  SystemTable   EFI system table

  @return EFI status of the application

**/
EFI_STATUS
EFIAPI
EdkBootLoaderEntry (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  CHAR8       CmdLine[MAX_CMD_LINE];
  CHAR16      *CommandLineVariable = NULL;
  CHAR16      *CommandLineVariableName = L"default-cmdline";
  UINTN       CommandLineVariableSize = 0;
  EFI_GUID    VendorGuid;

  // Initialize tables of commands
  EblInitializeCmdTable ();
  EblInitializeDeviceCmd ();
  EblInitializemdHwDebugCmds ();
  EblInitializemdHwIoDebugCmds ();
  EblInitializeDirCmd ();
  EblInitializeHobCmd ();
  EblInitializeScriptCmd ();
  EblInitializeExternalCmd ();
  EblInitializeNetworkCmd();
  EblInitializeVariableCmds ();

  if (gST->ConOut == NULL) {
    DEBUG((EFI_D_ERROR,"Error: No Console Output\n"));
    return EFI_NOT_READY;
  }

  // Disable the 5 minute EFI watchdog time so we don't get automatically reset
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  if (FeaturePcdGet (PcdEmbeddedMacBoot)) {
    // A MAC will boot in graphics mode, so turn it back to text here
    // This protocol was removed from edk2. It is only an edk thing. We need to make our own copy.
    // DisableQuietBoot ();

    // Enable the biggest output screen size possible
    gST->ConOut->SetMode (gST->ConOut, (UINTN)gST->ConOut->Mode->MaxMode - 1);

  }

  // Save current screen mode
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &gScreenColumns, &gScreenRows);

  EblPrintStartupBanner ();

  // Parse command line and handle commands separated by ;
  // The loop prints the prompt gets user input and saves history

  // Look for a variable with a default command line, otherwise use the Pcd
  ZeroMem(&VendorGuid, sizeof(EFI_GUID));

  Status = gRT->GetVariable(CommandLineVariableName, &VendorGuid, NULL, &CommandLineVariableSize, CommandLineVariable);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    CommandLineVariable = AllocatePool(CommandLineVariableSize);

    Status = gRT->GetVariable(CommandLineVariableName, &VendorGuid, NULL, &CommandLineVariableSize, CommandLineVariable);
    if (!EFI_ERROR(Status)) {
      UnicodeStrToAsciiStr(CommandLineVariable, CmdLine);
    }

    FreePool(CommandLineVariable);
  }

  if (EFI_ERROR(Status)) {
    AsciiStrCpy (CmdLine, (CHAR8 *)PcdGetPtr (PcdEmbeddedAutomaticBootCommand));
  }

  for (;;) {
    Status = ProcessCmdLine (CmdLine, MAX_CMD_LINE);
    if (Status == EFI_ABORTED) {
      // if a command returns EFI_ABORTED then exit the EBL
      EblShutdownExternalCmdTable ();
      return EFI_SUCCESS;
    }

    // get the command line from the user
    EblPrompt ();
    GetCmd (CmdLine, MAX_CMD_LINE);
    SetCmdHistory (CmdLine);

    if (FeaturePcdGet (PcdEmbeddedProbeRemovable)) {
      // Probe removable media devices to see if media has been inserted or removed.
      EblProbeRemovableMedia ();
    }
  }
}


