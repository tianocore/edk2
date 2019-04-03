/** @file
This module provides an implementation of the SMM Control PPI for use with
the QNC.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/SmmControl.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#include <IntelQNCPeim.h>
#include <Library/QNCAccessLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Generates an SMI using the parameters passed in.

  @param  PeiServices         Describes the list of possible PEI Services.
  @param  This                A pointer to an instance of
                              EFI_SMM_CONTROL_PPI
  @param  ArgumentBuffer      The argument buffer
  @param  ArgumentBufferSize  The size of the argument buffer
  @param  Periodic            TRUE to indicate a periodical SMI
  @param  ActivationInterval  Interval of the periodical SMI

  @retval EFI_INVALID_PARAMETER  Periodic is TRUE or ArgumentBufferSize > 1
  @retval EFI_SUCCESS            SMI generated

**/
EFI_STATUS
EFIAPI
PeiActivate (
  IN      EFI_PEI_SERVICES           **PeiServices,
  IN      PEI_SMM_CONTROL_PPI        *This,
  IN OUT  INT8                       *ArgumentBuffer OPTIONAL,
  IN OUT  UINTN                      *ArgumentBufferSize OPTIONAL,
  IN      BOOLEAN                    Periodic OPTIONAL,
  IN      UINTN                      ActivationInterval OPTIONAL
  );

/**
  Clears an SMI.

  @param  PeiServices         Describes the list of possible PEI Services.
  @param  This                Pointer to an instance of EFI_SMM_CONTROL_PPI
  @param  Periodic            TRUE to indicate a periodical SMI

  @return Return value from SmmClear()

**/
EFI_STATUS
EFIAPI
PeiDeactivate (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  PEI_SMM_CONTROL_PPI         *This,
  IN  BOOLEAN                     Periodic OPTIONAL
  );

PEI_SMM_CONTROL_PPI      mSmmControlPpi = {
  PeiActivate,
  PeiDeactivate
};

EFI_PEI_PPI_DESCRIPTOR   mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiSmmControlPpiGuid,
  &mSmmControlPpi
};

/**
  Clear SMI related chipset status and re-enable SMI by setting the EOS bit.

  @retval EFI_SUCCESS       The requested operation has been carried out successfully
  @retval EFI_DEVICE_ERROR  The EOS bit could not be set.

**/
EFI_STATUS
SmmClear (
  VOID
  )
{
  UINT16                       GPE0BLK_Base;

  //
  // Get GPE0BLK_Base
  //
  GPE0BLK_Base = PcdGet16 (PcdGpe0blkIoBaseAddress);

  //
  // Clear the Power Button Override Status Bit, it gates EOS from being set.
  // In QuarkNcSocId - Bit is read only. Handled by external SMC, do nothing.
  //

  //
  // Clear the APM SMI Status Bit
  //
  IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_APM);

  //
  // Set the EOS Bit
  //
  IoOr32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_EOS);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SmmTrigger (
  IN UINT8   Data
  )
/*++

Routine Description:

  Trigger the software SMI

Arguments:

  Data                          The value to be set on the software SMI data port

Returns:

  EFI_SUCCESS                   Function completes successfully

--*/
{
  UINT16        GPE0BLK_Base;
  UINT32        NewValue;

  //
  // Get GPE0BLK_Base
  //
  GPE0BLK_Base = PcdGet16 (PcdGpe0blkIoBaseAddress);

  //
  // Enable the APMC SMI
  //
  IoOr32 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIE, B_QNC_GPE0BLK_SMIE_APM);

  //
  // Enable SMI globally
  //
  NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  NewValue |= SMI_EN;
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);


  //
  // Generate the APMC SMI
  //
  IoWrite8 (PcdGet16 (PcdSmmActivationPort), Data);

  return EFI_SUCCESS;
}

/**
  Generates an SMI using the parameters passed in.

  @param  PeiServices         Describes the list of possible PEI Services.
  @param  This                A pointer to an instance of
                              EFI_SMM_CONTROL_PPI
  @param  ArgumentBuffer      The argument buffer
  @param  ArgumentBufferSize  The size of the argument buffer
  @param  Periodic            TRUE to indicate a periodical SMI
  @param  ActivationInterval  Interval of the periodical SMI

  @retval EFI_INVALID_PARAMETER  Periodic is TRUE or ArgumentBufferSize > 1
  @retval EFI_SUCCESS            SMI generated

**/
EFI_STATUS
EFIAPI
PeiActivate (
  IN      EFI_PEI_SERVICES           **PeiServices,
  IN      PEI_SMM_CONTROL_PPI        *This,
  IN OUT  INT8                       *ArgumentBuffer OPTIONAL,
  IN OUT  UINTN                      *ArgumentBufferSize OPTIONAL,
  IN      BOOLEAN                    Periodic OPTIONAL,
  IN      UINTN                      ActivationInterval OPTIONAL
  )
{
  INT8       Data;
  EFI_STATUS Status;
  //
  // Periodic SMI not supported.
  //
  if (Periodic) {
    DEBUG ((DEBUG_WARN, "Invalid parameter\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (ArgumentBuffer == NULL) {
    Data = 0xFF;
  } else {
    if (ArgumentBufferSize == NULL || *ArgumentBufferSize != 1) {
      return EFI_INVALID_PARAMETER;
    }

    Data = *ArgumentBuffer;
  }
  //
  // Clear any pending the APM SMI
  //
  Status = SmmClear ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SmmTrigger (Data);
}

/**
  Clears an SMI.

  @param  PeiServices         Describes the list of possible PEI Services.
  @param  This                Pointer to an instance of EFI_SMM_CONTROL_PPI
  @param  Periodic            TRUE to indicate a periodical SMI

  @return Return value from SmmClear()

**/
EFI_STATUS
EFIAPI
PeiDeactivate (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  PEI_SMM_CONTROL_PPI         *This,
  IN  BOOLEAN                     Periodic OPTIONAL
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }
  return SmmClear ();
}

/**
  This is the constructor for the SMM Control Ppi.

  This function installs EFI_SMM_CONTROL_PPI.

  @param   FileHandle       Handle of the file being invoked.
  @param   PeiServices      Describes the list of possible PEI Services.

  @retval EFI_UNSUPPORTED There's no Intel ICH on this platform
  @return The status returned from InstallPpi().

--*/
EFI_STATUS
EFIAPI
SmmControlPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status      = (**PeiServices).InstallPpi (PeiServices, &mPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
