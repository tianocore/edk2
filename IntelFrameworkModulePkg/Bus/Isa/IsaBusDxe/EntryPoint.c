/**@file
  Entry Point Source file.

  This file contains the user entry point 

  Copyright (c) 2006 - 2007, Intel Corporation.
  All rights reserved.
   This software and associated documentation (if any) is furnished
   under a license and may only be used or copied in accordance
   with the terms of the license. Except as permitted by such
   license, no part of this software or documentation may be
   reproduced, stored in a retrieval system, or transmitted in any
   form or by any means without the express written consent of
   Intel Corporation.
**/


//
// Include common header file for this module.
//
#include "InternalIsaBus.h"

/**
  The user Entry Point for module IsaBus. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeIsaBus(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallAllDriverProtocols (
             ImageHandle,
             SystemTable,
             &gIsaBusControllerDriver,
             ImageHandle,
             &gIsaBusComponentName,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}
