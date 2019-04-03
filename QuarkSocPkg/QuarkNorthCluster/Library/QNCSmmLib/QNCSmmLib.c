/** @file
QNC Smm Library Services that implements SMM Region access, S/W SMI generation and detection.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Base.h>
#include <IntelQNCRegs.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/QNCAccessLib.h>

#define BOOT_SERVICE_SOFTWARE_SMI_DATA          0
#define RUNTIME_SOFTWARE_SMI_DATA               1

/**
  Triggers a run time or boot time SMI.

  This function triggers a software SMM interrupt and set the APMC status with an 8-bit Data.

  @param  Data                 The value to set the APMC status.

**/
VOID
InternalTriggerSmi (
  IN UINT8                     Data
  )
{
  UINT16        GPE0BLK_Base;
  UINT32        NewValue;

  //
  // Get GPE0BLK_Base
  //
  GPE0BLK_Base = (UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF);


  //
  // Enable APM SMI
  //
  IoOr32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIE), B_QNC_GPE0BLK_SMIE_APM);

  //
  // Enable SMI globally
  //
  NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  NewValue |= SMI_EN;
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);

  //
  // Set APM_STS
  //
  IoWrite8 (PcdGet16 (PcdSmmDataPort), Data);

  //
  // Generate the APM SMI
  //
  IoWrite8 (PcdGet16 (PcdSmmActivationPort), PcdGet8 (PcdSmmActivationData));

  //
  // Clear the APM SMI Status Bit
  //
  IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_APM);

  //
  // Set the EOS Bit
  //
  IoOr32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_EOS);
}


/**
  Triggers an SMI at boot time.

  This function triggers a software SMM interrupt at boot time.

**/
VOID
EFIAPI
TriggerBootServiceSoftwareSmi (
  VOID
  )
{
  InternalTriggerSmi (BOOT_SERVICE_SOFTWARE_SMI_DATA);
}


/**
  Triggers an SMI at run time.

  This function triggers a software SMM interrupt at run time.

**/
VOID
EFIAPI
TriggerRuntimeSoftwareSmi (
  VOID
  )
{
  InternalTriggerSmi (RUNTIME_SOFTWARE_SMI_DATA);
}


/**
  Gets the software SMI data.

  This function tests if a software SMM interrupt happens. If a software SMI happens,
  it retrieves the SMM data and returns it as a non-negative value; otherwise a negative
  value is returned.

  @return Data                 The data retrieved from SMM data port in case of a software SMI;
                               otherwise a negative value.

**/
INTN
InternalGetSwSmiData (
  VOID
  )
{
  UINT8                        SmiStatus;
  UINT8                        Data;

  SmiStatus = IoRead8 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS);
  if (((SmiStatus & B_QNC_GPE0BLK_SMIS_APM) != 0) &&
       (IoRead8 (PcdGet16 (PcdSmmActivationPort)) == PcdGet8 (PcdSmmActivationData))) {
    Data = IoRead8 (PcdGet16 (PcdSmmDataPort));
    return (INTN)(UINTN)Data;
  }

  return -1;
}


/**
  Test if a boot time software SMI happened.

  This function tests if a software SMM interrupt happened. If a software SMM interrupt happened and
  it was triggered at boot time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE   A software SMI triggered at boot time happened.
  @retval FLASE  No software SMI happened or the software SMI was triggered at run time.

**/
BOOLEAN
EFIAPI
IsBootServiceSoftwareSmi (
  VOID
  )
{
  return (BOOLEAN) (InternalGetSwSmiData () == BOOT_SERVICE_SOFTWARE_SMI_DATA);
}


/**
  Test if a run time software SMI happened.

  This function tests if a software SMM interrupt happened. If a software SMM interrupt happened and
  it was triggered at run time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE   A software SMI triggered at run time happened.
  @retval FLASE  No software SMI happened or the software SMI was triggered at boot time.

**/
BOOLEAN
EFIAPI
IsRuntimeSoftwareSmi (
  VOID
  )
{
  return (BOOLEAN) (InternalGetSwSmiData () == RUNTIME_SOFTWARE_SMI_DATA);
}



/**

  Clear APM SMI Status Bit; Set the EOS bit.

**/
VOID
EFIAPI
ClearSmi (
  VOID
  )
{

  UINT16                       GPE0BLK_Base;

  //
  // Get GpeBase
  //
  GPE0BLK_Base = (UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF);

  //
  // Clear the APM SMI Status Bit
  //
  IoOr16 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIS, B_QNC_GPE0BLK_SMIS_APM);

  //
  // Set the EOS Bit
  //
  IoOr32 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIS, B_QNC_GPE0BLK_SMIS_EOS);
}

/**
  This routine is the chipset code that accepts a request to "open" a region of SMRAM.
  The region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all boot-service
  and SMM agents.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCOpenSmramRegion (
  VOID
  )
{
  UINT32                     Smram;

  // Read the SMRAM register
  Smram = QncHsmmcRead ();

  //
  //  Is the platform locked?
  //
  if (Smram & SMM_LOCKED) {
    // Cannot Open a locked region
    DEBUG ((EFI_D_WARN, "Cannot open a locked SMRAM region\n"));
    return FALSE;
  }

  //
  // Open all SMRAM regions for Host access only
  //
  Smram |= (SMM_WRITE_OPEN | SMM_READ_OPEN);  // Open for Host.
  Smram &= ~(NON_HOST_SMM_WR_OPEN | NON_HOST_SMM_RD_OPEN);  // Not for others.

  //
  // Write the SMRAM register
  //
  QncHsmmcWrite (Smram);

  return TRUE;
}

/**
  This routine is the chipset code that accepts a request to "close" a region of SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "close" means that the memory is only visible from SMM agents,
  not from BS or RT code.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCCloseSmramRegion (
  VOID
  )
{
  UINT32                    Smram;

  // Read the SMRAM register.
  Smram = QncHsmmcRead ();

  //
  //  Is the platform locked?
  //
  if(Smram & SMM_LOCKED) {
    // Cannot Open a locked region
    DEBUG ((EFI_D_WARN, "Cannot close a locked SMRAM region\n"));
    return FALSE;
  }

  Smram &= (~(SMM_WRITE_OPEN | SMM_READ_OPEN | NON_HOST_SMM_WR_OPEN | NON_HOST_SMM_RD_OPEN));

  QncHsmmcWrite (Smram);

  return TRUE;
}

/**
  This routine is the chipset code that accepts a request to "lock" SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to BS state.
**/
VOID
EFIAPI
QNCLockSmramRegion (
  VOID
  )
{
  UINT32                    Smram;

  // Read the SMRAM register.
  Smram = QncHsmmcRead ();
  if(Smram & SMM_LOCKED) {
    DEBUG ((EFI_D_WARN, "SMRAM region already locked!\n"));
  }
  Smram |= SMM_LOCKED;

  QncHsmmcWrite (Smram);

  return;
}
