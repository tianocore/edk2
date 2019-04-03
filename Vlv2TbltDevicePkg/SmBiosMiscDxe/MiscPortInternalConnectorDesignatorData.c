/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscPortInternalConnectorDesignatorData.c

Abstract:

  Static data of Port internal connector designator information.
  Port internal connector designator information is Misc. subclass type 6 and
  SMBIOS type 8.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Port connector designations
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortIde1) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_IDE1),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_IDE1),
  EfiPortConnectorTypeOnboardIde,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortIde2) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_IDE2),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_IDE2),
  EfiPortConnectorTypeOnboardIde,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortAtxPower) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_ATX_POWER),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_ATX_POWER),
  EfiPortConnectorTypeOther,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  0
};
