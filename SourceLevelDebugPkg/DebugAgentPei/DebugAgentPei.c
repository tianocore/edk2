/** @file
  Initialize Debug Agent in PEI by invoking Debug Agent Library.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/DebugAgentLib.h>

/**
  The Entry Point for Debug Agent PEI driver.

  It will invoke Debug Agent Library to enable source debugging feature in PEI phase.

  This function is the Entry point of the CPU I/O PEIM which installs CpuIoPpi.

  @param[in]  FileHandle   Pointer to image file handle.
  @param[in]  PeiServices  Pointer to PEI Services Table

  @retval EFI_SUCCESS    Debug Agent successfully initialized.
  @retval other          Some error occurs when initialzed Debug Agent.

**/
EFI_STATUS
EFIAPI
DebugAgentPeiInitialize (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                 Status;

  Status = EFI_UNSUPPORTED;
  InitializeDebugAgent (DEBUG_AGENT_INIT_PEI, &Status, NULL);

  return Status;
}
