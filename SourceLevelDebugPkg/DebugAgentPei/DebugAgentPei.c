/** @file
  Initialize Debug Agent in PEI by invoking Debug Agent Library.

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
