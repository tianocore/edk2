/** @file
  EFI SMM Access PPI definition.

  This PPI is used to control the visibility of the SMRAM on the platform.
  It abstracts the location and characteristics of SMRAM.  The expectation is
  that the north bridge or memory controller would publish this PPI.

  The principal functionality found in the memory controller includes the following: 
  - Exposing the SMRAM to all non-SMM agents, or the "open" state
  - Shrouding the SMRAM to all but the SMM agents, or the "closed" state
  - Preserving the system integrity, or "locking" the SMRAM, such that the settings cannot be 
    perturbed by either boot service or runtime agents 

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_ACCESS_PPI_H_
#define _SMM_ACCESS_PPI_H_

#define PEI_SMM_ACCESS_PPI_GUID \
  { 0x268f33a9, 0xcccd, 0x48be, { 0x88, 0x17, 0x86, 0x5, 0x3a, 0xc3, 0x2e, 0xd6 }}

typedef struct _PEI_SMM_ACCESS_PPI  PEI_SMM_ACCESS_PPI;

/**
  Opens the SMRAM area to be accessible by a PEIM driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM. The function 
  should return EFI_DEVICE_ERROR if the SMRAM configuration is locked.

  @param  PeiServices            General purpose services available to every PEIM.
  @param  This                   The pointer to the SMM Access Interface.
  @param  DescriptorIndex        The region of SMRAM to Open.
  
  @retval EFI_SUCCESS            The region was successfully opened.
  @retval EFI_DEVICE_ERROR       The region could not be opened because locked by chipset.
  @retval EFI_INVALID_PARAMETER  The descriptor index was out of bounds.
  
**/
typedef
EFI_STATUS
(EFIAPI *PEI_SMM_OPEN)(
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM.

  @param  PeiServices              General purpose services available to every PEIM.
  @param  This                     The pointer to the SMM Access Interface.
  @param  DescriptorIndex          The region of SMRAM to Close.
  
  @retval EFI_SUCCESS              The region was successfully closed.
  @retval EFI_DEVICE_ERROR         The region could not be closed because locked by chipset.                           
  @retval EFI_INVALID_PARAMETER    The descriptor index was out of bounds.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_SMM_CLOSE)(
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually implemented such 
  that it is a write-once operation. 

  @param  PeiServices              General purpose services available to every PEIM.
  @param  This                     The pointer to the SMM Access Interface.
  @param  DescriptorIndex          The region of SMRAM to Close.
  
  @retval EFI_SUCCESS            The region was successfully locked.
  @retval EFI_DEVICE_ERROR       The region could not be locked because at least
                                 one range is still open.
  @retval EFI_INVALID_PARAMETER  The descriptor index was out of bounds.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_SMM_LOCK)(
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  );

/**
  Queries the memory controller for the possible regions that will support SMRAM.

  @param  PeiServices           General purpose services available to every PEIM.
  @param This                   The pointer to the SmmAccessPpi Interface.
  @param SmramMapSize           The pointer to the variable containing size of the
                                buffer to contain the description information.
  @param SmramMap               The buffer containing the data describing the Smram
                                region descriptors.
  
  @retval EFI_BUFFER_TOO_SMALL  The user did not provide a sufficient buffer.
  @retval EFI_SUCCESS           The user provided a sufficiently-sized buffer.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_SMM_CAPABILITIES)(
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  );

///
///  EFI SMM Access PPI is used to control the visibility of the SMRAM on the platform.
///  It abstracts the location and characteristics of SMRAM.  The expectation is
///  that the north bridge or memory controller would publish this PPI.
/// 
struct _PEI_SMM_ACCESS_PPI {
  PEI_SMM_OPEN          Open;
  PEI_SMM_CLOSE         Close;
  PEI_SMM_LOCK          Lock;
  PEI_SMM_CAPABILITIES  GetCapabilities;
  BOOLEAN               LockState;
  BOOLEAN               OpenState;
};

extern EFI_GUID gPeiSmmAccessPpiGuid;

#endif
