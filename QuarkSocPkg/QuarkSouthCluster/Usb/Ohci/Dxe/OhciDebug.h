/** @file
This file contains the definination for host controller
debug support routines.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



/*++

Routine Description:

  Print the data of ED and the TDs attached to the ED

  @param  Uhc                   Pointer to OHCI private data
  @param  Ed                    Pointer to a ED to free
  @param  Td                    Pointer to the Td head

  @retval EFI_SUCCESS           ED

**/
EFI_STATUS
OhciDumpEdTdInfo (
  IN USB_OHCI_HC_DEV          *Uhc,
  IN ED_DESCRIPTOR     *Ed,
  IN TD_DESCRIPTOR     *Td,
  BOOLEAN Stage
  );










