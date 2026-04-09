/** @file

  A PEIM providing synchronous SMI activations via the
  EFI_PEI_MM_CONTROL_PPI.

  We expect the PEI phase to have covered the following:
  - ensure that the underlying QEMU machine type be Q35
    (responsible: OvmfPkg/SmmAccess/SmmAccessPei.inf)
  - ensure that the ACPI PM IO space be configured
    (responsible: OvmfPkg/PlatformPei/PlatformPei.inf)

  Our own entry point is responsible for confirming the SMI feature and for
  configuring it.

  Copyright (C) 2013, 2015, Red Hat, Inc.<BR>
  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Ppi/MmControl.h>

#include "SmiFeatures.h"

//
// The absolute IO port address of the SMI Control and Enable Register.
//
STATIC UINTN  mSmiEnable;

/**
  Invokes PPI activation from the PI PEI environment.

  @param  PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This                  The PEI_MM_CONTROL_PPI instance.
  @param  ArgumentBuffer        The value passed to the MMI handler. This value corresponds to the
                                SwMmiInputValue in the RegisterContext parameter for the Register()
                                function in the EFI_MM_SW_DISPATCH_PROTOCOL and in the Context parameter
                                in the call to the DispatchFunction
  @param  ArgumentBufferSize    The size of the data passed in ArgumentBuffer or NULL if ArgumentBuffer is NULL.
  @param  Periodic              An optional mechanism to periodically repeat activation.
  @param  ActivationInterval    An optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The MMI has been engendered.
  @retval EFI_DEVICE_ERROR      The timing is unsupported.
  @retval EFI_INVALID_PARAMETER The activation period is unsupported.
  @retval EFI_NOT_STARTED       The MM base service has not been initialized.

**/
STATIC
EFI_STATUS
EFIAPI
MmControlPeiTrigger (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_MM_CONTROL_PPI  *This,
  IN OUT INT8                *ArgumentBuffer OPTIONAL,
  IN OUT UINTN               *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                 Periodic OPTIONAL,
  IN UINTN                   ActivationInterval OPTIONAL
  )
{
  UINT8  Command;
  UINT8  Data;

  //
  // No support for queued or periodic activation.
  //
  if (Periodic || (ActivationInterval > 0)) {
    return EFI_DEVICE_ERROR;
  }

  if ((ArgumentBufferSize != NULL) && (ArgumentBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 0th byte as command, 1st as data... Right?
  //
  if ((NULL == ArgumentBuffer) || (*ArgumentBufferSize == 0)) {
    Command = 0;
    Data    = 0;
  } else if (*ArgumentBufferSize == 1) {
    Command = *ArgumentBuffer;
    Data    = 0;
  } else if (*ArgumentBufferSize == 2) {
    Command = ArgumentBuffer[0];
    Data    = ArgumentBuffer[1];
  } else {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The so-called "Advanced Power Management Status Port Register" is in fact
  // a generic data passing register, between the caller and the SMI
  // dispatcher. The ICH9 spec calls it "scratchpad register" --  calling it
  // "status" elsewhere seems quite the misnomer. Status registers usually
  // report about hardware status, while this register is fully governed by
  // software.
  //
  // Write to the status register first, as this won't trigger the SMI just
  // yet. Then write to the control register.
  //
  IoWrite8 (ICH9_APM_STS, Data);
  IoWrite8 (ICH9_APM_CNT, Command);
  return EFI_SUCCESS;
}

/**
  Clears any system state that was created in response to the Trigger() call.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  This                  The PEI_MM_CONTROL_PPI instance.
  @param  Periodic              Optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The MMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
STATIC
EFI_STATUS
EFIAPI
MmControlPeiClear (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_MM_CONTROL_PPI  *This,
  IN BOOLEAN                 Periodic OPTIONAL
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The PI spec v1.4 explains that Clear() is only supposed to clear software
  // status; it is not in fact responsible for deasserting the SMI. It gives
  // two reasons for this: (a) many boards clear the SMI automatically when
  // entering SMM, (b) if Clear() actually deasserted the SMI, then it could
  // incorrectly suppress an SMI that was asynchronously asserted between the
  // last return of the SMI handler and the call made to Clear().
  //
  // In fact QEMU automatically deasserts CPU_INTERRUPT_SMI in:
  // - x86_cpu_exec_interrupt() [target-i386/seg_helper.c], and
  // - kvm_arch_pre_run() [target-i386/kvm.c].
  //
  // So, nothing to do here.
  //
  return EFI_SUCCESS;
}

STATIC EFI_PEI_MM_CONTROL_PPI  mMmControl =
{
  .Trigger = MmControlPeiTrigger,
  .Clear   = MmControlPeiClear
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mSmmControlPpiList =
{
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMmControlPpiGuid,
  &mMmControl
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
MmControlPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINT32      PmBase;
  UINT32      SmiEnableVal;
  EFI_STATUS  Status;

  //
  // This module should only be included if SMRAM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  //
  // Calculate the absolute IO port address of the SMI Control and Enable
  // Register. (As noted at the top, the PEI phase has left us with a working
  // ACPI PM IO space.)
  //
  PmBase = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) &
           ICH9_PMBASE_MASK;
  mSmiEnable = PmBase + ICH9_PMBASE_OFS_SMI_EN;

  //
  // If APMC_EN is pre-set in SMI_EN, that's QEMU's way to tell us that SMI
  // support is not available. (For example due to KVM lacking it.) Otherwise,
  // this bit is clear after each reset.
  //
  SmiEnableVal = IoRead32 (mSmiEnable);
  if ((SmiEnableVal & ICH9_SMI_EN_APMC_EN) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: this Q35 implementation lacks SMI\n",
      __func__
      ));
    goto FatalError;
  }

  //
  // Otherwise, configure the board to inject an SMI when ICH9_APM_CNT is
  // written to. (See the Trigger() method above.)
  //
  SmiEnableVal |= ICH9_SMI_EN_APMC_EN | ICH9_SMI_EN_GBL_SMI_EN;
  IoWrite32 (mSmiEnable, SmiEnableVal);

  //
  // Prevent software from undoing the above (until platform reset).
  //
  PciOr16 (
    POWER_MGMT_REGISTER_Q35 (ICH9_GEN_PMCON_1),
    ICH9_GEN_PMCON_1_SMI_LOCK
    );

  //
  // If we can clear GBL_SMI_EN now, that means QEMU's SMI support is not
  // appropriate.
  //
  IoWrite32 (mSmiEnable, SmiEnableVal & ~(UINT32)ICH9_SMI_EN_GBL_SMI_EN);
  if (IoRead32 (mSmiEnable) != SmiEnableVal) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to lock down GBL_SMI_EN\n",
      __func__
      ));
    goto FatalError;
  }

  //
  // We have no pointers to convert to virtual addresses. The handle itself
  // doesn't matter, as protocol services are not accessible at runtime.
  //
  Status = (*PeiServices)->InstallPpi (PeiServices, &mSmmControlPpiList);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: InstallMultipleProtocolInterfaces: %r\n",
      __func__,
      Status
      ));
    goto FatalError;
  }

  return EFI_SUCCESS;

FatalError:
  //
  // We really don't want to continue in this case.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}
