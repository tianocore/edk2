/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4Option.h

Abstract:

  IP4 option support routines.


**/

#ifndef __EFI_IP4_OPTION_H__
#define __EFI_IP4_OPTION_H__

enum {
  IP4_OPTION_EOP       = 0,
  IP4_OPTION_NOP       = 1,
  IP4_OPTION_LSRR      = 131,  // Loss source and record routing,   10000011
  IP4_OPTION_SSRR      = 137,  // Strict source and record routing, 10001001
  IP4_OPTION_RR        = 7,    // Record routing, 00000111

  IP4_OPTION_COPY_MASK = 0x80
};

BOOLEAN
Ip4OptionIsValid (
  IN UINT8                  *Option,
  IN UINT32                 OptLen,
  IN BOOLEAN                Rcvd
  );

EFI_STATUS
Ip4CopyOption (
  IN UINT8                  *Option,
  IN UINT32                 OptLen,
  IN BOOLEAN                Fragment,
  IN UINT8                  *Buf,     OPTIONAL
  IN OUT UINT32             *BufLen
  );
#endif
