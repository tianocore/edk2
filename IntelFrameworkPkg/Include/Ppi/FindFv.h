/** @file
  This file declares FindFv PPI, which is used to locate FVs that contain PEIMs in PEI.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in PEI CIS
  Version 0.91.

**/

#ifndef _FIND_FV_H_
#define _FIND_FV_H_

///
///  Inconsistent with specification here: 
///  GUID value format has been changed to the standard GUID format.
///
#define EFI_PEI_FIND_FV_PPI_GUID \
  { \
    0x36164812, 0xa023, 0x44e5, {0xbd, 0x85, 0x5, 0xbf, 0x3c, 0x77, 0x0, 0xaa } \
  }

typedef struct _EFI_PEI_FIND_FV_PPI EFI_PEI_FIND_FV_PPI;

/**
  This interface returns the base address of the firmware volume whose index
  was passed in FvNumber. Once this function reports a firmware volume
  index/base address pair, that index/address pairing must continue throughout PEI.

  @param  PeiServices    The pointer to the PEI Services Table.
  @param  This           Interface pointer that implements the Find FV service.
  @param  FvNumber       The index of the firmware volume to locate.
  @param  FvAddress      The address of the volume to discover.

  @retval EFI_SUCCESS           An additional firmware volume was found.
  @retval EFI_OUT_OF_RESOURCES  There are no firmware volumes for the given FvNumber.
  @retval EFI_INVALID_PARAMETER *FvAddress is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FIND_FV_FINDFV)(
  IN EFI_PEI_FIND_FV_PPI             *This,
  IN EFI_PEI_SERVICES                **PeiServices,
  IN UINT8                           *FvNumber,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FVAddress
  );

/**
  Hardware mechanisms for locating FVs in a platform vary widely.
  EFI_PEI_FIND_FV_PPI serves to abstract this variation so that the
  PEI Foundation can remain standard across a wide variety of platforms.
**/
struct _EFI_PEI_FIND_FV_PPI {
  EFI_PEI_FIND_FV_FINDFV  FindFv;   ///< Service that abstracts the location of additional firmware volumes.
};

extern EFI_GUID gEfiFindFvPpiGuid;

#endif
