/** @file
  Tag GUID that must be installed by the TPM PEIM after the TPM hardware is
  initialized.  PEIMs that must execute after TPM hardware initialization
  may use this GUID in their dependency expressions.
    
Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_TPM_INITIALIZED_PPI_H_
#define _PEI_TPM_INITIALIZED_PPI_H_

///
/// Global ID for the PEI_TPM_INITIALIZED_PPI which always uses a NULL interface. 
///
#define PEI_TPM_INITIALIZED_PPI_GUID \
  { \
    0xe9db0d58, 0xd48d, 0x47f6, 0x9c, 0x6e, 0x6f, 0x40, 0xe8, 0x6c, 0x7b, 0x41 \
  }

extern EFI_GUID gPeiTpmInitializedPpiGuid;

///
/// Global ID for the PEI_TPM_INITIALIZATION_DONE_PPI which always uses a NULL interface. 
///
#define PEI_TPM_INITIALIZATION_DONE_PPI_GUID \
  { \
    0xa030d115, 0x54dd, 0x447b, { 0x90, 0x64, 0xf2, 0x6, 0x88, 0x3d, 0x7c, 0xcc \
  }

extern EFI_GUID gPeiTpmInitializationDonePpiGuid;

#endif
