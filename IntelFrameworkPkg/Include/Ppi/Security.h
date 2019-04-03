/** @file
  This file declares the Security Architectural PPI.

  This PPI is installed by a platform PEIM that abstracts the security policy to the PEI
  Foundation, namely the case of a PEIM's authentication state being returned during the PEI section
  extraction process.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is defined in PEI CIS.
  Version 0.91.

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

  @param  PeiServices             The pointer to the PEI Services Table.
  @param  This                    Interface pointer that implements the particular
                                  EFI_PEI_SECURITY_PPI instance.
  @param  AuthenticationStatus    Status returned by the verification service as
                                  part of section extraction.
  @param  FfsFileHeader           The pointer to the file under review.
  @param  DeferExecution          The pointer to a variable that alerts the PEI
                                  Foundation to defer execution of a PEIM.

  @retval EFI_SUCCESS             The service performed its action successfully.
  @retval EFI_SECURITY_VIOLATION  The object cannot be trusted.
**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_PEI_SECURITY_AUTHENTICATION_STATE)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_SECURITY_PPI         *This,
  IN UINT32                       AuthenticationStatus,
  IN EFI_FFS_FILE_HEADER          *FfsFileHeader,
  IN OUT BOOLEAN                  *DeferExecution
  );

//
// PPI interface structure of Security PPI
//
struct _EFI_PEI_SECURITY_PPI {
  FRAMEWORK_EFI_PEI_SECURITY_AUTHENTICATION_STATE  AuthenticationState;
};

extern EFI_GUID gEfiPeiSecurityPpiGuid;

#endif
