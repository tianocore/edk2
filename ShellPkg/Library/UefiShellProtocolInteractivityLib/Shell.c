/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolInteractivityLib.h>
#include <Library/ShellProtocolsLib.h>
#include <Library/SortLib.h>

//
// Initialize the global structure
//
SHELL_PROTOCOL_INTERACTIVITY_INFO  ShellProtocolInteractivityInfoObject = {
  FALSE,
  {
    {
      { NULL,NULL   }, NULL
    },
    0,
    0,
    TRUE
  },
  NULL,
  0,
  NULL,
  {
    { NULL,NULL   }, NULL, NULL
  },
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  FALSE
};

/**
  Execute tasks for each round of the loop.

**/
VOID
EFIAPI
UefiShellProtocolInteractivityLibExecuteWaitLoopTasks (
  VOID
  )
{
  //
  // Reset page break back to default.
  //
  ShellProtocolInteractivityInfoObject.PageBreakEnabled = PcdGetBool (PcdShellPageBreakDefault);
  ASSERT (ShellProtocolInteractivityInfoObject.ConsoleInfo != NULL);
  ShellProtocolInteractivityInfoObject.ConsoleInfo->Enabled    = TRUE;
  ShellProtocolInteractivityInfoObject.ConsoleInfo->RowCounter = 0;
}

/**
  The entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiShellProtocolInteractivityLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Populate the global structure from PCDs
  //
  ShellProtocolInteractivityInfoObject.PageBreakEnabled           = PcdGetBool (PcdShellPageBreakDefault);
  ShellProtocolInteractivityInfoObject.ViewingSettings.InsertMode = PcdGetBool (PcdShellInsertModeDefault);
  ShellProtocolInteractivityInfoObject.LogScreenCount             = PcdGet8 (PcdShellScreenLogCount);

  //
  // verify we dont allow for spec violation
  //
  ASSERT (ShellProtocolInteractivityInfoObject.LogScreenCount >= 3);

  //
  // Initialize the LIST ENTRY objects...
  //
  InitializeListHead (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link);
  InitializeListHead (&ShellProtocolInteractivityInfoObject.SplitList.Link);

  //
  // install our console logger.  This will keep a log of the output for back-browsing
  //
  Status = ConsoleLoggerInstall (ShellProtocolInteractivityInfoObject.LogScreenCount, &ShellProtocolInteractivityInfoObject.ConsoleInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // install our (solitary) HII package
  //
  ShellProtocolInteractivityInfoObject.HiiHandle = HiiAddPackages (&gShellProtocolInteractiveHiiGuid, gImageHandle, ShellProtocolInteractivityLibStrings, NULL);
  if (ShellProtocolInteractivityInfoObject.HiiHandle == NULL) {
    return EFI_NOT_STARTED;
  }

  return EFI_SUCCESS;
}

/**
  The entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiShellProtocolInteractivityLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SPLIT_LIST  *Split;

  if (!IsListEmpty (&ShellProtocolInteractivityInfoObject.SplitList.Link)) {
    ASSERT (FALSE); /// @todo finish this de-allocation (free SplitStdIn/Out when needed).

    for ( Split = (SPLIT_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.SplitList.Link)
          ; !IsNull (&ShellProtocolInteractivityInfoObject.SplitList.Link, &Split->Link)
          ; Split = (SPLIT_LIST *)GetNextNode (&ShellProtocolInteractivityInfoObject.SplitList.Link, &Split->Link)
          )
    {
      RemoveEntryList (&Split->Link);
      FreePool (Split);
    }

    DEBUG_CODE (
      InitializeListHead (&ShellProtocolInteractivityInfoObject.SplitList.Link);
      );
  }

  if (ShellProtocolInteractivityInfoObject.HiiHandle != NULL) {
    HiiRemovePackages (ShellProtocolInteractivityInfoObject.HiiHandle);
    DEBUG_CODE (
      ShellProtocolInteractivityInfoObject.HiiHandle = NULL;
      );
  }

  if (!IsListEmpty (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link)) {
    FreeBufferList (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory);
  }

  ASSERT (ShellProtocolInteractivityInfoObject.ConsoleInfo != NULL);
  if (ShellProtocolInteractivityInfoObject.ConsoleInfo != NULL) {
    ConsoleLoggerUninstall (ShellProtocolInteractivityInfoObject.ConsoleInfo);
    FreePool (ShellProtocolInteractivityInfoObject.ConsoleInfo);
    DEBUG_CODE (
      ShellProtocolInteractivityInfoObject.ConsoleInfo = NULL;
      );
  }

  return EFI_SUCCESS;
}

/**
  Add a buffer to the Line History List

  @param Buffer     The line buffer to add.
**/
VOID
AddLineToCommandHistory (
  IN CONST CHAR16  *Buffer
  )
{
  BUFFER_LIST  *Node;
  BUFFER_LIST  *Walker;
  UINT16       MaxHistoryCmdCount;
  UINT16       Count;

  Count              = 0;
  MaxHistoryCmdCount = PcdGet16 (PcdShellMaxHistoryCommandCount);

  if (MaxHistoryCmdCount == 0) {
    return;
  }

  Node = AllocateZeroPool (sizeof (BUFFER_LIST));
  if (Node == NULL) {
    return;
  }

  Node->Buffer = AllocateCopyPool (StrSize (Buffer), Buffer);
  if (Node->Buffer == NULL) {
    FreePool (Node);
    return;
  }

  for ( Walker = (BUFFER_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link)
        ; !IsNull (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Walker->Link)
        ; Walker = (BUFFER_LIST *)GetNextNode (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Walker->Link)
        )
  {
    Count++;
  }

  if (Count < MaxHistoryCmdCount) {
    InsertTailList (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link);
  } else {
    Walker = (BUFFER_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link);
    RemoveEntryList (&Walker->Link);
    if (Walker->Buffer != NULL) {
      FreePool (Walker->Buffer);
    }

    FreePool (Walker);
    InsertTailList (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link);
  }
}
