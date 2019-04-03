/** @file

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "FwBlockService.h"


/**
  This function allows the caller to determine if UEFI SetVirtualAddressMap() has been called.

  This function returns TRUE after all the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE functions have
  executed as a result of the OS calling SetVirtualAddressMap(). Prior to this time FALSE
  is returned. This function is used by runtime code to decide it is legal to access services
  that go away after SetVirtualAddressMap().

  @retval  TRUE  The system has finished executing the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  @retval  FALSE The system has not finished executing the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

**/
BOOLEAN
EfiGoneVirtual (
  VOID
  )
{
  return FALSE; //Hard coded to FALSE for SMM driver.
}
