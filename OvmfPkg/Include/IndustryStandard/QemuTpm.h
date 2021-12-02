/** @file
  Macro and type definitions corresponding to the QEMU TPM interface.

  Refer to "docs/specs/tpm.txt" in the QEMU source directory.

  Copyright (C) 2018, Red Hat, Inc.
  Copyright (c) 2018, IBM Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __QEMU_TPM_H__
#define __QEMU_TPM_H__

#include <Base.h>

//
// whether function is blocked by BIOS settings; bits 0, 1, 2
//
#define QEMU_TPM_PPI_FUNC_NOT_IMPLEMENTED      (0 << 0)
#define QEMU_TPM_PPI_FUNC_BIOS_ONLY            (1 << 0)
#define QEMU_TPM_PPI_FUNC_BLOCKED              (2 << 0)
#define QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ      (3 << 0)
#define QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ  (4 << 0)
#define QEMU_TPM_PPI_FUNC_MASK                 (7 << 0)

//
// The following structure is shared between firmware and ACPI.
//
#pragma pack (1)
typedef struct {
  UINT8     Func[256];        // func
  UINT8     In;               // ppin
  UINT32    Ip;               // ppip
  UINT32    Response;         // pprp
  UINT32    Request;          // pprq
  UINT32    RequestParameter; // pprm
  UINT32    LastRequest;      // lppr
  UINT32    FRet;             // fret
  UINT8     Res1[0x40];       // res1
  UINT8     NextStep;         // next_step
} QEMU_TPM_PPI;
#pragma pack ()

//
// The following structure is for the fw_cfg etc/tpm/config file.
//
#pragma pack (1)
typedef struct {
  UINT32    PpiAddress;
  UINT8     TpmVersion;
  UINT8     PpiVersion;
} QEMU_FWCFG_TPM_CONFIG;
#pragma pack ()

#define QEMU_TPM_VERSION_UNSPEC  0
#define QEMU_TPM_VERSION_1_2     1
#define QEMU_TPM_VERSION_2       2

#define QEMU_TPM_PPI_VERSION_NONE  0
#define QEMU_TPM_PPI_VERSION_1_30  1

#endif
