/** @file
This file contains the definination for host controller
debug support routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

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










