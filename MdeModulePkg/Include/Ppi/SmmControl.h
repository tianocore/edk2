/** @file
  EFI SMM Control PPI definition.

  This PPI is used to initiate SMI/PMI activations. This protocol could be published by either:
  - A processor driver to abstract the SMI/PMI IPI
  - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an
  Intel chipset
  Because of the possibility of performing SMI or PMI IPI transactions, the ability to generate this
  event from a platform chipset agent is an optional capability for both IA-32 and Itanium-based
  systems.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _SMM_CONTROL_PPI_H_
#define _SMM_CONTROL_PPI_H_

#define PEI_SMM_CONTROL_PPI_GUID \
  { 0x61c68702, 0x4d7e, 0x4f43, 0x8d, 0xef, 0xa7, 0x43, 0x5, 0xce, 0x74, 0xc5 }

typedef struct _PEI_SMM_CONTROL_PPI  PEI_SMM_CONTROL_PPI;

/**
  Invokes SMI activation from either the preboot or runtime environment.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  This                  The PEI_SMM_CONTROL_PPI instance.
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
(EFIAPI *PEI_SMM_ACTIVATE) (
  IN EFI_PEI_SERVICES                                **PeiServices,
  IN PEI_SMM_CONTROL_PPI                             * This,
  IN OUT INT8                                        *ArgumentBuffer OPTIONAL,
  IN OUT UINTN                                       *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                                         Periodic OPTIONAL,
  IN UINTN                                           ActivationInterval OPTIONAL
  );

/**
  Clears any system state that was created in response to the Active call.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  This                  The PEI_SMM_CONTROL_PPI instance.
  @param  Periodic              Optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_SMM_DEACTIVATE) (
  IN EFI_PEI_SERVICES                      **PeiServices,
  IN PEI_SMM_CONTROL_PPI                   * This,
  IN BOOLEAN                               Periodic OPTIONAL
  );

///
///  PEI SMM Control PPI is used to initiate SMI/PMI activations. This protocol could be published by either:
///  - A processor driver to abstract the SMI/PMI IPI
///  - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an
///  Intel chipset
///
struct _PEI_SMM_CONTROL_PPI {
  PEI_SMM_ACTIVATE    Trigger;
  PEI_SMM_DEACTIVATE  Clear;
};

extern EFI_GUID gPeiSmmControlPpiGuid;

#endif
