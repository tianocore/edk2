/** @file
	Non-existing BaseLib functions on Ia32

	Copyright (c) 2006, Intel Corporation
	All rights reserved. This program and the accompanying materials
	are licensed and made available under the terms and conditions of the BSD License
	which accompanies this distribution.  The full text of the license may be found at
	http://opensource.org/licenses/bsd-license.php

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

	Module Name:	Non-existing.c

**/

#include "../BaseLibInternals.h"

VOID
EFIAPI
InternalX86DisablePaging64 (
  IN      UINT16                    CodeSelector,
  IN      UINT32                    EntryPoint,
  IN      UINT32                    Context1,  OPTIONAL
  IN      UINT32                    Context2,  OPTIONAL
  IN      UINT32                    NewStack
  )
{
  ASSERT (FALSE);
}
