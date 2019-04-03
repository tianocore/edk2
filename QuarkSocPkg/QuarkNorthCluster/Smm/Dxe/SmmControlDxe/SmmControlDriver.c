/** @file
This module produces the SMM COntrol2 Protocol for QNC

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Protocol/SmmControl2.h>
#include <IndustryStandard/Pci.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <IntelQNCDxe.h>
#include <Library/QNCAccessLib.h>
#include <Uefi/UefiBaseType.h>

#define EFI_INTERNAL_POINTER  0x00000004

extern EFI_GUID gEfiEventVirtualAddressChangeGuid;

/**
  Generates an SMI using the parameters passed in.

  @param  This                A pointer to an instance of
                              EFI_SMM_CONTROL2_PROTOCOL
  @param  ArgumentBuffer      The argument buffer
  @param  ArgumentBufferSize  The size of the argument buffer
  @param  Periodic            TRUE to indicate a periodical SMI
  @param  ActivationInterval  Interval of the periodical SMI

  @retval EFI_INVALID_PARAMETER Periodic is TRUE or ArgumentBufferSize > 1
  @return Return value from SmmTrigger().

**/
EFI_STATUS
EFIAPI
Activate (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL     *This,
  IN OUT  UINT8                          *CommandPort       OPTIONAL,
  IN OUT  UINT8                          *DataPort          OPTIONAL,
  IN      BOOLEAN                        Periodic           OPTIONAL,
  IN      EFI_SMM_PERIOD                 ActivationInterval OPTIONAL
                  );

/**
  Clears an SMI.

  @param  This      Pointer to an instance of EFI_SMM_CONTROL2_PROTOCOL
  @param  Periodic  TRUE to indicate a periodical SMI

  @return Return value from SmmClear()

**/
EFI_STATUS
EFIAPI
Deactivate (
  IN CONST     EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN      BOOLEAN                         Periodic OPTIONAL
  );

///
/// Handle for the SMM Control2 Protocol
///
EFI_HANDLE  mSmmControl2Handle = NULL;

///
/// SMM COntrol2 Protocol instance
///
EFI_SMM_CONTROL2_PROTOCOL mSmmControl2 = {
  Activate,
  Deactivate,
  0
};

VOID
EFIAPI
SmmControlVirtualddressChangeEvent (
  IN EFI_EVENT                  Event,
  IN VOID                       *Context
  )
/*++

Routine Description:

  Fixup internal data pointers so that the services can be called in virtual mode.

Arguments:

  Event                         The event registered.
  Context                       Event context.

Returns:

  None.

--*/
{
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSmmControl2.Trigger));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSmmControl2.Clear));
}

/**
  Clear SMI related chipset status and re-enable SMI by setting the EOS bit.

  @retval EFI_SUCCESS The requested operation has been carried out successfully
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

/**
  Generates an SMI using the parameters passed in.

  @param  This                A pointer to an instance of
                              EFI_SMM_CONTROL_PROTOCOL
  @param  ArgumentBuffer      The argument buffer
  @param  ArgumentBufferSize  The size of the argument buffer
  @param  Periodic            TRUE to indicate a periodical SMI
  @param  ActivationInterval  Interval of the periodical SMI

  @retval EFI_INVALID_PARAMETER Periodic is TRUE or ArgumentBufferSize > 1
  @retval EFI_SUCCESS            SMI generated

**/
EFI_STATUS
EFIAPI
Activate (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL     *This,
  IN OUT  UINT8                          *CommandPort       OPTIONAL,
  IN OUT  UINT8                          *DataPort          OPTIONAL,
  IN      BOOLEAN                        Periodic           OPTIONAL,
  IN      EFI_SMM_PERIOD                 ActivationInterval OPTIONAL
  )
{
  UINT16        GPE0BLK_Base;
  UINT32        NewValue;

  //
  // Get GPE0BLK_Base
  //
  GPE0BLK_Base = PcdGet16 (PcdGpe0blkIoBaseAddress);

  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Clear any pending the APM SMI
  //
  if (EFI_ERROR (SmmClear())) {
    return EFI_DEVICE_ERROR;
    }

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
  // Set APMC_STS
  //
  if (DataPort == NULL) {
    IoWrite8 (PcdGet16 (PcdSmmDataPort), 0xFF);
  } else {
    IoWrite8 (PcdGet16 (PcdSmmDataPort), *DataPort);
  }

  //
  // Generate the APMC SMI
  //
  if (CommandPort == NULL) {
    IoWrite8 (PcdGet16 (PcdSmmActivationPort), 0xFF);
  } else {
    IoWrite8 (PcdGet16 (PcdSmmActivationPort), *CommandPort);
  }

  return EFI_SUCCESS;
}

/**
  Clears an SMI.

  @param  This      Pointer to an instance of EFI_SMM_CONTROL_PROTOCOL
  @param  Periodic  TRUE to indicate a periodical SMI

  @return Return value from SmmClear()

**/
EFI_STATUS
EFIAPI
Deactivate (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL     *This,
  IN      BOOLEAN                   Periodic
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  return SmmClear();
}

/**
  This is the constructor for the SMM Control protocol.

  This function installs EFI_SMM_CONTROL2_PROTOCOL.

  @param  ImageHandle Handle for the image of this driver
  @param  SystemTable Pointer to the EFI System Table

  @retval EFI_UNSUPPORTED There's no Intel ICH on this platform
  @return The status returned from InstallProtocolInterface().

--*/
EFI_STATUS
SmmControl2Init (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  UINT16      PM1BLK_Base;
  UINT16      GPE0BLK_Base;
  BOOLEAN     SciEn;
  UINT32      NewValue;

  //
  // Get PM1BLK_Base & GPE0BLK_Base
  //
  PM1BLK_Base  = PcdGet16 (PcdPm1blkIoBaseAddress);
  GPE0BLK_Base = PcdGet16 (PcdGpe0blkIoBaseAddress);

  //
  // Install our protocol interfaces on the device's handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmmControl2Handle,
                  &gEfiSmmControl2ProtocolGuid,  &mSmmControl2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Determine whether an ACPI OS is present (via the SCI_EN bit)
  //
  SciEn = (BOOLEAN)((IoRead16 (PM1BLK_Base + R_QNC_PM1BLK_PM1C) & B_QNC_PM1BLK_PM1C_SCIEN) != 0);
  if (!SciEn) {
    //
    // Clear any SMIs that double as SCIs (when SCI_EN==0)
    //
    IoWrite16 ((PM1BLK_Base + R_QNC_PM1BLK_PM1S), B_QNC_PM1BLK_PM1S_ALL);
    IoWrite16 ((PM1BLK_Base + R_QNC_PM1BLK_PM1E), 0x00000000);
    IoWrite32 ((PM1BLK_Base + R_QNC_PM1BLK_PM1C),  0x00000000);
    IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_GPE0S), B_QNC_GPE0BLK_GPE0S_ALL);
    IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_GPE0E), 0x00000000);
  }

  //
  // Clear and disable all SMIs that are unaffected by SCI_EN
  // Set EOS
  //
  IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIE), 0x00000000);
  IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), (B_QNC_GPE0BLK_SMIS_EOS + B_QNC_GPE0BLK_SMIS_ALL));

  //
  // Enable SMI globally
  //
  NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  NewValue |= SMI_EN;
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);

  //
  // Make sure to write this register last -- EOS re-enables SMIs for the QNC
  //
  IoAndThenOr32 (
    GPE0BLK_Base + R_QNC_GPE0BLK_SMIE,
    (UINT32)(~B_QNC_GPE0BLK_SMIE_ALL),
    B_QNC_GPE0BLK_SMIE_APM
    );

  //
  // Make sure EOS bit cleared
  //
  DEBUG_CODE_BEGIN ();
  if (IoRead32 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIS) & B_QNC_GPE0BLK_SMIS_EOS) {
    DEBUG ((
      EFI_D_ERROR,
      "******************************************************************************\n"
      "BIG ERROR: SmmControl constructor couldn't properly initialize the ACPI table.\n"
      "           SmmControl->Clear will probably hang.                              \n"
      "              NOTE: SCI_EN = %d                                               \n"
      "******************************************************************************\n",
      SciEn
      ));

    //
    // If we want the system to stop, then keep the ASSERT(FALSE).
    // Otherwise, comment it out.
    //
    ASSERT (FALSE);
  }
  DEBUG_CODE_END ();

  Status = gBS->CreateEventEx (
                EVT_NOTIFY_SIGNAL,
                TPL_NOTIFY,
                SmmControlVirtualddressChangeEvent,
                NULL,
                &gEfiEventVirtualAddressChangeGuid,
                &Event
                );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
