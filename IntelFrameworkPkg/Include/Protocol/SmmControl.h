/** @file
  This file declares the SMM Control abstraction protocol.
  This protocol is used to initiate SMI/PMI activations. This protocol could be published by either:
  - A processor driver to abstract the SMI/PMI IPI
  - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an
  Intel chipset
  Because of the possibility of performing SMI or PMI IPI transactions, the ability to generate this
  event from a platform chipset agent is an optional capability for both IA-32 and Itanium-based
  systems.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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
//
// SMM Access specification Data Structures
//
typedef struct {
  ///
  ///  Describes the I/O location of the particular port that engendered the synchronous
  ///  SMI. For example, this location can include but is not limited to the traditional
  ///  PCAT* APM port of 0B2h.
  ///
  UINT8 SmiTriggerRegister;
  ///
  ///  Describes the value that was written to the respective activation port.
  ///
  UINT8 SmiDataRegister;
} EFI_SMM_CONTROL_REGISTER;

//
// SMM Control specification member function
//
/**
  Invokes SMI activation from either the preboot or runtime environment.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  ArgumentBuffer        The optional sized data to pass into the protocol activation.
  @param  ArgumentBufferSize    The optional size of the data.
  @param  Periodic              An optional mechanism to periodically repeat activation.
  @param  ActivationInterval    An optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The timing is unsupported.
  @retval EFI_INVALID_PARAMETER The activation period is unsupported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ACTIVATE)(
  IN EFI_SMM_CONTROL_PROTOCOL                             *This,
  IN OUT INT8                                             *ArgumentBuffer OPTIONAL,
  IN OUT UINTN                                            *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                                              Periodic OPTIONAL,
  IN UINTN                                                ActivationInterval OPTIONAL
  );

/**
  Clears any system state that was created in response to the Active call.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  Periodic              Optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_DEACTIVATE)(
  IN EFI_SMM_CONTROL_PROTOCOL                   *This,
  IN BOOLEAN                                    Periodic OPTIONAL
  );

/**
  Provides information on the source register used to generate the SMI.

  @param  This                  The EFI_SMM_CONTROL_PROTOCOL instance.
  @param  SmiRegister           A pointer to the SMI register description structure.

  @retval EFI_SUCCESS           The register structure has been returned.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_REGISTER_INFO)(
  IN EFI_SMM_CONTROL_PROTOCOL           *This,
  IN OUT EFI_SMM_CONTROL_REGISTER       *SmiRegister
  );

/**
  @par Protocol Description:
  This protocol is used to initiate SMI/PMI activations.

  @param Trigger
  Initiates the SMI/PMI activation.

  @param Clear
  Quiesces the SMI/PMI activation.

  @param GetRegisterInfo
  Provides data on the register used as the source of the SMI.

  @param MinimumTriggerPeriod
  Minimum interval at which the platform can set the period.

  @retval EFI_SUCCESS The register structure has been returned.
**/

//
// SMM Control Protocol
//
/**
  This protocol is used to initiate SMI/PMI activations.
  This protocol could be published by either:
    - A processor driver to abstract the SMI/PMI IPI.
    - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an Intel chipset.
  Because of the possibility of performing SMI or PMI IPI transactions, the ability to generate this.

  The EFI_SMM_CONTROL_PROTOCOL is used by the platform chipset or processor driver. This
  protocol is usable both in boot services and at runtime. The runtime aspect enables an
  implementation of EFI_SMM_BASE_PROTOCOL.Communicate() to layer upon this service
  and provide an SMI callback from a general EFI runtime driver.
  This protocol provides an abstraction to the platform hardware that generates an
  SMI or PMI. There are often I/O ports that, when accessed, will engender the SMI or PMI.
  Also, this hardware optionally supports the periodic genearation of these signals.

**/
struct _EFI_SMM_CONTROL_PROTOCOL {
  ///
  ///  Initiates the SMI/PMI activation.
  ///
  EFI_SMM_ACTIVATE          Trigger;
  ///
  ///  Quiesces the SMI/PMI activation.
  ///
  EFI_SMM_DEACTIVATE        Clear;
  ///
  /// Provides data on the register used as the source of the SMI.
  ///
  EFI_SMM_GET_REGISTER_INFO GetRegisterInfo;
  ///
  /// Minimum interval at which the platform can set the period. A maximum is not
  /// specified in that the SMM infrastructure code can emulate a maximum interval that is
  /// greater than the hardware capabilities by using software emulation in the SMM
  /// infrastructure code.
  ///
  UINTN                     MinimumTriggerPeriod;
};

extern EFI_GUID gEfiSmmControlProtocolGuid;

#endif
