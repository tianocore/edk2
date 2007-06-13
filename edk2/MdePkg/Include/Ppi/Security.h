/** @file
  This file declares Security Architectural PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Security.h

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.0.

**/

#ifndef __SECURITY_PPI_H__
#define __SECURITY_PPI_H__

#define EFI_PEI_SECURITY_PPI_GUID \
  { \
    0x1388066e, 0x3a57, 0x4efa, {0x98, 0xf3, 0xc1, 0x2f, 0x3a, 0x95, 0x8a, 0x29 } \
  }

typedef struct _EFI_PEI_SECURITY_PPI  EFI_PEI_SECURITY_PPI;

/**
  Allows the platform builder to implement a security policy in response 
  to varying file authentication states.

  @param  PeiServices    Pointer to the PEI Services Table.
  @param  This           Interface pointer that implements the particular EFI_PEI_SECURITY_PPI instance.
  @param  AuthenticationStatus 
                         Status returned by the verification service as part of section extraction.
  @param  FfsFileHeader  Pointer to the file under review.
  @param  DeferExecution Pointer to a variable that alerts the PEI Foundation to defer execution of a PEIM.

  @retval EFI_SUCCESS           The service performed its action successfully.
  @retval EFI_SECURITY_VIOLATION The object cannot be trusted

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SECURITY_AUTHENTICATION_STATE) (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_SECURITY_PPI         *This,
  IN UINT32                       AuthenticationStatus,
  IN EFI_FFS_FILE_HEADER          *FfsFileHeader,
  IN OUT BOOLEAN                  *StartCrisisRecovery
  );

/**
  @par Ppi Description:
  This PPI is installed by some platform PEIM that abstracts the security 
  policy to the PEI Foundation, namely the case of a PEIM's authentication 
  state being returned during the PEI section extraction process. 

  @param AuthenticationState
  Allows the platform builder to implement a security policy in response 
  to varying file authentication states.

**/
struct _EFI_PEI_SECURITY_PPI {
  EFI_PEI_SECURITY_AUTHENTICATION_STATE  AuthenticationState;
};

extern EFI_GUID gEfiPeiSecurityPpiGuid;

#endif
