/** @file
  Provides Pci vendor name functions.

  Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_VEN_NAME_LIB_H_
#define PCI_VEN_NAME_LIB_H_

/**
  Looks up the PCI vendor name.

  @param VenId             PCI vendor ID.

  @return                  The PCI vendor string, or NULL if an error
                           occurred or the PCI vendor ID isn't valid.

**/
CONST CHAR8 *
EFIAPI
GetPciVenName (
  IN UINT16  VenId
  );

#endif /* PCI_VEN_NAME_LIB_H_ */
