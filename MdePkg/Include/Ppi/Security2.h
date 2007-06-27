/** @file
  This file declares Security2 Architectural PPI.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.0.

**/

#ifndef __SECURITY2_PPI_H__
#define __SECURITY2_PPI_H__

#define EFI_PEI_SECURITY2_PPI_GUID \
  { 0xdcd0be23, 0x9586, 0x40f4, { 0xb6, 0x43, 0x6, 0x52, 0x2c, 0xed, 0x4e, 0xde } }


typedef struct _EFI_PEI_SECURITY2_PPI  EFI_PEI_SECURITY2_PPI;

/**
   
  This service is published by some platform PEIM. The purpose of
  this service is to expose a given platform's policy-based
  response to the PEI Foundation. For example, if there is a PEIM
  in a GUIDed encapsulation section and the extraction of the PEI
  file section yields an authentication failure, there is no a
  priori policy in the PEI Foundation. Specifically, this
  situation leads to the question whether PEIMs that are either
  not in GUIDed sections or are in sections whose authentication
  fails should still be executed. In fact, it is the
  responsibility of the platform builder to make this decision.
  This platform-scoped policy is a result that a desktop system
  might not be able to skip or not execute PEIMs because the
  skipped PEIM could be the agent that initializes main memory.
  Alternately, a system may require that unsigned PEIMs not be
  executed under any circumstances. In either case, the PEI
  Foundation simply multiplexes access to the Section Extraction
  PPI and the Security PPI. The Section Extraction PPI determines
  the contents of a section, and the Security PPI tells the PEI
  Foundation whether or not to invoke the PEIM. The PEIM that
  publishes the AuthenticationState() service uses its parameters
  in the following ways: ?? AuthenticationStatus conveys the
  source information upon which the PEIM acts. 1) The
  DeferExecution value tells the PEI Foundation whether or not to
  dispatch the PEIM. In addition, between receiving the
  AuthenticationState() from the PEI Foundation and returning with
  the DeferExecution value, the PEIM that publishes
  AuthenticationState() can do the following: 2) Log the file
  state. 3) Lock the firmware hubs in response to an unsigned
  PEIM being discovered. These latter behaviors are platform-
  and market-specific and thus outside the scope of the PEI CIS.

  @param This   Interface pointer that implements the particular
                EFI_PEI_SECURITY2_PPI instance.


  @param AuthenticationStatus   Authentication status of the
                                file.

  @param FvHandle   Handle of the volume in which the file
                    resides. Type EFI_PEI_FV_HANDLE is defined
                    in FfsFindNextVolume. This allows different
                    policies depending on different firmware
                    volumes.

  @param FileHandle   Handle of the file under review. Type
                      EFI_PEI FILE HANDLE is defined in
                      FfsFindNextFile.

  @param DeferExecution   Pointer to a variable that alerts the
                          PEI Foundation to defer execution of a
                          PEIM.

  @retval EFI_SUCCESS   The service performed its action
                        successfully.

  @retval EFI_SECURITY_VIOLATION  The object cannot be trusted.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SECURITY_AUTHENTICATION_STATE) (
  IN CONST  EFI_PEI_SERVICES      **PeiServices,
  IN CONST  EFI_PEI_SECURITY2_PPI *This,
  IN CONST  UINT32                AuthenticationStatus,
  IN CONST  EFI_PEI_FV_HANDLE     FvHandle,
  IN CONST  EFI_PEI_FV_HANDLE     FileHandle,
  IN OUT    BOOLEAN               *DeferExecution
);

/**
   
  This PPI is a means by which the platform builder can indicate
  a response to a PEIM's authentication state. This can be in
  the form of a requirement for the PEI Foundation to skip a
  module using the DeferExecution Boolean output in the
  AuthenticationState() member function. Alternately, the
  Security PPI can invoke something like a cryptographic PPI
  that hashes the PEIM contents to log attestations, for which
  the FileHandle parameter in AuthenticationState() will be
  useful. If this PPI does not exist, PEIMs will be considered
  trusted.

  @param AuthenticationState  Allows the platform builder to
                              implement a security policy in
                              response to varying file
                              authentication states. See the
                              AuthenticationState() function
                              description.

**/
struct _EFI_PEI_SECURITY2_PPI {
  EFI_PEI_SECURITY_AUTHENTICATION_STATE   AuthenticationState;
};


extern EFI_GUID gEfiPeiSecurity2PpiGuid;

#endif
