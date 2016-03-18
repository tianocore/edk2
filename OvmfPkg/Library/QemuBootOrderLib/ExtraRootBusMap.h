/** @file
  Map positions of extra PCI root buses to bus numbers.

  Copyright (C) 2015, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __EXTRA_ROOT_BUS_MAP_H__
#define __EXTRA_ROOT_BUS_MAP_H__

/**
  Incomplete ("opaque") data type implementing the map.
**/
typedef struct EXTRA_ROOT_BUS_MAP_STRUCT EXTRA_ROOT_BUS_MAP;

EFI_STATUS
CreateExtraRootBusMap (
  OUT EXTRA_ROOT_BUS_MAP **ExtraRootBusMap
  );

VOID
DestroyExtraRootBusMap (
  IN EXTRA_ROOT_BUS_MAP *ExtraRootBusMap
  );

EFI_STATUS
MapRootBusPosToBusNr (
  IN  CONST EXTRA_ROOT_BUS_MAP *ExtraRootBusMap,
  IN  UINT64                   RootBusPos,
  OUT UINT32                   *RootBusNr
  );

#endif
