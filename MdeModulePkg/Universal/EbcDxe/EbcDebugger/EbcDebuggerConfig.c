/** @file
  Configuration application for the EBC Debugger.

  Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/ShellParameters.h>

#include "EdbCommon.h"
#include "EdbSupport.h"

/**

  The function that displays the utility usage message.

**/
VOID
PrintUsage (
  VOID
  )
{
  Print (
    L"EbcDebuggerConfig Version 1.0\n"
    L"Copyright (C) Intel Corp 2007-2016. All rights reserved.\n"
    L"\n"
    L"Configure EbcDebugger in EFI Shell Environment.\n"
    L"\n"
    L"usage: EdbCfg <Command>\n"
    L"  CommandList:\n"
    L"    BO[C|CX|R|E|T|K] <ON|OFF> - Enable/Disable BOC/BOCX/BOR/BOE/BOT/BOK.\n"
//    L"    SHOWINFO                - Show Debugger Information.\n"
    L"\n"
    );
  return;
}

/**

  The function is to show some information.

  @param  DebuggerConfiguration    Point to the EFI_DEBUGGER_CONFIGURATION_PROTOCOL.

**/
VOID
EdbShowInfo (
  EFI_DEBUGGER_CONFIGURATION_PROTOCOL *DebuggerConfiguration
  )
{
  Print (L"Not supported!\n");
  return ;
}

/**

  EdbConfigBreak function.

  @param  DebuggerConfiguration    Point to the EFI_DEBUGGER_CONFIGURATION_PROTOCOL.
  @param  Command                  Point to the command.
  @param  CommandArg               The argument for this command.

**/
VOID
EdbConfigBreak (
  EFI_DEBUGGER_CONFIGURATION_PROTOCOL *DebuggerConfiguration,
  CHAR16                              *Command,
  CHAR16                              *CommandArg
  )
{
  EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate;

  DebuggerPrivate = (EFI_DEBUGGER_PRIVATE_DATA *)DebuggerConfiguration->DebuggerPrivateData;

  if (StriCmp (Command, L"BOC") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOC) == EFI_DEBUG_FLAG_EBC_BOC) {
        Print (L"BOC on\n");
      } else {
        Print (L"BOC off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOC;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOC;
    } else {
      Print (L"Invalid parameter\n");
    }
  } else if (StriCmp (Command, L"BOCX") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOCX) == EFI_DEBUG_FLAG_EBC_BOCX) {
        Print (L"BOCX on\n");
      } else {
        Print (L"BOCX off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOCX;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOCX;
    } else {
      Print (L"Invalid parameter\n");
    }
  } else if (StriCmp (Command, L"BOR") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOR) == EFI_DEBUG_FLAG_EBC_BOR) {
        Print (L"BOR on\n");
      } else {
        Print (L"BOR off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOR;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOR;
    } else {
      Print (L"Invalid parameter\n");
    }
  } else if (StriCmp (Command, L"BOE") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOE) == EFI_DEBUG_FLAG_EBC_BOE) {
        Print (L"BOE on\n");
      } else {
        Print (L"BOE off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOE;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOE;
    } else {
      Print (L"Invalid parameter\n");
    }
  } else if (StriCmp (Command, L"BOT") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOT) == EFI_DEBUG_FLAG_EBC_BOT) {
        Print (L"BOT on\n");
      } else {
        Print (L"BOT off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOT;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOT;
    } else {
      Print (L"Invalid parameter\n");
    }
  } else if (StriCmp (Command, L"BOK") == 0) {
    if (CommandArg == NULL) {
      if ((DebuggerPrivate->FeatureFlags & EFI_DEBUG_FLAG_EBC_BOK) == EFI_DEBUG_FLAG_EBC_BOK) {
        Print (L"BOK on\n");
      } else {
        Print (L"BOK off\n");
      }
    } else if (StriCmp (CommandArg, L"ON") == 0) {
      DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_BOK;
    } else if (StriCmp (CommandArg, L"OFF") == 0) {
      DebuggerPrivate->FeatureFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOK;
    } else {
      Print (L"Invalid parameter\n");
    }
  }
  return ;
}

/**
  Alter the EBC Debugger configuration.

  @param[in]  ImageHandle        The image handle.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Usage error.
  @retval EFI_NOT_FOUND          A running debugger cannot be located.
**/
EFI_STATUS
EFIAPI
InitializeEbcDebuggerConfig (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                               Argc;
  CHAR16                              **Argv;
  EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters;
  EFI_DEBUGGER_CONFIGURATION_PROTOCOL *DebuggerConfiguration;
  EFI_STATUS                          Status;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID**)&ShellParameters
                  );
  if (EFI_ERROR(Status)) {
    Print (L"Please use UEFI Shell to run this application.\n");
    return EFI_INVALID_PARAMETER;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;

  if (Argc < 2) {
    PrintUsage ();
    return EFI_INVALID_PARAMETER;
  }

  if (Argc == 2) {
    if ((StrCmp (Argv[1], L"/?") == 0) ||
        (StrCmp (Argv[1], L"-?") == 0) ||
        (StrCmp (Argv[1], L"-h") == 0) ||
        (StrCmp (Argv[1], L"-H") == 0) ) {
      PrintUsage ();
      return EFI_SUCCESS;
    }
  }

  Status = gBS->LocateProtocol (
                 &gEfiDebuggerConfigurationProtocolGuid,
                 NULL,
                 (VOID**)&DebuggerConfiguration
                 );
  if (EFI_ERROR(Status)) {
    Print (L"Error: DebuggerConfiguration protocol not found.\n");
    return EFI_NOT_FOUND;
  }

  if (StriCmp (Argv[1], L"SHOWINFO") == 0) {
    EdbShowInfo (DebuggerConfiguration);
    return EFI_SUCCESS;
  }

  if (((Argc == 2) || (Argc == 3)) &&
      ((StriCmp (Argv[1], L"BOC")  == 0) ||
       (StriCmp (Argv[1], L"BOCX") == 0) ||
       (StriCmp (Argv[1], L"BOR")  == 0) ||
       (StriCmp (Argv[1], L"BOE")  == 0) ||
       (StriCmp (Argv[1], L"BOT")  == 0) ||
       (StriCmp (Argv[1], L"BOK")  == 0))) {
    if (Argc == 3) {
      EdbConfigBreak (DebuggerConfiguration, Argv[1], Argv[2]);
    } else {
      EdbConfigBreak (DebuggerConfiguration, Argv[1], NULL);
    }
    return EFI_SUCCESS;
  }

  Print (L"Error: Invalid Command.\n");
  return EFI_INVALID_PARAMETER;
}
