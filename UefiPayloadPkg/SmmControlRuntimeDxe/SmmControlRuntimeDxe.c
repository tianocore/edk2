/** @file
  This module produces the SMM Control2 Protocol

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Protocol/SmmControl2.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/SmmRegisterInfoGuid.h>

#define SMM_DATA_PORT     0xB3
#define SMM_CONTROL_PORT  0xB2

typedef struct {
  UINT8     GblBitOffset;
  UINT8     ApmBitOffset;
  UINT32    Address;
} SMM_CONTROL2_REG;

SMM_CONTROL2_REG  mSmiCtrlReg;

/**
  Invokes SMI activation from either the preboot or runtime environment.

  This function generates an SMI.

  @param[in]     This                The EFI_SMM_CONTROL2_PROTOCOL instance.
  @param[in,out] CommandPort         The value written to the command port.
  @param[in,out] DataPort            The value written to the data port.
  @param[in]     Periodic            Optional mechanism to engender a periodic stream.
  @param[in]     ActivationInterval  Optional parameter to repeat at this period one
                                     time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS            The SMI has been engendered.
  @retval EFI_DEVICE_ERROR       The timing is unsupported.
  @retval EFI_INVALID_PARAMETER  The activation period is unsupported.
  @retval EFI_INVALID_PARAMETER  The last periodic activation has not been cleared.
  @retval EFI_NOT_STARTED        The MM base service has not been initialized.
**/
EFI_STATUS
EFIAPI
Activate (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN OUT  UINT8                       *CommandPort       OPTIONAL,
  IN OUT  UINT8                       *DataPort          OPTIONAL,
  IN      BOOLEAN                     Periodic           OPTIONAL,
  IN      EFI_SMM_PERIOD              ActivationInterval OPTIONAL
  )
{
  UINT32  SmiEn;
  UINT32  SmiEnableBits;

  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  SmiEn         = IoRead32 (mSmiCtrlReg.Address);
  SmiEnableBits = (1 << mSmiCtrlReg.GblBitOffset) | (1 << mSmiCtrlReg.ApmBitOffset);
  if ((SmiEn & SmiEnableBits) != SmiEnableBits) {
    //
    // Set the "global SMI enable" bit and APM bit
    //
    IoWrite32 (mSmiCtrlReg.Address, SmiEn | SmiEnableBits);
  }

  IoWrite8 (SMM_DATA_PORT, DataPort    == NULL ? 0 : *DataPort);
  IoWrite8 (SMM_CONTROL_PORT, CommandPort == NULL ? 0 : *CommandPort);
  return EFI_SUCCESS;
}

/**
  Clears an SMI.

  @param  This      Pointer to an instance of EFI_SMM_CONTROL2_PROTOCOL
  @param  Periodic  TRUE to indicate a periodical SMI

  @return Return value from SmmClear ()

**/
EFI_STATUS
EFIAPI
Deactivate (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN       BOOLEAN                    Periodic
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Temporarily do nothing here
  //
  return EFI_SUCCESS;
}

///
/// SMM COntrol2 Protocol instance
///
EFI_SMM_CONTROL2_PROTOCOL  mSmmControl2 = {
  Activate,
  Deactivate,
  0
};

/**
  Get specified SMI register based on given register ID

  @param[in]  SmmRegister  SMI related register array from bootloader
  @param[in]  Id           The register ID to get.

  @retval NULL             The register is not found or the format is not expected.
  @return smi register

**/
PLD_GENERIC_REGISTER *
GetSmmCtrlRegById (
  IN PLD_SMM_REGISTERS  *SmmRegister,
  IN UINT32             Id
  )
{
  UINT32                Index;
  PLD_GENERIC_REGISTER  *PldReg;

  PldReg = NULL;
  for (Index = 0; Index < SmmRegister->Count; Index++) {
    if (SmmRegister->Registers[Index].Id == Id) {
      PldReg = &SmmRegister->Registers[Index];
      break;
    }
  }

  if (PldReg == NULL) {
    DEBUG ((DEBUG_INFO, "Register %d not found.\n", Id));
    return NULL;
  }

  //
  // Checking the register if it is expected.
  //
  if ((PldReg->Address.AccessSize       != EFI_ACPI_3_0_DWORD) ||
      (PldReg->Address.Address          == 0) ||
      (PldReg->Address.RegisterBitWidth != 1) ||
      (PldReg->Address.AddressSpaceId   != EFI_ACPI_3_0_SYSTEM_IO) ||
      (PldReg->Value != 1))
  {
    DEBUG ((DEBUG_INFO, "Unexpected SMM register.\n"));
    DEBUG ((DEBUG_INFO, "AddressSpaceId= 0x%x\n", PldReg->Address.AddressSpaceId));
    DEBUG ((DEBUG_INFO, "RegBitWidth   = 0x%x\n", PldReg->Address.RegisterBitWidth));
    DEBUG ((DEBUG_INFO, "RegBitOffset  = 0x%x\n", PldReg->Address.RegisterBitOffset));
    DEBUG ((DEBUG_INFO, "AccessSize    = 0x%x\n", PldReg->Address.AccessSize));
    DEBUG ((DEBUG_INFO, "Address       = 0x%lx\n", PldReg->Address.Address));
    return NULL;
  }

  return PldReg;
}

/**
  Fixup data pointers so that the services can be called in virtual mode.

  @param[in] Event                The event registered.
  @param[in] Context              Event context.

**/
VOID
EFIAPI
SmmControlVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&(mSmmControl2.Trigger));
  EfiConvertPointer (0x0, (VOID **)&(mSmmControl2.Clear));
}

/**
  This function installs EFI_SMM_CONTROL2_PROTOCOL.

  @param  ImageHandle Handle for the image of this driver
  @param  SystemTable Pointer to the EFI System Table

  @retval EFI_UNSUPPORTED There's no Intel ICH on this platform
  @return The status returned from InstallProtocolInterface().

**/
EFI_STATUS
EFIAPI
SmmControlEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HOB_GUID_TYPE     *GuidHob;
  PLD_SMM_REGISTERS     *SmmRegister;
  PLD_GENERIC_REGISTER  *SmiGblEnReg;
  PLD_GENERIC_REGISTER  *SmiApmEnReg;
  EFI_EVENT             Event;

  GuidHob = GetFirstGuidHob (&gSmmRegisterInfoGuid);
  if (GuidHob == NULL) {
    return EFI_UNSUPPORTED;
  }

  SmmRegister = (PLD_SMM_REGISTERS *)(GET_GUID_HOB_DATA (GuidHob));
  SmiGblEnReg = GetSmmCtrlRegById (SmmRegister, REGISTER_ID_SMI_GBL_EN);
  if (SmiGblEnReg == NULL) {
    DEBUG ((DEBUG_ERROR, "SMI global enable reg not found.\n"));
    return EFI_NOT_FOUND;
  }

  mSmiCtrlReg.Address      = (UINT32)SmiGblEnReg->Address.Address;
  mSmiCtrlReg.GblBitOffset = SmiGblEnReg->Address.RegisterBitOffset;

  SmiApmEnReg = GetSmmCtrlRegById (SmmRegister, REGISTER_ID_SMI_APM_EN);
  if (SmiApmEnReg == NULL) {
    DEBUG ((DEBUG_ERROR, "SMI APM enable reg not found.\n"));
    return EFI_NOT_FOUND;
  }

  if (SmiApmEnReg->Address.Address != mSmiCtrlReg.Address) {
    DEBUG ((DEBUG_ERROR, "SMI APM EN and SMI GBL EN are expected to have same register base\n"));
    DEBUG ((DEBUG_ERROR, "APM:0x%x, GBL:0x%x\n", SmiApmEnReg->Address.Address, mSmiCtrlReg.Address));
    return EFI_UNSUPPORTED;
  }

  mSmiCtrlReg.ApmBitOffset = SmiApmEnReg->Address.RegisterBitOffset;

  //
  // Install our protocol interfaces on the device's handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiSmmControl2ProtocolGuid,
                  &mSmmControl2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SmmControlVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
  return Status;
}
