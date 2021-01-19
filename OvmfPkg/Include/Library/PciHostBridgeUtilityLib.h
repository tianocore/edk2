/** @file
  Provide common utility functions to PciHostBridgeLib instances in
  ArmVirtPkg and OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, Huawei Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PCI_HOST_BRIDGE_UTILITY_LIB_H__
#define __PCI_HOST_BRIDGE_UTILITY_LIB_H__


/**
  Utility function to inform the platform that the resource conflict happens.

  @param[in] Configuration  Pointer to PCI I/O and PCI memory resource
                            descriptors. The Configuration contains the
                            resources for all the root bridges. The resource
                            for each root bridge is terminated with END
                            descriptor and an additional END is appended
                            indicating the end of the entire resources. The
                            resource descriptor field values follow the
                            description in
                            EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                            .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeUtilityResourceConflict (
  IN VOID  *Configuration
  );


#endif // __PCI_HOST_BRIDGE_UTILITY_LIB_H__
