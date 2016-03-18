/** @file

  Internal definitions for the virtio-net driver, which produces Simple Network
  Protocol instances for virtio-net devices.

  Copyright (C) 2013, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _VIRTIO_NET_DXE_H_
#define _VIRTIO_NET_DXE_H_

#include <IndustryStandard/VirtioNet.h>
#include <Library/DebugLib.h>
#include <Library/VirtioLib.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleNetwork.h>

#define VNET_SIG SIGNATURE_32 ('V', 'N', 'E', 'T')

//
// maximum number of pending packets, separately for each direction
//
#define VNET_MAX_PENDING 64

//
// State diagram:
//
//                  |     ^
//                  |     |
//        BindingStart  BindingStop
//        +SnpPopulate    |
//        ++GetFeatures   |
//                  |     |
//                  v     |
//                +---------+    virtio-net device is reset, no resources are
//                | stopped |    allocated for traffic, but MAC address has
//                +---------+    been retrieved
//                  |     ^
//                  |     |
//            SNP.Start SNP.Stop
//                  |     |
//                  v     |
//                +---------+
//                | started |    functionally identical to stopped
//                +---------+
//                  |     ^
//                  |     |
//       SNP.Initialize SNP.Shutdown
//                  |     |
//                  v     |
//              +-------------+  Virtio-net setup complete, including DRIVER_OK
//              | initialized |  bit. The receive queue is populated with
//              +-------------+  requests; McastIpToMac, GetStatus, Transmit,
//                               Receive are callable.
//

typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                          field              init function
  //                          ------------------ ------------------------------
  UINT32                      Signature;         // VirtioNetDriverBindingStart
  VIRTIO_DEVICE_PROTOCOL      *VirtIo;           // VirtioNetDriverBindingStart
  EFI_SIMPLE_NETWORK_PROTOCOL Snp;               // VirtioNetSnpPopulate
  EFI_SIMPLE_NETWORK_MODE     Snm;               // VirtioNetSnpPopulate
  EFI_EVENT                   ExitBoot;          // VirtioNetSnpPopulate
  EFI_DEVICE_PATH_PROTOCOL    *MacDevicePath;    // VirtioNetDriverBindingStart
  EFI_HANDLE                  MacHandle;         // VirtioNetDriverBindingStart

  VRING                       RxRing;            // VirtioNetInitRing
  UINT8                       *RxBuf;            // VirtioNetInitRx
  UINT16                      RxLastUsed;        // VirtioNetInitRx

  VRING                       TxRing;            // VirtioNetInitRing
  UINT16                      TxMaxPending;      // VirtioNetInitTx
  UINT16                      TxCurPending;      // VirtioNetInitTx
  UINT16                      *TxFreeStack;      // VirtioNetInitTx
  VIRTIO_NET_REQ              TxSharedReq;       // VirtioNetInitTx
  UINT16                      TxLastUsed;        // VirtioNetInitTx
} VNET_DEV;


//
// In order to avoid duplication of interface documentation, please find all
// leading comments near the respective function / variable definitions (not
// the declarations here), which is where your code editor of choice takes you
// anyway when jumping to a function.
//

//
// utility macros
//
#define VIRTIO_NET_FROM_SNP(SnpPointer) \
        CR (SnpPointer, VNET_DEV, Snp, VNET_SIG)

#define VIRTIO_CFG_WRITE(Dev, Field, Value)  ((Dev)->VirtIo->WriteDevice (  \
                                                (Dev)->VirtIo,              \
                                                OFFSET_OF_VNET (Field),     \
                                                SIZE_OF_VNET (Field),       \
                                                (Value)                     \
                                                ))

#define VIRTIO_CFG_READ(Dev, Field, Pointer) ((Dev)->VirtIo->ReadDevice (   \
                                                (Dev)->VirtIo,              \
                                                OFFSET_OF_VNET (Field),     \
                                                SIZE_OF_VNET (Field),       \
                                                sizeof *(Pointer),          \
                                                (Pointer)                   \
                                                ))

//
// component naming
//
extern EFI_COMPONENT_NAME_PROTOCOL gVirtioNetComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gVirtioNetComponentName2;

//
// driver binding
//
extern EFI_DRIVER_BINDING_PROTOCOL gVirtioNetDriverBinding;

//
// member functions implementing the Simple Network Protocol
//
EFI_STATUS
EFIAPI
VirtioNetStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
VirtioNetStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
VirtioNetInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize  OPTIONAL,
  IN UINTN                       ExtraTxBufferSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ExtendedVerification
  );

EFI_STATUS
EFIAPI
VirtioNetShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
VirtioNetReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      Enable,
  IN UINT32                      Disable,
  IN BOOLEAN                     ResetMCastFilter,
  IN UINTN                       MCastFilterCnt    OPTIONAL,
  IN EFI_MAC_ADDRESS             *MCastFilter      OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *New OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetStatistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN OUT UINTN                   *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS     *StatisticsTable  OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetMcastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     IPv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
  );

EFI_STATUS
EFIAPI
VirtioNetNvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ReadWrite,
  IN UINTN                       Offset,
  IN UINTN                       BufferSize,
  IN OUT VOID                    *Buffer
  );

EFI_STATUS
EFIAPI
VirtioNetGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus OPTIONAL,
  OUT VOID                       **TxBuf OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       HeaderSize,
  IN UINTN                       BufferSize,
  IN /* +OUT! */ VOID            *Buffer,
  IN EFI_MAC_ADDRESS             *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  IN UINT16                      *Protocol OPTIONAL
  );

EFI_STATUS
EFIAPI
VirtioNetReceive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINTN                      *HeaderSize OPTIONAL,
  IN OUT UINTN                   *BufferSize,
  OUT VOID                       *Buffer,
  OUT EFI_MAC_ADDRESS            *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS            *DestAddr   OPTIONAL,
  OUT UINT16                     *Protocol   OPTIONAL
  );

//
// utility functions shared by various SNP member functions
//
VOID
EFIAPI
VirtioNetShutdownRx (
  IN OUT VNET_DEV *Dev
  );

VOID
EFIAPI
VirtioNetShutdownTx (
  IN OUT VNET_DEV *Dev
  );

//
// event callbacks
//
VOID
EFIAPI
VirtioNetIsPacketAvailable (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  );

VOID
EFIAPI
VirtioNetExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  );

#endif // _VIRTIO_NET_DXE_H_
