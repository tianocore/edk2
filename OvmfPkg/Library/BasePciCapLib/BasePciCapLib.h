/** @file
  Work with PCI capabilities in PCI config space -- internal type definitions.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __BASE_PCI_CAP_LIB_H__
#define __BASE_PCI_CAP_LIB_H__

#include <Library/OrderedCollectionLib.h>

#include <Library/PciCapLib.h>

//
// Structure that uniquely identifies a capability instance and serves as key
// for insertion and lookup.
//
typedef struct {
  PCI_CAP_DOMAIN Domain;
  UINT16         CapId;
  UINT16         Instance;
} PCI_CAP_KEY;

//
// In Instance==0 PCI_CAP objects, store NumInstances directly. In Instance>0
// PCI_CAP objects, link Instance#0 of the same (Domain, CapId). This way
// NumInstances needs maintenance in one object only, per (Domain, CapId) pair.
//
typedef union {
  UINT16  NumInstances;
  PCI_CAP *InstanceZero;
} PCI_CAP_NUM_INSTANCES;

//
// Complete the incomplete PCI_CAP structure here.
//
struct PCI_CAP {
  PCI_CAP_KEY           Key;
  PCI_CAP_NUM_INSTANCES NumInstancesUnion;
  UINT16                Offset;
  UINT16                MaxSizeHint;
  UINT8                 Version;
};

//
// Complete the incomplete PCI_CAP_LIST structure here.
//
struct PCI_CAP_LIST {
  ORDERED_COLLECTION *Capabilities;
};

#endif // __BASE_PCI_CAP_LIB_H__
