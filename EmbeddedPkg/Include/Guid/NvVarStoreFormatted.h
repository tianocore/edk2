/** @file
  EDKII NvVarStore Formatted GUID

  A NULL protocol instance with this GUID in the DXE and/or MM protocol
  databases, and/or a NULL PPI with this GUID in the PPI database, implies that
  a DXE or MM driver, or a PEIM, has verified (or dynamically ensured) that the
  non-volatile variable store has valid and consistent headers
  (EFI_FIRMWARE_VOLUME_HEADER and VARIABLE_STORE_HEADER).

  Said predicate is required by the read-only variable PEIM, and the read side
  of the runtime variable DXE and MM drivers, immediately after they are
  dispatched. This GUID presents platforms with one way to coordinate between
  their module(s) that format the variable store FVB device and the variable
  service drivers.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __EDKII_NV_VAR_STORE_FORMATTED_H__
#define __EDKII_NV_VAR_STORE_FORMATTED_H__

#define EDKII_NV_VAR_STORE_FORMATTED_GUID \
  { \
    0xd1a86e3f, 0x0707, 0x4c35, \
    { 0x83, 0xcd, 0xdc, 0x2c, 0x29, 0xc8, 0x91, 0xa3 } \
  }

extern EFI_GUID  gEdkiiNvVarStoreFormattedGuid;

#endif
