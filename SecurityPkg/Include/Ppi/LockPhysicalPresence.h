/** @file
  This file defines the lock physical Presence PPI. This PPI is
  produced by a platform specific PEIM and consumed by the TPM
  PEIM.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PEI_LOCK_PHYSICAL_PRESENCE_H__
#define __PEI_LOCK_PHYSICAL_PRESENCE_H__

///
/// Global ID for the PEI_LOCK_PHYSICAL_PRESENCE_PPI_GUID.
///
#define PEI_LOCK_PHYSICAL_PRESENCE_PPI_GUID  \
  { \
    0xef9aefe5, 0x2bd3, 0x4031, { 0xaf, 0x7d, 0x5e, 0xfe, 0x5a, 0xbb, 0x9a, 0xd } \
  }

///
/// Forward declaration for the PEI_LOCK_PHYSICAL_PRESENCE_PPI
///
typedef struct _PEI_LOCK_PHYSICAL_PRESENCE_PPI PEI_LOCK_PHYSICAL_PRESENCE_PPI;

/**
  This interface returns whether TPM physical presence needs be locked.

  @param[in]  PeiServices       The pointer to the PEI Services Table.

  @retval     TRUE              The TPM physical presence should be locked.
  @retval     FALSE             The TPM physical presence cannot be locked.

**/
typedef
BOOLEAN
(EFIAPI *PEI_LOCK_PHYSICAL_PRESENCE)(
  IN CONST  EFI_PEI_SERVICES                    **PeiServices
  );

///
/// This service abstracts TPM physical presence lock interface. It is necessary for
/// safety to convey this information to the TPM driver so that TPM physical presence
/// can be locked as early as possible. This PPI is produced by a platform specific
/// PEIM and consumed by the TPM PEIM.
///
struct _PEI_LOCK_PHYSICAL_PRESENCE_PPI {
  PEI_LOCK_PHYSICAL_PRESENCE    LockPhysicalPresence;
};

extern EFI_GUID  gPeiLockPhysicalPresencePpiGuid;

#endif //  __PEI_LOCK_PHYSICAL_PRESENCE_H__
