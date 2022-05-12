/** @file
Definitions for AMD SEV-SNP Secrets Page

Copyright (c) 2022 AMD Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SNP_SECRETS_PAGE_H_
#define __SNP_SECRETS_PAGE_H_

//
// OS-defined area of secrets page
//
// As defined by "SEV-ES Guest-Hypervisor Communication Block Standardization",
// revision 2.01, section 2.7, "SEV-SNP Secrets Page".
//
typedef PACKED struct _SNP_SECRETS_OS_AREA {
  UINT32    Vmpl0MsgSeqNumLo;
  UINT32    Vmpl1MsgSeqNumLo;
  UINT32    Vmpl2MsgSeqNumLo;
  UINT32    Vmpl3MsgSeqNumLo;
  UINT64    ApJumpTablePa;
  UINT32    Vmpl0MsgSeqNumHi;
  UINT32    Vmpl1MsgSeqNumHi;
  UINT32    Vmpl2MsgSeqNumHi;
  UINT32    Vmpl3MsgSeqNumHi;
  UINT8     Reserved2[22];
  UINT16    Version;
  UINT8     GuestUsage[32];
} SNP_SECRETS_OS_AREA;

#define VMPCK_KEY_LEN  32

//
// SEV-SNP Secrets page
//
// As defined by "SEV-SNP Firmware ABI", revision 1.51, section 8.17.2.5,
// "PAGE_TYPE_SECRETS".
//
typedef PACKED struct _SNP_SECRETS_PAGE {
  UINT32                 Version;
  UINT32                 ImiEn    : 1,
                         Reserved : 31;
  UINT32                 Fms;
  UINT32                 Reserved2;
  UINT8                  Gosvw[16];
  UINT8                  Vmpck0[VMPCK_KEY_LEN];
  UINT8                  Vmpck1[VMPCK_KEY_LEN];
  UINT8                  Vmpck2[VMPCK_KEY_LEN];
  UINT8                  Vmpck3[VMPCK_KEY_LEN];
  SNP_SECRETS_OS_AREA    OsArea;
  UINT8                  Reserved3[3840];
} SNP_SECRETS_PAGE;

#endif
