/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (C) 2014, Red Hat, Inc.
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in, out] ShellParameters      Pointer to parameter structure to modify.
  @param[in] OldStdIn                  Pointer to old StdIn.
  @param[in] OldStdOut                 Pointer to old StdOut.
  @param[in] OldStdErr                 Pointer to old StdErr.
  @param[in] SystemTableInfo           Pointer to old system table information.
**/
EFI_STATUS
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN  SHELL_FILE_HANDLE                 *OldStdIn,
  IN  SHELL_FILE_HANDLE                 *OldStdOut,
  IN  SHELL_FILE_HANDLE                 *OldStdErr,
  IN  SYSTEM_TABLE_INFO                 *SystemTableInfo
  )
{
  SPLIT_LIST  *Split;

  if (  (ShellParameters == NULL)
     || (OldStdIn        == NULL)
     || (OldStdOut       == NULL)
     || (OldStdErr       == NULL)
     || (SystemTableInfo == NULL))
  {
    return (EFI_INVALID_PARAMETER);
  }

  if (!IsListEmpty (&ShellProtocolsInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST *)GetFirstNode (&ShellProtocolsInfoObject.SplitList.Link);
  } else {
    Split = NULL;
  }

  if (ShellParameters->StdIn  != *OldStdIn) {
    if (((Split != NULL) && (Split->SplitStdIn != ShellParameters->StdIn)) || (Split == NULL)) {
      gEfiShellProtocol->CloseFile (ShellParameters->StdIn);
    }

    ShellParameters->StdIn = *OldStdIn;
  }

  if (ShellParameters->StdOut != *OldStdOut) {
    if (((Split != NULL) && (Split->SplitStdOut != ShellParameters->StdOut)) || (Split == NULL)) {
      gEfiShellProtocol->CloseFile (ShellParameters->StdOut);
    }

    ShellParameters->StdOut = *OldStdOut;
  }

  if (ShellParameters->StdErr != *OldStdErr) {
    gEfiShellProtocol->CloseFile (ShellParameters->StdErr);
    ShellParameters->StdErr = *OldStdErr;
  }

  if (gST->ConIn != SystemTableInfo->ConIn) {
    CloseSimpleTextInOnFile (gST->ConIn);
    gST->ConIn           = SystemTableInfo->ConIn;
    gST->ConsoleInHandle = SystemTableInfo->ConInHandle;
  }

  if (gST->ConOut != SystemTableInfo->ConOut) {
    CloseSimpleTextOutOnFile (gST->ConOut);
    gST->ConOut           = SystemTableInfo->ConOut;
    gST->ConsoleOutHandle = SystemTableInfo->ConOutHandle;
  }

  if (gST->StdErr != SystemTableInfo->ErrOut) {
    CloseSimpleTextOutOnFile (gST->StdErr);
    gST->StdErr              = SystemTableInfo->ErrOut;
    gST->StandardErrorHandle = SystemTableInfo->ErrOutHandle;
  }

  CalculateEfiHdrCrc (&gST->Hdr);

  return (EFI_SUCCESS);
}
