/** @file
GUID definition for TtyTerm terminal type.  The TtyTerm terminal aims to
provide support for modern *nix terminals.


Copyright (c) 2015  Linaro Ltd.
Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TTYTERM_H__
#define __TTYTERM_H__

#define EFI_TTY_TERM_GUID    \
    {0x7d916d80, 0x5bb1, 0x458c, {0xa4, 0x8f, 0xe2, 0x5f, 0xdd, 0x51, 0xef, 0x94 } }

#define EDKII_LINUX_TERM_GUID   \
    {0xe4364a7f, 0xf825, 0x430e, {0x9d, 0x3a, 0x9c, 0x9b, 0xe6, 0x81, 0x7c, 0xa5 } }

#define EDKII_XTERM_R6_GUID     \
    {0xfbfca56b, 0xbb36, 0x4b78, {0xaa, 0xab, 0xbe, 0x1b, 0x97, 0xec, 0x7c, 0xcb } }

#define EDKII_VT400_GUID        \
    {0x8e46dddd, 0x3d49, 0x4a9d, {0xb8, 0x75, 0x3c, 0x08, 0x6f, 0x6a, 0xa2, 0xbd } }

#define EDKII_SCO_TERM_GUID     \
    {0xfc7dd6e0, 0x813c, 0x434d, {0xb4, 0xda, 0x3b, 0xd6, 0x49, 0xe9, 0xe1, 0x5a } }

extern EFI_GUID gEfiTtyTermGuid;
extern EFI_GUID gEdkiiLinuxTermGuid;
extern EFI_GUID gEdkiiXtermR6Guid;
extern EFI_GUID gEdkiiVT400Guid;
extern EFI_GUID gEdkiiSCOTermGuid;

#endif
