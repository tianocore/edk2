/** @file
  Implement the constructor and destructor for the EFI socket library

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Socket.h"


/**
  EFI Socket Library Constructor

  @retval EFI_SUCCESS       The initialization was successful

 **/
EFI_STATUS
EFIAPI
EslConstructor (
  VOID
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Call the image dependent constructor if available
  //
  if ( NULL != mpfnEslConstructor ) {
    Status = mpfnEslConstructor ( );
  }

  //
  //  Return the constructor status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  EFI Socket Library Destructor

  @retval EFI_SUCCESS       The shutdown was successful

 **/
EFI_STATUS
EFIAPI
EslDestructor (
  VOID
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Call the image dependent destructor if available
  //
  if ( NULL != mpfnEslDestructor ) {
    Status = mpfnEslDestructor ( );
  }

  //
  //  Return the constructor status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
