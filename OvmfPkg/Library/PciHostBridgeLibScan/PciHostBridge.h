/** @file
  OVMF's instance of the PCI Host Bridge Library, for Bhyve and Xen guests;
  internal declarations.

  Copyright (C) 2021, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

PCI_ROOT_BRIDGE *
ScanForRootBridges (
  UINTN  *NumberOfRootBridges
  );
