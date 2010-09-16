/** @file
  Allows an override of the SEC SEC PPI Dispatch Table. This allows 
  customized PPIs to be passed into the PEI Core.

Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __SEC_DISPATCH_TABLE_LIB_H__
#define __SEC_DISPATCH_TABLE_LIB_H__

/**
  Allow an override of the Sec PPI Dispatch Table. This table contains PPIs passed
  up from SEC to PEI. This function is responcible for allocating space for the 
  overridden table.


  @param  OriginalTable  SECs default PPI dispatch table

  @return OriginalTable or override of the table

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
OverrideDispatchTable (
  IN  CONST EFI_PEI_PPI_DESCRIPTOR  *OriginalTable
  );


#endif
