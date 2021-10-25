/** @file
  QEMU Physical Presence Interface

  Copyright (c) 2021 IBM Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QEMU_PPI__
#define __QEMU_PPI__

#include <IndustryStandard/QemuTpm.h>
#include <IndustryStandard/TcgPhysicalPresence.h>
#include <Guid/PhysicalPresenceData.h>

#define TPM_PPI_PROVISION_FLAGS(PpiFlags) \
  ((PpiFlags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION) != 0) \
  ? QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ \
  : QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ

#define TPM_PPI_CLEAR_FLAGS(PpiFlags) \
  ((PpiFlags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR) != 0) \
  ? QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ \
  : QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ

#define TPM_PPI_CLEAR_MAINT_FLAGS(PpiFlags) \
  ((PpiFlags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR) != 0 && \
   (PpiFlags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_MAINTENANCE) != 0) \
  ? QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ \
  : QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ

#endif
