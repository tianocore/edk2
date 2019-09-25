/** @file
GUID definition for TtyTerm terminal type.  The TtyTerm terminal aims to
provide support for modern *nix terminals.


Copyright (c) 2015  Linaro Ltd.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TTYTERM_H__
#define __TTYTERM_H__

#define EFI_TTY_TERM_GUID    \
    {0x7d916d80, 0x5bb1, 0x458c, {0xa4, 0x8f, 0xe2, 0x5f, 0xdd, 0x51, 0xef, 0x94 } }

extern EFI_GUID gEfiTtyTermGuid;

#endif
