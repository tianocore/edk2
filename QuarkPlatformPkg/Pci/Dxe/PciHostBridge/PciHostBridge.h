/** @file
The Header file of the Pci Host Bridge Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _PCI_HOST_BRIDGE_H_
#define _PCI_HOST_BRIDGE_H_


#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <PciRootBridge.h>
#include <Library/UefiDriverEntryPoint.h>
#include <IndustryStandard/Pci22.h>
#include <Library/UefiLib.h>
#include <Guid/HobList.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#define PCI_HOST_BRIDGE_SIGNATURE SIGNATURE_32 ('e', 'h', 's', 't')
typedef struct {
  UINTN                                             Signature;
  EFI_HANDLE                                        HostBridgeHandle;
  UINTN                                             RootBridgeCount;
  EFI_LIST_ENTRY                                    Head;
  BOOLEAN                                           ResourceSubmited;
  BOOLEAN                                           CanRestarted;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  ResAlloc;
} PCI_HOST_BRIDGE_INSTANCE;

#define INSTANCE_FROM_RESOURCE_ALLOCATION_THIS(a) CR (a, PCI_HOST_BRIDGE_INSTANCE, ResAlloc, PCI_HOST_BRIDGE_SIGNATURE)

typedef enum {
  SocketResourceRatioChanged,
  SocketResourceRatioNotChanged,
  SocketResourceAdjustMax
} SOCKET_RESOURCE_ADJUSTMENT_RESULT;

//
// Driver Entry Point
//
EFI_STATUS
EFIAPI
InitializePciHostBridge (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Entry point of this driver.

Arguments:

  ImageHandle  -  Image handle of this driver.
  SystemTable  -  Pointer to standard EFI system table.

Returns:

  EFI_SUCCESS       -  Succeed.
  EFI_DEVICE_ERROR  -  Fail to install PCI_ROOT_BRIDGE_IO protocol.

--*/
;

//
//  HostBridge Resource Allocation interface
//
EFI_STATUS
EFIAPI
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL   *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE      Phase
  )
/*++

Routine Description:

  Enter a certain phase of the PCI enumeration process.

Arguments:

  This   -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
  Phase  -  The phase during enumeration.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong phase parameter passed in.
  EFI_NOT_READY          -  Resources have not been submitted yet.

--*/
;

EFI_STATUS
EFIAPI
GetNextRootBridge (
  IN     EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT EFI_HANDLE                                       *RootBridgeHandle
  )
/*++

Routine Description:

  Return the device handle of the next PCI root bridge that is associated with
  this Host Bridge.

Arguments:

  This              - The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  Returns the device handle of the next PCI Root Bridge.
                       On input, it holds the RootBridgeHandle returned by the most
                       recent call to GetNextRootBridge().The handle for the first
                       PCI Root Bridge is returned if RootBridgeHandle is NULL on input.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_NOT_FOUND          -  Next PCI root bridge not found.
  EFI_INVALID_PARAMETER  -  Wrong parameter passed in.

--*/
;

EFI_STATUS
EFIAPI
GetAttributes (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  )
/*++

Routine Description:

  Returns the attributes of a PCI Root Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle  -  The device handle of the PCI Root Bridge
                       that the caller is interested in
  Attributes        -  The pointer to attributes of the PCI Root Bridge

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Attributes parameter passed in is NULL or
                            RootBridgeHandle is not an EFI_HANDLE
                            that was returned on a previous call to
                            GetNextRootBridge().

--*/
;

EFI_STATUS
EFIAPI
StartBusEnumeration (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:

  This is the request from the PCI enumerator to set up
  the specified PCI Root Bridge for bus enumeration process.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge to be set up.
  Configuration     -  Pointer to the pointer to the PCI bus resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_OUT_OF_RESOURCES   -  Not enough pool to be allocated.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
;

EFI_STATUS
EFIAPI
SetBusNumbers (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:

  This function programs the PCI Root Bridge hardware so that
  it decodes the specified PCI bus range.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge whose bus range is to be programmed.
  Configuration     -  The pointer to the PCI bus resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong parameters passed in.

--*/
;

EFI_STATUS
EFIAPI
SubmitResources (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:

  Submits the I/O and memory resource requirements for the specified PCI Root Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle  -  The PCI Root Bridge whose I/O and memory resource requirements
                       are being submitted
  Configuration     -  The pointer to the PCI I/O and PCI memory resource descriptor

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong parameters passed in.

--*/
;

EFI_STATUS
EFIAPI
GetProposedResources (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:

  This function returns the proposed resource settings for the specified
  PCI Root Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge handle.
  Configuration     -  The pointer to the pointer to the PCI I/O
                       and memory resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_OUT_OF_RESOURCES   -  Not enough pool to be allocated.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
;

EFI_STATUS
EFIAPI
PreprocessController (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN EFI_HANDLE                                                RootBridgeHandle,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS               PciAddress,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE              Phase
  )
/*++

Routine Description:

  This function is called for all the PCI controllers that the PCI
  bus driver finds. Can be used to Preprogram the controller.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge handle.
  PciAddress        -  Address of the controller on the PCI bus.
  Phase             -  The Phase during resource allocation.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
;

//
// Host Bridge Silicon specific hooks
//
UINT64
GetAllocAttributes (
  IN  UINTN        RootBridgeIndex
  )
/*++

Routine Description:

  Returns the Allocation attributes for the BNB Root Bridge.

Arguments:

  RootBridgeIndex  -  The root bridge number. 0 based.

Returns:

  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE

--*/
;

EFI_STATUS
GetHostBridgeMemApertures (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL               *PciRootBridgeIo,
  OUT UINT32                                        *Mem32Base,
  OUT UINT32                                        *Mem32Limit,
  OUT UINT64                                        *Mem64Base,
  OUT UINT64                                        *Mem64Limit
  )
/*++

Routine Description:

  Returns memory apertures for the BNB Root Bridge.

Arguments:

  PciRootBridgeIo  -  Pointer to Efi Pci root bridge Io protocol interface instance.
  Mem32Base        -  Pointer to 32 bit memory base. This is the lowest 32 bit memory address
                      that is decoded by the Host Bridge.
  Mem32Limit       -  Pointer to 32 bit memory limit.This is the highest 32 bit memory address
                      that is decoded by the Host Bridge. The size of the 32 bit window is
                      (Mem32Limit - Mem32base + 1).
  Mem64Base        -  Pointer to 64 bit memory base. This is the lowest 64 bit memory address
                      that is decoded by the Host Bridge.
  Mem64Limit       -  Pointer to 64 bit memory limit.This is the highest 64 bit memory address
                      that is decoded by the Host Bridge. The size of the 64 bit window is
                      (Mem64Limit - Mem64base + 1). Set Mem64Limit < Mem64Base if the host bridge
                      does not support 64 bit memory addresses.

Returns:

  EFI_SUCCESS  -  Success.

--*/
;

UINT64
Power2MaxMemory (
  IN UINT64         MemoryLength
  )
/*++

Routine Description:

  Calculate maximum memory length that can be fit to a mtrr.

Arguments:

  MemoryLength  -  Input memory length.

Returns:

  Returned Maximum length.

--*/
;

#endif
