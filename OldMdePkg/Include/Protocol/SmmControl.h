/** @file
  This file declares SMM Control abstraction protocol.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmControl.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _SMM_CONTROL_H_
#define _SMM_CONTROL_H_

typedef struct _EFI_SMM_CONTROL_PROTOCOL              EFI_SMM_CONTROL_PROTOCOL;

#define EFI_SMM_CONTROL_PROTOCOL_GUID \
  { \
    0x8d12e231, 0xc667, 0x4fd1, {0x98, 0xf2, 0x24, 0x49, 0xa7, 0xe7, 0xb2, 0xe5 } \
  }

// SMM Access specification Data Structures
//
typedef struct {
  UINT8 SmiTriggerRegister;
  UINT8 SmiDataRegister;
} EFI_SMM_CONTROL_REGISTER;

//
// SMM Control specification member function
//
/**
  Invokes SMI activation from either the preboot or runtime environment.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  ArgumentBuffer        Optional sized data to pass into the protocol activation.
  @param  ArgumentBufferSize    Optional size of the data.
  @param  Periodic              Optional mechanism to engender a periodic stream.
  @param  ActivationInterval    Optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The timing is unsupported.
  @retval EFI_INVALID_PARAMETER The activation period is unsupported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ACTIVATE) (
  IN EFI_SMM_CONTROL_PROTOCOL                             *This,
  IN OUT INT8                                             *ArgumentBuffer OPTIONAL,
  IN OUT UINTN                                            *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                                              Periodic OPTIONAL,
  IN UINTN                                                ActivationInterval OPTIONAL
  );

/**
  Clears any system state that was created in response to the Active call.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  Periodic              Optional parameter to repeat at this period one time

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_DEACTIVATE) (
  IN EFI_SMM_CONTROL_PROTOCOL                   *This,
  IN BOOLEAN                                    Periodic OPTIONAL
  );

/**
  Provides information on the source register used to generate the SMI.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  SmiRegister           Pointer to the SMI register description structure

  @retval EFI_SUCCESS           The register structure has been returned.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_REGISTER_INFO) (
  IN EFI_SMM_CONTROL_PROTOCOL           *This,
  IN OUT EFI_SMM_CONTROL_REGISTER       *SmiRegister
  );

/**
  @par Protocol Description:
  This protocol is used initiate SMI/PMI activations. 

  @param Trigger
  Initiates the SMI/PMI activation.

  @param Clear
  Quiesces the SMI/PMI activation. 

  @param GetRegisterInfo
  Provides data on the register used as the source of the SMI.

  @param MinimumTriggerPeriod
  Minimum interval at which the platform can set the period. 

**/

struct _EFI_SMM_CONTROL_PROTOCOL {
  EFI_SMM_ACTIVATE          Trigger;
  EFI_SMM_DEACTIVATE        Clear;
  EFI_SMM_GET_REGISTER_INFO GetRegisterInfo;
  UINTN                     MinimumTriggerPeriod;
};

extern EFI_GUID gEfiSmmControlProtocolGuid;

#endif
